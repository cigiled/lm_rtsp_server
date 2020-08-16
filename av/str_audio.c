#include "str_audio.h"
#include "rtp_rtcp.h"


static char *ptr = NULL;
static Apara_t apara;
static int frame_len = 0;

//==================COMMON=====================
int init_audio(Apara_t *para)
{
	PRT_COND(!para, "input para is null .");
	PRT_COND(para->samprate > SAM_MAX || para->samprate < SAM_8000, "input sample rate is not range .");
	PRT_COND(para->sampsz > SAMSZ_MAX || para->sampsz < SAMSZ_80, "input sample size is not range .");
	
	apara = *para;
	frame_len = apara.sampsz;
	
	init_audio_rmem();
	
	return 0;
}

int init_audio_rmem()
{
	ptr = malloc(MAX_A_BUFFF);
	PRT_COND(!ptr, "maloc audio mem failed !");
	return 0;
}

int dest_audio_rmem()
{
	if(ptr)
		free(ptr);
	
	ptr = NULL;
	
	return 0;
}

int aac_proc(a_fm_t *a, FILE *fd)
{
	PRT_COND(!a || !fd, "input point  a or fd is NULL .");
	
	int ret = 0;
	do
	{
		ret = get_aac_frame(a, fd);
	}while(ret == -1); //-1表示获取的帧不完整，则再去获取.

	// [TWT] 文件读完，关闭回话，还是接着从头继续回话.
	pack_audiortp(a);
	return 0;
}

int g711a_proc(a_fm_t *a, FILE *fd)
{
	PRT_COND(!a || !fd, "input point  a or fd is NULL .");

	int ret = 0;
	do
	{
		ret = get_g711x_frame(a, fd);
	}while(ret == -1); //-1表示获取的帧不完整，则再去获取.

	pack_audiortp(a);
	return 0;
}

//==================AAC PROC=====================
int spare_aac(FILE *fd,  Aac_t * aac)
{
	PRT_COND(!aac, "Input aac point is NULL, exit !");
	PRT_ERROR(fd <= 0, "Open file failed :");
	
	char head[9] = {'\0'};
	int sz = fread(head, sizeof(head), 1, fd);
	if(sz > 0)
	{
		if(( (head[0]&0xff) == 0xff) && ((head[1]&0xf0) == 0xf0))
		{
			aac->mpeg_ver = (head[1] & 0x08) >> 3;
			aac->profile  = (head[2] & 0xc0) >> 6;
			aac->sampling = (head[2] & 0x3c) >> 2;
			aac->chns 	  = (head[2] & 0x01) << 2 | ((head[3] & 0xc0) >> 6);
		}
	}

	fseek(fd, 0L, SEEK_SET);
	return 0;
}

