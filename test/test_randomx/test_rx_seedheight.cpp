//
// Created by mac on 2020/6/11.
//
#include <stdio.h>
#include <stdint.h>

#define SEEDHASH_EPOCH_BLOCKS_0	4096
#define SEEDHASH_EPOCH_LAG_0		128

uint64_t rx_seedheight(const uint64_t height)
{
	uint64_t s_height = (height <= SEEDHASH_EPOCH_BLOCKS_0 + SEEDHASH_EPOCH_LAG_0) ? 0 :
	                    (height - SEEDHASH_EPOCH_LAG_0 - 1) & ~(SEEDHASH_EPOCH_BLOCKS_0 - 1);
	return s_height;
}

void rx_seedheights(const uint64_t height, uint64_t *seed_height, uint64_t *next_height)
{
	*seed_height = rx_seedheight(height);
	*next_height = rx_seedheight(height + SEEDHASH_EPOCH_LAG_0);
}

int main(int argc,char* argv[])
{
	int h0=10;
	int h1=5000;
	int h2=10000;
	int h3=14000;
	int h4=20000;
	int h5=0xfff444;

	int r0=rx_seedheight(h0);
	int r1=rx_seedheight(h1);
	int r2=rx_seedheight(h2);
	int r3=rx_seedheight(h3);
	int r4=rx_seedheight(h4);
	int r5=rx_seedheight(h5);

	printf("r0 is %d\n",r0);
	printf("r1 is %d\n",r1);
	printf("r2 is %d\n",r2);
	printf("r3 is %d\n",r3);
	printf("r4 is %d\n",r4);
	printf("r5 is %d\n",r5);

	return 0;
}