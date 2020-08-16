#include "rtp_rtcp.h"

// 由于MTU 原因, 将数据分包传输,这里使用一个buff存储 分包的数据.
char vbuff[1024 * 140] = {0};
packfu_t vfu_t[100] = {0}; //记录视频包的位置及大小.
packst_t vst_t;

char abuff[1024 * 40] = {0};
packfu_t afu_t[100] = {0}; //记录音频包的位置及大小.
packst_t ast_t;


//RTP header: toal:12Byte v:2 + p:1 + x:1 + CC:4 + M:1 + PT:7 + SN:16
//大反小正
//arm 小端, 高位高地址, 地址:从左往右变高, 具体数据:左高右低.

int pack_rtp(u8 vtrue, void* argv)
{
	PRT_COND(!argv, "Input argv is NULL !");

	a_fm_t *a = NULL;
	v_fm_t *v = NULL;;
	
	int rtppayload , sqe = 0;
	int n, toal, offsz = 0;
	u8  *tmpdt, *nal_type = NULL;
	int mediaty;
	char* buff = NULL;
	packst_t *st_t = NULL;
	packfu_t *fu_t = NULL;
	if(vtrue)//视频参数.
	{
		v = (v_fm_t*)argv;
		n = v->seq;
		
		mediaty    = v->fmt;
		toal 	   = v->fm[n].sz -TCPST;
		tmpdt	   = v->fm[n].buff;
		rtppayload = v->pl;
		sqe        = v->fm[n -1].sqe +OFFSIZE;
		nal_type   = &tmpdt[sqe];
		buff       = vbuff;
		st_t	   = &vst_t;
		fu_t	   = vfu_t;
	}
	else //音频参数.
	{
		a = (a_fm_t*)argv;
		n = a->seq;
		
		mediaty    = a->fmt;
		toal 	   = a->fm[n].sz -TCPST;
		tmpdt	   = a->fm[n].buff;
		rtppayload = a->pl;
		sqe        = a->fm[n -1].sqe +OFFSIZE;
		nal_type   = &tmpdt[sqe];
		buff       = abuff;
		st_t	   = &ast_t;
		fu_t	   = afu_t;
	}

	uint CurOffset, s_pos, tmp_pos;
	uint offsize, currseq, len,num = 1;
	uint startoff, i = 0;

	CurOffset = s_pos = tmp_pos = 0;
	
	if(toal > MAX_SEND_SZ)
	{
		startoff = sqe -OFFSIZE;
		fill_4b_fortcp(tmpdt, toal);// ??????????
		offsize = set_rtp_head(tmpdt, startoff, rtppayload, &currseq);
		offsz = startoff + 12;
		exdata_fill(tmpdt +offsz, &toal, mediaty);
		packfu_with_sbit(&tmpdt[offsize], mediaty, nal_type);

		while(toal > 0)
		{
			if(!CurOffset)
			{
				CurOffset += MAX_SEND_SZ;
				len = MAX_SEND_SZ;
			}
			else
			{
				packfu_no_sbit(&tmpdt[offsize], mediaty);
				if(toal < MAX_SEND_SZ)
				{
					tmpdt[offsize -1] |= 0x40; // set the E bit in the FU header
					len = toal;
				}
				else
				{
					len = MAX_SEND_SZ;
				}
			}
			
			toal -= len;
			currseq++;

			//copy data  to send buff.
			memcpy(&buff[tmp_pos], &tmpdt[s_pos], len);
			buff[len+1] = '\0';
			
			fu_t[i].data = &buff[tmp_pos];
			fu_t[i].sz   = MAX_SEND_SZ;
			fu_t[i].fu_n = num++;

			tmp_pos += (len+1);
		}
	}
	else
	{
		//add  rtp header.
		startoff = sqe -RTPHEAD -TCPST;
		fill_4b_fortcp(tmpdt, toal);
		set_rtp_head(tmpdt, startoff, rtppayload, NULL);
		offsz = startoff + 12;
		exdata_fill(tmpdt +offsz, &toal, mediaty);

		len = toal;
		memcpy(buff, tmpdt, len);
		buff[len + 1] = '\0';
		
		st_t->data = buff;
		st_t->sz = len;
	}

	//signal send pthread.
	//free buff.
	if(v)
	{
		v->fm[n].buff = NULL;
		v->fm[n -1].sqe = 0;
		v->fm[n].sz = 0;
	}
	else if(a)
	{
		a->fm[n].buff = NULL;
		a->fm[n -1].sqe = 0;
		a->fm[n].sz = 0;
	}
		
	return 0;
}

//时间戳 参考: RTPSink::convertToRTPTimestamp
//tcp方式传输，要在包前面加4个字节， 这里统一从buff偏移4个自己后打包
//在send时根据具体的传输协议来选择 从 哪个位置开始发送.
int set_rtp_head(char *indata, int off, uint payload, uint *outseq)
{
	PRT_COND(!indata, "Input indata is NULL, exit!");
	PRT_COND((off >OFFSIZE) || (off <TCPST), "Input offsize error, exit!");

	int len;
	static uint seqno = 1; 

	struct timeval now;
	gettimeofday(&now, NULL);
	ullong mnow = now.tv_sec *1000 + now.tv_usec /1000;
	ullong timestamp = htonl(mnow *(90000 /1000)); // [TWT] 这里有问题,音频的时间戳不能按照90000来计算
	static uint ssrc = BASESEQ;

	//fille first 4 Byte.{version,payload,pad ...}
	uint hdr = 0x80000000;
	hdr  |= payload <<16; 
	hdr  |= seqno++;

	//fill 12B = HDR:4B + timestamp:4B + SSRC:4B
	len = sprintf(&indata[off], "%d%lld%d", hdr, timestamp, htonl(ssrc++));

	if(outseq)
		*outseq = ssrc;//返回序列号 给上层FU包用.

	return len;
}

