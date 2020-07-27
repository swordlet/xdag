//
// Created by mac on 2020/6/16.
//

#include "rx_miner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "system.h"
#include "../dfslib/dfslib_crypt.h"
#include "address.h"
#include "block.h"
#include "global.h"
#include "sync.h"
#include "transport.h"
#include "mining_common.h"
#include "network.h"
#include "algorithms/crc.h"
#include "utils/log.h"
#include "utils/utils.h"
#include "utils/random_utils.h"
#include <randomx.h>
#include <stdbool.h>
#include "rx_mine_hash.h"
#include "rx_mine_cache.h"
#include "utils/string_utils.h"

#define MINERS_PWD             "minersgonnamine"
#define SECTOR0_BASE           0x1947f3acu
#define SECTOR0_OFFSET         0x82e9d1b5u
#define SEND_PERIOD            10                                  /* share period of sending shares */
#define POOL_LIST_FILE         (g_xdag_testnet ? "pools-testnet.txt" : "pools.txt")

int g_rx_auto_swith_pool = 0;

struct miner {
	struct xdag_field id;
	uint64_t nfield_in;
	uint64_t nfield_out;
};

static struct miner g_local_miner;
static pthread_mutex_t g_miner_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_update_min_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_pick_task_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_miner_seed_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_miner_stop_cond = PTHREAD_COND_INITIALIZER;
//static xdag_hash_t g_fixed_miner_seed;
static xdag_hashlow_t g_miner_seed;
int g_rx_mining_threads;
static int g_socket = -1, g_rx_stop_mining = 1;

static void *rx_mining_thread(void *arg);
static void rx_update_task_min_hash(rx_pool_task rt,xdag_hash_t hash);
static int rx_send_to_pool(struct xdag_field *fld, int nfld);

static int can_send_share(time_t current_time, time_t task_time, time_t share_time)
{
	int can_send = current_time - share_time >= SEND_PERIOD && current_time - task_time <= 64;
	if(g_rx_mining_threads == 0 && share_time >= task_time) {
		can_send = 0;  //we send only one share per task if mining is turned off
	}
	return can_send;
}

int rx_initialize_miner(const char *pool_address){
	int err=0;
	pthread_t th;

	printf("initialize rx miner rx fork height %llu\n",g_rx_fork_height);

	memset(&g_local_miner, 0, sizeof(struct miner));
	xdag_get_our_block(g_local_miner.id.data);

	err = pthread_create(&th, 0, rx_miner_net_thread, (void*)pool_address);

	if(err != 0) {
		printf("create rx miner net thread failed, error : %s\n", strerror(err));
		return -1;
	}

	err = pthread_detach(th);
	if(err != 0) {
		printf("detach miner net thread failed, error : %s\n", strerror(err));
		return -1;
	}

	return 0;
}

int rx_mining_start(int n_mining_threads){
	int err=0;
	pthread_t th;

	if(n_mining_threads == g_rx_mining_threads) {

	} else if(!n_mining_threads) {
		g_rx_stop_mining = 1;
		g_rx_mining_threads = 0;
	} else if(!g_rx_mining_threads) {
		g_rx_stop_mining = 0;
	} else if(g_rx_mining_threads > n_mining_threads) {
		g_rx_stop_mining = 1;
		sleep(5);
		g_rx_stop_mining = 0;
		g_rx_mining_threads = 0;
	}
	
	while(g_rx_mining_threads < n_mining_threads) {
		g_rx_mining_threads++;
		
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
		pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
		
		err = pthread_create(&th, &attr, rx_mining_thread, (void*)(uintptr_t)g_rx_mining_threads);
		if(err != 0) {
			printf("create rx_mining_thread failed, error : %s\n", strerror(err));
			continue;
		}

		err = pthread_detach(th);
		if(err != 0) {
			printf("detach rx_mining_thread failed, error : %s\n", strerror(err));
			continue;
		}
		push_rx_mining_thread(th);
	}

	return 0;
}

void rx_mining_stop(){
	if(g_rx_stop_mining){
		return;
	}
	
	g_rx_stop_mining = 1;
	cancel_rx_mining_threads();
	xdag_info("waiting all mining threads quit");
	pthread_cond_wait(&g_miner_stop_cond,&g_miner_seed_mutex);
	xdag_info("rx mining stoped free vms");
	rx_mine_free();
}

