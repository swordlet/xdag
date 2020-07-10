//
// Created by mac on 2020/6/15.
//

#ifndef XDAG_RX_MINE_CACHE_H
#define XDAG_RX_MINE_CACHE_H

#include <stdint.h>
#include "mining_common.h"
#include "hash.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern int get_rx_task_cache_size();

extern int get_rx_task_by_idx(uint64_t index,rx_pool_task *task);

extern int get_rx_task_by_prehash(const xdag_hash_t prehash,rx_pool_task *task);

extern int enqueue_rx_task(rx_pool_task task);

extern int update_rx_task_by_prehash(const xdag_hash_t prehash,rx_pool_task task);

extern int get_rx_latest_prehash(xdag_hash_t prehash);

extern int get_rx_latest_task(rx_pool_task *task);

extern int get_remain_task_by_seqno(uint64_t seqno,struct xdag_field* fields,int* filed_count);

extern void printf_all_rx_tasks();

#ifdef __cplusplus
}
#endif

#endif //XDAG_RX_MINE_CACHE_H
