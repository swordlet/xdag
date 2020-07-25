/**
 * 只能串行调用的randomx hash函数
 * */

#ifndef XDAG_RX_HASHS_H
#define XDAG_RX_HASHS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

void rx_hashs_allocate_state(void);
void rx_hashs_free_state(void);
void rx_slow_hashs(const char *seedhash, const void *data, size_t length, uint8_t *hash);
void rx_reorg_s(const uint64_t split_height);
void rx_stop_mining_s(void);

#ifdef __cplusplus
}
#endif

#endif //XDAG_RX_HASHS_H
