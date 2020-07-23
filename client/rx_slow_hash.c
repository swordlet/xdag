// Copyright (c) 2019, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "randomx.h"
#include "c_threads.h"
#include "rx_slow_hash.h"

#define RX_LOGCAT	"randomx"

#if defined(_MSC_VER)
#define THREADV __declspec(thread)
#else
#define THREADV __thread
#endif

// 记录randomx状态的结构体
typedef struct rx_state {
	CTHR_MUTEX_TYPE rs_mutex;
	char rs_hash[HASH_SIZE];      // randomx全局的种子hash
	uint64_t  rs_height;          // randomx的种子高度
	randomx_cache *rs_cache;      // randomx的全局cache
} rx_state;

static CTHR_MUTEX_TYPE rx_mutex = CTHR_MUTEX_INIT;          //全局锁
static CTHR_MUTEX_TYPE rx_dataset_mutex = CTHR_MUTEX_INIT;  //修改dataset的锁

/**
 * 记录randomx计算相关信息的上下文,目的是为了可以让使用不同的seed的调用方并行的调用rx_slow_hash函数 0 为主，1为备
 * rx_slow_hash函数当中根据不同的高度以及不同的分叉链，来决定到底使用主还是备
 * */
static rx_state rx_s[2] = {
		{CTHR_MUTEX_INIT,{0},0,0},
		{CTHR_MUTEX_INIT,{0},0,0}
};

static randomx_dataset *rx_dataset;
static uint64_t rx_dataset_height;
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

static inline int disabled_flags(void) {
	static int flags = -1;

	if (flags != -1) {
		return flags;
	}

	const char *env = getenv("MONERO_RANDOMX_UMASK");
	if (!env) {
		flags = 0;
	}
	else {
		char* endptr;
		long int value = strtol(env, &endptr, 0);
		if (endptr != env && value >= 0 && value < INT_MAX) {
			flags = value;
		}
		else {
			flags = 0;
		}
	}

	return flags;
}

static inline int enabled_flags(void) {
	static int flags = -1;

	if (flags != -1) {
		return flags;
	}

	flags = randomx_get_flags();

	return flags;
}

#define SEEDHASH_EPOCH_BLOCKS	2048	/* Must be same as BLOCKS_SYNCHRONIZING_MAX_COUNT in cryptonote_config.h */
#define SEEDHASH_EPOCH_LAG		64

void rx_reorg(const uint64_t split_height) {
	int i;
	CTHR_MUTEX_LOCK(rx_mutex);
	for (i=0; i<2; i++) {
		if (split_height <= rx_s[i].rs_height) {
			if (rx_s[i].rs_height == rx_dataset_height)
				rx_dataset_height = 1;
			rx_s[i].rs_height = 1;	/* set to an invalid seed height */
		}
	}
	CTHR_MUTEX_UNLOCK(rx_mutex);
}

uint64_t rx_seedheight(const uint64_t height) {
	// 如果高度小于2048+64个，则直接以高度为0的块hash作为种子
	// 否则以 高度为 (当前高度-64-1) & ~(2048-1) 的高度的块的hash作为种子
	// 大改就是向前2048个块作为种子
	uint64_t s_height =  (height <= SEEDHASH_EPOCH_BLOCKS+SEEDHASH_EPOCH_LAG) ? 0 :
	                     (height - SEEDHASH_EPOCH_LAG - 1) & ~(SEEDHASH_EPOCH_BLOCKS-1);
	return s_height;
}

void rx_seedheights(const uint64_t height, uint64_t *seedheight, uint64_t *nextheight) {
	*seedheight = rx_seedheight(height);
	*nextheight = rx_seedheight(height + SEEDHASH_EPOCH_LAG);
}

//randomx种子和cache的信息
typedef struct seedinfo {
	randomx_cache *si_cache;
	unsigned long si_start;
	unsigned long si_count;
} seedinfo;

static CTHR_THREAD_RTYPE rx_seedthread(void *arg) {
	seedinfo *si = (seedinfo*)arg;
	randomx_init_dataset(rx_dataset, si->si_cache, si->si_start, si->si_count);
	CTHR_THREAD_RETURN;
}