void *rx_miner_net_thread(void *arg){
	struct xdag_block b;
	struct xdag_field data[2];
	xdag_hash_t hash;
	char pool_address[50] = {0};
	strncpy(pool_address, (const char*)arg, 49);
	const char *mess = NULL;
	int res = 0;
	int64_t pos=0;
	xtime_t t;
	struct miner *m = &g_local_miner;
	
	while(!g_xdag_sync_on) {
		sleep(1);
	}

	begin:
	m->nfield_in = m->nfield_out = 0;

	int ndata = 0;
	int maxndata = sizeof(struct xdag_field);
	time_t share_time = 0;
	time_t task_time = 0;

	if(g_miner_address) {
		if(xdag_address2hash(g_miner_address, hash)) {
			mess = "incorrect miner address";
			goto err;
		}
	} else if(xdag_get_our_block(hash)) {
		mess = "can't create a block";
		goto err;
	}

	pos = xdag_get_block_pos(hash, &t, &b);
	if (pos == -2l) {
		;
	} else if (pos < 0) {
		mess = "can't find the block";
		goto err;
	} else {
		xdag_info("%s load block hash %016llx%016llx%016llx%016llx time %llu pos %llu ",__FUNCTION__,
		          hash[3],hash[2],hash[1],hash[0],t,pos);
		struct xdag_block *blk = xdag_storage_load(hash, t, pos, &b);
		if(!blk) {
			mess = "can't load the block";
			goto err;
		}
		if(blk != &b) memcpy(&b, blk, sizeof(struct xdag_block));
	}

	pthread_mutex_lock(&g_miner_mutex);
	g_socket = xdag_connect_pool(pool_address, &mess);
	if(g_socket == INVALID_SOCKET) {
		pthread_mutex_unlock(&g_miner_mutex);
		if(g_rx_auto_swith_pool) {
			if(!rx_pick_pool(pool_address)) {
				mess = "no active pool available";
			}
		} else {
			mess = "connect pool failed.";
		}
		goto err;
	} else {
		xdag_mess("connected to pool %s", pool_address);
	}

	if(rx_send_to_pool(b.field, XDAG_BLOCK_FIELDS) < 0) {
		mess = "socket is closed";
		pthread_mutex_unlock(&g_miner_mutex);
		goto err;
	}
	pthread_mutex_unlock(&g_miner_mutex);

	for(;;) {
		struct pollfd p;

		pthread_mutex_lock(&g_miner_mutex);

		if(g_socket < 0) {
			pthread_mutex_unlock(&g_miner_mutex);
			mess = "socket is closed";
			goto err;
		}

		p.fd = g_socket;
		time_t current_time = time(0);
		p.events = POLLIN | (can_send_share(current_time, task_time, share_time) ? POLLOUT : 0);

		if(!poll(&p, 1, 0)) {
			pthread_mutex_unlock(&g_miner_mutex);
			sleep(1);
			continue;
		}

		if(p.revents & POLLHUP) {
			pthread_mutex_unlock(&g_miner_mutex);
			mess = "socket hangup";
			goto err;
		}

		if(p.revents & POLLERR) {
			pthread_mutex_unlock(&g_miner_mutex);
			mess = "socket error";
			goto err;
		}

		if(p.revents & POLLIN) {
			res = read(g_socket, (uint8_t*)data + ndata, maxndata - ndata);
			if(res < 0) {
				pthread_mutex_unlock(&g_miner_mutex); mess = "read error on socket"; goto err;
			}
			ndata += res;
			if(ndata == maxndata) {
				struct xdag_field *last = data + (ndata / sizeof(struct xdag_field) - 1);

				dfslib_uncrypt_array(g_crypt, (uint32_t*)last->data, DATA_SIZE, m->nfield_in++);

				if(!memcmp(last->data, hash, sizeof(xdag_hashlow_t))) {
					xdag_set_balance(hash, last->amount);
					atomic_store_explicit_uint_least64(&g_xdag_last_received, current_time, memory_order_relaxed);
					ndata = 0;
					maxndata = sizeof(struct xdag_field);
				} else if(maxndata == 2 * sizeof(struct xdag_field)) {
					const uint64_t task_index = g_xdag_pool_task_index + 1;
					g_xdag_pool_task_index = task_index;

					task_time = time(0);
					xdag_frame_t task_frame = xdag_get_frame();
					rx_pool_task rt;
					if(get_rx_task_by_prehash(data[0].data,&rt)==0){
						xdag_warn("warning: redundant prehash %016llx%016llx%016llx%016llx from pool",
						          data[0].data[0],data[0].data[1],data[0].data[2],data[0].data[3]);
						ndata = 0;
						maxndata = sizeof(struct xdag_field);
						continue;
					}

					GetRandBytes(rt.lastfield, sizeof(xdag_hash_t));
					rt.task_time = xdag_get_frame();
					rt.nonce0=rt.lastfield[3];
					rt.hashed=false;
					rt.discards=0;
					rt.discard_flag=0;
					memcpy(rt.prehash,data[0].data, sizeof(xdag_hash_t));
					memcpy(rt.seed,data[1].data, sizeof(xdag_hashlow_t));
					memcpy(rt.lastfield,hash,sizeof(xdag_hashlow_t));
					memset(rt.minhash,0xff,sizeof(xdag_hash_t));

					if(memcmp(rt.seed,g_miner_seed,sizeof(xdag_hashlow_t))!=0){
						pthread_mutex_lock(&g_miner_seed_mutex);
						clear_all_rx_tasks();
						//rx_mining_stop();
						rx_mine_init_seed(rt.seed, sizeof(xdag_hashlow_t),4);
						rx_mine_alloc_vms(4);
						memcpy(g_miner_seed,rt.seed, sizeof(xdag_hashlow_t));
						rx_mining_start(4);
						printf("rx pow reinit seed %16llx%16llx%16llx\n",rt.seed[0],rt.seed[1],rt.seed[2]);
						pthread_mutex_unlock(&g_miner_seed_mutex);
					}
					enqueue_rx_task(rt);

					xdag_info("enqueue rx pre : %016llx%016llx%016llx%016llx",rt.prehash[0],rt.prehash[1],rt.prehash[2],rt.prehash[3]);
					xdag_info("enqueue rx seed : %016llx%016llx%016llx",rt.seed[0],rt.seed[1],rt.seed[2]);
					xdag_info("rx mine task  : t=%llx N=%llu", task_frame << 16 | 0xffff, task_index);

					ndata = 0;
					maxndata = sizeof(struct xdag_field);
				} else {
					maxndata = 2 * sizeof(struct xdag_field);
				}
			}
		}

		if(p.revents & POLLOUT) {
				share_time = time(0);
				size_t task_num=get_rx_task_cache_size();
				xdag_info("share task num %lu",task_num);
				for(int i=0;i<task_num;++i){
					rx_pool_task rt;
					get_rx_task_by_idx(i,&rt);
					//task is hashed and not discard by all thread
					if(rt.hashed && rt.discards < g_rx_mining_threads){
						xdag_info("rx task %016llx%016llx%016llx%016llx hashed %d discards %d",i,rt.prehash,rt.discards);
						struct xdag_field fld[RX_POW_BLOCK_FIELDS];
						struct rx_pow_block pow;

						memcpy(fld[1].data, rt.prehash, sizeof(xdag_hash_t));
						memcpy(fld[2].data, rt.seed, sizeof(xdag_hashlow_t));
						memcpy(fld[3].data, rt.lastfield, sizeof(xdag_hash_t));
						res = rx_send_to_pool(fld, RX_POW_BLOCK_FIELDS);
						if(res) {
							mess = "write error on socket"; goto err;
						}
						xdag_info("rx pow share pre %016llx%016llx%016llx%016llx",
						          rt.prehash[0], rt.prehash[1], rt.prehash[2], rt.prehash[3]);
						xdag_info("rx pow share seed %016llx%016llx%016llx",
								rt.seed[0],rt.seed[1],rt.seed[2]);
						xdag_info("rx pow share last %016llx%016llx%016llx%016llx",
								rt.lastfield[0],rt.lastfield[1],rt.lastfield[2],rt.lastfield[3]);
						xdag_info("rx pow share hash: %016llx%016llx%016llx%016llx t=%llx res=%d",
								rt.minhash[0], rt.minhash[1], rt.minhash[2], rt.minhash[3], rt.task_time << 16 | 0xffff, res);
					}
				}
				pthread_mutex_unlock(&g_miner_mutex);
		} else {
			pthread_mutex_unlock(&g_miner_mutex);
		}
	}

	err:
	xdag_err("Miner: %s (error %d)", mess, res);

	pthread_mutex_lock(&g_miner_mutex);

	if(g_socket != INVALID_SOCKET) {
		close(g_socket);
		g_socket = INVALID_SOCKET;
	}

	pthread_mutex_unlock(&g_miner_mutex);

	sleep(10);

	goto begin;
}

