#include <deque>
#include <map>
#include <string>
#include <pthread.h>
#include <iostream>
#include <utility>
#include <vector>
#include "utils/string_tools.h"
#include "utils/log.h"
#include "mining_common.h"
#include "rx_mine_cache.h"

#define RX_MAX_CACHE_TASK_SIZE 4

static std::vector<pthread_t> g_mining_threads;
static std::deque<std::string> task_deque;
static std::map<std::string,rx_pool_task> task_map;

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

static std::string hash2hex(const xdag_hash_t h){
	char buf[64];
	sprintf(buf,"%016llx%016llx%016llx%016llx",h[0],h[1],h[2],h[3]);
	return buf;
}

static std::string hashlow2hex(const xdag_hashlow_t h){
	char buf[48];
	sprintf(buf,"%016llx%016llx%016llx",h[0],h[1],h[2]);
	return buf;
}

static void hex2hash(std::string hex,xdag_hash_t hash){
	string_tools::hex2bin(std::move(hex),(uint8_t*)hash);
}

int get_rx_task_cache_size(){
	size_t s;
	pthread_mutex_lock(&cache_mutex);

	if(task_map.size() != task_deque.size()){
		std::cout << "warning map size " << task_deque.size()
		<< " not euqual deque size " << task_deque.size() << std::endl;
	}
	s = task_deque.size();
	pthread_mutex_unlock(&cache_mutex);
	return s;
}

/**
 * get rx task by index
 * */
