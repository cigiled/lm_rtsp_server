#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__

#ifdef __cplusplus
	extern "C"
#endif

#include "avnet_base.h"

#define MAX_LEVEL_SZ   100
#define MIN_LEVEL_SZ   60
#define SEQ_OFFSET     32

enum
{
	RTSP_OPTION_CMD = 0,
	RTSP_DESCRIBE_CMD,
	RTSP_SETUP_CMD,
	RTSP_PLAY_CMD,
	RTSP_PAUSE_CMD,
	RTSP_TEARDOWN_CMD,
	RTSP_MAX_CMD,
};

typedef struct
{
	u8 ty;
	c8 *com;
	u8 len;
}judge_t;

typedef struct
{
	u8    ty;
	uint  s_rate; //采样率
	c8    *name;   
	
}a_cnf_t;

enum
{
	STAT_100 = 0,
	STAT_200,
	STAT_201,
	STAT_202,
	STAT_203,
	STAT_204,
	STAT_205,
	STAT_206,
	STAT_300,
	STAT_301,
	STAT_302,
	STAT_400,
	STAT_401,
	STAT_402,
	STAT_403,
	STAT_404,
	STAT_405,
	STAT_406,
	STAT_407,
	STAT_408,
	STAT_409,
	STAT_410,
	STAT_411,
	STAT_412,
	STAT_413,
	STAT_414,
	STAT_415,
	STAT_420,
	STAT_450,
	STAT_451,
	STAT_452,
	STAT_453,
	STAT_454,
	STAT_455,
	STAT_456,
	STAT_457,
	STAT_458,
	STAT_461,
	STAT_500,
	STAT_501,
	STAT_502,
	STAT_503,
	STAT_504,
	STAT_505,
	STAT_551,
	STAT_911,
	STAT_MAX,
};


typedef struct 
{
    char *token;
    int code;
	int s_seq;
}netstat_t;

int url_check(u8 *indata, av_t *av);
int get_type(c8 *data, uint *sz, u8 *type);

int option_proc(int     sock, char *indata, int sz, av_t *av);

int describe_proc(int      sock, char *indata, int sz, av_t *av);
int get_rtspurl(char *data, av_t *av, char *ip);
int accept_check(char *data, int sock);
int generate_sdp_description(char *data, int *sz, av_t *av, char *ip);
int get_stand_inf(char *data, rtsp_t *t, char *ip);
int get_media_inf(char *data, av_t *av, char *ip);

int setup_proc(int     sock, u8 *indata, int sz, av_t *av);
int get_transport_inf(u8 *data, char *desption, char *field, sess_t *ses);
int create_session(sess_t *session,  av_t *av);
int transport_proc(u8 *indata, u8 *oudata, av_t *av);

int play_proc(int    sock, u8 *indata, int sz, av_t *av);
int session_check(char *data, int sock);
int play_ctl(av_t *av);
int get_rtpinfo(char *outbuf, av_t *av);

int teardown_proc(int       sock, u8*indata, int sz, av_t *av);

int get_cseq(char *data, int sz);
void get_date(char *date, int sz);

netstat_t* get_status(int st);
int request_check(u8 *indata, int sock, int seq);

int error_respond(int stat, int sock, int cseq);
uint preset_next_Timestamp(uint tt_Freq);

#ifdef __cplusplus
 	};
#endif

#endif