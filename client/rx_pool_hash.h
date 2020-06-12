//
// Created by mac on 2020/6/11.
//

#ifndef XDAG_RX_POOL_HASH_H
#define XDAG_RX_POOL_HASH_H

#include <stdint.h>
#include <stdlib.h>

#define SEEDHASH_EPOCH_BLOCKS	4096
#define SEEDHASH_EPOCH_LAG		128

#ifdef __cplusplus
extern "C"
{
#endif

//init the seed for randomx mining
extern int rx_pool_init_seed(void *seed_data, size_t seed_size);

//get randomx seed height
extern uint64_t rx_seedheight(const uint64_t height);

//get randomx seed heights
extern void rx_seedheights(const uint64_t height, uint64_t *seed_height, uint64_t *next_height);

//calc randomx hash
extern int rx_pool_calc_hash(void *seed_data, size_t seed_size,void* data,size_t data_size,void* output_hash);

#ifdef __cplusplus
}
#endif

#endif //XDAG_RX_POOL_HASH_H
