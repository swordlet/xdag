//
// Created by mac on 2020/6/6.
//

#include <randomx.h>
#include <string.h>
#include <log.h>
#include <stdio.h>
#include "rx_hash.h"
#include "hash.h"
#include "address.h"
#include "block.h"

static randomx_cache *g_rx_cache = nullptr;
static randomx_dataset *g_rx_dataset = nullptr;
static randomx_vm *g_rx_vm = nullptr;
static xdag_hash_t rx_key;

static randomx_cache *g_rx_final_hash_cache = nullptr;
static randomx_dataset *g_rx_final_hash_dataset = nullptr;
static randomx_vm *g_rx_final_hash_vm = nullptr;

int xdag_rx_final_hash_init(const xdag_hash_t key)
{
	if(g_rx_final_hash_vm != nullptr)
	{
		randomx_destroy_vm(g_rx_final_hash_vm);
	}

	if(g_rx_final_hash_dataset != nullptr)
	{
		randomx_release_dataset(g_rx_final_hash_dataset);
	}

	if(g_rx_final_hash_cache != nullptr)
	{
		randomx_release_cache(g_rx_final_hash_cache);
	}

	//TODO: enable flags based on cpu features
	if((g_rx_final_hash_cache=randomx_alloc_cache(RANDOMX_FLAG_DEFAULT)) == nullptr)
	{
		xdag_err("rx hash final cache alloc failed");
		return -1;
	}

	if((g_rx_final_hash_dataset=randomx_alloc_dataset(RANDOMX_FLAG_DEFAULT)) == nullptr)
	{
		xdag_err("rx final hash dataset alloc failed");
		return -1;
	}

	randomx_init_cache(g_rx_final_hash_cache,key,sizeof(xdag_hash_t));
	if((g_rx_final_hash_vm=randomx_create_vm(RANDOMX_FLAG_DEFAULT,g_rx_final_hash_cache,g_rx_final_hash_dataset))== nullptr)
	{
		xdag_err("rx vm final hash alloc failed");
		return -1;
	}

	return 0;
}

int xdag_rx_init(const xdag_hash_t key)
{
	if(g_rx_vm != nullptr)
	{
		randomx_destroy_vm(g_rx_vm);
	}

	if(g_rx_dataset != nullptr)
	{
		randomx_release_dataset(g_rx_dataset);
	}

	if(g_rx_vm != nullptr)
	{
		randomx_release_cache(g_rx_cache);
	}

	memcpy(rx_key,key, sizeof(xdag_hash_t));

	//TODO: enable flags based on cpu features
	if((g_rx_cache=randomx_alloc_cache(RANDOMX_FLAG_JIT)) == nullptr)
	{
		xdag_err("rx cache alloc failed");
		return -1;
	}

	if((g_rx_dataset=randomx_alloc_dataset(RANDOMX_FLAG_JIT)) == nullptr)
	{
		xdag_err("rx dataset alloc failed");
		return -1;
	}

	randomx_init_cache(g_rx_cache,rx_key,sizeof(xdag_hash_t));
	if((g_rx_vm=randomx_create_vm(RANDOMX_FLAG_JIT,g_rx_cache,g_rx_dataset))== nullptr)
	{
		xdag_err("rx vm alloc failed");
		return -1;
	}

	return 0;
}

void xdag_get_rx_key(xdag_hash_t input)
{
	memcpy(input,rx_key,sizeof(xdag_hash_t));
}

int xdag_rx_hash(void* data, size_t data_size, xdag_hash_t output_hash)
{
	if(g_rx_cache == nullptr || g_rx_dataset == nullptr || g_rx_vm == nullptr)
	{
		xdag_err("xdag rx not initialized");
		return -1;
	}

	randomx_calculate_hash(g_rx_vm,data,data_size,output_hash);

	return 0;
}

