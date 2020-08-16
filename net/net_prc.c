#include <sys/select.h>

#include "net_prc.h"
#include "file_man.h"
#include "rtp_rtcp.h"
#include "rtsp_server.h"
#include "str_audio.h"
#include "str_video.h"
#include "utility.h"

int send_rtpdata(sock_t *sc, int mtyp);
int send_video_rtpdata(sock_t *sc);
int send_audio_rtpdata(sock_t *sc);
int send_audio_rtcpdata(sock_t *sc);
int send_video_rtcpdata(sock_t *sc);

void *rtsp_server(void *argv);
void *send_stream(void *argv);
int data_anal_proc(int sock, av_t *av, u8 *indata, uint sz);
int check_client_is_valid(sock_t *st);

int close_csock(sock_t *cs, fd_set *fd_r);

//u8 is_f, const char *file, u8 f_ty, u8 v_fmt
int init_LMSver(media_t *md, av_t *av)
{
	PRT_COND(!md, "Input [md] is NULL !");
	
	if(md->m_ty == T_VIDEO_F)
	{
		av->rt.v_fmt  	 = md->v_fmt;
		av->rt.v_res	 = md->v_res;
		av->v_ft.pl      = md->v_pl; //default  //default h264,h265.
		av->v_ft.fmt     = md->v_fmt; 
		init_fifo(av->rt.v_res);
	}
	else if(md->m_ty == T_AUDIO_F)
	{
		av->rt.a_fmt     = md->a_fmt;
		av->rt.a_res	 = md->a_res;
		av->rt.apara     = md->aparam;
		av->a_ft.pl      = md->a_pl;  //default    aac.
		av->a_ft.fmt     = md->a_fmt; 
		init_fifo(av->rt.a_res);
	}
	else if(md->m_ty == T_AV_F)
	{
		av->rt.v_fmt  	 = md->v_fmt;
		av->rt.v_res	 = md->v_res;
		av->v_ft.pl      = md->v_pl;
		av->v_ft.fmt     = md->v_fmt; 
		av->rt.a_fmt	 = md->a_fmt;
		av->rt.a_res	 = md->a_res;
		av->rt.apara	 = md->aparam;
		av->a_ft.pl 	 = md->a_pl;
		av->a_ft.fmt     = md->v_fmt; 
		init_fifo(av->rt.v_res);
		init_fifo(av->rt.a_res);
	}
	
	if(!md->blive)
	{
		c8 * file = md->file;
        PRT_COND(!file, "file is NULL !");

		char *fpth = NULL;
		int len = strlen(file);
		fpth = malloc(len);
		PRT_COND(!fpth, "malloc file is NULL !");
		memcpy(fpth, file, len);	
		
		if(md->m_ty == T_VIDEO_F)
		{
			av->v_ft.fl.file = fpth;
			av->v_ft.fl.ftpy = T_VIDEO_F;
		}	
		else if(md->m_ty == T_AUDIO_F)
		{
			av->a_ft.fl.file = fpth;	
			av->v_ft.fl.ftpy = T_AUDIO_F;
		}	
		else if(md->m_ty == T_AV_F)
		{
			av->v_ft.fl.file = fpth;		
			av->a_ft.fl.file = fpth;	
			av->v_ft.fl.ftpy = T_AV_F;
		}
		
		av->rt.v_type = T_FILE;
		init_file_man(av);
	}
	else
	{// live stream
		av->rt.v_type = T_LIVE;
	}
	
    pthread_cond_init(&av->pth.f_cond, NULL);
    pthread_mutex_init(&av->pth.f_mutex, NULL);

	av->ss.type  = TYPE_TCP;
	av->ss.port	 = 8554;
	return 0;
}

int start_LMav_pth(av_t *av)
{
	static uint cnts = 0;
	pthread_t pth1, pth2;
	pthread_attr_t thread_attr; 

    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, PTH_STATIC_SZ);
	
	pthread_create(&pth1, &thread_attr, rtsp_server, av);
	pthread_create(&pth2, &thread_attr, send_stream, av);

	av->pth.rtsp_manpth[av->seq] = pth1;
	av->pth.streampth[av->seq] = pth2;
	av->seq  = cnts++;
	return 0;
}

