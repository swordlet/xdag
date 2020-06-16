//
// Created by mac on 2020/6/15.
//

#include <deque>
#include <map>
#include <string>
#include <pthread.h>
#include <iostream>
#include <string_tools.h>
#include "rx_mine_cache.h"

#define RA_MAX_CACHE_TASK_SIZE 8

static std::deque<std::string> task_deque;
static std::map<std::string,std::string> task_info_map;

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

static std::string hash2hex(const xdag_hash_t hash){
	return string_tools::bin2hex((uint8_t*)hash,sizeof(uint64_t)*4);
}

static void hex2hash(std::string hex,xdag_hash_t hash){
	string_tools::hex2bin(hex,(uint8_t*)hash);
}


int get_rx_seed_by_prehash(const xdag_hash_t prehash,xdag_hash_t seed) {

	pthread_mutex_lock(&cache_mutex);

	std::string prehex=hash2hex(prehash);
	if(task_deque.empty() || task_info_map.empty()){
		std::cout << "task queue or task info map empty" << std::endl;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}

	auto it=task_info_map.find(prehex);
	if(it==task_info_map.end()){
		std::cout << "can not find seed by prehash " << prehex << std::endl;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}

	std::cout << "find seed " << it->second << " by prehash " << prehex << std::endl;
	hex2hash(it->second,seed);

	pthread_mutex_unlock(&cache_mutex);
	return 0;
}


int enqueue_rx_task(const xdag_hash_t prehash,const xdag_hash_t seed){

	pthread_mutex_lock(&cache_mutex);

	std::string prehex=hash2hex(prehash);
	std::string seedhex=hash2hex(seed);

	if(task_deque.size() >= RA_MAX_CACHE_TASK_SIZE){
		//resize task deque to RA_MAX_CACHE_TASK_SIZE-1
		while(task_deque.size() >= RA_MAX_CACHE_TASK_SIZE){
			std::string front_hash=task_deque.front();
			std::cout << "task queue is full, pop front hash "<< front_hash <<"of rx task queue" << std::endl;
			task_deque.pop_front();
			task_info_map.erase(front_hash);
		}
	}

	task_deque.emplace_back(prehex);
	task_info_map.insert(std::make_pair(prehex,seedhex));

	pthread_mutex_unlock(&cache_mutex);
	return 0;
}

/**
 * get the latest prehash of randomx
 * */
int get_rx_latest_prehash(xdag_hash_t prehash){

	pthread_mutex_lock(&cache_mutex);

	if(task_deque.empty() || task_info_map.empty()){
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
		std::cout << "prehash : " << *it;
		auto itv=task_info_map.find(*it);
		if(itv!=task_info_map.end()){
			std::cout << "\tseed : " << itv->second;
		}
		std::cout << std::endl;
	}
	pthread_mutex_unlock(&cache_mutex);
}




