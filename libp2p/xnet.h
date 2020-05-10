#ifndef XDAG_LIBP2P_H
#define XDAG_LIBP2P_H

#ifdef __cplusplus
extern "C" {
#endif
	
/* intializes the libp2p module */
extern int xdag_libp2p_init(void);

extern int xdag_libp2p_identify(void);
	
#ifdef __cplusplus
};
#endif

#endif
