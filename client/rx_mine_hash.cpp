//
// Created by mac on 2020/6/11.
//

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <randomx.h>
#include "utils/log.h"
#include <dataset.hpp>

#include "rx_mine_hash.h"

//struct for muti-thread init dataset
typedef struct miner_seedinfo {
	randomx_cache *si_cache;
	unsigned long si_start;
	unsigned long si_count;
} miner_seed_info;

//global randomx virables
static uint32_t g_mining_thread_count=1;
static randomx_flags g_mine_flags;
static randomx_cache *g_rx_mine_cache;
static randomx_dataset *g_rx_mine_dataset;
static std::vector<randomx_vm*> g_rx_mine_vms;
static uint64_t g_current_miner_seed[4];  //must be the same size as xdag_hash_t

static void rx_abort(const char *msg){
	fprintf(stderr, "%s\n", msg);
#ifdef NDEBUG
	_exit(1);
#else
	abort();
#endif
}

static void * rx_seedthread(void *arg) {
	miner_seedinfo *si = (miner_seedinfo*)arg;
	xdag_info("init mine dataset with thread %lu start %lu count %lu",pthread_self(), si->si_start,si->si_count);
	randomx_init_dataset(g_rx_mine_dataset, si->si_cache, si->si_start, si->si_count);
	return NULL;
}

static void rx_mine_init_dataset(randomx_cache *rs_cache, const int thread_count) {
	if (thread_count > 1) {
		// 平均分配给每一个线程的dataset item
		unsigned long delta = randomx_dataset_item_count() / thread_count;
		unsigned long start = 0;
		int i;
		miner_seedinfo *si;
		pthread_t *st;

		si = (miner_seedinfo*)malloc(thread_count * sizeof(miner_seedinfo));
		if (si == NULL){
			rx_abort("Couldn't allocate RandomX mining threadinfo");
			return;
		}

		st = (pthread_t*)malloc(thread_count * sizeof(pthread_t));
		if (st == NULL) {
			free(si);
			rx_abort("Couldn't allocate RandomX mining threadlist");
			return;
		}

		// 记录每个线程的dataset item的起始索引，item个数
		uint32_t item_count = randomx_dataset_item_count();
		uint32_t item_count_per = item_count / thread_count;
		uint32_t item_count_remain = item_count % thread_count;

		xdag_info("miner init dataset count %u per %u remain %u",item_count,item_count_per,item_count_remain);

		// 给每个线程分配itemcount，最后一个线程使用剩余的所有的itemcount
		for (i=0; i < thread_count; i++) {
			auto count = item_count_per + (i == thread_count - 1 ? item_count_remain : 0);
			si[i].si_cache = rs_cache;
			si[i].si_start = start;   //起始索引
			si[i].si_count = count;   //结束个数
			start += count;
		}

		// 开启多线程，每个线程都初始化自己的dataset
		for (i=0; i < thread_count; i++) {
			pthread_create(&st[i],NULL, rx_seedthread, &si[i]);
		}

		// 等待线程初始化的结束
		for (i=0; i < thread_count; i++) {
			pthread_join(st[i],NULL);
		}
		free(st);
		free(si);
	} else {
		// 如果只有一个线程则使用dataset里面所有的item
		randomx_init_dataset(g_rx_mine_dataset, rs_cache, 0, randomx_dataset_item_count());
	}
}

