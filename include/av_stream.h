#ifndef __AV_STREAM_H__
#define __AV_STREAM_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "avnet_base.h"

enum
{
	SPS_FRAME,
	PPS_FRAME,
	I_FRAME,
	P_FRAME,
	B_FRAME,
};

int init_file(av_t *av);
int av_file_opt(av_t *av);
int set_av_fun(av_t *av);
int get_sps_pps(av_t *av, FILE *fd);
int get_audio_inf(av_t *av);
int get_video_inf(av_t *av);
int get_framerate(av_t *av);
int get_profie_level(av_t *av);


int (*video_pro)(v_fm_t *v, FILE *fd);
int (*audio_pro)(a_fm_t *a, FILE *fd);


#ifdef __cplusplus
	  }
#endif

#endif
