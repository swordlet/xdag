#include <gtest/gtest.h>
#include <randomx.h>
#include <VirtualMemory.h>
#include <configuration.h>

constexpr char hexmap[] = "0123456789abcdef";
inline void outputHex(std::ostream& os, const char* data, int length) {
	for (int i = 0; i < length; ++i) {
		os << hexmap[(data[i] & 0xF0) >> 4];
		os << hexmap[data[i] & 0x0F];
	}
	std::cout << std::endl;
}

TEST(testRandomX,randomX){

	//config randomx parameter
	randomx_apply_config(RandomX_MoneroConfig);

	std::string key="test fast mode rx";
	xmrig::VirtualMemory *cache_memory = new xmrig::VirtualMemory(RANDOMX_CACHE_MAX_SIZE, true, false, false, 0);
	randomx_cache* cache=randomx_create_cache(RANDOMX_FLAG_JIT, cache_memory->raw());
	randomx_init_cache(cache,key.c_str(),key.length());

	xmrig::VirtualMemory *dataset_memory = new xmrig::VirtualMemory(RANDOMX_DATASET_MAX_SIZE, true, false, false, 0);
	randomx_dataset* dataset=randomx_create_dataset(dataset_memory->raw());

	xmrig::VirtualMemory *scratchpad_memory=new xmrig::VirtualMemory(RANDOMX_SCRATCHPAD_L3_MAX_SIZE, true, false, false, 0);
	randomx_vm* vm=randomx_create_vm(RANDOMX_FLAG_JIT, cache, dataset, scratchpad_memory->scratchpad(), 0);

	uint64_t nonce=0;
	uint64_t tempHash[8] = {0};
	char input_blob[256]={0};
	uint8_t output_hash[RANDOMX_HASH_SIZE]={0};
	sprintf(input_blob,"test fast mode randomx with nonce %llu",nonce++);
	for(int i=0;i < 100;i++){
		sprintf(input_blob,"test fast mode randomx with nonce %llu",nonce++);
		randomx_calculate_hash(vm,input_blob,sizeof(input_blob),output_hash);
		std::cout << "calculate hash in fast mode " << std::endl;
		outputHex(std::cout,(const char*)output_hash,RANDOMX_HASH_SIZE);
		std::cout << std::endl;
	}

	//release the cache dataset and vm
	randomx_release_cache(cache);
	delete cache_memory;
	randomx_release_dataset(dataset);
	delete dataset_memory;
	randomx_destroy_vm(vm);
	delete scratchpad_memory;

	//create new vm and use default flags and check the hash
	xmrig::VirtualMemory *check_cache_memory = new xmrig::VirtualMemory(RANDOMX_CACHE_MAX_SIZE, true, false, false, 0);
	randomx_cache* check_cache=randomx_create_cache(RANDOMX_FLAG_DEFAULT, check_cache_memory->raw());
	randomx_init_cache(check_cache,key.c_str(),key.length());

	xmrig::VirtualMemory *check_dataset_memory = new xmrig::VirtualMemory(RANDOMX_DATASET_MAX_SIZE, true, false, false, 0);
	randomx_dataset* check_dataset=randomx_create_dataset(check_dataset_memory->raw());

	xmrig::VirtualMemory *check_sp_memory=new xmrig::VirtualMemory(RANDOMX_SCRATCHPAD_L3_MAX_SIZE, true, false, false, 0);
	randomx_vm* check_vm=randomx_create_vm(RANDOMX_FLAG_DEFAULT, check_cache, check_dataset, check_sp_memory->scratchpad(), 0);

	uint8_t check_output_hash[RANDOMX_HASH_SIZE]={0};
	randomx_calculate_hash(check_vm,input_blob,sizeof(input_blob),check_output_hash);

	std::cout << "check calculate hash in fast mode " << std::endl;
	outputHex(std::cout,(const char*)check_output_hash,RANDOMX_HASH_SIZE);

	//release the cache dataset and vm
	randomx_release_cache(check_cache);
	delete check_cache_memory;
	randomx_release_dataset(check_dataset);
	delete check_dataset_memory;
	randomx_destroy_vm(check_vm);
	delete check_sp_memory;

	ASSERT_TRUE(0 == memcmp(output_hash,check_output_hash,RANDOMX_HASH_SIZE)) << "random x ok";
}

int main() {
	std::cout << "test randomX functions" << std::endl;
	::testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}