#ifndef __NET_BASE_H__
#define __NET_BASE_H__

#include <ifaddrs.h>
#include "avnet_base.h"

#ifdef __cplusplus
	extern "C" {
#endif

struct sock_inf_t;
typedef struct sk sock_t;

int create_socket(struct sock_inf_t *inf);
int send_data(struct sock_inf_t *inf, char *data, int size);
int recv_data_tcp(int sock, char *data, int size);

int set_socket(int sock,  int bblock, int bset_tmout);
uint get_self_ip(int sock);
int get_net_card_ip(char *ip);
int get_client_ip_port(sock_t *sc);

#ifdef __cplisplus
		};
#endif

#endif
