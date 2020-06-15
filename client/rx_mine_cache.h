//
// Created by mac on 2020/6/15.
//

#ifndef XDAG_RX_MINE_CACHE_H
#define XDAG_RX_MINE_CACHE_H

#include <cstdint>
#include <string>
#include "hash.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern int get_rx_seed_by_prehash(const xdag_hash_t prehash,xdag_hash_t seed);

extern int enqueue_rx_task(const xdag_hash_t prehash,const xdag_hash_t seed);

extern int get_rx_latest_prehash(xdag_hash_t prehash);

extern void printf_all_rx_tasks();

#ifdef __cplusplus
}
#endif

#endif //XDAG_RX_MINE_CACHE_H
