#include "myOsFile.h"

void test_osfile()
{
	HMYOSFILE hf = myOsFileOpenReadWrite("test_osfile", NULL);
	unsigned char actemp[3] = {0};
	int64 fsz = 0;

	myOsFileSeek(hf, 1024);
	myOsFileWrite(hf, "abc", 3, NULL);
	myOsFileSize(hf, &fsz);
	assert(fsz == 1027);
	myOsFileSyn(hf);

	myOsFileSeek(hf, 1024);
	myOsFileRead(hf, actemp, sizeof(actemp), NULL);
	assert(0 == memcmp(actemp, "abc", 3));

	myOsFileTruncate(hf, 1024);
	myOsFileSize(hf, &fsz);
	assert(fsz == 1024);

	myOsFileClose(hf);

	assert(myOsFileExists("test_osfile"));
	myOsFileDel("test_osfile");
	assert(!myOsFileExists("test_osfile"));

	MyMemPoolMemReport(1);
}

