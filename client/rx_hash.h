//
// Created by mac on 2020/6/6.
//

#ifndef XDAG_RX_HASH_H
#define XDAG_RX_HASH_H

#include <stdint.h>
#include "hash.h"
#include "block.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern int xdag_rx_init(const xdag_hash_t key);
extern void xdag_get_rx_key(xdag_hash_t input);
extern int xdag_rx_hash(void* data, size_t data_size, xdag_hash_t output_hash);
extern void xdag_rx_hash_final(xdag_hash_t pre_hash, struct xdag_field *field, uint64_t *nonce,xdag_hash_t output_hash);
extern int xdag_rx_calculate_final_hash(void* data, size_t data_size, xdag_hash_t output_hash);
extern int xdag_rx_final_hash_init(const xdag_hash_t key);
extern uint64_t xdag_rx_mine_slow_hash(xdag_hash_t pre_hash, struct xdag_field *field, uint64_t *nonce,
                                       int attemps, int threads_count, xdag_hash_t output_hash);

extern void xdag_rx_deinit();

#ifdef __cplusplus
}
#endif

#endif //XDAG_RX_HASH_H
