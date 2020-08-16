#include "rtsp_server.h"
#include "net_base.h"
#include "utility.h"

//为了方便解析数据时，同时也为了避免每个字符都检测带来的cpu效率问题.
//当多个客户端时，要考虑粘包等问题.
judge_t jd[] = {
				 {RTSP_OPTION_CMD, "OPTION", 86}, {RTSP_DESCRIBE_CMD, "DESCRIBE", 100}, 
				 {RTSP_SETUP_CMD,  "SETUP", 136}, {RTSP_PLAY_CMD, "PLAY", 100}, 
				 {RTSP_PAUSE_CMD, "PAUSE", 60},   {RTSP_TEARDOWN_CMD, "TEARDOWN", 80}, 
			     {0}
			   };

//PCMA:   G.711A（PCMU:   G.711U）  ：  采样频率8000，采样精度16bit，单声道
a_cnf_t av_c[2][4] = {                                                  //33                 //97
					{{T_H264, 90000, "h264"}, {T_H264, 90000, "H265"},  {T_TS, 90000, "TS"}, {T_MP4V_ES, 2500, "MP4V-ES"}},
				 	{{T_AAC, 1024, "AAC"},  {T_G711A, 8000, "G711a"},  {T_G711U, 8000, "G711u"}, {T_PCM, 8000, "PCM"}}
				};


#define  E_BIT 64

#define RTSP_VER "RTSP/1.0"
#define S_VER NAME" v1.0.1"
#define D_SDP "Session Streamed by "NAME

#define ERRO_404 "404 Stream Not Found"

uint SessionId;
static uint base_tt = 0;

netstat_t status[] = {
			{"Continue", 100, STAT_100},
			{"OK", 200, STAT_200}, 
			{"Created", 201, STAT_201}, 
			{"Accepted", 202, STAT_202}, 
			{"Non-Authoritative Information", 203, STAT_203}, 
			{"No Content", 204, STAT_204}, 
			{"Reset Content", 205, STAT_205}, 
			{"Partial Content", 206, STAT_206}, 
			{"Multiple Choices", 300, STAT_300}, 
			{"Moved Permanently", 301, STAT_301}, 
			{"Moved Temporarily", 302, STAT_302}, 
			{"Bad Request", 400, STAT_400}, 
			{"Unauthorized", 401, STAT_401}, 
			{"Payment Required", 402, STAT_402}, 
			{"Forbidden", 403, STAT_403}, 
			{"Not Found", 404, STAT_404}, 
			{"Method Not Allowed", 405, STAT_405}, 
			{"Not Acceptable", 406, STAT_406}, 
			{"Proxy Authentication Required", 407, STAT_407}, 
			{"Request Time-out", 408, STAT_408}, 
			{"Conflict", 409, STAT_409}, 
			{"Gone", 410, STAT_410}, 
			{"Length Required", 411, STAT_411}, 
			{"Precondition Failed", 412, STAT_412}, 
			{"Request Entity Too Large", 413, STAT_413}, 
			{"Request-URI Too Large", 414, STAT_414}, 
			{"Unsupported Media Type", 415, STAT_415}, 
			{"Bad Extension", 420, STAT_420}, 
			{"Invalid Parameter", 450, STAT_450}, 
			{"Parameter Not Understood", 451, STAT_451}, 
			{"Conference Not Found", 452, STAT_452}, 
			{"Not Enough Bandwidth", 453, STAT_453}, 
			{"Session Not Found", 454, STAT_454}, 
			{"Method Not Valid In This State", 455, STAT_455}, 
			{"Header Field Not Valid for Resource", 456, STAT_456}, 
			{"Invalid Range", 457, STAT_457}, 
			{"Parameter Is Read-Only", 458, STAT_458}, 
			{"Unsupported transport", 461, STAT_461}, 
			{"Internal Server Error", 500, STAT_500}, 
			{"Not Implemented", 501, STAT_501}, 
			{"Bad Gateway", 502, STAT_502}, 
			{"Service Unavailable", 503, STAT_503}, 
			{"Gateway Time-out", 504, STAT_504}, 
			{"RTSP Version Not Supported", 505, STAT_505}, 
			{"Option not supported", 551, STAT_551}, 
			{"Extended Error:", 911, STAT_911}, 
			{NULL, -1, STAT_MAX}
};

