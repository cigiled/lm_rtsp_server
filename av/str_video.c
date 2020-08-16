#include "str_video.h"
#include "rtp_rtcp.h"
#include "utility.h"

extern char profie_level[10];
static char *ptr = NULL;

int init_video_rmem()
{
	ptr = malloc(MAX_V_BUFFF);
	PRT_COND(!ptr, "maloc video mem failed !");
	return 0;
}

int dest_video_rmem()
{
	if(ptr)
		free(ptr);
	
	ptr = NULL;
	
	return 0;
}

inline int get_frame(v_fm_t *v, FILE *fd)
{
	PRT_COND(!v || !fd, "Input v or fd is NULL !");
	PRT_COND(!ptr, "vidoe mem is NULL !");

	int tmpsz = 1024*80;
	static int l_sqe = 0;
	static int sqe = 0;
	static int sz  = 0;
    static int p   = 0;
    static int len = 0;
	static int remain = 0;
	static int file_sz = 0;
	static char end_flag = 1;
	static char *start = NULL;
	char *vbuf = NULL;
	while(1)
	{
		if(end_flag)
		{
			p = 0;
			if((len = fread(ptr, 1, PER_RD_SZ, fd)) <= 0)
			{
				return -2; //file over.
			}
		}

        if( (ptr[p+0] == 0 && ptr[p+1] == 0 && ptr[p+2] == 0 && ptr[p+3] == 1)
		 || (ptr[p+0] == 0 && ptr[p+1] == 0 && ptr[p+2] == 1))
		{				
			(ptr[p+2] == 1)?(sqe = 3):(sqe = 4);
			
			if(!start)
			{ //首次检测到帧.
				if(v->seq > MAX_FRAME_NUM -1)
					v->seq = 0;
				
				v->fm[v->seq].pos = file_sz + p;	
								
				start = &ptr[p];
				p    += sqe;
				l_sqe = sqe;
				end_flag = 0;
				continue;
			}
			else
			{
				int type = 0;
				int pos2 = file_sz + p; //加上上次read的长度，得到当前在文件中的位置.
				
				if(remain > 0)
				{
					memcpy(&vbuf[RTPFUHEAD] + remain, &ptr[0], p); //加上 上次不完整部分数据.
					sz = p + remain;
					remain = 0; //上次read 剩下的不完整帧.
				}
				else
				{
					sz = &ptr[pos2] - start;
					vbuf = get_vbuff(sz + OFFSIZE);

					memcpy(&vbuf[OFFSIZE], start, sz); // skip rtp header.
				}

				get_nalu_type(v->fmt, l_sqe, &vbuf[OFFSIZE], &type);
				set_vpack_inf(v, vbuf, sz, type, l_sqe);	
				
				start  = &ptr[p];
				p     += sqe;
				l_sqe  = sqe; 
				end_flag = 0;
				
				return 1;
			} 
		}
		else if( len == p)
		{//存下这次read帧中, 剩下的不完整帧数据.
			remain = &ptr[p] - start;	
			if(remain > 0)
			{
				int add = start - ptr -file_sz;
				vbuf = get_vbuff(tmpsz);
				memcpy(&vbuf[OFFSIZE], &ptr[add] ,remain);
				if(file_sz + p == v->fl.sz)
				{ //写最后一帧数据.
					int type = 0;
					get_nalu_type(v->fmt, l_sqe, vbuf, &type);
					set_vpack_inf(v, vbuf, remain+OFFSIZE, type, l_sqe);						
					return 1;
				}
			}

			file_sz += len;
			end_flag = 1;
			continue;
		}
		
		p++;
	}

	return 0;
}

int h264or5_proc(v_fm_t *v, FILE *fd)
{
   // get video frame.
	int ret = get_frame(v, fd);
	PRT_COND(ret < 0, "get frame error !");
	
	//add rtp header.	
	pack_videortp(v);
	return 0;
}

