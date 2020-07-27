#ifndef XDAG_NETWORK_H
#define XDAG_NETWORK_H

#ifdef __cplusplus
extern "C"
{
#endif

int xdag_network_init(void);
int xdag_connect_pool(const char* pool_address, const char** error_message);
void xdag_connection_close(int socket);

#ifdef __cplusplus
}
#endif

#endif // XDAG_NETWORK_H
