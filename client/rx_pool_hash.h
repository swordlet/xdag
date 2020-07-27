//
// Created by mac on 2020/6/11.
//

#ifndef XDAG_RX_POOL_HASH_H
#define XDAG_RX_POOL_HASH_H

#include <randomx.h>
#include <stdint.h>
#include <stdlib.h>
#include "hash.h"
#include "time.h"

#define SEEDHASH_EPOCH_BLOCKS	4096
#define SEEDHASH_EPOCH_LAG		128

#define SEEDHASH_EPOCH_TESTNET_BLOCKS	16
#define SEEDHASH_EPOCH_TESTNET_LAG		4

struct rx_pool_mem {
    xdag_hash_t seed;
    xtime_t s_time;
    randomx_cache * rx_cache;
    randomx_dataset *rx_dataset;
};

#ifdef __cplusplus
extern "C"
{
#endif
extern struct rx_pool_mem g_rx_pool_mem[2];
extern uint64_t g_rx_pool_mem_index;


//init the seed for randomx mining
extern int rx_pool_init_seed(void *seed_data, size_t seed_size, xtime_t s_time);

//get randomx seed height
extern uint64_t rx_seed_height(const uint64_t height, int xdag_testnet);

//get randomx seed heights
extern void rx_seed_heights(const uint64_t height, uint64_t *seed_height, uint64_t *next_height, int xdag_testnet);

//calc randomx hash
extern int rx_pool_calc_hash(void* data,size_t data_size,void* output_hash);

//calc new main block hash
extern int rx_block_hash(void* data,size_t data_size,void* output_hash);

//get randomx seed time
extern xtime_t rx_seed_time(const xtime_t time, int xdag_testnet);

//get randomx seed times
extern void rx_seed_times(const xtime_t time, xtime_t *seed_time, xtime_t *next_time, int xdag_testnet);

extern void rx_pool_init_flags(int is_full_mem);

extern int rx_pool_update_seed(void *seed_data, size_t seed_size, xtime_t s_time);

#ifdef __cplusplus
}
#endif

#endif //XDAG_RX_POOL_HASH_H
