#ifndef __FILE_MAN_H__
#define __FILE_MAN_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "avnet_base.h"

int init_file_man(av_t *av);
void* av_file_prc(void *argv);

#ifdef __cplusplus
	  }
#endif

#endif