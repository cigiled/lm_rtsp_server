
#ifndef __BASE_H__
#define __BASE_H__

#include <stdio.h>
#include <errno.h>

#ifdef __cplusplus
	extern "C" {
#endif

typedef unsigned long long  ullong;
typedef unsigned long       ulong;
typedef unsigned int        uint;
typedef char      			u8;
typedef const   char        c8;
typedef unsigned short      ushort;


#define  NAME  "LMSver"

#define True  1
#define False 0

#define PRT_COND(val, str)  \
		{					\
			if((val) == True){		\
				printf(NAME":{%s}\n", str);	\
				return -1;	\
			}	\
		}

#define PRT_CONDX(val, str, X)  \
				{					\
					if((val) == True){		\
						printf(NAME":{%s}\n", str); \
						return X;	\
					}	\
				}

#define PRT_COND_NULL(val, str)  \
		{					\
			if((val) == True){		\
				printf(NAME":{%s}\n", str);	\
				return NULL;	\
			}	\
		}

#define PRT_ERROR(val, str)  \
		{					\
			if((val) == True){		\
				printf(NAME":{%s}\n", str);	\
				perror("ERROR:");	\
				return -1;	\
			}	\
		}


#define PRT_FAILED_CONTINUE(val, str)  \
		{					\
			if((val) == True)  \
			{\
				continue;	\
			}	\
		}

#define PRT_COND_GOTO(val, str, to)  \
		{					\
			if((val) == True){		\
				printf(NAME":{%s}\n", str);	\
				perror("ERROR:");	\
				goto to;	\
			}	\
		}


#ifdef __cplisplus
		};
#endif

#endif