int start_LMSver(void *argv)
{
	int sock, c_cnt = 0;
	int socklen = sizeof(sock_t);
	av_t *av = (av_t *)argv;
	sock_t *st = av->cs.sock_m;
	sock_t tmp;	

	struct sock_inf_t serinf;
	
	struct sockaddr_in client_addr;
	socklen_t  len = sizeof(struct sockaddr_in);

	memset(&tmp, 0, sizeof(sock_t));

	serinf.istcp = TYPE_TCP;
	serinf.port  = av->ss.port;

	
	sock = create_socket(&serinf);
	PRT_COND(sock < 0, "create socket failed !");

	av->ss.sock = sock;

	while(1)
	{
		usleep(300 * 1000);
		
		tmp.csock = accept(sock, (struct sockaddr *)&client_addr, &len);
		PRT_COND(tmp.csock < 0,  "server accept failed !");

		av->ss.ip = get_self_ip(tmp.csock);
		if(get_client_ip_port(&tmp) > 0)
		{
			if((c_cnt> 0) && (!(c_cnt / SOCK_NUMS)))
			{
				start_LMav_pth(av);
			}
			st[c_cnt]= tmp;
			set_socket(tmp.csock, O_NONBLOCK, 0);
			st[c_cnt].sockid = c_cnt++;
			av->cs.sock_nums = c_cnt;

			//检测是否有客户端有异常断网情况.
			//if()
			{
				check_client_is_valid(st);
			}

			memset(&tmp, 0, socklen);
		}
		else
		{
			error_respond(STAT_415, sock, 0);
		}
	}
}

void *rtsp_server(void *argv)
{

	av_t *av = (av_t *)argv;
	sock_t *tmpcsock = av->cs.sock_m;
	fd_set *fd_read = &(av->r_fd);
	int nums = av->cs.sock_nums;

	static int lastcnt = 0;
	int ret, i, currsock;
	struct timeval tmout;
	ret = i = currsock = 0;
	int maxfd = 24;
	FD_ZERO(fd_read);
	
	u8 *recv_date = malloc(MAX_NET_RBUFF);
	PRT_COND_NULL(!recv_date, "malloc net buff [select_date] failed !");

	tmout.tv_sec  = 0;
	tmout.tv_usec = 20 *1000;
	
	while(1)
	{
		usleep(100 *10);
		if(av->cs.sock_nums <= 0)
			continue;

		if(av->cs.sock_nums)
		{	
			FD_ZERO(fd_read);
			lastcnt = nums = av->cs.sock_nums;
			
			for(i = 0; i < nums; i++)
			{	
				if(tmpcsock[i].csock < 0)
					continue;	
				currsock = tmpcsock[i].csock;
				FD_SET(currsock, fd_read); //WT: 这里调用FD_ZERO(fd_read)导致服务器socket从监听数组被清除，导致只接受到一个client的链接.
			}
		}
		
		ret = select(maxfd, fd_read, NULL, NULL, &tmout);
		if(ret < 0)
		{	
			//[TWT] select 监听的某个sock, 还是sock集.
			printf("select failed !");
			continue;
		}
		else if(ret == 0)
		{
			//printf("select timeout !");
			//continue;
		}

		for(i = 0; i < nums; i++)
		{
			if(tmpcsock[i].csock < 0)
				continue;
			
			if(FD_ISSET(tmpcsock[i].csock, fd_read))
			{
				memset(recv_date, 0, MAX_NET_RBUFF/4);
				ret = recv(tmpcsock[i].csock, recv_date, MAX_NET_RBUFF, MSG_DONTWAIT);
				printf("===[%d]:[%d]====\n", ret, tmpcsock[i].csock);
				if((ret == -1) && ((errno == EWOULDBLOCK || errno == EAGAIN)) )
				{
				    printf("recv client <%d> data timeout !\n", tmpcsock[i].csock);
					close_csock(&tmpcsock[i], fd_read);
					continue;
				}
				else if((ret == 0) && errno != EINTR)
				{ 
					printf("client <%d> disconnect !\n", tmpcsock[i].csock);
					close_csock(&tmpcsock[i], fd_read);
					continue;
				}
					
				data_anal_proc(tmpcsock[i].csock, av, recv_date, ret);
			}
		}
		
		if(i++ >= nums)
		{
			i = 0;
			nums = av->cs.sock_nums; //update    the number of the client.
			if(nums >= SOCK_NUMS)
				nums = SOCK_NUMS;
		}
	}

	
	return 0;
}

// 检测 url里的 path 不存在 则回复 451.
int data_anal_proc(int sock, av_t *av, u8 * indata, uint sz)
{
	u8 *tmp = NULL;
	u8 type = RTSP_MAX_CMD;
	int ret = get_type((c8 *)indata, &sz, &type);
	if(ret == -1)
		error_respond(STAT_400, sock, 0);
	if(ret == -2)
		error_respond(STAT_404, sock, 0);
	
	PRT_COND(ret < 0, "Waring: request packet error !");
	
	tmp = indata + ret; //Where the command starts.
	switch(type)
	{
		case RTSP_OPTION_CMD:
				option_proc(sock, tmp, sz, av);
			break;
			
		case RTSP_DESCRIBE_CMD:
				describe_proc(sock, tmp, sz, av);
			break;
		
		case RTSP_SETUP_CMD:
				setup_proc(sock, tmp, sz, av);
			break;
		
		case RTSP_PLAY_CMD:
				play_proc(sock, tmp, sz, av);
			break;
		
		case RTSP_TEARDOWN_CMD:
				teardown_proc(sock, tmp, sz, av);
			break;
			
		default:
			error_respond(STAT_551, sock, 0);
			break;
	}
	return 0;
}

