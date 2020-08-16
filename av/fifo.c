#include "fifo.h"

//1080p: 
//levels:         1, 3, 4, 7, 7, 5.
//per_memsz:      120, 96, 76, 60, 50, 20.
//all mem_sz: 1582k.
//frmae_numq: 27 fps.
buff_t Bt[] = {
				{RES_4K,    {1, 3, 4, 7, 7, 5 }, {120*VUNIT, 96*VUNIT, 76*VUNIT, 60*VUNIT, 50*VUNIT, 20*VUNIT}},
				{RES_1080P, {1, 3, 4, 7, 7, 5 }, {120*VUNIT, 96*VUNIT, 76*VUNIT, 60*VUNIT, 50*VUNIT, 20*VUNIT}},
				{RES_720P,  {1, 3, 4, 7, 7, 5 }, {120*VUNIT, 96*VUNIT, 76*VUNIT, 60*VUNIT, 50*VUNIT, 20*VUNIT}},
				{RES_VGA,   {1, 3, 4, 7, 7, 5 }, {120*VUNIT, 96*VUNIT, 76*VUNIT, 60*VUNIT, 50*VUNIT, 20*VUNIT}},
				{RES_D1,    {1, 3, 4, 7, 7, 5 }, {120*VUNIT, 96*VUNIT, 76*VUNIT, 60*VUNIT, 50*VUNIT, 20*VUNIT}},
				{RES_AAC,   {1, 3, 8, 8, 7, 3 }, {4*AUNIT, 4*AUNIT, 4*AUNIT, 4*AUNIT, 2*AUNIT, 1*AUNIT}},
				{RES_OPUS,  {1, 3, 4, 7, 7, 5 }, {120*AUNIT, 96*AUNIT, 76*AUNIT, 60*AUNIT, 50*AUNIT, 20*AUNIT}},
				{RES_G711A, {1, 3, 4, 7, 7, 5 }, {AUNIT, AUNIT, AUNIT, AUNIT, AUNIT, AUNIT}},
				{RES_G711U, {1, 3, 4, 7, 7, 5 }, {AUNIT, AUNIT, AUNIT, AUNIT, AUNIT, AUNIT}},
				{RES_G726,  {1, 3, 4, 7, 7, 5 }, {120*AUNIT, 96*AUNIT, 76*AUNIT, 60*AUNIT, 50*AUNIT, 20*AUNIT}},
				{RES_G729,  {1, 3, 4, 7, 7, 5 }, {120*AUNIT, 96*AUNIT, 76*AUNIT, 60*AUNIT, 50*AUNIT, 20*AUNIT}},
				{RES_PCM,   {1, 3, 4, 7, 7, 5 }, {120*AUNIT, 96*AUNIT, 76*AUNIT, 60*AUNIT, 50*AUNIT, 20*AUNIT}},
			};

fifo_t vfifo[MAX_NUMS];
uint *vle  = NULL;
uint *vmz  = NULL;
le_t vLeRec; 

fifo_t afifo[MAX_NUMS];
uint *ale  = NULL;
uint *amz  = NULL;
le_t aLeRec;

int init_fifo(uint t_res)
{
	PRT_COND(t_res < RES_4K || t_res > RES_PCM, "input video or audio resolution is not rang.");

	fifo_t *fifo = NULL;
	uint *le  = NULL;
	uint *mz  = NULL;
	le_t *LeRec = NULL; 
	if((t_res >= RES_4K) && (t_res <= RES_D1))
	{
		fifo  = vfifo;
		LeRec = &vLeRec;
	}
	else
	{
		fifo  = afifo;
		LeRec = &aLeRec;
	}
	
	int i = 0;
	uint nums = LEVEL_MUM;
	
	le = Bt[t_res].levl;
	mz = Bt[t_res].mem_sz;
	
	for(; i < nums; i++)
	{
		malloc_fifo(fifo, i, le[i], mz[i], LeRec);
	}

	if((t_res >= RES_4K) && (t_res <= RES_D1))
	{
		vle = le;
		vmz = mz;
	}
	else
	{
		ale = le;
		amz = mz;
	}

	return 0;
}

int malloc_fifo(fifo_t *ft, int seq, int levels, int mem_sz, le_t *LeRec)
{
	int i = 0;
	static int k = 0;

	LeRec->start[seq] = k; //记录每个等级的起始位置，方便查找.
	for(; i < levels; i++)
	{
		ft[k].buff = malloc(mem_sz * VUNIT);
		ft[k].sz   = mem_sz;
		ft[k].key  = k;
		k++;
	}
	
	return 0;
}

char* get_buff(uint sz, fifo_t* tmpfifo, uint *le, uint *mz, le_t *LeRec)
{
	PRT_COND_NULL(!le || !mz, "le or mz is NULL .");

	int seq = 0;
	int i = 0;
	uint *offs;
	char *buff = NULL;
	//这样写 效率 高一点吗？
	if(sz > mz[2])
	{
		if(sz <= mz[1])
		{
			seq  = le[1];  //le[1]
			i    = LeRec->start[1];
			offs = &LeRec->offsize[1];
		}
		else
		{
			seq = le[0];//le[0]
			i = LeRec->start[0];
			offs = &LeRec->offsize[0];
		}
	}
	else
	{
		if(sz > mz[3])
		{
			seq = le[2];//le[2]
			i = LeRec->start[2];
			offs = &LeRec->offsize[2];
		}
		else if(sz > mz[4])
		{
			seq = le[3];//le[3]
			i = LeRec->start[3];
			offs = &LeRec->offsize[3];
		}
		else if(sz > mz[5])
		{
			seq = le[4];//le[4]
			i = LeRec->start[4];
			offs = &LeRec->offsize[4];
		}
		else
		{
			seq = le[5];//le[5]
			i = LeRec->start[5];
			offs = &LeRec->offsize[5];
		}
	}

	//add the offsize       when get the levels buff every time.
	if(*offs >= i+seq) //Can't more than the numbers of the levels buff.
		*offs = 0;
	
	buff = tmpfifo[i-1 + *offs].buff;
	//printf("[%d]:[%d]:[%d]\n", sz, i, *offs);
	(*offs)++;
	
	return buff;
}

char* get_vbuff(uint sz)
{
	return get_buff(sz, vfifo, vle, vmz, &vLeRec);
}

char* get_abuff(uint sz)
{
	return get_buff(sz, afifo, ale, amz, &aLeRec);
}
