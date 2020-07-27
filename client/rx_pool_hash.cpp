//
// Created by mac on 2020/6/11.
//


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "utils/log.h"
#include "rx_pool_hash.h"

uint64_t g_rx_pool_mem_index = 0;
struct rx_pool_mem g_rx_pool_mem[2];

//global randomx variables
//static uint64_t g_current_pool_seed[4]; //size must be as same as xdag_hash_t
//static uint32_t g_mining_thread_count=1;
static randomx_flags g_pool_flags = RANDOMX_FLAG_DEFAULT;
static randomx_cache *g_rx_pool_cache = nullptr;
static randomx_dataset *g_rx_pool_dataset = nullptr;
// randomx vm for every thread using rx hash
static randomx_vm *g_rx_pool_vm = nullptr; // rx vm for pool_main_thread
//static randomx_vm *g_rx_create_block_vm = nullptr; // rx vm for xdag_create_block
static randomx_vm *g_rx_block_vm = nullptr; // rx vm for add_block_nolock

static pthread_rwlock_t  g_rx_cache_rwlock = PTHREAD_RWLOCK_INITIALIZER; //rwlock for init & update rx cache

//struct for muti-thread init dataset
typedef struct miner_seedinfo {
	randomx_cache *si_cache;
	unsigned long si_start;
	unsigned long si_count;
} pool_seed_info;

xtime_t rx_seed_time(const xtime_t time, int xdag_testnet){
    xtime_t s_time;
    if(xdag_testnet){
        s_time = (MAIN_TIME(time) - SEEDHASH_EPOCH_TESTNET_LAG - 1) & ~(SEEDHASH_EPOCH_TESTNET_BLOCKS - 1);
    } else {
        s_time = (MAIN_TIME(time) - SEEDHASH_EPOCH_LAG - 1) & ~(SEEDHASH_EPOCH_BLOCKS - 1);
    }
    s_time = s_time << 16 | 0xffff;
    if (s_time < g_xdag_era) {
        return g_xdag_era;
    }
    return s_time;
}

void rx_seed_times(const xtime_t time, xtime_t *seed_time, xtime_t *next_time, int xdag_testnet){
    *seed_time = rx_seed_time(time, xdag_testnet);
    xtime_t lag_time = xdag_testnet ? SEEDHASH_EPOCH_TESTNET_LAG : SEEDHASH_EPOCH_LAG;
    lag_time = lag_time << 16;
    *next_time = rx_seed_time(time + lag_time, xdag_testnet);
}

uint64_t rx_seed_height(const uint64_t height, int xdag_testnet){
	uint64_t s_height;
	if(xdag_testnet){

        s_height = (height <= SEEDHASH_EPOCH_TESTNET_BLOCKS + SEEDHASH_EPOCH_TESTNET_LAG) ? 0 :
                   (height - SEEDHASH_EPOCH_TESTNET_LAG - 1) & ~(SEEDHASH_EPOCH_TESTNET_BLOCKS - 1);
	} else {
        s_height = (height <= SEEDHASH_EPOCH_BLOCKS + SEEDHASH_EPOCH_LAG) ? 0 :
                   (height - SEEDHASH_EPOCH_LAG - 1) & ~(SEEDHASH_EPOCH_BLOCKS - 1);
	}

	return s_height;
}

void rx_seed_heights(const uint64_t height, uint64_t *seed_height, uint64_t *next_height, int xdag_testnet){
	*seed_height = rx_seed_height(height, xdag_testnet);
	*next_height = xdag_testnet ? rx_seed_height(height + SEEDHASH_EPOCH_TESTNET_LAG, xdag_testnet) :
                   rx_seed_height(height + SEEDHASH_EPOCH_LAG, xdag_testnet);
}