int rx_mine_init_seed(void *seed_data, size_t seed_size,uint32_t init_thread_count){

	g_mine_flags = randomx_get_flags();

//	if (large_pages) {
//		flags |= RANDOMX_FLAG_LARGE_PAGES;
//	}
#ifndef __OpenBSD__
//	if (secure) {
//		g_mine_flags |= RANDOMX_FLAG_SECURE;
//	}
#endif

	g_mine_flags |= RANDOMX_FLAG_FULL_MEM;

	// printf randomx flags info
	if (g_mine_flags & RANDOMX_FLAG_ARGON2_AVX2) {
		xdag_info(" - rx mine Argon2 implementation: AVX2");
	}
	else if (g_mine_flags & RANDOMX_FLAG_ARGON2_SSSE3) {
		xdag_info(" - rx mine Argon2 implementation: SSSE3");
	}
	else {
		xdag_info(" - rx mine Argon2 implementation: reference");
	}

	if (g_mine_flags & RANDOMX_FLAG_FULL_MEM) {
		xdag_info(" - rx mine full memory mode (2080 MiB)");
	}
	else {
		xdag_info(" - rx mine light memory mode (256 MiB)");
	}

	if (g_mine_flags & RANDOMX_FLAG_JIT) {
		xdag_info(" - JIT compiled mode");
		if (g_mine_flags & RANDOMX_FLAG_SECURE) {
			xdag_info("(rx mine secure)");
		}
	}
	else {
		xdag_info(" - rx mine interpreted mode");
	}

	if (g_mine_flags & RANDOMX_FLAG_HARD_AES) {
		xdag_info(" - rx mine hardware AES mode");
	}
	else {
		xdag_info(" - rx mine software AES mode");
	}

	if (g_mine_flags & RANDOMX_FLAG_LARGE_PAGES) {
		xdag_info(" - rx mine large pages mode");
	}
	else {
		std::cout << " - rx mine small pages mode" << std::endl;
	}

	std::cout << "Initializing rx mine ";
	std::cout << " (" << init_thread_count << " thread" << (init_thread_count > 1 ? "s)" : ")");
	std::cout << " ..." << std::endl;

	if (randomx::selectArgonImpl(g_mine_flags)==NULL) {
		rx_abort("rx mine Unsupported Argon2 implementation");
		return -1;
	}
	if ((g_mine_flags & RANDOMX_FLAG_JIT) && !RANDOMX_HAVE_COMPILER) {
		rx_abort("rx mine JIT compilation is not supported on this platform. Try without --jit");
		return -1;
	}
	if (!(g_mine_flags & RANDOMX_FLAG_JIT) && RANDOMX_HAVE_COMPILER) {
		g_mine_flags |= RANDOMX_FLAG_JIT;
	}

	g_rx_mine_cache = randomx_alloc_cache(g_mine_flags);
	if (g_rx_mine_cache == nullptr) {
		rx_abort("rx mine can not alloc mine cache");
		return -1;
	}

	randomx_init_cache(g_rx_mine_cache, seed_data, seed_size);

	g_rx_mine_dataset = randomx_alloc_dataset(g_mine_flags);
	if (g_rx_mine_dataset == NULL) {
		rx_abort("rx mine alloc dataset failed");
		return -1;
	}

	// init dataset with muti-thread
	rx_mine_init_dataset(g_rx_mine_cache,init_thread_count);
	//randomx_release_cache(g_rx_mine_cache);
	//g_rx_mine_cache = NULL;
	xdag_info("rx mine rx mine init seed finished");
	return 0;
}

int rx_mine_alloc_vms(uint32_t mining_thread_count) {

	if(g_rx_mine_dataset==NULL){
		rx_abort("data set not allocated and initialized");
		return -1;
	}

	g_mining_thread_count = mining_thread_count;

	for(int i=0;i < g_mining_thread_count;++i){
		randomx_vm *vm=randomx_create_vm(g_mine_flags,g_rx_mine_cache,g_rx_mine_dataset);
		if(!vm){
			rx_abort("create randomx vm failed");
			return -1;
		}
		g_rx_mine_vms.push_back(vm);
	}
	printf("alloced %u vm for mining\n",g_mining_thread_count);
	return 0;
}

void rx_mine_hash(uint32_t thread_index,const void* data,size_t data_size,void* output_hash) {
	if(thread_index > g_mining_thread_count){
		rx_abort("thread index is larger than mining thread count");
		return;
	}

	if(g_rx_mine_vms[thread_index] == NULL){
		printf("thread index is %u\n",thread_index);
		rx_abort("rx mine vm not allocated");
		return;
	}

	randomx_calculate_hash(g_rx_mine_vms[thread_index],data,data_size,output_hash);
}

void rx_mine_free() {
	if (g_rx_mine_vms.size() != 0) {
		for(int i=0;i < g_mining_thread_count;++i){
			randomx_destroy_vm(g_rx_mine_vms[i]);
		}
		g_rx_mine_vms.clear();
	}

	if(g_rx_mine_dataset != NULL){
		randomx_release_dataset(g_rx_mine_dataset);
		g_rx_mine_dataset=NULL;
	}
	if(g_rx_mine_cache != NULL){
		randomx_release_cache(g_rx_mine_cache);
		g_rx_mine_cache=NULL;
	}
}

//uint64_t* get_current_rx_seed(){
//	return g_current_miner_seed;
//}