void *send_stream(void *argv)
{
	av_t* av = (av_t*)argv;
	sock_t *csocks = av->cs.sock_m;
	int socknum = av->cs.sock_nums;
	int i = 0;
		
	while(1)
	{
		for(i = 0; i < socknum; i++)
		{
			if(csocks[i].vtrue)
			{
				send_video_rtpdata(&csocks[i]);
				if(csocks[i].ctype == TYPE_UDP)
					send_video_rtcpdata(&csocks[i]);
			}
			
			if(csocks[i].atrue)
			{
				send_audio_rtpdata(&csocks[i]);
				if(csocks[i].ctype == TYPE_UDP)
					send_audio_rtcpdata(&csocks[i]);
			}
		}

		//free date.
		if(csocks[i].atrue)
			free_rtpdate(0); //free audio mem

		if(csocks[i].vtrue)
			free_rtpdate(1); //free video mem
		//do other...

		usleep(100 *100);
	}
	
	return 0;
}

int send_video_rtpdata(sock_t *sc)
{
	send_rtpdata(sc, 1);
	return 0;
}

int send_video_rtcpdata(sock_t *sc)
{

	return 0;
}

int send_audio_rtpdata(sock_t *sc)
{
	send_rtpdata(sc, 0);
	return 0;
}

int send_audio_rtcpdata(sock_t *sc)
{
	return 0;
}


int send_rtpdata(sock_t *sc, int mtyp)
{
	char *data = NULL;
	int fuover, offsz, len = 0;	
	struct sock_inf_t inf;

	inf.istcp = sc->ctype;
	inf.sock  = sc->rtcpsock;
	inf.add   = sc->rtcpadd; 

	if(sc->ctype == TYPE_TCP)
		offsz = 0;
	else
		offsz = 4;
					
	if(is_fu_pack(&fuover, mtyp))
	{
		do
		{
			//tcp 方式发送会在头部多加4个字节, 由于避免麻烦,在rtp填充时，不管udp还是tcp，
			//都填充了4字节的头, 这里根据网络类型来做相应偏移.
			if(get_fudata(data, &len, &fuover, mtyp) >0)
			{
				send_data(&inf, &data[offsz], len - offsz);
			}
				
		}while(fuover);
	}
	else
	{
		if(get_stdata(data, &len, mtyp) >0)
		{
			send_data(&inf, &data[offsz], len - offsz);
		}
	}

	return 0;
}

int  check_client_is_valid(sock_t *st)
{

	return 0;
}

//video
// the data have been filter the NALU.
int send_vframe_to_LMSver(char* data, int sz, v_fm_t *v)
{
	char* buff = NULL;
	int type = 0;
	static int vsqe = 0;
	int i = 0;
	do
	{
		buff = get_vbuff(sz +OFFSIZE);
	}while(buff || (i++ > 3));

	PRT_COND(!buff, "get video mem failed !");
	
	memcpy(&buff[OFFSIZE], data, sz);
	get_nalu_type(v->fmt, 0, data, &type);
	set_vpack_inf(v, buff, sz, type, vsqe++);
	
	pack_videortp(v);
	return 0;
}

int send_aframe_to_LMSver(char * data, int sz, a_fm_t *a)
{
	char* buff = NULL;
	static int asqe = 0;
	int media = a->fmt;
	int i = 0;
	do
	{
		buff = get_abuff(sz);
	}while(buff || (i++ > 3));

	PRT_COND(!buff, "get audio mem failed !");

	if(media == T_AAC)
	{
		memcpy(buff +OFFSIZE +AAC_OFFSZ, data, sz);
		set_apack_inf(a, buff, sz +RTPFUHEAD +AAC_OFFSZ, asqe++);
	}
	else if((media == T_G711A) || (media == T_G711U))
	{
		memcpy(buff, data +OFFSIZE -G711_OFFSZ, sz);
		set_apack_inf(a, buff, sz, asqe++);
	}	
	
	pack_audiortp(a);
	return 0;
}

int close_csock(sock_t *cs, fd_set *fd_r)
{
	PRT_COND( !cs || !fd_r, "input cs or fd_r is NULL .");
	
	close(cs->csock);
	FD_CLR(cs->csock, fd_r);
	cs->csock = -1;
	return 0;
}
