#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <c_threads.h>
#include <randomx.h>
#include "rx_hashs.h"
#include "rx_common.h"

// 记录并行计算的randomx状态的结构体
typedef struct tag_rxs_state {
	CTHR_MUTEX_TYPE state_mutex;      // 访问全局rxs_state的锁
	char rs_hash[RX_HASH_SIZE];       // randomx全局的种子hash
	randomx_cache *rs_cache;          // randomx的全局cache
} rxs_state;

//randomx种子和cache的信息
typedef struct tag_seedinfo_s {
	randomx_cache *si_cache;
	unsigned long si_start;
	unsigned long si_count;
} seedinfo_s;

static CTHR_MUTEX_TYPE rx_mutex = CTHR_MUTEX_INIT;          //全局锁
static CTHR_MUTEX_TYPE rx_dataset_mutex = CTHR_MUTEX_INIT;  //修改dataset的锁
static rxs_state rx_state;
static randomx_dataset *rx_dataset;
static THREADV randomx_vm *rx_vm = NULL;

static void local_abort(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
#ifdef NDEBUG
	_exit(1);
#else
	abort();
#endif
}

static CTHR_THREAD_RTYPE rx_seedthread(void *arg) {
	seedinfo_s *si = (seedinfo_s*)arg;
	randomx_init_dataset(rx_dataset, si->si_cache, si->si_start, si->si_count);
	CTHR_THREAD_RETURN;
}

static void rx_initdata(randomx_cache *rs_cache, const int miners) {
	if (miners > 1) {
		// 平均分配给每一个线程的dataset item
		unsigned long delta = randomx_dataset_item_count() / miners;
		unsigned long start = 0;
		int i;
		seedinfo_s *si;
		CTHR_THREAD_TYPE *st;

		si = (seedinfo_s*)malloc(miners * sizeof(seedinfo_s));
		if (si == NULL)
			local_abort("Couldn't allocate RandomX mining threadinfo");

		st = (CTHR_THREAD_TYPE *)malloc(miners * sizeof(CTHR_THREAD_TYPE));
		if (st == NULL) {
			free(si);
			local_abort("Couldn't allocate RandomX mining threadlist");
		}

		// 记录每个线程的dataset item的起始索引，item个数
		for (i=0; i<miners-1; i++) {
			si[i].si_cache = rs_cache;
			si[i].si_start = start;   //起始索引
			si[i].si_count = delta;   //结束个数
			start += delta;
		}
		// 最后一个线程使用dataset里面剩余的所有的 item
		si[i].si_cache = rs_cache;
		si[i].si_start = start;
		si[i].si_count = randomx_dataset_item_count() - start;

		// 开启多线程，每个线程都初始化自己的dataset
		for (i=1; i<miners; i++) {
			CTHR_THREAD_CREATE(st[i], rx_seedthread, &si[i]);
		}
		//第一块dataset由当前主线程初始化
		randomx_init_dataset(rx_dataset, rs_cache, 0, si[0].si_count);
		// 等待线程初始化的结束
		for (i=1; i<miners; i++) {
			CTHR_THREAD_JOIN(st[i]);
		}
		free(st);
		free(si);
	} else {
		// 如果只有一个线程则使用dataset里面所有的item
		randomx_init_dataset(rx_dataset, rs_cache, 0, randomx_dataset_item_count());
	}
}

/**
 * @brief 计算数据data的哈希值到hash里面
 *
 * @param seedhash		调用函数的调用房希望使用的种子哈希
 * @param data				被哈希的数据
 * @param length			被哈希的数据的长度
 * @param hash				最终输出的哈希值
 */