int rx_send_block_via_pool(struct xdag_block *b){
	if(g_socket < 0) return -1;

	pthread_mutex_lock(&g_miner_mutex);
	int ret = rx_send_to_pool(b->field, XDAG_BLOCK_FIELDS);
	pthread_mutex_unlock(&g_miner_mutex);
	return ret;
}

int rx_pick_pool(char *pool_address){
	char addresses[30][50] = {{0}, {0}};
	const char *error_message;
	srand(time(NULL));

	int count = 0;
	FILE *fp = xdag_open_file(POOL_LIST_FILE, "r");
	if(!fp) {
		printf("List of pools is not found\n");
		return 0;
	}
	while(fgets(addresses[count], 50, fp)) {
		// remove trailing newline character
		addresses[count][strcspn(addresses[count], "\n")] = 0;
		++count;
	}
	fclose(fp);

	int start_index = count ? rand() % count : 0;
	int index = start_index;
	do {
		int socket = xdag_connect_pool(addresses[index], &error_message);
		if(socket != INVALID_SOCKET) {
			xdag_connection_close(socket);
			strncpy(pool_address, addresses[index], 49);
			return 1;
		} else {
			++index;
			if(index >= count) {
				index = 0;
			}
		}
	} while(index != start_index);

	printf("Wallet is unable to connect to network. Check your network connection\n");
	return 0;
}

