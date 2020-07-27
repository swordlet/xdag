//
// Created by mac on 2020/6/11.
//

#ifndef XDAG_RX_MINE_HASH_H
#define XDAG_RX_MINE_HASH_H

#include <stdint.h>
#include <stdlib.h>

typedef uint64_t xdag_hash_t[4];
typedef uint64_t xdag_hashlow_t[3];

#ifdef __cplusplus
extern "C"
{
#endif

//init for randomx mining
extern void rx_mine_init_flags(uint32_t init_thread_count);
extern void rx_mine_init_dataset(void *seed_data, size_t seed_size);


extern uint64_t xdag_rx_mine_worker_hash(xdag_hash_t pre_hash, xdag_hash_t last_field ,uint64_t *nonce,
                                         uint64_t attempts, int step, xdag_hash_t hash);

#ifdef __cplusplus
}
#endif

#endif //XDAG_RX_MINE_HASH_H
