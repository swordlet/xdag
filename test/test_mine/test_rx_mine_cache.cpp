//
// Created by mac on 2020/6/15.
//

#include <rx_mine_cache.h>
#include <string_tools.h>
#include <hash.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <random_utils.h>

static std::string hash2hex(xdag_hash_t hash){
	return string_tools::bin2hex((uint8_t*)hash,sizeof(uint64_t)*4);
}

static void hex2hash(std::string hex,xdag_hash_t hash){
	string_tools::hex2bin(hex,(uint8_t*)hash);
}

void test1(){
	//string_tools::bin2hex()
	xdag_hash_t hash1;
	GetRandBytes(hash1,sizeof(hash1));
	std::string hex1=hash2hex(hash1);
	std::cout << "hash1 after rand hex is " << hash2hex(hash1) << std::endl;

	xdag_hash_t hash2;
	hex2hash(hex1,hash2);
	std::cout << "hash2 hex is " << hash2hex(hash2) << std::endl;


	xdag_hash_t prehash1;
	xdag_hash_t seedhash1;
	GetRandBytes(prehash1,sizeof(prehash1));
	GetRandBytes(seedhash1,sizeof(seedhash1));
	std::string prehex1=hash2hex(prehash1);
	std::string seedhex1=hash2hex(seedhash1);
	std::cout << "prehash1 is " << prehex1 << std::endl;
	std::cout << "seedhash1 is " << seedhex1 << std::endl;

	std::map<std::string,std::string> m1;
	m1.insert(std::make_pair(prehex1,seedhex1));
	auto it=m1.find(prehex1);
	if(it!= m1.end()){
		std::cout << "find seed to prehash1 " << it->second << std::endl;
	}
}

void test2(){

	xdag_hash_t pre_hash;
	xdag_hash_t seed;

	for(int i=0;i<8;++i){
		GetRandBytes(pre_hash,sizeof(pre_hash));
		GetRandBytes(seed,sizeof(seed));

		std::cout << "gen pre hash:\t" << hash2hex(pre_hash);
		std::cout << "\tgen seed:\t\t"  << hash2hex(seed) << std::endl;
		enqueue_rx_task(pre_hash,seed);
	}
	std::cout << "print all pre hash and seed" << std::endl;
	printf_all_rx_tasks();

	for(int i=0;i<3;i++){
		GetRandBytes(pre_hash,sizeof(pre_hash));
		GetRandBytes(seed,sizeof(seed));

		std::cout << "push pre hash:\t" << hash2hex(pre_hash);
		std::cout << "\tseed:\t"  << hash2hex(seed) << std::endl;

		enqueue_rx_task(pre_hash,seed);
		std::cout << "after push new pre hash " << std::endl;
		printf_all_rx_tasks();
		sleep(10);
	}
}

int main(int argc,char* argv[]){

	//test1();
	test2();
	return 0;
}