void rx_slow_hashs(const char *seedhash, const void *data, size_t length, uint8_t *hash) {

	// 默认初始化dataset的线程个数为4个
	int miners=RX_DATASET_INIT_THREAD_COUNT;

	// 获取randomx的标志位
	randomx_flags flags = enabled_flags() & ~disabled_flags();

	rxs_state *rx_sp=&rx_state;
	randomx_cache *cache=rx_sp->rs_cache;;

	// 开始初始化randomx的cache，dataset,新建vm，然后计算数据的哈希
	CTHR_MUTEX_LOCK(rx_sp->state_mutex);

	if (cache == NULL) {
		// 尝试分配Large Pages
		cache = randomx_alloc_cache(flags | RANDOMX_FLAG_LARGE_PAGES);
		if (cache == NULL) {
			// 如果无法分配Large Pages则按照默认选项分配
			cache = randomx_alloc_cache(flags);
		}
		if (cache == NULL)
			local_abort("Couldn't allocate RandomX cache");
	}

	// 如果randomx cache为空，或者调用函数传入的种子跟当前的种子不一致
	bool seed_same = memcmp(seedhash, rx_sp->rs_hash, RX_HASH_SIZE)==0;
	if (rx_sp->rs_cache == NULL || !seed_same) {
		// 初始化cache
		randomx_init_cache(cache, seedhash, RX_HASH_SIZE);
		rx_sp->rs_cache = cache;
		memcpy(rx_sp->rs_hash, seedhash, RX_HASH_SIZE);
	}

	// 如果vm为空
	if (rx_vm == NULL) {
		if ((flags & RANDOMX_FLAG_JIT) && !miners) {
			flags |= RANDOMX_FLAG_SECURE & ~disabled_flags();
		}
		if (miners && (disabled_flags() & RANDOMX_FLAG_FULL_MEM)) {
			miners = 0;
		}

		// 如果需要多线程初始化dataset
		if (miners) {
			CTHR_MUTEX_LOCK(rx_dataset_mutex);
			// dataset为空则分配dataset
			if (rx_dataset == NULL) {
				// 尝试large page模式分配dataset
				rx_dataset = randomx_alloc_dataset(RANDOMX_FLAG_LARGE_PAGES);
				if (rx_dataset == NULL) {
					// large page无法分配成功则使用默认模式分配
					printf("Couldn't use largePages for RandomX dataset");
					rx_dataset = randomx_alloc_dataset(RANDOMX_FLAG_DEFAULT);
				}
				// 重新多线程初始化dataset
				if (rx_dataset != NULL)
					rx_initdata(rx_sp->rs_cache, miners);
			}

			if (rx_dataset != NULL)
				flags |= RANDOMX_FLAG_FULL_MEM;
			else {
				// 如果无法分配dataset 则退出
				miners = 0;
				local_abort("Couldn't allocate RandomX dataset for miner");
			}
			CTHR_MUTEX_UNLOCK(rx_dataset_mutex);
		}

		// 创建vm，如果large page模式创建失败，则使用默认的模式来创建vm
		rx_vm = randomx_create_vm(flags | RANDOMX_FLAG_LARGE_PAGES, rx_sp->rs_cache, rx_dataset);
		if(rx_vm == NULL) {
			printf("Couldn't use largePages for RandomX VM");
			rx_vm = randomx_create_vm(flags, rx_sp->rs_cache, rx_dataset);
		}
		if(rx_vm == NULL) {
			flags = RANDOMX_FLAG_DEFAULT | (miners ? RANDOMX_FLAG_FULL_MEM : 0);
			rx_vm = randomx_create_vm(flags, rx_sp->rs_cache, rx_dataset);
		}
		if (rx_vm == NULL)
			local_abort("Couldn't allocate RandomX VM");
	} else {
		// 如果vm不为空多线程初始化dataset
		CTHR_MUTEX_LOCK(rx_dataset_mutex);
		if (rx_dataset != NULL && !seed_same)
			rx_initdata(cache, miners);
		CTHR_MUTEX_UNLOCK(rx_dataset_mutex);

		// 重新设置vm的cache
		randomx_vm_set_cache(rx_vm, rx_sp->rs_cache);
	}

	// 计算hash值
	randomx_calculate_hash(rx_vm, data, length, hash);

	// 释放state锁
	CTHR_MUTEX_UNLOCK(rx_sp->state_mutex);
}