int get_type(c8 *data, uint *sz, u8 *type)
{
	printf("\033[0;31mRECV:{%s}\n", data);
	c8 *tmp = data;
	c8 *st = data;
		
	int i,t;
	int seq = 0; 

	PRT_CONDX(*sz < MIN_LEVEL_SZ, "recv size < min pakcet Judgement standard .", -1);
	
	for(i = 0; i < MAX_LEVEL_SZ;)
	{
		if(strstr(&tmp[i], jd[seq].com)) //judge type
		{
			*type = jd[seq].ty;
			tmp = &tmp[i];
			i = jd[seq].len; //从偏移i处开始解析,避免每个字符都检查,提高效率.
			t = &data[*sz] - tmp; //剩余总的.
			while(i <= t) 
			{//judge complate request packet.
			
				if(tmp[i] == '\r' && tmp[i+1] == '\n')
				{
					if(&tmp[i] - st == 2)
					{// is complate request packet.
						*sz = i + 2;
						return tmp - data; //
					}
					st = &tmp[i];
				}
				i++;
			}
			return -1;
		}
		
		i++;
		if(i == MAX_LEVEL_SZ -1)
		{
			if( ((*sz-MAX_LEVEL_SZ) / MAX_LEVEL_SZ) != 0 )
			{//若后面还有至少 MAX_LEVEL_SZ 个数据，则接着判断.  [主要处理粘包问题.]
				tmp = &tmp[i];
				i = 0;
			}
			else 
			{//next command judge.
				seq++;
				i = 0;
				if(!jd[seq].com)
					break;
			}
		}
	}	

	return -2;
}

