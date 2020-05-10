#ifndef XDAG_BASE58_H
#define XDAG_BASE58_H

#include <string.h>
#include <math.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif
extern int xdag_base58_encode(const unsigned char* data, size_t binsz, unsigned char** b58, size_t* b58sz);

#ifdef __cplusplus
};
#endif

#endif
