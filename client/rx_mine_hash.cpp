//
// Created by mac on 2020/6/11.
//

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
//#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <randomx.h>
#include "utils/log.h"

#include "rx_mine_hash.h"
#include "hash.h"

//struct for muti-thread init dataset
typedef struct miner_seedinfo {
	randomx_cache *si_cache;
	unsigned long si_start;
	unsigned long si_count;
} miner_seed_info;

//global randomx virables
static randomx_flags g_mine_flags;
static randomx_cache *g_rx_mine_cache = nullptr;
static randomx_dataset *g_rx_mine_dataset = nullptr;
static uint32_t g_mine_n_threads;

static pthread_mutex_t g_rx_dataset_mutex = PTHREAD_MUTEX_INITIALIZER;

static void rx_abort(const char *msg){
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

void rx_mine_init_flags(uint32_t init_thread_count) {

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
//    g_mine_flags |= RANDOMX_FLAG_LARGE_PAGES;

    // printf randomx flags info
    if (g_mine_flags & RANDOMX_FLAG_ARGON2_AVX2) {
        xdag_info(" - rx mine Argon2 implementation: AVX2");
    } else if (g_mine_flags & RANDOMX_FLAG_ARGON2_SSSE3) {
        xdag_info(" - rx mine Argon2 implementation: SSSE3");
    } else {
        xdag_info(" - rx mine Argon2 implementation: reference");
    }

    if (g_mine_flags & RANDOMX_FLAG_FULL_MEM) {
        xdag_info(" - rx mine full memory mode (2080 MiB)");
    } else {
        xdag_info(" - rx mine light memory mode (256 MiB)");
    }

    if (g_mine_flags & RANDOMX_FLAG_JIT) {
        xdag_info(" - JIT compiled mode");
        if (g_mine_flags & RANDOMX_FLAG_SECURE) {
            xdag_info("(rx mine secure)");
        }
    } else {
        xdag_info(" - rx mine interpreted mode");
    }

    if (g_mine_flags & RANDOMX_FLAG_HARD_AES) {
        xdag_info(" - rx mine hardware AES mode");
    } else {
        xdag_info(" - rx mine software AES mode");
    }

    if (g_mine_flags & RANDOMX_FLAG_LARGE_PAGES) {
        xdag_info(" - rx mine large pages mode");
    } else {
        xdag_info(" - rx mine small pages mode");
    }

    if (g_mine_flags & RANDOMX_FLAG_JIT) {
        xdag_info(" - rx mine JIT compiled mode");
    } else {
        xdag_info(" - rx mine interpreted mode");
    }
    g_mine_n_threads = init_thread_count;
    xdag_info("Initializing rx mine flag (%d  thread %s ...",
            init_thread_count, init_thread_count > 1 ? "s)" : ")");
}

static void * rx_seedthread(void *arg) {
	miner_seedinfo *si = (miner_seedinfo*)arg;
	xdag_info("init mine dataset with thread %lu start %lu count %lu",pthread_self(), si->si_start,si->si_count);
	randomx_init_dataset(g_rx_mine_dataset, si->si_cache, si->si_start, si->si_count);
	return NULL;
}

void rx_mine_init_dataset(void *seed_data, size_t seed_size) {
    pthread_mutex_lock(&g_rx_dataset_mutex);

    if (g_rx_mine_dataset == nullptr) {
        g_rx_mine_cache = randomx_alloc_cache(g_mine_flags);
        if (g_rx_mine_cache == nullptr) {
            rx_abort("rx mine can not alloc mine cache");
            pthread_mutex_unlock(&g_rx_dataset_mutex);
            return;
        }
        g_rx_mine_dataset = randomx_alloc_dataset(g_mine_flags);
        if (g_rx_mine_dataset == nullptr) {
            rx_abort("rx mine alloc dataset failed");
            pthread_mutex_unlock(&g_rx_dataset_mutex);
            return;
        }
    }
    randomx_init_cache(g_rx_mine_cache, seed_data, seed_size);

	if (g_mine_n_threads > 1) {
		unsigned long start = 0;
		int i;
		miner_seedinfo *si;
		pthread_t *st;

		si = (miner_seedinfo*)malloc(g_mine_n_threads * sizeof(miner_seedinfo));
		if (si == NULL){
			rx_abort("Couldn't allocate RandomX mining threadinfo");
            pthread_mutex_unlock(&g_rx_dataset_mutex);
			return;
		}

		st = (pthread_t*)malloc(g_mine_n_threads * sizeof(pthread_t));
		if (st == NULL) {
			free(si);
			rx_abort("Couldn't allocate RandomX mining threadlist");
            pthread_mutex_unlock(&g_rx_dataset_mutex);
			return;
		}

		// 记录每个线程的dataset item的起始索引，item个数
		uint32_t item_count = randomx_dataset_item_count();
		uint32_t item_count_per = item_count / g_mine_n_threads;
		uint32_t item_count_remain = item_count % g_mine_n_threads;

		xdag_info("miner init dataset count %u per %u remain %u",item_count,item_count_per,item_count_remain);

		// 给每个线程分配itemcount，最后一个线程使用剩余的所有的itemcount
		for (i=0; i < g_mine_n_threads; i++) {
			auto count = item_count_per + (i == g_mine_n_threads - 1 ? item_count_remain : 0);
			si[i].si_cache = g_rx_mine_cache;
			si[i].si_start = start;   //起始索引
			si[i].si_count = count;   //结束个数
			start += count;
		}

		// 开启多线程，每个线程都初始化自己的dataset
		for (i=0; i < g_mine_n_threads; i++) {
			pthread_create(&st[i],NULL, rx_seedthread, &si[i]);
		}

		// 等待线程初始化的结束
		for (i=0; i < g_mine_n_threads; i++) {
			pthread_join(st[i],NULL);
		}
		free(st);
		free(si);
	} else {
		// 如果只有一个线程则使用dataset里面所有的item
		randomx_init_dataset(g_rx_mine_dataset, g_rx_mine_cache, 0, randomx_dataset_item_count());
	}
    pthread_mutex_unlock(&g_rx_dataset_mutex);
}

uint64_t xdag_rx_mine_worker_hash(xdag_hash_t pre_hash, xdag_hash_t last_field ,uint64_t *nonce,
                                  uint64_t attemps, int step, xdag_hash_t output_hash){

    int pos=sizeof(xdag_hash_t) + sizeof(xdag_hashlow_t);
    xdag_hash_t hash0;
    uint64_t min_nonce=*nonce;

    if (g_rx_mine_dataset == nullptr) {
        rx_abort("rx mine dataset is null");
        return -1;
    }

    randomx_vm *vm = randomx_create_vm(g_mine_flags, NULL, g_rx_mine_dataset);
    if (vm == NULL) {
        rx_abort("rx mine alloc vm failed");
        return -1;
    }
    last_field[3]=min_nonce;
    uint8_t data2hash[sizeof(xdag_hash_t)+sizeof(xdag_hash_t)];
    memcpy(data2hash,pre_hash,sizeof(xdag_hash_t));
    memcpy(data2hash+sizeof(xdag_hash_t),last_field,sizeof(xdag_hash_t));

    randomx_calculate_hash_first(vm, data2hash, sizeof(data2hash));
    memset(output_hash, 0xff, sizeof(xdag_hash_t));

    for(int i=0;i < attemps;i++)
    {
        uint64_t curNonce = *nonce;
        *nonce += step;

        memcpy(data2hash+pos,nonce,sizeof(uint64_t));
        randomx_calculate_hash_next(vm, data2hash, sizeof(data2hash), hash0);
        if(xdag_cmphash(hash0,output_hash) < 0) {
            memcpy(output_hash, hash0, sizeof(xdag_hash_t));
            min_nonce = curNonce;
        }
    }
    randomx_destroy_vm(vm);

    xdag_info("*#*# rx final min hash %016llx%016llx%016llx%016llx",
              output_hash[3],output_hash[2],output_hash[1],output_hash[0]);
    xdag_info("*#*# rx final min nonce %016llx",min_nonce);

    return min_nonce;
}