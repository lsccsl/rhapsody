#include "myrand.h"

void test_rand()
{
	HMYRAND hrand = myrandConstruct(NULL, NULL, 0, 0);

	unsigned char byte_rand = myrandGetByte(hrand);

	byte_rand = myrandGetByte(hrand);
	byte_rand = myrandGetByte(hrand);
	byte_rand = myrandGetByte(hrand);
	byte_rand = myrandGetByte(hrand);
	byte_rand = myrandGetByte(hrand);
	byte_rand = myrandGetByte(hrand);

	myrandDestruct(hrand);

	MyMemPoolMemReport(1);
}


