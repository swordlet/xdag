//
// Created by mac on 2020/6/15.
//

#ifndef XDAG_RX_MINE_H
#define XDAG_RX_MINE_H

#include <stdio.h>
#include "block.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int g_rx_auto_swith_pool;

/* a number of mining threads */
extern int g_rx_mining_threads;

/* changes the number of mining threads */
extern int xdag_rx_mining_start(int n_mining_threads);

/* initialization of connection the miner to pool */
extern int xdag_rx_initialize_miner(const char *pool_address);

extern void *rx_miner_net_thread(void *arg);

/* send block to network via pool */
extern int xdag_rx_send_block_via_pool(struct xdag_block *block);

/* picks random pool from the list of pools */
extern int xdag_rx_pick_pool(char *pool_address);

#ifdef __cplusplus
};
#endif


#endif //XDAG_RX_MINE_H