int option_proc(int      sock, char *indata, int sz, av_t *av)
{	
	char s_buff[160] = {'\0'};
	char date[54]= {'\0'};
	int len = sizeof(s_buff);
	int ret, seq = 1;

	struct sock_inf_t inf;
	inf.istcp = True;
	inf.sock = sock;
	ret = request_check(indata, sock, seq);
	PRT_COND(ret < 0, "Bad request")
		
	seq = get_cseq(indata + ret, sz);
	av->rt.cseq = seq;
	get_date(date, sizeof(date));
	c8 *cmands = "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER";

	snprintf(s_buff, len, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n%sPublic: %s\r\n\r\n", seq, date, cmands);

	send_data(&inf, s_buff, len);
	return 0;
}


const char* test1 = "RTSP/1.0 200 OK\r\n" 
					"CSeq: 3\r\n"
					"Date: Sat, Apr 25 2020 04:26:12 GMT\r\n"
					"Content-Base: rtsp://192.168.1.104:8554/nat.264/\r\n"
					"Content-Type: application/sdp\r\n"
					"Content-Length: 499\r\n\r\n"
					"v=0\r\n" 
					"o=- 1582594486294163 1 IN IP4 192.168.1.104\r\n"
					"s=H.264 Video, streamed by the LIVE555 Media Server\r\n"
					"i=nat.264\r\n"
					"t=0 0\r\n"
					"a=tool:LIVE555 Streaming Media v2020.02.11\r\n"
					"a=type:broadcast\r\n"
					"a=control:*\r\n"
					"a=range:npt=0-\r\n"
					"a=x-qt-text-nam:H.264 Video, streamed by the LIVE555 Media Server\r\n"
					"a=x-qt-text-inf:nat.264\r\n"
					"m=video 0 RTP/AVP 96\r\n"
					"c=IN IP4 0.0.0.0\r\n"
					"b=AS:500\r\n"
					"a=rtpmap:96 H264/90000\r\n"
					"a=fmtp:96 packetization-mode=1;profile-level-id=4D401E;sprop-parameter-sets=Z01AHpUVGIBYCTI=,aOqOIA==\r\n"
					"a=control:track1\r\n";

int describe_proc(int      sock, char* indata, int sz, av_t *av)
{	
#if 1
	char s_buff[1024] = {'\0'};
	int len = sizeof(s_buff);
	char date[54]  = {'\0'};
	char url[86]   = {'\0'};
	char ip[16] = {'\0'};
	char sdp_inf[800] = {'\0'};
	int sdl_len = 0;
	int ret, seq;

	struct sock_inf_t inf;
	inf.istcp = True;
	inf.sock = sock;

	seq = av->rt.cseq+1;
	ret = request_check(indata, sock, seq);
	PRT_COND(ret < 0, "Bad request")
		
	seq = get_cseq(indata + ret, sz);
	ret = accept_check(indata +ret +6, sock);
	PRT_COND(ret < 0, "spare request error")
		
	av->rt.cseq = seq;
	get_date(date, sizeof(date));
	get_rtspurl(url, av, ip);
	ret = generate_sdp_description(sdp_inf, &sdl_len, av, ip);
	
	len = sprintf(s_buff,
				"RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
				"%s"
				"Content-Base: %s/\r\n"
				"Content-Type: application/sdp\r\n" 
				"Content-Length: %d\r\n\r\n"
				"%s",
				seq, date,url,sdl_len,sdp_inf);
	//printf("toal len:[%d]:[%d]\n", sdl_len, len);
	printf("33[%d]:[%ld]\n", len, strlen(s_buff));
	send_data(&inf, s_buff, len);
	#else
	struct sock_inf_t inf;
	char s_buff[1024] = {'\0'};
	int len = sizeof(s_buff);
	
	inf.istcp = True;
	inf.sock = sock;
	printf("33[%d]:[%d]\n", len, strlen(test1));
	send_data(&inf, test1, strlen(test1));

	#endif
	return 0;
}

/*
SETUP rtsp://192.168.1.104:8554/nat.264/track1 RTSP/1.0 
CSeq: 4
User-Agent: LibVLC/3.0.5 (LIVE555 Streaming Media v2016.11.28) 
Transport: RTP/AVP;sunicast;client_port=52956-52957

RTSP/1.0 200 OK 
CSeq: 4 
Date: Tue, Feb 25 2020 01:34:46 GMT 
Transport: RTP/AVP;unicast;destination=192.168.1.100;source=192.168.1.104;client_port=52956-52957;server_port=6970-6971 
Session: CF34BBE4;timeout=65
*/
int setup_proc(int      sock, u8 *indata, int sz, av_t *av)
{	
	int len, ret, seq;
	char date[46]  = {'\0'};
	char s_buff[256] = {'\0'};
	char tp[152] = {'\0'};

	struct sock_inf_t inf;
	inf.istcp = True;
	inf.sock = sock;

	seq = av->rt.cseq+1;
	ret = request_check(indata, sock, seq);
	PRT_COND(ret < 0, "Bad request");

	if(url_check(indata, av) < 0)
	{
		error_respond(STAT_400, sock, seq); //bad request
		return -1;
	}

	seq = get_cseq(indata + ret, sz);
	av->rt.cseq = seq;
	
	transport_proc(indata, tp, av);
	get_date(date, sizeof(date));
	SessionId = get_random32();
	
	len = sprintf(s_buff,
					"RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
					"%s"
					"Transport: %s"
					"Session:%08X\r\n\r\n", 
					seq, date, tp, SessionId);
	
	send_data(&inf, s_buff, len);
	return len;
}

/*
PLAY rtsp://192.168.1.104:8554/nat.264/ RTSP/1.0 
CSeq: 5 
User-Agent: LibVLC/3.0.5 (LIVE555 Streaming Media v2016.11.28) 
Session: CF34BBE4 Range: npt=0.000-
*/
int play_proc(int    sock, u8 *indata, int sz, av_t *av)
{	
	int ret, seq, len = 0;
	char date[46]  = {'\0'};
	char s_buff[256] = {'\0'};
	char rtpinfo[186] = {'\0'};

	struct sock_inf_t inf;
	inf.istcp = True;
	inf.sock = sock;
	
	seq = av->rt.cseq+1;
	ret = request_check(indata, sock, seq);
	PRT_COND(ret < 0, "Bad request");

	seq = get_cseq(indata + ret, sz);
	av->rt.cseq = seq;

	session_check(indata +ret +5, sock);
	PRT_COND(ret < 0, "session don't found");
	play_ctl(av);
	
	get_rtpinfo(rtpinfo, av);
	get_date(date, sizeof(date));

	len = sprintf(s_buff,
					"RTSP/1.0 200 OK\r\n"
					"CSeq: %d\r\n"
					"%s"
					"Range: npt=0.000-\r\n"
					"Session:%08X\r\n"
					"RTP-Info: %s\r\n",
					seq, date, SessionId, rtpinfo);
	
	send_data(&inf, s_buff, len);

	return len;
}

int session_check(char *data, int sock)
{
	char *p = NULL;
	if( (p = strstr(data, "Session")))
	{
	    if (sscanf(p +7, "%d",  &SessionId) != 1) 
        {
            error_respond(STAT_454, sock, 0);/* Session Not Found */
            return -1;
        }
			
	}
	else {
		return -1;
	}
	
	return 0;
}

int play_ctl(av_t *av)
{
	//Do't pars "Scale:", "Range:"

	// start stream proc.
	if(av->pth.is_f_start != True)
	{
		pthread_mutex_lock(&av->pth.f_mutex);
		pthread_cond_signal(&av->pth.f_cond);
		pthread_mutex_unlock(&av->pth.f_mutex);
	}

	return 0;
}

//RTP-Info: url=rtsp://192.168.1.114:5544/live1.264/track1;seq=33799;rtptime=2147418367,url=rtsp://192.168.1.114:5544/live1.264/track2;seq=37410;rtptime=2147418367
int get_rtpinfo(char *outbuf, av_t *av)
{
	char ip[16] ={'\0'};

	int len = 0;
	ushort rtpSeqNum  = random();
	uint rtpTimestamp = get_random32();
	uint s_ip = av->ss.ip;

	uint seq = av->v_ft.fmt - T_H264; 
	uint tt = av_c[0][seq].s_rate;
	
	rtpTimestamp = preset_next_Timestamp(tt);
	sprintf(ip, "%u.%u.%u.%u", (s_ip>>24)&0xFF, (s_ip>>16)&0xFF, (s_ip>>8)&0xFF, s_ip&0xFF);

	if(av->rt.v_track > 0)
	{
		len = sprintf(outbuf, "url=rtsp://%s:%d/%s/track1;seq=%d;rtptime=%u\r\n", ip, av->ss.port, 
																	av->v_ft.fl.file, rtpSeqNum, rtpTimestamp);
	}

	if(av->rt.a_track > 0)
	{
		len += sprintf(outbuf+len, ";url=rtsp://%s:%d/%s/track2;seq=%d;rtptime=%u\r\n", ip, av->ss.port,
																	av->v_ft.fl.file, rtpSeqNum, rtpTimestamp);
	}
	return len;
}

uint preset_next_Timestamp(uint tt_Freq)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	uint timetamp = 0;
	base_tt = get_random32();
	
	timetamp = tv.tv_sec * tt_Freq + ((tv.tv_usec/1000000.0) + 0.5) * tt_Freq;
	timetamp += base_tt;

	return timetamp;
}

