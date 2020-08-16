#ifndef __RTP_RTCP_H__
#define __RTP_RTCP_H__

#include "avnet_base.h"

#ifdef __cplusplus
	extern "C" {
#endif

#define  BASESEQ  (100)

int pack_rtp(u8 vtrue, void* argv);
int pack_videortp(v_fm_t *v);
int pack_audiortp(a_fm_t *a);

int set_fu(char *indata);
int is_fu_pack(int *fuover, int media);

int fill_4b_fortcp(char *indata, int sz);
int set_rtp_head(char *indata, int off, uint payload, uint *outseq);

int packfu_no_sbit(char *indata, int media);
int packfu_with_sbit(char *indata, int media, u8* nal_ty);

int exdata_fill(char *indata, int *len, int media);
int set_apack_inf(a_fm_t *a, char *data, uint len, int aseq);
int set_vpack_inf(v_fm_t *v, char *data, uint len,  int type, int vseq);

int get_fudata(char *data, int *len, int *fufg, int media);
int get_stdata(char *data, int *len, int media);
int free_rtpdate(int media);

#ifdef __cplusplus
		};
#endif

#endif

