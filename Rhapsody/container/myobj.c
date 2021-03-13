
#include "myobj.h"

typedef struct __myobj_tag_
{	
	myobj_ops * op;
	void * buf;
	size_t buf_len;
}myobj_tag;












