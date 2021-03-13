#include <stdio.h>
#ifdef __cplusplus
extern "C"
{
#endif
#include "myini.h"
#ifdef __cplusplus
}
#endif
void test_ini()
{
	HMYINI hini = MyIniConstructFromFile("abc.ini");

	HMYLIST hseclist = MyIniGetSecList(hini);

	{
		HMYLIST_ITER it = MyListGetHead(hseclist);
		HMYLIST_ITER end = MyListGetTail(hseclist);

		for(; it != end; it = MyListGetNext(hseclist, it))
		{
			HMYINI_SEC hsec = (HMYINI_SEC)MyListGetIterData(it);
			HMYLIST hkeyslist = MyIniGetKeyList(hsec);
			HMYLIST_ITER itkey = MyListGetHead(hkeyslist);
			HMYLIST_ITER endkey = MyListGetTail(hkeyslist);

			char acSecName[32] = {0};

			MyIniGetSecName(hsec, acSecName, sizeof(acSecName));
			printf("sec:%s\n", acSecName);

			for(; itkey != endkey; itkey = MyListGetNext(hkeyslist, itkey))
			{
				char acKeyName[32] = {0};
				char acValName[32] = {0};
				HMYINI_KEY hkey = (HMYINI_KEY)MyListGetIterData(itkey);

				MyIniGetKeyValString(hkey, acValName, sizeof(acValName));
				MyIniGetKeyName(hkey, acKeyName, sizeof(acKeyName));
				printf("%s = %s\n", acKeyName, acValName);
			}
		}
	}

	MyMemPoolMemReport(0);

	MyIniDestruct(hini);

	MyMemPoolMemReport(1);
}



