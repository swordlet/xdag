//
// Created by mac on 2020/7/23.
//

#ifndef XDAG_RX_SLOW_HASH_H
#define XDAG_RX_SLOW_HASH_H

#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif

enum {
	HASH_SIZE = 32,
	HASH_DATA_AREA = 136
};

#define RX_BLOCK_VERSION	1
void rx_slow_hash_allocate_state(void);
void rx_slow_hash_free_state(void);
uint64_t rx_seedheight(const uint64_t height);
void rx_seedheights(const uint64_t height, uint64_t *seed_height, uint64_t *next_height);
void rx_slow_hash(const uint64_t mainheight, const uint64_t seedheight, const char *seedhash, const void *data, size_t length, char *hash, int miners, int is_alt);
void rx_reorg(const uint64_t split_height);
void rx_stop_mining(void);

#ifdef __cplusplus
}
#endif

#endif //XDAG_RX_SLOW_HASH_H