static void mining_thread_clean(void *arg)
{
	static int stop_thread_count;
	stop_thread_count++;
	xdag_info("stoped thread count %llu clean \n",stop_thread_count);
	if(stop_thread_count == 4){
		xdag_info("xdag all mining thread stoped");
		stop_thread_count = 0;
		pthread_cond_signal(&g_miner_stop_cond);
	}
}

static void *rx_mining_thread(void *arg)
{
	xdag_hash_t current_hash;
	xdag_hash_t lastest_prehash;
	rx_pool_task rt;
	uint64_t nonce;
	struct xdag_field last;
	const int nthread = (int)(uintptr_t)arg;

	xdag_info("start rx mining thread %lu",pthread_self());
	pthread_cleanup_push(mining_thread_clean,NULL);
	while(!g_xdag_sync_on && !g_rx_stop_mining) {
		pthread_testcancel();
		sleep(1);
	}

	while(!g_rx_stop_mining) {
		pthread_testcancel();
		// pick up a task
		xdag_info("start pick a task");
		if(get_rx_latest_task(&rt)==0){
			//pthread_mutex_lock(&g_pick_task_mutex);
			xdag_info("get latest rx task pre : %016llx%016llx%016llx%016llx discard %d",
					rt.prehash[0],rt.prehash[1],rt.prehash[2],rt.prehash[3],rt.discards);
			xdag_info("get current rx task pre : %016llx%016llx%016llx%016llx",
					current_hash[0],current_hash[1],current_hash[2],current_hash[3]);
			if(!memcmp(rt.prehash,current_hash,sizeof(xdag_hash_t))
				&& rt.discards < g_rx_mining_threads){
				//if task not changed continue mining
				xdag_info("rx task not changed continue mining");
			}else if(rt.discards >= g_rx_mining_threads) {
				xdag_info("latest task discarded waiting...");
				sleep(1);
				continue;
			}else{
				//if task changed and not discard
				xdag_info("task changed and not discard");
				rx_pool_task tmp_rt;
				if(get_rx_task_by_prehash(current_hash,&tmp_rt)!=0){
					//if current hash not initialized
					xdag_info("current hash not exist set to %016llx%016llx%016llx%016llx",rt.prehash);
					nonce=rt.nonce0;
					memcpy(current_hash,rt.prehash,sizeof(xdag_hash_t));
				}else{
					//if current hash initialized and task changed discard it
					xdag_info("discard current hash %016llx%016llx%016llx%016llx",current_hash);
					nonce=rt.nonce0;
					tmp_rt.discards+=1;
					tmp_rt.discard_flag |= (1 << nthread);
					update_rx_task_by_prehash(current_hash,tmp_rt);
					memcpy(current_hash,rt.prehash,sizeof(xdag_hash_t));
				}
			}
			
			pthread_testcancel();
			//do slow hash
			xdag_hash_t output_hash;
			xdag_rx_mine_slow_hash((uint32_t) (nthread - 1), &rt, &nonce, g_rx_mining_threads,1024, output_hash);

			g_xdag_extstats.nhashes += 4096;
			xd_rsdb_put_extstats();

			//update the task min hash
			xdag_info("update task : rt hashed %d rt discard %d",rt.hashed,rt.discards);
			rx_update_task_min_hash(rt,output_hash);
			xdag_info("update task : finished");
			pthread_testcancel();
		}else{
			pthread_testcancel();
			xdag_info("task queue empty waiting ...");
			//pthread_mutex_unlock(&g_pick_task_mutex);
			sleep(1);
			continue;
		}
	}
	pthread_cleanup_pop(NULL);

	return NULL;
}

