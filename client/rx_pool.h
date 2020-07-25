//
// Created by mac on 2020/6/19.
//

#ifndef XDAG_RX_POOL_H
#define XDAG_RX_POOL_H

#include "hash.h"

#define MAX_CONNECTIONS_COUNT          8192
#define CONFIRMATIONS_COUNT            16

enum rx_disconnect_type
{
	RX_DISCONNECT_INIT = 0,
	RX_DISCONNECT_BY_ADRESS = 1,
	RX_DISCONNECT_BY_IP = 2,
	RX_DISCONNECT_ALL = 3
};

extern xdag_hash_t g_xdag_mined_hashes[CONFIRMATIONS_COUNT];
extern xdag_hash_t g_xdag_mined_nonce[CONFIRMATIONS_COUNT];
extern xdag_remark_t rx_g_pool_tag;
extern int g_pool_has_tag;

/* initialization of the pool */
extern int rx_initialize_pool(const char *pool_arg);

/* gets pool parameters as a string, 0 - if the pool is disabled */
extern char *rx_pool_get_config(char *buf);

/* sets pool parameters */
extern int rx_pool_set_config(const char *pool_config);

/* output to the file a list of miners */
extern int rx_print_miners(FILE *out, int printOnlyConnections);

// prints miner's stats
extern int rx_print_miner_stats(const char* address, FILE *out);

// disconnect connections by condition
// condition type: all, ip or address
// value: address of ip depending on type
extern void rx_disconnect_connections(enum rx_disconnect_type type, char *value);

// completes global mining thread
void rx_pool_finish(void);

#endif //XDAG_RX_POOL_H
