#ifndef __NET_PRC_H__
#define __NET_PRC_H__

#include "net_base.h"

#ifdef __cplusplus
	extern "C" {
#endif


enum
{
	T_VIDEO_F = 0, //视频文件
	T_AUDIO_F,
	T_AV_F    //包含音频和视频的文件
};

typedef struct
{
	c8		 *file;
	u8 		 blive;
	u8 		 m_ty; //media type: 0:video, 1:audio, 2:audio and video.
	
	u8 		 v_fmt;
	uint 	 v_pl;  //payload;
	u8       v_res;

	u8 		 a_fmt;
	uint 	 a_pl;  //payload;
	u8       a_res;
	Apara_t  aparam;
}media_t;

int init_LMSver(media_t *md, av_t *av);
int start_LMSver(void *argv);
int start_LMav_pth(av_t *av);

int send_vframe_to_LMSver(char* data, int sz, v_fm_t *v);
int send_aframe_to_LMSver(char* data, int sz, a_fm_t *a);

#ifdef __cplusplus
		};
#endif


#endif
