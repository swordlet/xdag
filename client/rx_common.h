//
// Created by mac on 2020/7/25.
//

#ifndef XDAG_RX_COMMON_H
#define XDAG_RX_COMMON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_MSC_VER)
#define THREADV __declspec(thread)
#else
#define THREADV __thread
#endif

#define RX_HASH_SIZE 32
#define RX_SEED_SIZE 24
#define RX_HASH_DATA_AREA 136
//#define SEEDHASH_EPOCH_BLOCKS	4096
//#define SEEDHASH_EPOCH_LAG		128
#define SEEDHASH_EPOCH_BLOCKS	8
#define SEEDHASH_EPOCH_LAG		2
#define RX_BLOCK_VERSION	1
#define RX_DATASET_INIT_THREAD_COUNT   4

int disabled_flags(void);
int enabled_flags(void);
uint64_t rx_seedheight(const uint64_t height);
void rx_seedheights(const uint64_t height, uint64_t *seed_height, uint64_t *next_height);

#ifdef __cplusplus
}
#endif

#endif //XDAG_RX_COMMON_H