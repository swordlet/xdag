/**
 * 可以进行并行调用的randomx hash
 * */
#ifndef XDAG_RX_SLOW_HASH_P
#define XDAG_RX_SLOW_HASH_P

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

void rx_hashp_allocate_state(void);
void rx_hashp_free_state(void);
void rx_slow_hashp(const uint64_t mainheight, const uint64_t seedheight, const char *seedhash, const void *data, size_t length, uint8_t *hash, int miners, int is_alt);
void rx_reorg_p(const uint64_t split_height);
void rx_stop_mining_p(void);

#ifdef __cplusplus
}
#endif

#endif //XDAG_RX_SLOW_HASH_P_H