int packfu_with_sbit(char *indata, int media, u8* nal_ty)
{
	if(media == T_H264)
	{
		indata[0] = (*nal_ty & 0xE0) | 28; // FU indicator
		indata[1] = 0x80 | (*nal_ty & 0x1F); // FU header (with S bit)
	}
	else if(media == T_H265)
	{
		u8 nal_unit_type = (nal_ty[0]&0x7E)>>1;
		indata[0] = (nal_ty[0] & 0x81) | (49<<1); // Payload header (1st byte)
		indata[1] = nal_ty[1]; // Payload header (2nd byte)
		indata[2] = 0x80 | nal_unit_type; // FU header (with S bit)
	}
	else if(media == T_TS)
	{

	}
	else if(media == T_AAC)
	{

	}
	else if(media == T_PCM)
	{

	}
	
	return 0;
}

int packfu_no_sbit(char *indata, int media)
{
	if(media == T_H264)
	{
		indata[-2] = indata[0]; // FU indicator
		indata[-1] = indata[1]&~0x80; // FU header (no S bit)
	}
	else if(media == T_H265)
	{
		indata[-3] = indata[0];  // Payload header (1st byte)
		indata[-2] = indata[1]; // Payload header (2nd byte)
		indata[-1] = indata[2]&~0x80;  // FU header (no S bit)
	}
	else if(media == T_TS)
	{

	}
	else if(media == T_AAC)
	{

	}
	else if(media == T_PCM)
	{

	}

	return 0;
}

int fill_4b_fortcp(char *indata, int sz)
{
    indata[0] = '$';
    indata[1] = 1; //这个接口默认填充的都是rtp数据. rtcp的另外写.
    indata[2] = (u8)((sz&0xFF00)>>8);
    indata[3] = (u8)(sz&0xFF);
	return 0;
}

int is_fu_pack(int *fuover, int media)
{
	packfu_t *fu = NULL;

	if(media)
		fu = vfu_t;
	else 
		fu = afu_t;
	
	*fuover = (fu[0].sz > 0) ? 1 : 0;
	 return *fuover;
}

int exdata_fill(char *indata, int *len, int media)
{
	if(media == T_AAC)
	{
		indata[0] = 0x00;
		indata[1] = 0x10;
		indata[2] = (*len & 0x1fe0) >> 5;
		indata[3] = (*len & 0x1f) << 3;
		*len += 4; 
	}
	else if(media == T_PCM)
	{

	}
	
	return 0;
}

int get_fudata(char *data, int *len, int *fufg, int media)
{
	int n;
	static int vn = 0;
	static int an = 0;
	
	packfu_t *fu = NULL;
	
	if(media) //video data
	{
		fu = vfu_t;
		n  = vn;
	}
	else //audio data
	{
		fu = afu_t;
		n  = an;
	}
	
	if((fu[n].sz > 0) && (fu[n].fu_n > 0))
	{	
		data  = fu[n].data;
		*len  = fu[n].sz;
		*fufg = 1;
		n++;
	}
	else
	{
		n     = 0; //poll get data.
		*fufg = 0;
		return -1;
	}

	if(media) //video data
		vn = n;
	else
		an = n;

	return 0;
}

int get_stdata(char *data, int *len, int media)
{
	packst_t *st = NULL;
	if(media)  //video data
		st = &vst_t;
	else //audio data
		st = &ast_t;

	if(st->sz <= 0)
	{
		return  -1;
	}

	data  = st->data;
	*len  = st->sz;
	
	return 0;
}

int free_rtpdate(int media)
{
	packst_t *st = NULL;
	packfu_t *fu = NULL;

	if(media)
	{
		fu = vfu_t;
		st = &vst_t;
	}
	else
	{
		fu = afu_t;
		st = &ast_t;
	}
	
	if((fu[0].sz > 0))
	{
		memset(fu, 0, sizeof(vfu_t)/sizeof(packfu_t));
	}
	else if(st->sz > 0)
	{
		st->data = NULL;
		st->sz   = 0;
	}

	return 0;
}

int pack_videortp(v_fm_t *v)
{
	PRT_COND(!v, "Input v is NULL.");
	pack_rtp(True, v);
	return 0;
}

int pack_audiortp(a_fm_t *a)
{
	PRT_COND(!a, "Input a is NULL.");
	pack_rtp(False, a);
	return 0;
}
int set_vpack_inf(v_fm_t *v, char *data, uint len, int type, int vseq)
{
	PRT_COND(!v || !data, "input *v or *data is NULL .");
	
	v->fm[v->seq].buff = data; //last frame date
	v->fm[v->seq].sz   += len;  //last frame sz
	v->fm[v->seq].type = type;
	v->fm[v->seq].sqe  = vseq; 

	v->seq++;
	if(v->seq > MAX_FRAME_NUM -1)
		v->seq = 0;
	return 0;
}

int set_apack_inf(a_fm_t *a, char *data, uint len, int aseq)
{
	PRT_COND(!a || !data, "input *a or *data is NULL .");
	
	a->fm[a->seq].buff = data; //last frame date
	a->fm[a->seq].sz   += len;  //last frame sz
	a->fm[a->seq].type = -1;
	a->fm[a->seq].sqe  = aseq; 

	a->seq++;
	if(a->seq > MAX_FRAME_NUM -1)
		a->seq = 0;
	return 0;
}

