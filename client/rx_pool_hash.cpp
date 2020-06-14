//
// Created by mac on 2020/6/11.
//

#include <randomx.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dataset.hpp>
#include <log.h>
#include "rx_pool_hash.h"


//global randomx virables
static uint64_t g_current_pool_seed[4]; //size must be as same as xdag_hash_t
static uint32_t g_mining_thread_count=1;
static randomx_flags g_pool_flags = RANDOMX_FLAG_DEFAULT;
static randomx_cache *g_rx_pool_cache = NULL;
static randomx_dataset *g_rx_pool_dataset = NULL;
static randomx_vm *g_rx_pool_vm = NULL;

static pthread_mutex_t g_rx_calc_mutex = PTHREAD_MUTEX_INITIALIZER;       //mutex for calc hash
static pthread_mutex_t g_rx_init_mutex = PTHREAD_MUTEX_INITIALIZER;       //mutex for init rx seed

//struct for muti-thread init dataset
typedef struct miner_seedinfo {
	randomx_cache *si_cache;
	unsigned long si_start;
	unsigned long si_count;
} pool_seed_info;

uint64_t rx_seedheight(const uint64_t height){
	uint64_t s_height = (height <= SEEDHASH_EPOCH_BLOCKS + SEEDHASH_EPOCH_LAG) ? 0 :
	                    (height - SEEDHASH_EPOCH_LAG - 1) & ~(SEEDHASH_EPOCH_BLOCKS - 1);
	return s_height;
}

void rx_seedheights(const uint64_t height, uint64_t *seed_height, uint64_t *next_height){
	*seed_height = rx_seedheight(height);
	*next_height = rx_seedheight(height + SEEDHASH_EPOCH_LAG);
}

static void * init_pool_dataset_thread(void *arg) {
	miner_seedinfo *si = (miner_seedinfo*)arg;
	xdag_info("init pool dataset with thread %lu start %lu count %lu",pthread_self(), si->si_start,si->si_count);
	randomx_init_dataset(g_rx_pool_dataset, si->si_cache, si->si_start, si->si_count);
	return NULL;
}

static void rx_pool_init_dataset(randomx_cache *rs_cache, const int thread_count) {
	if (thread_count > 1) {
		// 平均分配给每一个线程的dataset item
		unsigned long start = 0;
		int i;
		pool_seed_info *si;
		pthread_t *st;

		si = (pool_seed_info*)malloc(thread_count * sizeof(pool_seed_info));
		if (si == NULL){
			xdag_fatal("Couldn't allocate RandomX mining threadinfo");
			return;
		}

		st = (pthread_t*)malloc(thread_count * sizeof(pthread_t));
		if (st == NULL) {
			free(si);
			xdag_fatal("Couldn't allocate RandomX mining threadlist");
			return;
		}

		// 记录每个线程的dataset item的起始索引，item个数
		uint32_t item_count = randomx_dataset_item_count();
		uint32_t item_count_per = item_count / thread_count;
		uint32_t item_count_remain = item_count % thread_count;

		xdag_info("pool init dataset count %u per %u remain %u",item_count,item_count_per,item_count_remain);

		//给每个线程分配itemcount，最后一个线程使用剩余的所有的itemcount
		for (i=0; i < thread_count; i++) {
			auto count = item_count_per + (i == thread_count - 1 ? item_count_remain : 0);
			si[i].si_cache = rs_cache;
			si[i].si_start = start;   //起始索引
			si[i].si_count = count;   //结束个数
			start += count;
		}

		xdag_info("init dataset for pool with %d thread",thread_count);
		// 开启多线程，每个线程都初始化自己的dataset
		for (i=0; i < thread_count; i++) {
			pthread_create(&st[i],NULL, init_pool_dataset_thread, &si[i]);
		}

		// 等待线程初始化的结束
		for (i=0; i < thread_count; i++) {
			pthread_join(st[i],NULL);
		}
		free(st);
		free(si);
	} else {
		// 如果只有一个线程则使用dataset里面所有的item
		randomx_init_dataset(g_rx_pool_dataset, rs_cache, 0, randomx_dataset_item_count());
	}
}

