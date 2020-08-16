#ifndef __AVNET_BASE_H__
#define __AVNET_BASE_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>

#include "base.h"
#include "fifo.h"


#ifdef __cplusplus
	extern "C" {
#endif


#define  LSTR  "live"
#define  FSTR  "file"

#define  MAX_FRMAES    1024 *256
#define  PER_RD_SZ	   1024*1024
#define  DEF_FRAMERATE (30)

#define  PTH_STATIC_SZ  (4*1024*1024)
#define  MAX_V_BUFFF    (1024*1024)
#define  MAX_A_BUFFF    (620*1024)
#define  MAX_FRAME_NUM  (30)  //max buff frame numbers。

#define  MAX_SEND_SZ    (1430)

#define MAX_NET_RBUFF   (2*1024)  //max net recv buff.
#define MAX_LISTENS     20      //max client socket listen nums.
#define SOCK_NUMS       10      //max client socket listen nums for per pthread.
#define INIPORT         (6970) 

#define RTPHEAD   	12
#define TCPST       4
#define RTPFUHEAD   (RTPHEAD+2)
#define OFFSIZE 	(RTPFUHEAD+TCPST) //tcp的rtp 包 比udp 多4字节.

#define AAC_OFFSZ   4  //AAC格式 在rtp打包时要在rtp负载前面加4字节，并且采样率高的数据可能要分片.
#define G711_OFFSZ  2  //sample 是几百个字节,不存在FU分片.

enum {
	TYPE_UDP,
	TYPE_TCP,
};

enum{
	T_LIVE,
	T_FILE,
};

enum
{
 	T_I,
	T_P,
	T_B,
	T_IDR,
	T_VPS,
	T_SPS,
	T_PPS,
	T_SEI
};

enum
{
	V_TRACK=1,
	A_TRACK,
};

enum{
	T_H264,
	T_H265,
	T_TS,
	T_MP4V_ES,
	T_AAC,
	T_G711A,
	T_G711U,
	T_PCM,
	T_MAX
};

enum{
	SAM_8000,
	SAM_11025,
	SAM_12000,
	SAM_16000,
	SAM_22050,
	SAM_24000,
	SAM_32000,
	SAM_44100,
	SAM_48000,
	SAM_MAX,
};

enum{
	SAMSZ_80,
	SAMSZ_160,
	SAMSZ_240,
	SAMSZ_320,
	SAMSZ_480,
	SAMSZ_1024,
	SAMSZ_MAX
};

enum
{
  RTP_UDP,
  RTP_TCP,
  RAW_UDP
};

typedef struct 
{
	int  fu_n;
	int  sz;
	char *data;
}packfu_t;

typedef struct 
{
	int  sz;
	char *data;
}packst_t;

//client socket.
struct sock_inf_t
{
	u8   istcp;
	uint sock;
	uint ip;
	uint port;
	struct sockaddr_in add; 
};

//
typedef struct sk
{
	u8    atrue;
	u8    vtrue;
	uint  sockid;
	uint  ctype;
	int   csock;
	uint  rtpsock;
	uint  rtcpsock;
	ulong ip;
	uint  port;
	struct sockaddr_in rtcpadd; 
	struct sockaddr_in rtpadd; 
}sock_t;

//if upper levels do not provide, will sparse file and get it if it's can.
typedef struct 
{ 
	uint sampsz;  //sample size
	uint chn;
	uint samprate; //sample rate
}Apara_t;


typedef struct
{
	uint 	 sock_nums; 
	u8 	 	 bfull;
	sock_t   sock_m[SOCK_NUMS];	
}c_sock_t; //client socket.

typedef struct
{
	uint 	 type; //server socket type.
	uint 	 sock; //server socket .
	uint 	 ip;
	uint 	 port; //server port .
	struct sockaddr_in rtcpadd; 
}s_sock_t; //client socket.


typedef struct 
{
	pthread_t rtsp_manpth[SOCK_NUMS];    //服务器管理的线程
	pthread_t streampth[SOCK_NUMS];   //负责发送流到客户端的线程
	
	pthread_t file_opt; 
	pthread_cond_t  f_cond;
	pthread_mutex_t f_mutex;
	u8 is_f_start;
}pth_t;

typedef struct
{
	u8   iscreate;
	uint c_rtp_port; //client rtp port
	uint c_rtcp_port;

	uint s_rtp_port; //server rtp port
	uint s_rtcp_port;
	uint ttl;
}stream_t;

typedef struct
{
	u8       net_ty;
	stream_t o_tcp; //over_tcp;
	stream_t o_udp;
	stream_t o_rawudp;
	stream_t o_http;
}sess_t; //session

typedef struct
{
	u8      v_type;
	u8      v_res;
	u8      v_fmt;
	u8      a_type;
	u8      a_res;
	u8      a_fmt;
	Apara_t apara;
	
	uint   cseq;
	uint   pl; //profile-level-id
	uint   len;
	uint   snums;
	sess_t ses[SOCK_NUMS];
	uint   a_track;
	uint   v_track;
	u8     *sps;
	u8     *pps;
}rtsp_t;

typedef struct
{
	int pos; //能够记录1024 *256帧的位置.
	int sz;  //记录帧的大小.
	int type;
	int sqe;//记录 startid, is 3 byte or 4 byte.
	u8* buff;//video stream buff,  get from fifo.
}frame_t;

typedef struct
{
	u8   *buff;
	uint remlen;
	uint sz;
}Sbuff_t;

// recoder video or audio key frmae , the addr ans time.
typedef struct
{
	uint c_addr; //当前数据位置
	uint c_time; //当前时间
	uint l_addr;//上一个I帧位置.
	uint l_time;
}kframe_t; 

typedef struct 
{
	char*   file;
	u8      ftpy;
	FILE*   fd;
	ullong  sz;
}file_t;

typedef struct
{
	uint 	 pl;  //payload;
	uint 	 fps; //vidoe frameRate
	uint 	 seq;
	uint 	 fmt;  //eg: T_H264,  T_H265
	file_t   fl;
	frame_t	 fm[MAX_FRAME_NUM]; //记录30个帧的位置,方便定位, 可以是连续的位置,也可以是倍数级的位置.	
	kframe_t kfm;
	
}v_fm_t;

typedef struct
{	
	uint	 pl;   //payload;
	uint 	 seq;
	uint 	 fmt;  //eg: T_AAC,  T_PCM
	uint 	 ts;   //audio timeScale
	uint 	 sd;   //audio sampleDuration
	file_t   fl;
	frame_t	 fm[MAX_FRAME_NUM -10];
	kframe_t kfm;
	u8*      str_bf;//audio stream buff
}a_fm_t;

typedef  struct 
{
	uint     seq;    //The serial number of       av_t struct.
	pth_t    pth;
	fd_set   r_fd;

	s_sock_t ss;
	c_sock_t cs;
	rtsp_t   rt;
	
	v_fm_t	 v_ft;
	a_fm_t	 a_ft;
}av_t;


#ifdef __cplusplus
		};
#endif

#endif