static void rx_initdata(randomx_cache *rs_cache, const int miners, const uint64_t seedheight) {
	if (miners > 1) {
		// 平均分配给每一个线程的dataset item
		unsigned long delta = randomx_dataset_item_count() / miners;
		unsigned long start = 0;
		int i;
		seedinfo *si;
		CTHR_THREAD_TYPE *st;

		si = (seedinfo*)malloc(miners * sizeof(seedinfo));
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

	rx_dataset_height = seedheight;
}

/**
 * @brief 计算数据data的哈希值到hash里面
 *
 * @param mainheight 	主链的主块高度
 * @param seedheight	调用函数的调用方希望使用的种子块高度
 * @param seedhash		调用函数的调用房希望使用的种子哈希
 * @param data				被哈希的数据
 * @param length			被哈希的数据的长度
 * @param hash				最终输出的哈希值
 * @param miners			初始化dataset的线程个数
 * @param is_alt			是否是分叉链(0.主链 1.分叉链)
 */
void rx_slow_hash(const uint64_t mainheight, const uint64_t seedheight, const char *seedhash, const void *data, size_t length,
                  char *hash, int miners, int is_alt) {
	// 获取种子高度
	uint64_t s_height = rx_seedheight(mainheight);
	// 切换主备rx_state的标志位
	int toggle = (s_height & SEEDHASH_EPOCH_BLOCKS) != 0;
	// 获取randomx的标志位
	randomx_flags flags = enabled_flags() & ~disabled_flags();
	// randomx状态
	rx_state *rx_sp;
	randomx_cache *cache;

	CTHR_MUTEX_LOCK(rx_mutex);

	// 如果分叉链上的块和主链上的种子块一样，则没有必要给分叉链上的块分配cache
	if (is_alt) {
		// 如果调用方要计算的是分叉链的块的哈希

		// 如果分叉链的种子高度跟当前主链的一致并且当前区块的高度小于2048，并且参数seedhash和rx_s[0]一致，则继续使用rx_s[0]做randomx哈希运算
		// 如果分叉链的种子高度跟当前主链的一致并且当前区块的高度大于2048，并且参数seedhash和rx_s[1]一致，则继续使用rx_s[1]做randomx哈希运算
		// 如果分叉链的种子高度跟当前珠链的不一致或者  (当前高度小于2048并且seedhash和rx_s[0]不一致)，则切换使用rx_s[1]做randomx哈希运算
		// 如果分叉链的种子高度跟当前珠链的不一致或者  (当前高度大于2048并且seedhash和rx_s[1]不一致)，则切换使用rx_s[1]做randomx哈希运算
		if (s_height == seedheight && !memcmp(rx_s[toggle].rs_hash, seedhash, HASH_SIZE))
			is_alt = 0;
	} else {
		// 如果调用方要计算的是主链的块的哈希

		// RPC有可能请求一个在主链上更早的区块，该区块的哈希使用的块的哈希可能是比当前主链所使用的代码
		// 如果当前主链的高度小于2048，则使用rx_s[0]进行哈希运算
		// 如果当前区块的高度大于2048，则使用rx_s[1]进行哈希运算
		if (s_height > seedheight){
			is_alt = 1;
		}else if (s_height < seedheight){
			// 矿工提交的区块高度可能大于矿池的，如果矿工使用的种子高度大于矿池的
			// 如果当前主链的高度小于2048，则使用rx_s[1]进行哈希运算
			// 如果当前主链的高度大于2048，则使用rx_s[0]进行哈希运算
			toggle ^= 1;
		}
	}

	// 如果不是分叉链，并且切换到rx_s[1]，则切换回rx_s[0]，否则继续使用rx_s[1]
	// 如果是分叉链，并且切换到了rx_s[0]，则切换会rx_s[1]，否则继续使用rx_s[0]
	// rx_s[0]用于主链的randomx计算，rx_s[1]用户分叉链的randomx计算
	toggle ^= (is_alt != 0);

	// 选定使用哪个rx_state完毕
	rx_sp = &rx_s[toggle];

	// 开始初始化randomx的cache，dataset,新建vm，然后计算数据的哈希
	CTHR_MUTEX_LOCK(rx_sp->rs_mutex);
	CTHR_MUTEX_UNLOCK(rx_mutex);

	// 分配cache
	cache = rx_sp->rs_cache;
	if (cache == NULL) {
		if (cache == NULL) {
			// 尝试分配Large Pages
			cache = randomx_alloc_cache(flags | RANDOMX_FLAG_LARGE_PAGES);
			if (cache == NULL) {
				printf("Couldn't use largePages for RandomX cache");
				// 如果无法分配Large Pages则按照默认选项分配
				cache = randomx_alloc_cache(flags);
			}
			if (cache == NULL)
				local_abort("Couldn't allocate RandomX cache");
		}
	}

	// 如果调用该函数传入的参数，包括高度，种子，缓存跟选定的rx_stae不一致
	// 则重新初始化cache，并重新记录cahce和种子高度到缓存状态
	if (rx_sp->rs_height != seedheight || rx_sp->rs_cache == NULL || memcmp(seedhash, rx_sp->rs_hash, HASH_SIZE)) {
		randomx_init_cache(cache, seedhash, HASH_SIZE);
		rx_sp->rs_cache = cache;
		rx_sp->rs_height = seedheight;
		memcpy(rx_sp->rs_hash, seedhash, HASH_SIZE);
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
			//分配dataset
			if (rx_dataset == NULL) {
				rx_dataset = randomx_alloc_dataset(RANDOMX_FLAG_LARGE_PAGES);
				if (rx_dataset == NULL) {
					printf("Couldn't use largePages for RandomX dataset");
					rx_dataset = randomx_alloc_dataset(RANDOMX_FLAG_DEFAULT);
				}
				if (rx_dataset != NULL)
					// 多线程初始化dataset
					rx_initdata(rx_sp->rs_cache, miners, seedheight);
			}
			if (rx_dataset != NULL)
				flags |= RANDOMX_FLAG_FULL_MEM;
			else {
				miners = 0;
				printf("Couldn't allocate RandomX dataset for miner");
			}
			CTHR_MUTEX_UNLOCK(rx_dataset_mutex);
		}

		// 创建vm，如果large page模式创建失败，则使用默认的模式来创建vm
		rx_vm = randomx_create_vm(flags | RANDOMX_FLAG_LARGE_PAGES, rx_sp->rs_cache, rx_dataset);
		if(rx_vm == NULL) { //large pages failed
			printf("Couldn't use largePages for RandomX VM");
			rx_vm = randomx_create_vm(flags, rx_sp->rs_cache, rx_dataset);
		}
		if(rx_vm == NULL) {//fallback if everything fails
			flags = RANDOMX_FLAG_DEFAULT | (miners ? RANDOMX_FLAG_FULL_MEM : 0);
			rx_vm = randomx_create_vm(flags, rx_sp->rs_cache, rx_dataset);
		}
		if (rx_vm == NULL)
			local_abort("Couldn't allocate RandomX VM");
	} else if (miners) {
		// 如果vm不为空并且如果使用多线程初始化dataset
		CTHR_MUTEX_LOCK(rx_dataset_mutex);
		// 如果dataset不为空并且dataset记录的高度跟调用者使用的高度不一致，则重新初始化dataset
		if (rx_dataset != NULL && rx_dataset_height != seedheight)
			rx_initdata(cache, miners, seedheight);
		CTHR_MUTEX_UNLOCK(rx_dataset_mutex);
	} else {
		/* this is a no-op if the cache hasn't changed */
		// 如果虚拟机不为空，并且是单线程初始化dataset，则重新
		randomx_vm_set_cache(rx_vm, rx_sp->rs_cache);
	}
	/* mainchain users can run in parallel */
	if (!is_alt)
		CTHR_MUTEX_UNLOCK(rx_sp->rs_mutex);

	// 计算hash值
	randomx_calculate_hash(rx_vm, data, length, hash);

	/* altchain slot users always get fully serialized */
	if (is_alt)
		CTHR_MUTEX_UNLOCK(rx_sp->rs_mutex);
}

void rx_slow_hash_allocate_state(void) {
}

void rx_slow_hash_free_state(void) {
	if (rx_vm != NULL) {
		randomx_destroy_vm(rx_vm);
		rx_vm = NULL;
	}
}

void rx_stop_mining(void) {
	CTHR_MUTEX_LOCK(rx_dataset_mutex);
	if (rx_dataset != NULL) {
		randomx_dataset *rd = rx_dataset;
		rx_dataset = NULL;
		randomx_release_dataset(rd);
	}
	CTHR_MUTEX_UNLOCK(rx_dataset_mutex);
}