void rx_pool_init_flags(int is_full_mem) {

    g_pool_flags = randomx_get_flags();

//	if (large_pages) {
//		flags |= RANDOMX_FLAG_LARGE_PAGES;
//	}
#ifndef __OpenBSD__
//	if (secure) {
//		g_pool_flags |= RANDOMX_FLAG_SECURE;
//	}
#endif
    if (is_full_mem) {
        g_pool_flags |= RANDOMX_FLAG_FULL_MEM;
    }
    g_pool_flags |= RANDOMX_FLAG_LARGE_PAGES;

    // printf randomx flags info
    if (g_pool_flags & RANDOMX_FLAG_ARGON2_AVX2) {
        xdag_info(" - rx pool Argon2 implementation: AVX2");
    } else if (g_pool_flags & RANDOMX_FLAG_ARGON2_SSSE3) {
        xdag_info(" - rx pool Argon2 implementation: SSSE3");
    } else {
        xdag_info(" - rx pool Argon2 implementation: reference");
    }

    if (g_pool_flags & RANDOMX_FLAG_FULL_MEM) {
        xdag_info(" - rx pool full memory mode (2080 MiB)");
    } else {
        xdag_info(" - rx pool light memory mode (256 MiB)");
    }

    if (g_pool_flags & RANDOMX_FLAG_JIT) {
        xdag_info(" - JIT compiled mode");
        if (g_pool_flags & RANDOMX_FLAG_SECURE) {
            xdag_info("(rx pool secure)");
        }
    } else {
        xdag_info(" - rx pool interpreted mode");
    }

    if (g_pool_flags & RANDOMX_FLAG_HARD_AES) {
        xdag_info(" - rx pool hardware AES mode");
    } else {
        xdag_info(" - rx pool software AES mode");
    }

    if (g_pool_flags & RANDOMX_FLAG_LARGE_PAGES) {
        xdag_info(" - rx pool large pages mode");
    } else {
        xdag_info(" - rx pool small pages mode");
    }

    if (g_pool_flags & RANDOMX_FLAG_JIT) {
        xdag_info(" - rx pool JIT compiled mode");
    } else {
        xdag_info(" - rx pool interpreted mode");
    }
}

static void * init_pool_dataset_thread(void *arg) {
	miner_seedinfo *si = (miner_seedinfo*)arg;
	xdag_info("init pool dataset with thread %lu start %lu count %lu",pthread_self(), si->si_start,si->si_count);
	randomx_init_dataset(g_rx_pool_dataset, si->si_cache, si->si_start, si->si_count);
	return NULL;
}

static void rx_pool_init_dataset(randomx_cache *rx_cache, const int thread_count) {
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
			si[i].si_cache = rx_cache;
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
        randomx_release_cache(g_rx_pool_cache);
    	g_rx_pool_cache = nullptr;
	} else {
		// 如果只有一个线程则使用dataset里面所有的item
		randomx_init_dataset(g_rx_pool_dataset, rx_cache, 0, randomx_dataset_item_count());
	}
}

int rx_pool_init_seed(void *seed_data, size_t seed_size, xtime_t s_time)
{
    pthread_rwlock_wrlock(&g_rx_cache_rwlock);

    xdag_fatal("alloc pool rx cache ...");
    if(g_rx_pool_cache == nullptr){
        g_rx_pool_cache = randomx_alloc_cache(g_pool_flags);
        if (g_rx_pool_cache == nullptr) {
            xdag_fatal("alloc rx cache failed");
            pthread_rwlock_unlock(&g_rx_cache_rwlock);
            return -1;
        }
        randomx_init_cache(g_rx_pool_cache, seed_data, seed_size);
    }

    if(g_pool_flags & RANDOMX_FLAG_FULL_MEM && g_rx_pool_dataset == nullptr){
        xdag_info("alloc pool rx dataset ...");
        g_rx_pool_dataset = randomx_alloc_dataset(g_pool_flags);
        if (g_rx_pool_dataset == nullptr) {
            xdag_fatal("alloc dataset failed");
            pthread_rwlock_unlock(&g_rx_cache_rwlock);
            return -1;
        }
        // init dataset with 4 thread
        rx_pool_init_dataset(g_rx_pool_cache,4);

    }

    xdag_info("alloc pool rx vm ...");
    if(g_rx_pool_vm == nullptr){
        g_rx_pool_vm = randomx_create_vm(g_pool_flags,g_rx_pool_cache,nullptr);
        if (g_rx_pool_vm == nullptr) {
            xdag_fatal("create pool vm failed");
            pthread_rwlock_unlock(&g_rx_cache_rwlock);
            return -1;
        }
    }
    xdag_info("alloc pool vm finished");

    xdag_info("alloc block rx vm ...");
    if(g_rx_block_vm == nullptr){
        g_rx_block_vm = randomx_create_vm(g_pool_flags, g_rx_pool_cache, nullptr);
        if (g_rx_block_vm == nullptr) {
            xdag_fatal("create block vm failed");
            pthread_rwlock_unlock(&g_rx_cache_rwlock);
            return -1;
        }
    }
    xdag_info("alloc block rx vm finished");

    const uint64_t rx_mem_index = g_rx_pool_mem_index + 1;
    struct rx_pool_mem *rx_memory = &g_rx_pool_mem[rx_mem_index & 1];
    rx_memory->rx_cache = g_rx_pool_cache;
    rx_memory->rx_dataset = g_rx_pool_dataset;
    memcpy(rx_memory->seed, seed_data, seed_size);
    rx_memory->s_time = s_time;
    g_rx_pool_mem_index = rx_mem_index;

    pthread_rwlock_unlock(&g_rx_cache_rwlock);
	return 0;
}