static void rx_update_task_min_hash(rx_pool_task task,xdag_hash_t hash){
	xdag_info("rx update task min hash");
	pthread_mutex_lock(&g_update_min_mutex);
	if(!task.hashed){
		task.hashed=true;
		memcpy(task.minhash,hash,sizeof(xdag_hash_t));
		update_rx_task_by_prehash(task.prehash,task);
		pthread_mutex_unlock(&g_update_min_mutex);
		return;
	}

	if(xdag_cmphash(hash,task.minhash) < 0){
		memcpy(task.minhash,hash,sizeof(xdag_hash_t));
		update_rx_task_by_prehash(task.prehash,task);
		pthread_mutex_unlock(&g_update_min_mutex);
		return;
	}
	pthread_mutex_unlock(&g_update_min_mutex);
	return;
}

static int rx_send_to_pool(struct xdag_field *fld, int nfld)
{
	struct xdag_field f[XDAG_BLOCK_FIELDS];
	xdag_hash_t h;
	struct miner *m = &g_local_miner;
	int todo = nfld * sizeof(struct xdag_field), done = 0;

	if(g_socket < 0) {
		return -1;
	}

	memcpy(f, fld, todo);

	if(nfld == XDAG_BLOCK_FIELDS) {
		f[0].transport_header = 0;
		xdag_hash(f, sizeof(struct xdag_block), h);
		f[0].transport_header = BLOCK_HEADER_WORD;
		uint32_t crc = crc_of_array((uint8_t*)f, sizeof(struct xdag_block));
		f[0].transport_header |= (uint64_t)crc << 32;
	}

	// send rx pow to rx pool
	if(nfld == RX_POW_BLOCK_FIELDS){
		f[0].transport_header = 0;
		f[0].transport_header = RX_POW_HEADER_WORD;
		uint32_t crc = crc_of_array((uint8_t*)f, sizeof(struct xdag_field)*RX_POW_BLOCK_FIELDS);
		f[0].transport_header |= (uint64_t)crc << 32;
	}

	for(int i = 0; i < nfld; ++i) {
		dfslib_encrypt_array(g_crypt, (uint32_t*)(f + i), DATA_SIZE, m->nfield_out++);
	}

	while(todo) {
		struct pollfd p;

		p.fd = g_socket;
		p.events = POLLOUT;

		if(!poll(&p, 1, 1000)) continue;

		if(p.revents & (POLLHUP | POLLERR)) {
			return -1;
		}

		if(!(p.revents & POLLOUT)) continue;

		int res = write(g_socket, (uint8_t*)f + done, todo);
		if(res <= 0) {
			return -1;
		}

		done += res;
		todo -= res;
	}

	if(nfld == XDAG_BLOCK_FIELDS) {
		xdag_info("Sent  : %016llx%016llx%016llx%016llx t=%llx res=%d",
		          h[3], h[2], h[1], h[0], fld[0].time, 0);
	}
	
	return 0;
}