int get_rx_task_by_idx(uint64_t index,rx_pool_task *task){

	pthread_mutex_lock(&cache_mutex);

	if(index > RX_MAX_CACHE_TASK_SIZE-1){
		std::cout << "index larger than " << (RX_MAX_CACHE_TASK_SIZE-1) << std::endl;
		pthread_mutex_unlock(&cache_mutex);
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

/**
 * get all rx task info by presh
 * */
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
	//std::cout << "find seed " << hashlow2hex(it->second.seed) << " by prehash " << prehex << std::endl;
	pthread_mutex_unlock(&cache_mutex);

	return 0;
}

/**
 * update rx task info
 * */
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

/**
 * enqueue the task
 * */
int enqueue_rx_task(rx_pool_task t){

	pthread_mutex_lock(&cache_mutex);

	std::string prehex=hash2hex(t.prehash);
	if(task_deque.size() >= RX_MAX_CACHE_TASK_SIZE){
		while(task_deque.size() >= RX_MAX_CACHE_TASK_SIZE){
			std::string front_hash=task_deque.front();
			//std::cout << "task queue is full, pop front hash "<< front_hash <<" of rx task queue" << std::endl;
			task_deque.pop_front();
			task_map.erase(front_hash);
		}
	}
	task_deque.emplace_back(prehex);
	task_map.insert(std::make_pair(prehex, t));

	pthread_mutex_unlock(&cache_mutex);
	xdag_info("enqueue task return ");
	return 0;
}

/**
 * clear the task
 * */
void clear_all_rx_tasks(){
	pthread_mutex_lock(&cache_mutex);
	task_map.clear();
	task_deque.clear();
	pthread_mutex_unlock(&cache_mutex);
	xdag_info("clear all the tasks ");
}

/**
 * pop rx task from deque and erase it from map
 * */
int pop_rx_task(){

	pthread_mutex_lock(&cache_mutex);

	if(task_deque.empty() || task_map.empty()){
		std::cout << "del task failed : task queue or task info map empty" << std::endl;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}

	std::string prehex=task_deque.front();
	auto it=task_map.find(prehex);
	if(it==task_map.end()){
		std::cout << "del task failed : can not find task by prehex " << prehex << std::endl;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}
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
	std::string prehex=task_deque.back();
	std::cout << "get latest pre hash " << prehex << std::endl;
	hex2hash(prehex,prehash);
	pthread_mutex_unlock(&cache_mutex);
	return 0;
}

/**
 * get the latest rx pool task
 * */
int get_rx_latest_task(rx_pool_task *task){

	pthread_mutex_lock(&cache_mutex);

	if(task_deque.empty() || task_map.empty()){
		xdag_info("rx task cache: deque or task info map empty");
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}
	std::string prehex=task_deque.back();
	auto it=task_map.find(prehex);
	if(it==task_map.end()){
		xdag_info("rx task cache: can not get task by pre hash %s",prehex.c_str());
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}
	*task=it->second;
	pthread_mutex_unlock(&cache_mutex);
	return 0;
}

int get_remain_task_by_seqno(uint64_t seqno,struct xdag_field* fields,int* fields_count){
	pthread_mutex_lock(&cache_mutex);

	if(task_deque.empty() || task_map.empty()){
		xdag_info("rx task cache: deque or task info map empty");
		*fields_count=0;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}

	int pos=0;
	for(auto it_prehash=task_deque.begin();it_prehash!= task_deque.end();it_prehash++){
		auto it_info=task_map.find(*it_prehash);
		if(it_info != task_map.end()){
			rx_pool_task task=it_info->second;
			if(task.seqno > seqno){
				memcpy(fields[pos].data,task.prehash,sizeof(xdag_hash_t));
				memcpy(fields[pos+1].data,task.seed,sizeof(xdag_hashlow_t));
				pos+=2;
				*fields_count+=2;
			}
		}
	}

	pthread_mutex_unlock(&cache_mutex);
	return 0;
}

int get_latest_task(struct xdag_field* fields,int* fields_count){
	pthread_mutex_lock(&cache_mutex);
	
	if(task_deque.empty() || task_map.empty()){
		xdag_info("rx task cache: deque or task info map empty");
		*fields_count=0;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}
	
	auto it_prehash=task_deque.back();
	auto it_info=task_map.find(it_prehash);
	if(it_info==task_map.end()){
		xdag_info("can not find latest block by prehash %s",it_prehash.c_str());
		*fields_count=0;
		pthread_mutex_unlock(&cache_mutex);
		return -1;
	}
	*fields_count=2;
	rx_pool_task task=it_info->second;
	memcpy(fields[0].data,task.prehash,sizeof(xdag_hash_t));
	memcpy(fields[1].data,task.seed,sizeof(xdag_hashlow_t));
	
	pthread_mutex_unlock(&cache_mutex);
	return 0;
}

void push_rx_mining_thread(pthread_t th){
	pthread_mutex_lock(&cache_mutex);
	std::cout << "push rx mining thread " << th << std::endl;
	g_mining_threads.emplace_back(th);
	pthread_mutex_unlock(&cache_mutex);
}

void cancel_rx_mining_threads(){
	pthread_mutex_lock(&cache_mutex);
	std::cout << "stop mining ...." << std::endl;
	auto it = g_mining_threads.begin();
	while(it != g_mining_threads.end()){
		pthread_cancel(*it);
		pthread_join(*it,NULL);
		it++;
	}
	g_mining_threads.clear();
	pthread_mutex_unlock(&cache_mutex);
	std::cout << "mining stopped ...." << std::endl;
}

/**
 * traverse the task queue from begin to end
 * and print the task info in map
 * */
void printf_miner_rx_tasks(){
	pthread_mutex_lock(&cache_mutex);
	for(auto it=task_deque.begin();it!=task_deque.end();it++){
		//std::cout << "pre : " << *it;
		auto itv=task_map.find(*it);
		if(itv != task_map.end()){
			std::cout << "\ttime: " << itv->second.task_time;
			std::cout << "\tpre : " << hash2hex(itv->second.prehash);
			std::cout << "\tseed : " << hashlow2hex(itv->second.seed);
			std::cout << "\tlastfield: " << hash2hex(itv->second.lastfield);
			std::cout << "\tnonce1: " << itv->second.nonce0;
			std::cout << "\tfirst hashed: " << itv->second.hashed;
		}
		std::cout << std::endl;
	}
	pthread_mutex_unlock(&cache_mutex);
}

/**
 * traverse the task queue from begin to end
 * and print the task info in map
 * */
void printf_pool_rx_tasks(){
	pthread_mutex_lock(&cache_mutex);
	for(auto it=task_deque.begin();it!=task_deque.end();it++){
		//std::cout << "pre : " << *it;
		auto itv=task_map.find(*it);
		if(itv != task_map.end()){
			std::cout << "\ttime: " << itv->second.task_time;
			std::cout << "\tpre : " << hash2hex(itv->second.prehash);
			std::cout << "\tseed : " << hashlow2hex(itv->second.seed);
		}
		std::cout << std::endl;
	}
	pthread_mutex_unlock(&cache_mutex);
}




