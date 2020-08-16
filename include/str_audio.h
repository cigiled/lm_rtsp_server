#ifndef __STR_AUDIO_H__
#define __STR_AUDIO_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "avnet_base.h"

//int FM_SE = 1024*16;

typedef struct
{
	char mpeg_ver;
	char profile;
	char sampling;
	char chns;
}Aac_t;

int init_audio(Apara_t *para);
int init_audio_rmem();
int dest_audio_rmem();

int get_aac_frame(a_fm_t *a, FILE *fd);
int get_g711x_frame(a_fm_t *v, FILE *fd);

int spare_aac(FILE *fd,  Aac_t *aac);
inline int aac_proc(a_fm_t *a, FILE *fd); 
inline int g711a_proc(a_fm_t *a, FILE *fd);
inline int g711u_proc(a_fm_t *a, FILE *fd);
inline int pcm_proc(a_fm_t *a, FILE *fd);

#ifdef __cplusplus
	  }
#endif

#endif