int error_respond(int stat, int sock, int cseq)
{
	struct sock_inf_t inf;
	inf.istcp = True;
	inf.sock = sock;
	
	int len = 0;
	char st_buff[82]={'\0'};
	netstat_t* st = get_status(stat);

    len = sprintf(st_buff, "%s %d %s\r\n""CSeq: %d\r\n", RTSP_VER, st->code, st->token, cseq);

	send_data(&inf, st_buff, len);

	return 0;
}

int request_check(u8 *indata, int sock, int seq)
{
	int len = 0;
	char url[140] = {'\0'};
		
	if (!sscanf(indata, " %*s %254s ", url))
    {
        error_respond(STAT_400, sock, seq); //bad request
        return -1;
    }

	len = strlen(url) + 4 + 8;//this len = comd + url + version;
	return len;
}

int url_check(u8 *indata, av_t *av)
{
	//only check track id.
	if(av->rt.a_track > 0)
	{
		if(strstr(indata+10, "track1"))
			return 1;
	}
	else if(av->rt.v_track > 0)
	{
		if(strstr(indata+10, "track2"))
			return 1;
	}
	
	return -1;
}

int transport_proc(u8 *indata, u8 *oudata, av_t *av)
{
	int ret, len = 0;
	char tmp[24] = {'\0'};;
	char field[12] = {'\0'};
	char sour[24]={'\0'};

	uint s_ip = av->ss.ip;
	
	sess_t *ses = &av->rt.ses[av->rt.snums++];
	
	get_transport_inf(indata, tmp, field, ses);
	ret = create_session(ses, av);
	PRT_COND(ret < 0, "create session failed");
	
	sprintf(sour, "%u.%u.%u.%u", (s_ip>>24)&0xFF, (s_ip>>16)&0xFF, (s_ip>>8)&0xFF, s_ip&0xFF);

    //单播模式:
	if(ses->net_ty == RTP_UDP)
	{
		len = sprintf(oudata, "%s;unicast;destination=%s;source=%s;client_port=%d-%d;server_port=%d-%d\r\n",
					field, tmp, sour, 
					ntohs(ses->o_udp.c_rtp_port), ntohs(ses->o_udp.c_rtcp_port), 
					ntohs(ses->o_udp.s_rtp_port), ntohs(ses->o_udp.s_rtcp_port));
	}
	else if(ses->net_ty == RTP_TCP)
	{
		len = sprintf(oudata, "Transport: %s;unicast;destination=%s;source=%s;interleaved=%d-%d\r\n",
					field, tmp, sour, 
					ses->o_udp.s_rtp_port, ses->o_udp.s_rtcp_port);
	}	
	else if(ses->net_ty == RAW_UDP)
	{
		len = sprintf(oudata, "Transport: %s;unicast;destination=%s;source=%s;client_port=%d;server_port=%d\r\n",
					field, tmp, sour, 
					ntohs(ses->o_udp.c_rtp_port),  ntohs(ses->o_udp.s_rtp_port));
	}	
	
	return len;
}

