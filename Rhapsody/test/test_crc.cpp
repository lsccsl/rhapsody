#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned short calc_crc16(unsigned char* ptr,unsigned short count)
{
	unsigned short crc=0xffff;
	unsigned char i;

	while (count-- > 0) 
	{
		crc = crc ^ *ptr++;
		for (i = 0; i < 8; ++i)
		{
			if (crc & 0x0001)
			{
				crc = crc >> 1 ^ 0x8408;
			}
			else
			{
				crc = crc >> 1;
			}
		}
	}
	return ( crc ^ 0xffff);
}


unsigned long read_ini_crc()
{
#define INI_PATH "D:/工作/star-net/cvs/istywt/srccode/files/IST/sys.ini"

	FILE * pfile = NULL;
	unsigned char *p = NULL;
	unsigned long len = 0;
	unsigned long chksum = 0;
	unsigned char actemp[3] = {0x1a,0x1a,0x00};
	char * pend = NULL;

	pfile = fopen(INI_PATH, "rt");

	if(NULL == pfile)
		return -1;

	/* 取得文件大小 */
	fseek(pfile, 0, SEEK_END);
	len = ftell(pfile);
	fseek(pfile, 0, SEEK_SET);

	p = (unsigned char *)malloc(len + (4 - len%4) + 4);
	memset(p, 0, len + (4 - len%4) + 4);

	/* 读出文件至内存 */
	fread(p, 1, len, pfile);

	/*  */
	pend = strstr((char *)p, (char *)actemp);
	if(pend)
		*pend = 0;

	/* 计算校验和 */
	chksum = calc_crc16(p, strlen((char *)p));

	fprintf(stderr, "[%s:%d]%lX %d \r\n", __FILE__, __LINE__,
		chksum, len);

	free(p);

	return chksum;

#undef INI_PATH 
}


