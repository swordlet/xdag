#ifndef _XDAG_CONFIG_H_
#define _XDAG_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

struct pool_configuration {
	char *node_address;
	char *mining_configuration;
};

/**
 * get xdag pool config.
 * @param <buf> pool arg.
 * @param <path> configuration file path.
 * @return (none).
 */
int get_pool_config(const char *path, struct pool_configuration *pool_configuration);

#ifdef __cplusplus
}
#endif

#endif
