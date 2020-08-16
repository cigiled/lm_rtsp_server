#include "file_man.h"
#include "av_stream.h"

int init_file_man(av_t *av)
{
	pthread_t pth;
	pthread_attr_t thread_attr; 
	
	init_file(av);
	
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, PTH_STATIC_SZ);

	pthread_create(&pth, &thread_attr, av_file_prc, av);

	av->pth.file_opt = pth;	

	return 0;
}

void* av_file_prc(void *argv)
{	
	av_t *av = (av_t *)argv;
	av_file_opt(av);

	return NULL;	
}

