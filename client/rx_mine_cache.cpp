//
// Created by mac on 2020/6/15.
//

#include <deque>
#include <map>
#include <string>
#include <pthread.h>
#include <iostream>
#include <string_tools.h>
#include "mining_common.h"
#include "rx_mine_cache.h"

#define RX_MAX_CACHE_TASK_SIZE 8

static std::deque<std::string> task_deque;
static std::map<std::string,rx_pool_task> task_map;

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

static std::string hash2hex(const xdag_hash_t hash){
	return string_tools::bin2hex((uint8_t*)hash,sizeof(uint64_t)*4);
}

static void hex2hash(std::string hex,xdag_hash_t hash){
	string_tools::hex2bin(hex,(uint8_t*)hash);
}

int get_rx_task_by_idx(uint64_t index,rx_pool_task *task){

	pthread_mutex_lock(&cache_mutex);

	if(index > RX_MAX_CACHE_TASK_SIZE-1){
		std::cout << "index larger than " << (RX_MAX_CACHE_TASK_SIZE-1) << std::endl;
		return -1;
	}

	if(task_deque.empty() || task_map.empty()){
		std::cout << "task queue or task info map empty" << std::endl;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}

	std::string prehex=task_deque.at(index);
	auto it=task_map.find(prehex);
	if(it==task_map.end()){
		std::cout << "can not find task by index " << index << std::endl;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}

	*task=it->second;
	pthread_mutex_unlock(&cache_mutex);

	return 0;
}

int get_rx_task_by_prehash(const xdag_hash_t prehash,rx_pool_task *task) {

	pthread_mutex_lock(&cache_mutex);

	std::string prehex=hash2hex(prehash);
	if(task_deque.empty() || task_map.empty()){
		std::cout << "task queue or task info map empty" << std::endl;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}

	auto it=task_map.find(prehex);
	if(it == task_map.end()){
		std::cout << "can not find seed by prehash " << prehex << std::endl;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}
	*task=it->second;
	std::cout << "find seed " << hash2hex(it->second.seed) << " by prehash " << prehex << std::endl;
	pthread_mutex_unlock(&cache_mutex);

	return 0;
}

int update_rx_task_by_prehash(const xdag_hash_t prehash,rx_pool_task task){

	pthread_mutex_lock(&cache_mutex);
	std::string prehex=hash2hex(prehash);
	if(task_deque.empty() || task_map.empty()){
		std::cout << "task queue or task info map empty" << std::endl;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}

	auto it=task_map.find(prehex);
	if(it==task_map.end()){
		std::cout << "prehash not exist in task map" << std::endl;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}else{
		task_map.erase(it);
		task_map.insert(std::make_pair(prehex,task));
	}

	pthread_mutex_unlock(&cache_mutex);

	return 0;
}

int enqueue_rx_task(rx_pool_task t){

	pthread_mutex_lock(&cache_mutex);
	rx_pool_task task;
	std::string prehex=hash2hex(t.prehash);

	if(task_deque.size() >= RX_MAX_CACHE_TASK_SIZE){
		//resize task deque to RA_MAX_CACHE_TASK_SIZE-1
		while(task_deque.size() >= RX_MAX_CACHE_TASK_SIZE){
			std::string front_hash=task_deque.front();
			std::cout << "task queue is full, pop front hash "<< front_hash <<" of rx task queue" << std::endl;
			task_deque.pop_front();
			task_map.erase(front_hash);
		}
	}

	task_deque.emplace_back(prehex);
	task_map.insert(std::make_pair(prehex, t));

	pthread_mutex_unlock(&cache_mutex);
	return 0;
}

/**
 * get the latest prehash of randomx
 * */
int get_rx_latest_prehash(xdag_hash_t prehash){

	pthread_mutex_lock(&cache_mutex);

	if(task_deque.empty() || task_map.empty()){
		std::cout << "task deque or task info map empty" << std::endl;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}
	std::string prehex=task_deque.front();
	std::cout << "get latest pre hash " << prehex << std::endl;
	hex2hash(prehex,prehash);
	pthread_mutex_unlock(&cache_mutex);
	return 0;
}

/**
 * traverse the task queue from begin to end
 * and print the task info in map
 * */
void printf_all_rx_tasks(){
	pthread_mutex_lock(&cache_mutex);
	for(auto it=task_deque.begin();it!=task_deque.end();it++){
		//std::cout << "pre : " << *it;
		auto itv=task_map.find(*it);
		if(itv != task_map.end()){
			std::cout << "\ttime: " << itv->second.task_time;
			std::cout << "\tpre : " << hash2hex(itv->second.prehash);
			std::cout << "\tseed : " << hash2hex(itv->second.seed);
			std::cout << "\tminhash: " << hash2hex(itv->second.minhash);
			std::cout << "\tnonce1: " << itv->second.first_nonce;
			std::cout << "\tfirst hashed: " << itv->second.firsthashed;

		}
		std::cout << std::endl;
	}
	pthread_mutex_unlock(&cache_mutex);
}