int xdag_rx_calculate_final_hash(void* data, size_t data_size, xdag_hash_t output_hash)
{
	if(g_rx_final_hash_cache == nullptr ||
		g_rx_final_hash_dataset == nullptr ||
		g_rx_final_hash_vm == nullptr)
	{
		xdag_err("xdag rx final hash not initialized");
		return -1;
	}

	randomx_calculate_hash(g_rx_final_hash_vm,data,data_size,output_hash);

	return 0;
}

void xdag_rx_hash_final(xdag_hash_t pre_hash, struct xdag_field *field, uint64_t *nonce,xdag_hash_t output_hash)
{
	field->amount=*nonce;
	uint8_t data2hash[sizeof(xdag_hash_t)+sizeof(struct xdag_field)];
	memcpy(data2hash,pre_hash,sizeof(xdag_hash_t));
	memcpy(data2hash+sizeof(xdag_hash_t),field,sizeof(struct xdag_field));
	xdag_rx_calculate_final_hash(data2hash, sizeof(data2hash), output_hash);
}

uint64_t xdag_rx_mine_slow_hash(xdag_hash_t pre_hash, struct xdag_field *field, uint64_t *nonce,
                                int attemps, int threads_count, xdag_hash_t output_hash)
{
	int pos=0;
	xdag_hash_t hash0;
	uint64_t nonce0=*nonce;
	uint64_t min_nonce=nonce0;
	struct xdag_field field0;
	field->amount=min_nonce;

	memcpy(&field0,field,sizeof(xdag_field));
	uint8_t data2hash[sizeof(xdag_hash_t)+sizeof(struct xdag_field)];
	memcpy(data2hash,pre_hash,sizeof(xdag_hash_t));
	memcpy(data2hash+sizeof(xdag_hash_t),&field0,sizeof(struct xdag_field));

	xdag_rx_hash(data2hash, sizeof(data2hash), output_hash);
	xdag_info("xdag mine hash first %016llx%016llx%016llx%016llx",output_hash[0],output_hash[1],output_hash[2],output_hash[3]);

	for(int i=0;i < attemps;i++)
	{
		field0.amount=nonce0;
		memcpy(data2hash+sizeof(xdag_hash_t),&field0,sizeof(struct xdag_field));
		xdag_rx_hash(data2hash, sizeof(data2hash), hash0);
		if(xdag_cmphash(hash0,output_hash) < 0)
		{
			memcpy(output_hash, hash0, sizeof(xdag_hash_t));
			min_nonce = nonce0;
			field->amount=min_nonce;
			xdag_info("xdag mine pre hash %016llx%016llx%016llx%016llx find min hash %016llx%016llx%016llx%016llx",
			    pre_hash[0],pre_hash[1],pre_hash[2],pre_hash[3],
					output_hash[0],output_hash[1],output_hash[2],output_hash[3]);
			xdag_info("xdag mine min nonce %016llx",min_nonce);
		}
		nonce0+=threads_count;
	}

	field->amount=min_nonce;
	memcpy(data2hash+sizeof(xdag_hash_t),field,sizeof(struct xdag_field));
	xdag_rx_hash(data2hash, sizeof(data2hash), output_hash);

	xdag_info("xdag mine pre hash %016llx%016llx%016llx%016llx final find min hash %016llx%016llx%016llx%016llx",
	    pre_hash[0],pre_hash[1],pre_hash[2],pre_hash[3],
			output_hash[0],output_hash[1],output_hash[2],output_hash[3]);
	xdag_info("xdag mine final min nonce %016llx",min_nonce);

	return min_nonce;
}

void xdag_rx_deinit()
{
	if(g_rx_vm != nullptr)
	{
		randomx_destroy_vm(g_rx_vm);
	}

	if(g_rx_dataset != nullptr)
	{
		randomx_release_dataset(g_rx_dataset);
	}

	if(g_rx_vm != nullptr)
	{
		randomx_release_cache(g_rx_cache);
	}
}