int create_session(sess_t *session,  av_t *av)
{	
	PRT_COND(!session, "Input session is null, exit !");
	static uint baseport = INIPORT;
	static int num = 0;
	sock_t *socks = &av->cs.sock_m[num++];
	int sock;
	
	struct sock_inf_t sessinf;
	
	if(av->rt.a_track > 0)
		socks->atrue = True;
	if(av->rt.v_track > 0)
		socks->vtrue = True;
	
	if(session->net_ty == RTP_UDP)
	{
		if(!session->o_udp.iscreate)
		{
			sessinf.istcp = RTP_UDP;
			sessinf.port = baseport++;
			
			sock = create_socket(&sessinf);
			PRT_COND(sock < 0, "Create sock failed !");
			// for other place use it. 
			socks->rtpsock = sock;
			socks->ctype = TYPE_UDP;
			socks->rtpadd = sessinf.add;

			//for setup session use
			session->o_udp.s_rtp_port = sessinf.port;
			
			if(session->o_udp.c_rtcp_port > 0)
			{
				sessinf.port = baseport++;
				sock =  create_socket(&sessinf);
				PRT_COND(sock < 0, "Create sock failed !");
				
				socks->rtcpsock = sock;
				socks->rtcpadd = sessinf.add;

				//for setup session use
				session->o_udp.s_rtcp_port = sessinf.port;
			}
			session->o_udp.iscreate = True;
		}
	}
	else if(session->net_ty == RTP_TCP)
	{
		//....tcp accept default be used 
		socks->rtpsock = socks->csock;
		socks->rtpsock = socks->csock;
		socks->ctype = TYPE_TCP;
	}
	else if(session->net_ty == RAW_UDP)
	{
		if(!session->o_rawudp.iscreate)
		{
			sessinf.istcp = RTP_UDP;
			sessinf.port = baseport++;
			
			sock = create_socket(&sessinf);
			PRT_COND(sock < 0, "Create sock failed !");
			
			socks->ctype = TYPE_UDP;
			socks->rtcpadd = sessinf.add;

			session->o_rawudp.s_rtp_port = sessinf.port;
			session->o_rawudp.iscreate = True;
		}
	}
	else
	{
		error_respond(STAT_461, socks->csock, 0);
		return -1;
	}

	return 0;
}

