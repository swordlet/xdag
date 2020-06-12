//
// Created by mac on 2020/6/8.
//
#include <randomx.h>
#include <thread>
#include <sstream>
#include <iostream>
#include <dataset.hpp>
#include "stopwatch.hpp"
#include "affinity.hpp"

std::string g_dynamic_seed ="global dynamic seed 1";
randomx_flags g_flags;
randomx_dataset* g_rx_dataset= nullptr;
randomx_cache* g_rx_cache= nullptr;

pthread_mutex_t g_log_mutex=PTHREAD_MUTEX_INITIALIZER;

constexpr char hexmap[] = "0123456789abcdef";
inline std::string hash2hex(uint8_t * hash)
{
	std::stringstream ss;
	for (int i = 0; i < RANDOMX_HASH_SIZE; ++i) {
		ss << hexmap[(hash[i] & 0xF0) >> 4];
		ss << hexmap[hash[i] & 0x0F];
	}
	return ss.str();
}

struct MemoryException : public std::exception {
};
struct CacheAllocException : public MemoryException {
	const char * what() const throw () {
		return "Cache allocation failed";
	}
};
struct DatasetAllocException : public MemoryException {
	const char * what() const throw () {
		return "Dataset allocation failed";
	}
};

void* mine(randomx_vm *vm, int nonces_count, int thread_no,uint32_t startItem,uint32_t count);

void test1()
{
	std::string seed1="fix seed 1";
	randomx_cache *cache=randomx_alloc_cache(RANDOMX_FLAG_JIT);
	randomx_dataset *dataset=randomx_alloc_dataset(RANDOMX_FLAG_JIT);

	randomx_init_cache(cache, seed1.c_str(), strlen(seed1.c_str()));
	randomx_init_dataset(dataset,cache,0,randomx_dataset_item_count());
	randomx_release_cache(cache);
	cache= nullptr;
	randomx_vm *vm=randomx_create_vm(RANDOMX_FLAG_JIT,cache,dataset);

	uint8_t buf[128]={0};
	memcpy(buf,"hello randomx",strlen("hello randomx"));

	uint8_t output_hash[RANDOMX_HASH_SIZE];

	randomx_calculate_hash(vm,buf, sizeof(buf),output_hash);
	std::cout << "rx calc with seed1 " << seed1 << "\t\t\t" << hash2hex(output_hash) << std::endl;

	randomx_calculate_hash(vm,buf, sizeof(buf),output_hash);
	std::cout << "rx re-calc hash with seed1" << seed1 << "\t\t\t" << hash2hex(output_hash) << std::endl;

	// change seed and calculate hash
	std::string seed2="seed2";
	randomx_init_cache(cache,seed2.c_str(),strlen(seed2.c_str()));

	// output hash changed
	randomx_calculate_hash(vm,buf, sizeof(buf),output_hash);
	std::cout << "rx re-calc hash with seed2 " << seed2 << "\t\t\t" << hash2hex(output_hash) << std::endl;

	randomx_destroy_vm(vm);
	randomx_release_dataset(dataset);
	randomx_release_cache(cache);
}

