#include "av_stream.h"
#include "str_audio.h"
#include "str_video.h"
#include "utility.h"

int set_av_fun(av_t *av)
{
	//vidoe
	if(av->rt.v_fmt == (T_H264 | T_H265))
		video_pro = h264or5_proc;
	else if(av->rt.v_fmt == (T_TS) )
		video_pro = ts_proc;

	//audio
	if(av->rt.a_fmt == (T_G711A) )
		audio_pro = g711a_proc;
	else if(av->rt.a_fmt == (T_G711U))
		audio_pro = g711u_proc;
	else if(av->rt.a_fmt == (T_PCM))
		audio_pro = pcm_proc;
	else if(av->rt.a_fmt == (T_AAC))
		audio_pro = aac_proc;
	
	return 0;
}

int av_file_opt(av_t *av)
{	
	//video ---  
	FILE *v_fd = NULL;
	uint v_tick_gap = 0;
	if(av->v_ft.fl.file)
	{
		v_fd       = av->v_ft.fl.fd;
		v_tick_gap = (1000.0 + 1.0) / av->v_ft.fps; //计算视频 每帧的时间.
	}
	
	//audio --- 
	FILE *a_fd = NULL;
	uint a_tick_gap = 0;
	if(av->a_ft.fl.file)
	{
		a_fd       = av->a_ft.fl.fd;
		a_tick_gap = av->a_ft.sd * 1000 / av->a_ft.ts; 
	}

	v_fm_t *v = &av->v_ft;
	a_fm_t *a = &av->a_ft;;
	int ret = 0;
	uint tick_exp_tmp  = 0;
    uint tick_exp  = 0; 
	char video_end = 0; //file over。
	char audio_end = 0;
	uint audio_tick_now, video_tick_now, last_update;
	audio_tick_now = video_tick_now = last_update = 0;

	//pthread_cond_timedwait(&av->pth.f_cond, &av->pth.f_mutex, NULL);
	pthread_cond_wait(&av->pth.f_cond, &av->pth.f_mutex);
	av->pth.is_f_start = True;
	
	video_tick_now = audio_tick_now = get_time_ms();
    while(1)
    {
    	last_update = get_time_ms();
    	
		if((v_fd > 0) && (!video_end))
		{ // video 
			if(last_update - video_tick_now > v_tick_gap - tick_exp)
			{	
				ret = video_pro(v, v_fd);
				if(ret == 2)
					video_end = 1;
				
				video_tick_now = get_time_ms();
			 }
		}

		
		if((a_fd > 0) && (!audio_end))
		{// audio. 
			if(last_update - audio_tick_now > a_tick_gap - tick_exp)
			{	
  				ret = audio_pro(a, a_fd);
				if(ret == 2)
					video_end = 1;
					
				audio_tick_now = get_time_ms();
			}
		}

		tick_exp_tmp = get_time_ms();
		tick_exp = tick_exp_tmp - last_update;//计算循环一次的时间间隔.

		if(audio_end && video_end)
			break;

		usleep(800);
    }

	printf("pack end-\n");

    return 0;
}

int init_file(av_t *av)
{
	set_av_fun(av);
	
	if(av->v_ft.fl.file)
	{
		init_video_rmem();		
		get_video_inf(av);
	}
	
	if(av->a_ft.fl.file)
	{
		init_audio(&av->rt.apara);
		get_audio_inf(av);
	}

	return 0;
}

int get_audio_inf(av_t *av)
{
	Aac_t aac;
	av->a_ft.fl.fd = fopen(av->a_ft.fl.file, "rb");
	PRT_ERROR(av->a_ft.fl.fd <= 0,  "open audio src file failed");

	if(av->rt.a_fmt == T_AAC)
	{ 
		spare_aac(av->a_ft.fl.fd, &aac);
		av->a_ft.sd = 1024;
		av->a_ft.ts = aac.sampling;
	}

	fseek(av->a_ft.fl.fd, 0, SEEK_SET);
	return 0;
}

int get_video_inf(av_t *av)
{	
	int ret = 0;
	struct stat st;

	lstat(av->v_ft.fl.file, &st);
	av->v_ft.fl.sz = st.st_size;	
	
 	av->v_ft.fl.fd = fopen(av->v_ft.fl.file, "rb");
	PRT_ERROR(!av->v_ft.fl.fd,  "open vidoe src file failed");
	
	if(av->rt.v_fmt == T_H264 || av->rt.v_fmt == T_H265)
	{	
		ret = get_sps_pps(av, av->v_ft.fl.fd);
		PRT_ERROR(ret < 0,  "get video sps failed !");

		get_framerate(av);
		get_profie_level(av);
	}

	//文件定位到头部.
	fseek(av->v_ft.fl.fd, 0, SEEK_SET);
	return 0;
}

int get_sps_pps(av_t *av, FILE *fd)
{
	int len = 512;
	av->rt.sps = malloc(len);
	PRT_ERROR( !av->rt.sps,  "malloc sps space failed !");
	av->rt.pps = malloc(len);
	PRT_ERROR( !av->rt.pps,  "malloc pps space failed !");

	memset(av->rt.sps, 0, len);
	memset(av->rt.pps, 0, len);

	len = get_sps_pps_frame(av, fd);

	return 0;
}

int get_framerate(av_t *av)
{
	int frame_rate = 0;
	
	av->v_ft.fps = frame_rate > 10 ? frame_rate : DEF_FRAMERATE;
	return 0;
}

int get_profie_level(av_t *av)
{
	char *bf = profie_level;
	//rmov_h264or5_eb(bf, 6, profie_level, 512);
	av->rt.pl = (bf[1]<<16) | (bf[2]<<8) | (bf[3]);
	return 0;
}

