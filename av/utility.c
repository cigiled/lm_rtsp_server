#include "utility.h"


uint get_time_ms()
{
	struct timeval _stime;
	
	gettimeofday(&_stime, NULL);
	return (_stime.tv_sec * 1000 + _stime.tv_usec / 1000);
}

void print_tnt(char *buf, int len, const char *comment)
{
	int i = 0;
	printf("\r\n%s start:%p len:%d \r\n", comment, buf, len);
	for(i = 0; i < len; i++)
	{
    	if(i%16 == 0)
		printf("\r\n");
    	 printf("%02x ", buf[i] & 0xff); //&0xff 是为了防止64位平台中打印0xffffffa0 这样的，实际打印0xa0即可
	}
	printf("\n");
}

#if 0
const char base64Char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

uint base64_encode(char *dest, c8 *src, uint len) 
{
	u8 const* orig = (u8 const*)src; // in case any input bytes have the MSB set
	if (orig == NULL) return -1;

	uint const numSrc24BitValues = len/3;
	char havePadding  = len > numSrc24BitValues*3;
	char havePadding2 = len == numSrc24BitValues*3 + 2;
	uint const numResultBytes = 4*(numSrc24BitValues + havePadding);
	char* result =  dest;

	// Map each full group of 3 input bytes into 4 output base-64 characters:
	uint i;
	for (i = 0; i < numSrc24BitValues; ++i) 
	{
		result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];
		result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
		result[4*i+2] = base64Char[((orig[3*i+1]<<2) | (orig[3*i+2]>>6))&0x3F];
		result[4*i+3] = base64Char[orig[3*i+2]&0x3F];
	}

	// Now, take padding into account.	(Note: i == numOrig24BitValues)
	if (havePadding) 
	{
		result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];
		if (havePadding2) 
		{
			result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
			result[4*i+2] = base64Char[(orig[3*i+1]<<2)&0x3F];
		} 
		else 
		{
			result[4*i+1] = base64Char[((orig[3*i]&0x3)<<4)&0x3F];
			result[4*i+2] = '=';
		}
		result[4*i+3] = '=';
	}

  result[numResultBytes] = '\0';
  return numResultBytes;
}
#else
const char base64Char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char* EncodeBase64(char *outbuff, char const* origSigned, unsigned origLength) 
{
	unsigned char const* orig = (unsigned char const*)origSigned; // in case any input bytes have the MSB set
	if (orig == NULL) return NULL;

	unsigned const numOrig24BitValues = origLength/3;
	char havePadding = origLength > numOrig24BitValues*3;
	char havePadding2 = origLength == numOrig24BitValues*3 + 2;
	unsigned const numResultBytes = 4*(numOrig24BitValues + havePadding);
	char* result =  outbuff;

	// Map each full group of 3 input bytes into 4 output base-64 characters:
	unsigned i;
	for (i = 0; i < numOrig24BitValues; ++i) 
	{
		result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];
		result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
		result[4*i+2] = base64Char[((orig[3*i+1]<<2) | (orig[3*i+2]>>6))&0x3F];
		result[4*i+3] = base64Char[orig[3*i+2]&0x3F];
	}

	// Now, take padding into account.	(Note: i == numOrig24BitValues)
	if (havePadding) 
	{
		result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];
		if (havePadding2) 
		{
			result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
			result[4*i+2] = base64Char[(orig[3*i+1]<<2)&0x3F];
		} 
		else 
		{
			result[4*i+1] = base64Char[((orig[3*i]&0x3)<<4)&0x3F];
			result[4*i+2] = '=';
		}
		result[4*i+3] = '=';
	}

	result[numResultBytes] = '\0';
	//result[numResultBytes -3] = '\0';
  return result;
}

#endif

uint get_random32()
{
	long random_1 = random();
	uint random16_1 = (uint)(random_1&0x00FFFF00);

	long random_2 = random();
	uint random16_2 = (uint)(random_2&0x00FFFF00);

	return (random16_1<<8) | (random16_2>>8);
}

