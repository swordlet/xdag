//
// Created by mac on 2020/6/15.
//
#include <time.h>
#include <rx_mine_cache.h>
#include <hash.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <mining_common.h>
#include <sstream>
#include <utils/random_utils.h>

static std::string hash2hex(xdag_hash_t hash){
	return bin2hex((uint8_t*)hash,sizeof(uint64_t)*4);
}

static void hex2hash(std::string hex,xdag_hash_t hash){
	hex2bin(hex,(uint8_t*)hash);
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
	rx_pool_task task;

	for(int i=0;i<8;++i){
		task.hashed = false;
		task.nonce0 = 0;
		task.task_time = time(NULL);
		GetRandBytes(task.prehash,sizeof(task.prehash));
		GetRandBytes(task.seed,sizeof(task.seed));
		GetRandBytes(task.lastfield, sizeof(task.lastfield));

		std::cout << "\ttime:" << task.task_time;
		std::cout << "\tpre:" << hash2hex(task.prehash);
		std::cout << "\tseed:"  << hash2hex(task.seed);
		std::cout << "\tlastfield:" << hash2hex(task.lastfield) << std::endl;
		std::cout << "\tnonce"<<task.nonce0;

		enqueue_rx_task(task);
	}
	std::cout << "print all pre hash and seed" << std::endl;
	printf_all_rx_tasks();

	xdag_hash_t latest_prehash;
	for(int i=0;i<3;i++){
		task.hashed = false;
		task.nonce0 = 0;
		task.task_time = time(NULL);
		GetRandBytes(task.prehash,sizeof(task.prehash));
		GetRandBytes(task.seed,sizeof(task.seed));
		GetRandBytes(task.lastfield, sizeof(task.lastfield));

		std::cout << "\ttime:" << task.task_time;
		std::cout << "\tpre:" << hash2hex(task.prehash);
		std::cout << "\tseed:"  << hash2hex(task.seed);
		std::cout << "\tminhash:" << hash2hex(task.lastfield) << std::endl;
		std::cout << "\tnonce:"<<task.nonce0;

		memcpy(latest_prehash,task.prehash,sizeof(latest_prehash));
		enqueue_rx_task(task);

		printf_all_rx_tasks();
		sleep(1);
	}

	//test get the latest task
	rx_pool_task lastet_task;
	get_rx_task_by_prehash(latest_prehash,&lastet_task);

	std::cout << "get latest rx task" << std::endl;
	std::cout << "\ttime:" << lastet_task.task_time;
	std::cout << "\tpre:" << hash2hex(lastet_task.prehash);
	std::cout << "\tseed:"  << hash2hex(lastet_task.seed);
	std::cout << "\tlastfield:" << hash2hex(lastet_task.lastfield) << std::endl;
	std::cout << "\tnonce"<<lastet_task.nonce0;

	//test update task in cache
	GetRandBytes(lastet_task.lastfield, sizeof(task.lastfield));
	update_rx_task_by_prehash(lastet_task.prehash,lastet_task);
	get_rx_task_by_prehash(latest_prehash,&lastet_task);

	std::cout << "updated latest rx task" << std::endl;
	std::cout << "\ttime:" << lastet_task.task_time;
	std::cout << "\tpre:" << hash2hex(lastet_task.prehash);
	std::cout << "\tseed:"  << hash2hex(lastet_task.seed);
	std::cout << "\tlastfield:" << hash2hex(lastet_task.lastfield) << std::endl;
	std::cout << "\tnonce"<<lastet_task.nonce0;

	std::cout << "update task 1 3 5" <<std::endl;
	xdag_hash_t pre1;
	xdag_hash_t pre3;
	xdag_hash_t pre5;

	rx_pool_task rt1;
	rx_pool_task rt3;
	rx_pool_task rt5;

	get_rx_task_by_idx(1,&rt1);
	get_rx_task_by_idx(3,&rt3);
	get_rx_task_by_idx(5,&rt5);

	GetRandBytes(rt1.lastfield, sizeof(rt1.lastfield));
	GetRandBytes(rt3.lastfield, sizeof(rt3.lastfield));
	GetRandBytes(rt5.lastfield, sizeof(rt5.lastfield));

	update_rx_task_by_prehash(rt1.prehash,rt1);
	update_rx_task_by_prehash(rt3.prehash,rt3);
	update_rx_task_by_prehash(rt5.prehash,rt5);

	std::cout << "updated task 1 3 5" <<std::endl;
	printf_all_rx_tasks();
}

int main(int argc,char* argv[]){

	//test1();
	test2();
	return 0;
}