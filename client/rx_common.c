//
// Created by mac on 2020/7/25.
//

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "rx_common.h"
#include "randomx.h"
#include "c_threads.h"

int disabled_flags(void) {
	static int flags = -1;

	if (flags != -1) {
		return flags;
	}

	const char *env = getenv("XDAG_RANDOMX_UMASK");
	if (!env) {
		flags = 0;
	}
	else {
		char* endptr;
		long int value = strtol(env, &endptr, 0);
		if (endptr != env && value >= 0 && value < INT_MAX) {
			flags = value;
		}
		else {
			flags = 0;
		}
	}

	return flags;
}

int enabled_flags(void) {
	static int flags = -1;

	if (flags != -1) {
		return flags;
	}

	flags = randomx_get_flags();

	return flags;
}

uint64_t rx_seedheight(const uint64_t height) {
	// 如果高度小于2048+64个，则直接以高度为0的块hash作为种子
	// 否则以 高度为 (当前高度-64-1) & ~(2048-1) 的高度的块的hash作为种子
	// 大改就是向前2048个块作为种子
	uint64_t s_height =  (height <= SEEDHASH_EPOCH_BLOCKS + SEEDHASH_EPOCH_LAG) ? 0 :
	                     (height - SEEDHASH_EPOCH_LAG - 1) & ~(SEEDHASH_EPOCH_BLOCKS-1);
	return s_height;
}

void rx_seedheights(const uint64_t height, uint64_t *seedheight, uint64_t *nextheight) {
	*seedheight = rx_seedheight(height);
	*nextheight = rx_seedheight(height + SEEDHASH_EPOCH_LAG);
}