int get_nalu_type(u8 fmt, int sqe, char *data, int *type)
{
	u8  naluType;
	char *pNalu = NULL;
	
	pNalu = data+sqe;
			
	if(fmt == T_H264)
	{
		naluType = pNalu[0]&0x1F;
		switch (naluType)
		{
			case 6: 
					*type = T_SEI;
					break;
			
			case 7:
					*type = T_SPS;
					break;
			
			case 8: 
					*type = T_PPS;
					break;
			
			default:
					*type = T_P;
					break;
		}		
	}
	else if (fmt == T_H265)
	{
		naluType = (pNalu[0] & 0x7E)>>1;
		switch (naluType)
		{
		    case 0: 
					*type = T_B;
					break;
			
			case 1: 
					*type = T_P;
					break;
			
			case 19: 
					*type = T_IDR;
					break;
			
			case 32: 
					*type = T_VPS;
					break;
			
			case 33:
					*type = T_SPS;
					break;
			
			case 34: 
					*type = T_PPS;
					break;
						
			default:
					*type = T_P;
					break;
		}	
	}
	return 0;
}

int get_sps_pps_frame(av_t *vfmt, FILE *fd)
{	
	int len, sqe, type;
	u8 s1 ,s2 = 0;
	char *tmpbf = NULL;
	v_fm_t *v = &vfmt->v_ft;

	len = sqe = type= 0;
	
	while(1)
	{
		int ret = get_frame(v, fd);
		PRT_COND(ret < 0, "Get frame error !");

		tmpbf = v->fm[v->seq -1].buff;
		type  = v->fm[v->seq - 1].type;
		if(type == T_SPS  || type == T_PPS )
		{
			len = v->fm[v->seq - 1].sz ; //- v->fm[v->seq - 1].sqe;
			sqe = v->fm[v->seq - 1].sqe + 12;
			if(type == T_SPS)
			{
				//printf("sps[%d]:[%d]\n", len, sqe);
				//print_tnt(&tmpbf[12], len, "SPS:");
				s1 = 1;
				//vfmt->rt.len += EncodeBase64(vfmt->rt.sps, &tmpbf[sqe], len);
				//EncodeBase64(vfmt->rt.sps, &tmpbf[sqe], len);
				strcpy(vfmt->rt.sps, "Z01AHpUVGIBYCTI="); 
				memcpy(profie_level, &tmpbf[sqe], 10);
				tmpbf = NULL;
			}
			else if(type == T_PPS)
			{
				//printf("pps[%d]:[%d]\n", len, sqe);
				//print_tnt(&tmpbf[12], len, "PPS:");
				s2 = 1;
				//vfmt->rt.len += EncodeBase64(vfmt->rt.pps, &tmpbf[sqe], len);
				//EncodeBase64(vfmt->rt.pps, &tmpbf[sqe], len);
				strcpy(vfmt->rt.pps, "aOqOIA==");
				tmpbf = NULL;
			}
		}	

		if(s2 && s1)
				break;
		if(v->seq > MAX_FRAME_NUM -2)
		{
			PRT_COND(True, "Don't find the sps frame !");
			return -1;
		}
	}

	return 0;
}

//emulation_bytes filter.
uint rmov_h264or5_eb(u8* to, int u_sz, u8* from, uint fr_sz) 
{
	uint i = 0;
	uint to_sz = 0;
	while (i < fr_sz) 
	{
		if(from[i] == 0 && from[i+1] == 0 && from[i+2] == 3)
		{
			  to[i] = to[i+1] = 0;
			  i += 3;
			  to_sz = i-1;
		} 
		else 
		{
			  to[i] = from[i];
			  i += 1;
			  to_sz = i;
		}

		if(i >= u_sz) //有的场景下 只需要前面几个，此时没必要全部都解析，通过这里来制定处理的字节数.
			break;
	}

	return to_sz;
}

int ts_proc(v_fm_t *v, FILE *fd)
{
	
	return 0;
}


