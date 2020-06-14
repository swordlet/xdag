//
// Created by mac on 2020/6/11.
//

#ifndef XDAG_RX_MINE_HASH_H
#define XDAG_RX_MINE_HASH_H

#include <stdint.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C"
{
#endif

//init the seed for randomx mining
extern int rx_mine_init_seed(void *seed_data, size_t seed_size,uint32_t init_thread_count);

//get randomx mining hash,calculate the min hash
extern void rx_mine_hash(uint32_t thread_index,const void* data,size_t data_size,void* output_hash);

//alloc vms after init seed
extern int rx_mine_alloc_vms(uint32_t mining_thread_count);

//free the resource of randomx
extern void rx_mine_free();

extern int rx_mine_calc_first_hash(void *seed_data, size_t seed_size,void* data,size_t data_size,void* output_hash);

extern uint64_t* get_current_rx_seed();

#ifdef __cplusplus
}
#endif

#endif //XDAG_RX_MINE_HASH_H