int get_transport_inf(u8 *data, char *desption, char *field, sess_t *ses)
{
	PRT_COND(!data || !ses , "Input data or ses is null, exit !");

	u8 flag = False;
	char *str = NULL;
	int len = 0;
	while(1)
	{
		if(strncasecmp(data++, "Transport:", 10) == 0) //filter "Transport:"
		{ 
			 data += 10;
			 flag = True;
			 continue;
		}
		
		if(flag && *data != ' ') //filter ' '
			break;
		
		data++;
		
		if (*data == '\0') return -1; // not found
		if (*data == '\r' && *(data+1) == '\n' && *(data+2) == '\r') 
		return -1; 
	}

	//Transport: RTP/AVP;unicast;destination=192.168.1.100;source=192.168.1.104;client_port=52956-52957;server_port=6970-6971 
    char bf[84]= {'\0'};
	ushort  p1, p2;
	uint ttl, rtpCid, rtcpCid;
	sess_t session;
	
	str = data;	
	while(sscanf(str, "%[^;\r\n]", bf) == 1)
	{
		if (strcmp(bf, "RTP/AVP") == 0) 
		{
		 	 session.net_ty = RTP_UDP;
			 memcpy(field, bf, 7);
		} 
		else if (strcmp(bf, "RTP/AVP/TCP") == 0) 
		{
		 	 session.net_ty = RTP_TCP;
			 memcpy(field, bf, 11);
		} 
		else if (strcmp(bf, "RAW/RAW/UDP") == 0 || strcmp(bf, "MP2T/H2221/UDP") == 0)
		{
			session.net_ty = RAW_UDP;
			memcpy(field, bf, strlen(bf));
		} 
		else if (strncasecmp(bf, "destination=", 12) == 0) 
		{
			memcpy(desption, bf+12, strlen(bf+12));
		} else if (sscanf(bf, "ttl%u", &ttl) == 1) {
		  	 //session.ttl = ttl;
		} 
		else if (sscanf(bf, "client_port=%hu-%hu", &p1, &p2) == 2) 
		{
			  session.o_udp.c_rtp_port  = p1;
			  session.o_udp.c_rtcp_port = session.net_ty == RAW_UDP ? 0 : p2; // ignore the second port number if the client asked for raw UDP
		} 
		else if (sscanf(bf, "client_port=%hu", &p1) == 1)
		{
			  session.o_rawudp.c_rtp_port  = p1;
			  session.o_rawudp.c_rtcp_port = session.net_ty == RAW_UDP ? 0 : p1 + 1;
		} 
		else if (sscanf(bf, "interleaved=%u-%u", &rtpCid, &rtcpCid) == 2) 
		{
			 session.o_tcp.c_rtp_port  = rtpCid;
			 session.o_tcp.c_rtcp_port = rtcpCid;
		}
		
		printf("1:[%s]:[%ld]\n", bf, strlen(bf));
		
		str += strlen(bf)+1;
		if (*str == '\0' ) break;
	}

	*ses = session;
	return len;
}

int teardown_proc(int       sock, u8 *indata, int sz, av_t *av)
{	
	int len = 0;
	
	return len;
}

void get_date(char *date, int sz)
{
	time_t tm = time(NULL);
	strftime(date, sz, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tm));
}

int get_cseq(char *data, int sz)
{
	char *tmp = NULL;
	if( (tmp = strstr(data, "CSeq: ")) )
	{
		data = tmp + strlen("CSeq: ") + 2;
		return atoi(&tmp[6]);
	}
	else
		return 2;
}

int get_rtspurl(char *data, av_t *av, char *ip)
{
	uint s_ip = av->ss.ip;
	sprintf(ip, "%u.%u.%u.%u", (s_ip>>24)&0xFF, (s_ip>>16)&0xFF, (s_ip>>8)&0xFF, s_ip&0xFF);

	if(av->ss.port == 554)
		sprintf(data, "rtsp://%s/", ip);
	else
		sprintf(data, "rtsp://%s:%hu/", ip, av->ss.port);

	strcat(data, "av.h264"); //av is custom flage.
	return 0;
}

