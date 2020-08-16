#include "net_base.h"

uint addlen = sizeof(struct sockaddr_in);

int create_socket(struct sock_inf_t *inf)
{
	PRT_COND(!inf, "[create_sock]==>Input [inf ]  point is NULL, exit !");

	u8	 bistcp = inf->istcp;
	uint port = inf->port;

	int sock = socket(AF_INET, bistcp ? SOCK_STREAM : SOCK_DGRAM, 0);
	PRT_COND(sock < 0, "Create socket faile,  exit !");

	int on= 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

	struct sockaddr_in ser_addr;
	socklen_t  len = sizeof(struct sockaddr_in);
	memset(&ser_addr, 0, len);

	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port   = htons(port);
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int err = bind(sock, (struct sockaddr *)&ser_addr, len);
	if(err < 0)
	{
		close(sock);
		PRT_ERROR(True, "fail to bind");
	}

	inf->add = ser_addr;
	inf->sock = sock;
	if(bistcp)
	{
		err = listen(sock, MAX_LISTENS);
		if(err < 0)
		{
			close(sock);
			PRT_ERROR(True, "fail to listen");
		}
	}
	
	set_socket(sock, ~O_NONBLOCK, 0);
	return sock;
}

int set_socket( int sock, int bblock, int bset_tmout)
{
	signal(SIGPIPE, SIG_IGN); //ignore SIGPIPE signal.
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0)|(bblock));//set be no block mode.
	if(bset_tmout)
	{
		struct timeval timeout ={1,0};//3s  st sock recv timeout.
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	}
	//影响着对应sock的 io读写操作.
    return 0;
}

// call after connect
int get_client_ip_port(sock_t *sc)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	
	if( getpeername(sc->csock, (struct sockaddr *)&addr, &len) == 0)
	{
	   #if 1
		sc->port = ntohs(addr.sin_port);
		sc->ip = addr.sin_addr.s_addr;
		printf("port:[%d], ip:[%ld], sock:[%d]\n", sc->port, sc->ip, sc->csock);
	  #else  // debug
	 	char addr_str[128];
		inet_ntop(AF_INET, &addr.sin_addr, addr_str, sizeof(addr_str));
		fprintf(stderr, "%s ", addr_str);
		fprintf(stderr, "Port:%d\n",ntohs(((struct sockaddr_in *)&addr)->sin_port));
	  #endif
		return  1;
	}		

	return -1;
}

//get loacl ip or port.
uint get_self_ip(int sock)
{
	struct sockaddr_in sd;
	socklen_t  namelen = sizeof(sd);
	
	if(!getsockname(sock, (struct sockaddr*)&sd, &namelen))
	{
		#if 1
			uint addrNBO = htonl(sd.sin_addr.s_addr);
			//sprintf(ip, "%u.%u.%u.%u", (addrNBO>>24)&0xFF, (addrNBO>>16)&0xFF, (addrNBO>>8)&0xFF, addrNBO&0xFF);
			return addrNBO;
		#else
			char addr_str[20];
			inet_ntop(AF_INET, &sd.sin_addr, addr_str, sizeof(addr_str)); //转化成点分十进制
			printf("serverip:[%s]\n", addr_str);
		#endif
	}
	return -1;
}

int get_net_card_ip(char *ip)
{
	if(!ip)
		return -1;

	void *temadd = NULL;
	sa_family_t sa_f;
	struct ifaddrs *ia = NULL;
	
	getifaddrs(&ia);
	
	while(ia)
	{
		if(strncmp(ia->ifa_name, "lo", 2))
		{
			 char adbuff[16] = {'\0'};
			 temadd = &((struct sockaddr_in *)ia->ifa_addr)->sin_addr;
			 sa_f = ia->ifa_addr->sa_family;
			 inet_ntop((sa_f == AF_INET)?AF_INET:AF_INET6, temadd, adbuff, 16);
			 
			 if(adbuff[0] != '\0')
			 	sprintf(ip, "%s ", adbuff);
		}
		ia = ia->ifa_next;
	}

	return 0;
}

int send_data(struct sock_inf_t *inf, char *data, int size)
{
	PRT_COND(!data || !inf,  "[send_data]:Input [data or inf] point is NULL, exit !");
	PRT_COND(size < 0, "[send_data]:Input [size] is < 0 , exit !");

	int ret = 0;
	
	if(inf->istcp)
	{
		ret = send(inf->sock, data, size, 0);
		perror("send:");
	}
	else
	{
		ret = sendto(inf->sock, data, size, 0, (struct sockaddr *)&inf->add, addlen);
		perror("sendto:");
	}
	PRT_ERROR(ret < 0, "[send_data]: send data failed, ERROR:");
	
	printf("\033[0;32m""[send TNT]:[%d][%d]:[%d]:[%s]\n\n",  inf->sock, ret, size, data);
	return ret;
}