//use auto flags and dynamic seed
int test2()
{
	int init_thread_count=4;
	int thread_count=4;
	int mining_mode=1;
	int large_pages=0;
	int secure=0;
	int nonces_count=1024;
	int thread_affinity=0;

	std::vector<randomx_vm*> vms;
	std::vector<std::thread> threads;

	//init_thread_count = std::thread::hardware_concurrency();
	g_flags = randomx_get_flags();

//	if (large_pages) {
//		flags |= RANDOMX_FLAG_LARGE_PAGES;
//	}

	/**
	 * mining mode with muti thread must set this flag
	 * otherwise creating vm might be failed
	 * */
	if (mining_mode) {
		g_flags |= RANDOMX_FLAG_FULL_MEM;
	}
#ifndef __OpenBSD__
	if (secure) {
		g_flags |= RANDOMX_FLAG_SECURE;
	}
#endif

	// printf randomx flags info
	if (g_flags & RANDOMX_FLAG_ARGON2_AVX2) {
		std::cout << " - Argon2 implementation: AVX2" << std::endl;
	}
	else if (g_flags & RANDOMX_FLAG_ARGON2_SSSE3) {
		std::cout << " - Argon2 implementation: SSSE3" << std::endl;
	}
	else {
		std::cout << " - Argon2 implementation: reference" << std::endl;
	}

	if (g_flags & RANDOMX_FLAG_FULL_MEM) {
		std::cout << " - full memory mode (2080 MiB)" << std::endl;
	}
	else {
		std::cout << " - light memory mode (256 MiB)" << std::endl;
	}

	if (g_flags & RANDOMX_FLAG_JIT) {
		std::cout << " - JIT compiled mode ";
		if (g_flags & RANDOMX_FLAG_SECURE) {
			std::cout << "(secure)";
		}
		std::cout << std::endl;
	}
	else {
		std::cout << " - interpreted mode" << std::endl;
	}

	if (g_flags & RANDOMX_FLAG_HARD_AES) {
		std::cout << " - hardware AES mode" << std::endl;
	}
	else {
		std::cout << " - software AES mode" << std::endl;
	}

	if (g_flags & RANDOMX_FLAG_LARGE_PAGES) {
		std::cout << " - large pages mode" << std::endl;
	}
	else {
		std::cout << " - small pages mode" << std::endl;
	}

	std::cout << "Initializing";
	if (mining_mode)
		std::cout << " (" << init_thread_count << " thread" << (init_thread_count > 1 ? "s)" : ")");
	std::cout << " ..." << std::endl;

	try {
		if (nullptr == randomx::selectArgonImpl(g_flags)) {
			throw std::runtime_error("Unsupported Argon2 implementation");
		}
		if ((g_flags & RANDOMX_FLAG_JIT) && !RANDOMX_HAVE_COMPILER) {
			throw std::runtime_error("JIT compilation is not supported on this platform. Try without --jit");
		}
		if (!(g_flags & RANDOMX_FLAG_JIT) && RANDOMX_HAVE_COMPILER) {
			std::cout << "WARNING: You are using the interpreter mode. Use --jit for optimal performance." << std::endl;
		}

		Stopwatch sw(true);
		g_rx_cache = randomx_alloc_cache(g_flags);
		if (g_rx_cache == nullptr) {
			throw CacheAllocException();
		}

		randomx_init_cache(g_rx_cache, &g_dynamic_seed, strlen(g_dynamic_seed.c_str()));
		if (mining_mode) {
			g_rx_dataset = randomx_alloc_dataset(g_flags);
			if (g_rx_dataset == nullptr) {
				throw DatasetAllocException();
			}
			uint32_t datasetItemCount = randomx_dataset_item_count();
			if (init_thread_count > 1) {
				auto perThread = datasetItemCount / init_thread_count;
				auto remainder = datasetItemCount % init_thread_count;
				uint32_t startItem = 0;
				for (int i = 0; i < init_thread_count; ++i) {
					auto count = perThread + (i == init_thread_count - 1 ? remainder : 0);
					threads.push_back(std::thread(&randomx_init_dataset, g_rx_dataset, g_rx_cache, startItem, count));
					startItem += count;
				}
				for (unsigned i = 0; i < threads.size(); ++i) {
					threads[i].join();
				}
			}
			else {
				std::cout << "rx dataset initialize  with single thread" << std::endl;
				randomx_init_dataset(g_rx_dataset, g_rx_cache, 0, datasetItemCount);
			}
			std::cout << "rx dataset initialized release the cache" << std::endl;
			randomx_release_cache(g_rx_cache);
			g_rx_cache = nullptr;
			threads.clear();
		}

		std::cout << "Memory initialized in " << sw.getElapsed() << " s" << std::endl;
		std::cout << "Initializing " << thread_count << " virtual machine(s) ..." << std::endl;

		for (int i = 0; i < thread_count; ++i) {
			std::cout << "rx create vm with cache " << g_rx_cache << std::endl;
			randomx_vm *vm = randomx_create_vm(g_flags, g_rx_cache, g_rx_dataset);

			if (vm == nullptr) {
				if ((g_flags & RANDOMX_FLAG_HARD_AES)) {
					throw std::runtime_error("Cannot create VM with the selected options. Try using --softAes");
				}
				if (large_pages) {
					throw std::runtime_error("Cannot create VM with the selected options. Try without --largePages");
				}
				throw std::runtime_error("Cannot create VM");
			}
			vms.push_back(vm);
		}
		std::cout << "Running benchmark (" << nonces_count << " nonces) ..." << std::endl;
		sw.restart();

		uint32_t datasetItemCount = randomx_dataset_item_count();
		if (thread_count > 1) {
			auto perThread = datasetItemCount / thread_count;
			auto remainder = datasetItemCount % thread_count;
			uint32_t startItem = 0;

			for (unsigned i = 0; i < vms.size(); ++i) {
				uint32_t count = perThread + (i == thread_count - 1 ? remainder : 0);
				std::cout << "create thread " << i << " with dataset start item " << startItem << " count " << count << std::endl;
				threads.push_back(std::thread(&mine, vms[i], nonces_count, i, startItem, count));
				startItem+=count;
			}
			for (unsigned i = 0; i < threads.size(); ++i) {
				threads[i].join();
			}
		}
		else {
			mine(vms[0], nonces_count, 0,0,datasetItemCount);
		}

		double elapsed = sw.getElapsed();
		for (unsigned i = 0; i < vms.size(); ++i)
			randomx_destroy_vm(vms[i]);
		if (mining_mode)
			randomx_release_dataset(g_rx_dataset);
		else
			randomx_release_cache(g_rx_cache);
	}
	catch (MemoryException& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		if (large_pages) {
#ifdef _WIN32
			std::cout << "To use large pages, please enable the \"Lock Pages in Memory\" policy and reboot." << std::endl;
			if (!IsWindows8OrGreater()) {
				std::cout << "Additionally, you have to run the benchmark from elevated command prompt." << std::endl;
			}
#else
			std::cout << "To use large pages, please run: sudo sysctl -w vm.nr_hugepages=1250" << std::endl;
#endif
		}
		return 1;
	}
	catch (std::exception& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		return 1;
	}
}