int rx_pool_init_seed(void *seed_data, size_t seed_size)
{
	pthread_mutex_lock(&g_rx_init_mutex);

	bool toggled = false;
	if(memcmp(seed_data,g_current_pool_seed,sizeof(g_current_pool_seed))){
		xdag_info("pool seed changed reinit pool rx paramters");
		memcpy(g_current_pool_seed,seed_data,sizeof(g_current_pool_seed));
		xdag_info("pool seed reinited %016llx%016llx%016llx%016llx not changed",
		          g_current_pool_seed[0],g_current_pool_seed[1],g_current_pool_seed[2],g_current_pool_seed[3]);
		toggled = true;
	} else{
		xdag_info("pool seed %016llx%016llx%016llx%016llx not changed",
		          g_current_pool_seed[0],g_current_pool_seed[1],g_current_pool_seed[2],g_current_pool_seed[3]);
		pthread_mutex_unlock(&g_rx_init_mutex);
		return 0;
	}

	if(toggled){
		//g_pool_flags=RANDOMX_FLAG_DEFAULT;
		if(g_rx_pool_vm){
			randomx_destroy_vm(g_rx_pool_vm);
			g_rx_pool_vm=NULL;
		}

		if(g_rx_pool_dataset){
			randomx_release_dataset(g_rx_pool_dataset);
			g_rx_pool_dataset=NULL;
		}

		if(g_rx_pool_cache){
			randomx_release_cache(g_rx_pool_cache);
			g_rx_pool_cache=NULL;
		}

		g_pool_flags = randomx_get_flags();

//	if (large_pages) {
//		flags |= RANDOMX_FLAG_LARGE_PAGES;
//	}
#ifndef __OpenBSD__
//	if (secure) {
//		g_mine_flags |= RANDOMX_FLAG_SECURE;
//	}
#endif

		// 矿池暂时不需要full memory
		//g_pool_flags |= RANDOMX_FLAG_FULL_MEM;

		// printf randomx flags info
		if (g_pool_flags & RANDOMX_FLAG_ARGON2_AVX2) {
			printf(" - Argon2 implementation: AVX2\n");
		}
		else if (g_pool_flags & RANDOMX_FLAG_ARGON2_SSSE3) {
			printf(" - Argon2 implementation: SSSE3\n");
		}
		else {
			printf(" - Argon2 implementation: reference\n");
		}

		if (g_pool_flags & RANDOMX_FLAG_FULL_MEM) {
			printf(" - full memory mode (2080 MiB)\n");
		}
		else {
			printf(" - light memory mode (256 MiB)\n");
		}

		if (g_pool_flags & RANDOMX_FLAG_JIT) {
			printf(" - JIT compiled mode \n");
			if (g_pool_flags & RANDOMX_FLAG_SECURE) {
				printf("(secure)\n");
			}
		}
		else {
			printf(" - interpreted mode\n");
		}

		if (g_pool_flags & RANDOMX_FLAG_HARD_AES) {
			printf(" - hardware AES mode\n");
		}
		else {
			printf(" - software AES mode\n");
		}

		if (g_pool_flags & RANDOMX_FLAG_LARGE_PAGES) {
			printf(" - large pages mode\n");
		}
		else {
			printf(" - small pages mode\n");
		}

		printf("Initializing Pool\n");
	}

	if (randomx::selectArgonImpl(g_pool_flags) == NULL) {
		printf("Unsupported Argon2 implementation");
		pthread_mutex_unlock(&g_rx_init_mutex);
		return -1;
	}

	if ((g_pool_flags & RANDOMX_FLAG_JIT) && !RANDOMX_HAVE_COMPILER) {
		printf("JIT compilation is not supported on this platform. Try without --jit");
		pthread_mutex_unlock(&g_rx_init_mutex);
		return -1;
	}

	if (!(g_pool_flags & RANDOMX_FLAG_JIT) && RANDOMX_HAVE_COMPILER) {
		//printf("WARNING: You are using the interpreter mode. Use --jit for optimal performance.\n");
		g_pool_flags |= RANDOMX_FLAG_JIT;
	}

	xdag_fatal("alloc pool hash cache ...");
	if(g_rx_pool_cache == NULL){
		g_rx_pool_cache = randomx_alloc_cache(g_pool_flags);
		if (g_rx_pool_cache == NULL) {
			xdag_fatal("can not alloc mine cache");
			pthread_mutex_unlock(&g_rx_init_mutex);
			return -1;
		}
		randomx_init_cache(g_rx_pool_cache, g_current_pool_seed, sizeof(g_current_pool_seed));
	}

	xdag_info("alloc pool hash dataset ...");
	if(g_rx_pool_dataset == NULL){
		g_rx_pool_dataset = randomx_alloc_dataset(g_pool_flags);
		if (g_rx_pool_dataset == NULL) {
			xdag_fatal("alloc dataset failed");
			pthread_mutex_unlock(&g_rx_init_mutex);
			return -1;
		}

		// init dataset with 4 thread
		rx_pool_init_dataset(g_rx_pool_cache,4);
		//randomx_release_cache(g_rx_pool_cache);
		//g_rx_pool_cache = NULL;
	}

	xdag_info("alloc pool hash vm ...");
	if(g_rx_pool_vm == NULL){
		g_rx_pool_vm = randomx_create_vm(g_pool_flags,g_rx_pool_cache,g_rx_pool_dataset);
		if (g_rx_pool_vm == NULL) {
			xdag_fatal("create randomx vm failed");
			pthread_mutex_unlock(&g_rx_init_mutex);
			return -1;
		}
	}
	xdag_info("alloc pool hash vm finished");

	pthread_mutex_unlock(&g_rx_init_mutex);

	return 0;
}

// 矿池更换key必须释放所有资源
int rx_pool_calc_hash(void *seed_data, size_t seed_size,void* data,size_t data_size,void* output_hash){

	pthread_mutex_lock(&g_rx_calc_mutex);
	rx_pool_init_seed(seed_data,seed_size);
	randomx_calculate_hash(g_rx_pool_vm,data,data_size,output_hash);
	pthread_mutex_unlock(&g_rx_calc_mutex);

	return 0;
}