// [TWT] 存在不完整帧问题.
int get_aac_frame(a_fm_t *a, FILE *fd)
{
	PRT_COND(!a || !fd, "Input a or fd is NULL !");
	PRT_COND(!ptr, "audio mem is NULL !");
	static int i = 0;
	static int sqe = 0;
	static int re_sz = 0;
	static int a_remain = 0;
	static int frame_len = 0;
	static char end_flag = 1;
	static u8  fullframe = True;
	char *abuf = NULL;

	if(end_flag)
	{
		re_sz = fread(ptr, 1, MAX_A_BUFFF, fd);
		PRT_CONDX(re_sz<= 0, "read over !", -2);
		end_flag = True;
	}
			
	while(1)
	{				
		//Sync words: 0xfff;
		if(( (ptr[i]&0xff) == 0xff) && ((ptr[i+1]&0xf0) == 0xf0) )
		{   
			//frame_length: 13 bit
			frame_len = 0;
			frame_len |= (ptr[i+3] & 0x03) <<11;   //low    2 bit
			frame_len |= (ptr[i+4] & 0xff) <<3;	//middle 8 bit
			frame_len |= (ptr[i+5] & 0xe0) >>5;	//hight  3bit
			
			if(re_sz >= frame_len)
			{
				abuf= get_abuff(frame_len);
				memcpy(abuf +OFFSIZE +AAC_OFFSZ, &ptr[i], frame_len);
				set_apack_inf(a, abuf, frame_len +RTPFUHEAD +AAC_OFFSZ, sqe++);
				
				i += frame_len;    //加一帧的长度.
				re_sz -= frame_len;//剩下的帧数据...
				
				return 0;
			} 
			else
			{ //剩下帧不完整.
				abuf= get_abuff(re_sz);
				memcpy(abuf +OFFSIZE +AAC_OFFSZ, &ptr[i], re_sz); //保存剩下的帧数.
				set_apack_inf(a, abuf, re_sz +RTPFUHEAD +AAC_OFFSZ, sqe++);

				a_remain = frame_len - re_sz;
				
				end_flag  = fullframe = False;
				frame_len = i = re_sz = 0;
				return -1;
			}
		}
		else
		{
			if(a_remain > 0)
			{//处理上次不完整的帧.
				if(!fullframe)
				{
					fullframe = True;
					abuf = a->fm[a->seq -1].buff;
					if(abuf)
					{
						memcpy(abuf , &ptr[i], a_remain);
						set_apack_inf(a, abuf, frame_len, sqe -1);

						i += a_remain;   
						re_sz -= a_remain;
						a_remain = 0;
						return 0;
					}
				}
			}
		}
		i++;
	} 

	return 0;
}

//==================G711A PROC(pcma enc)=====================
int get_g711x_frame(a_fm_t *a, FILE *fd)
{
	PRT_COND(!a || !fd, "Input a or fd is NULL !");
	PRT_COND(!ptr, "audio mem is NULL !");

	static int i = 0;
	static int sqe = 0;
	static int re_sz = 0;
	static int a_remain = 0;
	static char end_flag = 1;
	char *abuf = NULL;
	int tmplen, offsize; 
	
	if(end_flag)
	{
		end_flag = 0;
		re_sz = fread(ptr, 1, MAX_A_BUFFF, fd);
		PRT_CONDX(re_sz<= 0, "read over !", -2);
	}
			
	while(1)
	{				
		if(re_sz >= frame_len)
		{
			if(a_remain > 0)
			{
				a_remain = frame_len - a_remain;
				tmplen = a_remain;
				abuf = a->fm[a->seq -1].buff;
				offsize = a->fm[a->seq -1].sz;
				memcpy(abuf + offsize, &ptr[i],tmplen); // TCPFLAG:4B + RTP_HEAD:12B + data
				tmplen += offsize; 
				sqe--;
			}
			else
			{
				abuf= get_abuff(frame_len);
				tmplen = frame_len;
				memcpy(abuf +OFFSIZE -G711_OFFSZ, &ptr[i],tmplen); // TCPFLAG:4B + RTP_HEAD:12B + data
				tmplen += RTPFUHEAD;
				sqe++;
			}
			
			set_apack_inf(a, abuf, tmplen, sqe); // sz = datasz + RTPFUHEAD
			
			i += tmplen;    //加一帧的长度.
			re_sz -= tmplen;//剩下的帧数据
			a_remain = 0;
			
			return 0;
		} 
		else
		{
			a_remain = frame_len - re_sz;
			memcpy(abuf+OFFSIZE -G711_OFFSZ, &ptr[i], re_sz); //保存剩下的帧数.
			set_apack_inf(a, abuf +RTPFUHEAD, re_sz, sqe++);

			re_sz = 0;
			i = 0;
			end_flag = 1;
			return -1;
		}
		i++;
	} 

	return 0;
}

//==================G711U PROC=====================
int g711u_proc(a_fm_t *a, FILE *fd)
{

	return 0;
}

//==================PCM PROC=====================
int pcm_proc(a_fm_t *a, FILE *fd)
{

	return 0;
}