void* mine(randomx_vm *vm, int nonces_count, int thread_no,uint32_t startItem,uint32_t count)
{
	randomx_vm *priv_vm=vm;
	bool seed_changed=false;
	uint8_t output_hash[RANDOMX_HASH_SIZE];

	while(nonces_count-- > 0){
		randomx_calculate_hash(priv_vm,g_dynamic_seed.c_str(),strlen(g_dynamic_seed.c_str()),output_hash);
		pthread_mutex_lock(&g_log_mutex);
		std::cout << "rx calc thread " << thread_no << " " << hash2hex(output_hash) << std::endl;
		pthread_mutex_unlock(&g_log_mutex);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		if(nonces_count < 1000 && thread_no == 0 && !seed_changed){
			seed_changed=true;
			std::cout << "thread 0 change seed " << std::endl;
			randomx_cache *cache=randomx_alloc_cache(g_flags);
			randomx_init_cache(cache,"dynamic seed 2",strlen("dynamic seed 2"));
			randomx_init_dataset(g_rx_dataset,cache,startItem,count);
			randomx_destroy_vm(priv_vm);

			priv_vm=randomx_create_vm(g_flags,cache,g_rx_dataset);
			randomx_release_cache(cache);
		}
	}
	return nullptr;
}

int main(int argc,char* argv[])
{
	//test1();
	test2();

	return 0;
}