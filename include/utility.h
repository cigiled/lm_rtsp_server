#ifndef __UTILITY_H__
#define __UTILITY_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "avnet_base.h"

uint get_time_ms();

char* EncodeBase64(char *outbuff, char const* origSigned, unsigned origLength);

uint get_random32();

void print_tnt(char *buf, int len, const char *comment);


#ifdef __cplusplus
	  }
#endif

#endif