int generate_sdp_description(char *data, int *sz, av_t *av, char *ip)
{
	int len = 0;
	
	len = get_stand_inf(data, &av->rt, ip);
	printf("11[%d]:[%ld]\n", len, strlen(data));
	len += get_media_inf(data + len, av, ip);
	printf("22[%d]:[%ld]\n", len, strlen(data));
	*sz = len;
	return 0;
}
int accept_check(char *data, int sock)
{
	int ret = 0;
	if (strstr(data, "Accept") != NULL) 
    {
        if (strstr(data + 6, "application/sdp") == NULL) 
            ret = -1;
    }
	else{
         ret = -1;
	}

	if(ret < 0)
		error_respond(STAT_551, sock, 0); /* 不支持的媒体类型*/

	return ret;
}

// https://blog.csdn.net/weixin_36983723/article/details/85125161
// https://my.oschina.net/u/1431835/blog/393313?fromerr=nhT8dNT4
int get_stand_inf(char *data, rtsp_t *t, char *ip)
{
	PRT_COND(data==NULL || ip == NULL , "data or ip is NULL,");

	int len = 0;
	u8 isvideo = False;
	if(t->v_fmt >= T_H264 && t->v_fmt <= T_MP4V_ES)
		isvideo = True;
	
	struct timeval ct;
	gettimeofday(&ct, NULL); //default 0.	
	const char* PreFmt ="v=0\r\n"
						"o=- %ld%06ld %d IN IP4 %s\r\n"
						"s=%s\r\n"              //会话信息          eg:s=Session Streamed by LIBZRTSP
						"i=%s.%s\r\n"           //        eg: i=live1.264
						"t=0 0\r\n"             //<开始时间><结束时间> 以秒表示的NTP
						"a=tool:%s\r\n"       //SDP创建者名字和版本号。
						"a=type:broadcast\r\n"  //会议类型:`broadcast', `meeting', `moderated', `test' and `H332'.
						"a=control:*\r\n"
						"a=range:npt=0-\r\n";  
	int seq = t->v_fmt - T_H264;
	len = sprintf(data, PreFmt, ct.tv_sec, ct.tv_usec, 1, ip,D_SDP, (t->v_type == T_FILE)?"av":"live",\
														 isvideo ? av_c[0][seq].name : av_c[1][seq].name, S_VER);
	//printf("description sz:[%d]\n", len);							  
	return len;
}

//ts流 占时不添加.
int get_media_inf(char *data, av_t *av, char *ip)
{
	PRT_COND(data==NULL || ip == NULL , "data or ip is NULL,");

	int len = 0;
	int seq = 0;
	rtsp_t *t = &(av->rt);
	if(t->v_fmt >= T_H264 && t->v_fmt <= T_MP4V_ES)
	{
		seq = t->v_fmt - T_H264;
		t->v_track = V_TRACK;
		const char* sdpVideoFmt =
								  "m=video 0 RTP/AVP 96\r\n"
								  "c=IN IP4 %s\r\n"
								  "b=AS:%u\r\n"
								  "a=rtpmap:96 %s/%d\r\n"
								  "a=fmtp:96 packetization-mode=1;profile-level-id=4D401E;sprop-parameter-sets=%s,%s\r\n" 
								  "a=control:track1\r\n";
		
		len = sprintf(data, sdpVideoFmt, ip, E_BIT, av_c[0][seq].name, av_c[0][seq].s_rate, av->rt.sps, av->rt.pps);	
	}
	
	if(t->a_fmt >= T_AAC && t->a_fmt < T_MAX)
	{
		seq = t->a_fmt - T_AAC;
		t->a_track = A_TRACK;
		const char* sdpAudioFmt =
								  "m=audio 0 RTP/AVP 0\r\n"
								  "c=IN IP4 %s\r\n"
								  "b=AS:%u\r\n"
								  "a=rtpmap:0 %s/%d"        //"a=rtpmap:0 PCMU/8000"
								  "a=control:track2\r\n";
		len += sprintf(data + len, sdpAudioFmt, ip, E_BIT, av_c[1][seq].name, av_c[1][seq].s_rate);	
	}

	len += sprintf(data + len, "\r\n");
	//printf("-media len:[%d]-----\n", len);
	return len;
}

//frome network.
netstat_t* get_status(int st)
{
	if(st < STAT_100 || st > STAT_MAX)
		return NULL;
    
    return &status[st];
}
