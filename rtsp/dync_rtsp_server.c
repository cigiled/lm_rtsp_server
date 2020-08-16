#include "file_man.h"
#include "dync_rtsp_server.h"

int start_app()
{
	av_t av;
	media_t media;
	memset(&av, 0, sizeof(av_t));
	memset(&media, 0, sizeof(media));

	const char* file ="./res/av.h264";
	
	media.blive = False;
	media.file  = file;
	media.m_ty  = T_VIDEO_F;
	media.v_fmt = T_H264;
	media.v_res = RES_1080P;
	
	media.a_pl  = 96; 
	media.a_fmt = T_AAC;
	media.a_res = RES_AAC; 
	media.a_pl  = 120; 
	media.aparam.chn      = 1; 
	media.aparam.samprate = SAM_8000; 
	media.aparam.sampsz   = SAMSZ_320; //一次的采样点数.
	init_LMSver(&media, &av);
	
	start_LMav_pth(&av);
	start_LMSver(&av);

	return 0;
}
