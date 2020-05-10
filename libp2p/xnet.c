#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include "xnet.h"
#include "libp2p.pb-c.h"
#include "base58.h"

#define UNIX_LIBP2P_SOCK  "/tmp/p2pd.sock"

static uv_loop_t *loop;
static uv_pipe_t libp2p_pipe;
static uv_connect_t conn;
static uv_tty_t tty_stdout;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

static void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
}

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

static void on_libp2p_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){
    Xdag__Response *response;
    Xdag__IdentifyResponse *identify;
    size_t len = buf->base[0];
    response = xdag__response__unpack(NULL, len, (uint8_t *)buf->base + 1);
    identify = response->identify;
    printf("response type:%d\n", response->type);
    printf("response identify n_addrs:%zu\n", identify->n_addrs);
    printf("response identify len:%zu\n", identify->id.len);

    size_t b58_size = 64;
    unsigned char b58[b58_size];
    memset(b58, 0, b58_size);
    unsigned char *ptr_b58 = b58;
    xdag_base58_encode(identify->id.data, identify->id.len, &ptr_b58, &b58_size);
    printf("response identify id :%s\n", ptr_b58);
//    uv_write((uv_write_t*)wri,(uv_stream_t*)&tty_stdout, &wri->buf,1, on_client_write_stdout);
}

static void on_libp2p_write(uv_write_t* req, int status){
    if(status){
        fprintf(stderr, "pipe write error %s\n", uv_strerror(status));
//        exit(0);
    }
    free_write_req(req);
    uv_read_start((uv_stream_t*)&libp2p_pipe, alloc_buffer, on_libp2p_read);
}

static void command_complete(uv_work_t* work, int status) {
    if(status < 0){
        fprintf(stderr, "pipe new conect error...\n");
    }
    free(work->data);
}

static void command_work(uv_work_t* work) {
    write_req_t *wri = work->data;
    uv_write((uv_write_t*)wri, (uv_stream_t*)&libp2p_pipe, &wri->buf, 1, on_libp2p_write);
}

static void on_libp2p_connect(uv_connect_t* req, int status) {
    if(status < 0){
        fprintf(stderr, "pipe new conect error...\n");
    }
//    uv_queue_work(loop, &libp2p_work, command_work, command_complete);
}

// intializes the libp2p module
int xdag_libp2p_init(void)
{
    loop = uv_default_loop();
    uv_pipe_init(loop, &libp2p_pipe, 0);
    uv_pipe_connect((uv_connect_t*)&conn, &libp2p_pipe, UNIX_LIBP2P_SOCK, on_libp2p_connect);
	return uv_run(loop, UV_RUN_DEFAULT);
}

int xdag_libp2p_get_identify(void)
{
    write_req_t *wri = (write_req_t *)malloc(sizeof(write_req_t));
    uv_work_t  *libp2p_work = malloc(sizeof(uv_work_t));
    Xdag__Request request = XDAG__REQUEST__INIT;
    request.type = XDAG__REQUEST__TYPE__IDENTIFY;
    size_t len = xdag__request__get_packed_size(&request);

    wri->buf=uv_buf_init((char*) malloc(len + 1), len + 1);
    wri->buf.base[0] = len;
    xdag__request__pack(&request, (uint8_t *)wri->buf.base + 1);
    libp2p_work->data = wri;
    uv_queue_work(loop, libp2p_work, command_work, command_complete);
    return uv_run(loop, UV_RUN_DEFAULT);
}
