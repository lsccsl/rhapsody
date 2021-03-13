#include <stdlib.h>
void getcallid(char * callid, size_t callid_len)
{
	/*int i = 0;
	for(i=0; i<callid_len - 1; i++)
	{		
		char ucRandomNum = (char)(rand()%36);
		if(ucRandomNum<=9)
			callid[i] = '0'+ucRandomNum;
		else
			callid[i] = 'a'+ucRandomNum-10;
	}

	callid[callid_len - 1] = 0;*/

	int i = 0;
	for(i=0; i<callid_len - 1; i++)
	{	
		/*if(i <= 23)
		{
			callid[i] = 'a';
			continue;
		}
		else*/
		{
			char ucRandomNum = (char)(rand()%26);
			//if(ucRandomNum<=9)
			//	callid[i] = '0'+ucRandomNum;
			//else
				callid[i] = 'a'+ucRandomNum;
		}
	}

	callid[callid_len - 1] = 0;
}
