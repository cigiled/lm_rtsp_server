#ifndef __STR_VIDEO_H__
#define __STR_VIDEO_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "avnet_base.h"

char profie_level[10];

int init_video_rmem();
int dest_video_rmem();
inline int get_frame(v_fm_t *v, FILE *fd);
inline int h264or5_proc(v_fm_t *v, FILE *fd);
int get_sps_pps_frame(av_t *v, FILE *fd);
int get_nalu_type(u8 fmt, int sqe, char *data, int *type);
uint rmov_h264or5_eb(u8* to, int u_sz, u8* from, uint fr_sz);

inline int ts_proc(v_fm_t *v, FILE *fd);


#ifdef __cplusplus
	  }
#endif

#endif