// pool verify share hash from miner
int rx_pool_calc_hash(void* data,size_t data_size,void* output_hash){

    pthread_rwlock_rdlock(&g_rx_cache_rwlock);
	randomx_calculate_hash(g_rx_pool_vm,data,data_size,output_hash);
	pthread_rwlock_unlock(&g_rx_cache_rwlock);

	return 0;
}

//// rx hash of xdag_create_block
//int rx_create_block_hash(void* data,size_t data_size,void* output_hash){
//
//    pthread_rwlock_rdlock(&g_rx_cache_rwlock);
//    randomx_calculate_hash(g_rx_create_block_vm, data, data_size, output_hash);
//    pthread_rwlock_unlock(&g_rx_cache_rwlock);
//
//    return 0;
//}

// rx hash of add_block_nolock
int rx_block_hash(void* data,size_t data_size,void* output_hash){

    pthread_rwlock_rdlock(&g_rx_cache_rwlock);
    randomx_calculate_hash(g_rx_block_vm, data, data_size, output_hash);
    pthread_rwlock_unlock(&g_rx_cache_rwlock);

    return 0;
}

//// prepare next seed
//int rx_pool_prepare_seed(void *seed_data, size_t seed_size, xtime_t s_time)
//{
//    const uint64_t rx_mem_index = g_rx_pool_mem_index + 1;
//    struct rx_pool_mem *rx_memory = &g_rx_pool_mem[rx_mem_index & 1];
//    memcpy(rx_memory->seed, seed_data, seed_size);
//    rx_memory->s_time = s_time;
//    return 0;
//}

int rx_pool_update_seed(void *seed_data, size_t seed_size, xtime_t s_time)
{
    pthread_rwlock_wrlock(&g_rx_cache_rwlock);

    if(g_pool_flags & RANDOMX_FLAG_FULL_MEM){
        g_rx_pool_cache = randomx_alloc_cache(g_pool_flags);
        if (g_rx_pool_cache == nullptr) {
            xdag_fatal("alloc rx cache failed");
            pthread_rwlock_unlock(&g_rx_cache_rwlock);
            return -1;
        }
    }

    if(g_rx_pool_cache != nullptr) {
        xdag_fatal("update pool rx cache ...");
        randomx_init_cache(g_rx_pool_cache, seed_data, seed_size);
    }

    if((g_pool_flags & RANDOMX_FLAG_FULL_MEM) && g_rx_pool_dataset != nullptr){
        xdag_info("update pool rx dataset ...");
        // init dataset with 4 thread
        rx_pool_init_dataset(g_rx_pool_cache,4);
    }

    xdag_info("update pool rx vm ...");
    if(g_rx_pool_vm != nullptr){
        if (g_pool_flags & RANDOMX_FLAG_FULL_MEM) {
            randomx_vm_set_dataset(g_rx_pool_vm,g_rx_pool_dataset);
        } else {
            randomx_vm_set_cache(g_rx_pool_vm, g_rx_pool_cache);
        }
    }
    xdag_info("update pool vm finished");

    xdag_info("update block rx vm ...");
    if(g_rx_block_vm != nullptr){
        if (g_pool_flags & RANDOMX_FLAG_FULL_MEM) {
            randomx_vm_set_dataset(g_rx_block_vm,g_rx_pool_dataset);
        } else {
            randomx_vm_set_cache(g_rx_block_vm, g_rx_pool_cache);
        }
    }
    xdag_info("update block rx vm finished");

    const uint64_t rx_mem_index = g_rx_pool_mem_index + 1;
    struct rx_pool_mem *rx_memory = &g_rx_pool_mem[rx_mem_index & 1];
    rx_memory->rx_cache = g_rx_pool_cache;
    rx_memory->rx_dataset = g_rx_pool_dataset;
    memcpy(rx_memory->seed, seed_data, seed_size);
    rx_memory->s_time = s_time;
    g_rx_pool_mem_index = rx_mem_index;

    pthread_rwlock_unlock(&g_rx_cache_rwlock);
    return 0;
}