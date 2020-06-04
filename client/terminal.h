#ifndef XDAG_TERMINAL_H
#define XDAG_TERMINAL_H

#define UNIX_SOCK  "unix_sock.dat"

#ifdef __cplusplus
extern "C"
{
#endif

int terminal_client();
void *terminal_server(void *arg);

#ifdef __cplusplus
}
#endif

#endif //XDAG_TERMINAL_H
