#ifndef __FIFO_H__
#define __FIFO_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "avnet_base.h"
#include "net_base.h"

#define MAX_NUMS    (27)
#define VUNIT   		(1024)
#define AUNIT   		(512)

#define LEVEL_MUM   (6)

typedef struct
{
	int  key;
	uint sz;
	char *buff;
}fifo_t;

enum 
{
  RES_4K,
  RES_1080P,
  RES_720P,
  RES_VGA,
  RES_D1,
  RES_AAC,
  RES_OPUS,
  RES_G711A,
  RES_G711U,
  RES_G726,
  RES_G729,
  RES_PCM
};

typedef struct
{
  u8    v_res;
  uint  levl[LEVEL_MUM];
  uint  mem_sz[LEVEL_MUM];  //k单位.
}buff_t;

//记录每个 levels 对应 内存的使用情况.
typedef struct
{
	uint start[LEVEL_MUM]; // the level start adrr.
	uint offsize[LEVEL_MUM]; //in the curr level ， have used mem reference number.
}le_t;

int init_fifo(uint t_res);
int malloc_fifo(fifo_t *ft, int seq, int levels, int mem_sz, le_t *LeRec);
char* get_buff(uint sz, fifo_t* tmpfifo, uint *le, uint *mz, le_t *LeRec);
char* get_abuff(uint sz);
char* get_vbuff(uint sz);

#ifdef __cplusplus
	  }
#endif

#endif

