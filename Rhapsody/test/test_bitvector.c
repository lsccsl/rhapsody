#include "mybitvector.h"
#include <assert.h>

void test_bitvector()
{
	HMYBITVECTOR hbv = mybitVectorConstruct(NULL, 10, 0);

	mybitVectorPrint(hbv);

	mybitVectorSetbit(hbv, 1, 5);
	mybitVectorSetbit(hbv, 1, 8);
	mybitVectorSetbit(hbv, 1, 0);
	mybitVectorSetbit(hbv, 1, 10);
	mybitVectorSetbit(hbv, 1, 12);
	mybitVectorSetbit(hbv, 1, 64);

	printf("\r\n");

	mybitVectorPrint(hbv);

	assert(mybitVectorGetbit(hbv, 0) == 1);
	assert(mybitVectorGetbit(hbv, 1) == 0);
	assert(mybitVectorGetbit(hbv, 2) == 0);
	assert(mybitVectorGetbit(hbv, 3) == 0);
	assert(mybitVectorGetbit(hbv, 4) == 0);
	assert(mybitVectorGetbit(hbv, 5) == 1);
	assert(mybitVectorGetbit(hbv, 6) == 0);
	assert(mybitVectorGetbit(hbv, 7) == 0);
	assert(mybitVectorGetbit(hbv, 8) == 1);
	assert(mybitVectorGetbit(hbv, 9) == 0);
	assert(mybitVectorGetbit(hbv, 10) == 1);
	assert(mybitVectorGetbit(hbv, 11) == 0);
	assert(mybitVectorGetbit(hbv, 12) == 1);
	assert(mybitVectorGetbit(hbv, 13) == 0);
	assert(mybitVectorGetbit(hbv, 14) == 0);
	assert(mybitVectorGetbit(hbv, 15) == 0);
	assert(mybitVectorGetbit(hbv, 16) == 0);
	assert(mybitVectorGetbit(hbv, 63) == 0);
	assert(mybitVectorGetbit(hbv, 64) == 1);

	mybitVectorDestruct(hbv);

	MyMemPoolMemReport(1);
}










