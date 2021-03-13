/*
*
* myheap.h ��,ͨ�����������±�����,�������� 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/
#include "myheap.h"
#include "myhashtable.h"


typedef struct __myheap_t_
{
	myvector_t base;

	HMYHASHTABLE hhs;
}myheap_t;


/*
*
*��ϣ����
*
*/
static __INLINE__ size_t heap_hash(const void * key)
{
	return (size_t)key;
}

/*
*
*�Ƚϼ�ֵ�Ƿ����
*1:��� 0:�����
*
*/
static __INLINE__ int heap_equl(const void * key1, const void * key2)
{
	return (key1 == key2)?1:0;
}


static __INLINE__ void heap_update_hashtable(myheap_t * h, const myvector_element * key, size_t index)
{
	HMYHASHTABLE_ITER it = NULL;

	assert(h && h->hhs && key && index < h->base.el_count);

	it = MyHashTableSearch(h->hhs, key);
	if(it)
	{
		MyHashTableSetIterData(it, (void *)index);
		return;
	}

	MyHashTableInsertUnique(h->hhs, key, (void *)index);
}

static __INLINE__ void heap_set_index(myheap_t * h, const myvector_element * el, size_t index)
{
	assert(h );
	assert(h->hhs);
	assert(el);
	assert(index < h->base.el_count);

	h->base.el_array[index] = (myvector_element *)el;

	heap_update_hashtable(h, el, index);
}

static __INLINE__ void heap_del_index(myheap_t * h, size_t index)
{
	assert(h && index < h->base.el_count && h->hhs);

	MyHashTableDelKey(h->hhs, h->base.el_array[index], NULL, NULL);
}

static __INLINE__ size_t heap_get_index(myheap_t * h, myvector_element * el)
{
	HMYHASHTABLE_ITER it = NULL;
	assert(h && h->hhs && el);

	it = MyHashTableSearch(h->hhs, el);
	if(NULL == it)
		return (size_t)-1;

	return (size_t)MyHashTableGetIterData(it);
}


static __INLINE__ void heap_destroy(myheap_t * h)
{
	if(NULL == h)
		return;

	MyHashTableDestruct(h->hhs);

	__vector_inter_destroy((myvector_t *)h);
}

static __INLINE__ void heap_remove_tail(myheap_t * h)
{
	assert(h);

	heap_del_index(h, h->base.el_count - 1);

	__vector_inter_del(&h->base, h->base.el_count - 1);	
}

static __INLINE__ myvector_element * heap_set_index_data(myheap_t * h, size_t index, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;

	assert(h && h->base.el_array && index < h->base.el_count);

	el = h->base.el_array[index];

	if(data && data_size)
	{
		if(el->data_size && el->data && el->data_size >= data_size)
		{
			//����
			if(h->base.data_ops.destruct && el->data_size && el->data)
				h->base.data_ops.destruct(el->data, el->data_size);

			if(h->base.data_ops.construct)
				h->base.data_ops.construct(el->data, el->data_size, NULL, 0);

			//����
			if(h->base.data_ops.copy)
				h->base.data_ops.copy(el->data, el->data_size, data, data_size);
			else
				memcpy(el->data, data, el->data_size);
		}
		else
		{
			//�ͷŽڵ�
			__vector_inter_destroy_element(&h->base, el);

			//���´����ڵ�
			el = h->base.el_array[index] = __vector_inter_create_element(&h->base, data, data_size);
		}
	}
	else
	{
		if(el->data_size && el->data)
		{
			//�ͷŽڵ�
			__vector_inter_destroy_element(&h->base, el);

			//���´����ڵ�
			el = h->base.el_array[index] = __vector_inter_create_element(&h->base, data, data_size);
			//����
		}

		//ֱ��ָ�븳ֵ
		el->data = (void *)data;
		el->data_size = 0;
	}

	return el;
}


/*
*
*����һ���ڵ�,
*
*/
static __INLINE__ void heap_inter_push(myheap_t * h, size_t hole_index, const size_t top_index, const myvector_element * el_v)
{
	size_t parent_index = HEAP_PARENT_INDEX(hole_index);

	assert(h && 
		h->base.el_array && 
		h->base.el_array_size && 
		hole_index < h->base.el_count && 
		top_index < h->base.el_count && 
		h->base.compare);

	while(hole_index > top_index && h->base.compare(el_v->data, h->base.el_array[parent_index]->data, h->base.context) > 0)
	{
		heap_set_index(h, h->base.el_array[parent_index], hole_index);

		hole_index = parent_index;
		parent_index = HEAP_PARENT_INDEX(hole_index);
	}

	heap_set_index(h, el_v, hole_index);
}

/*
*
*������
*
*/
static __INLINE__ void heap_inter_adjust(myheap_t * h, size_t top_index, size_t hole_index, const size_t last, const myvector_element * el_v)
{
	size_t second_child = HEAP_SECOND_CHILD_INDEX(hole_index);

	assert(h && h->base.el_array && h->base.el_array_size >= last + 1 && h->base.compare);

	while(second_child <= last)
	{
		if(h->base.compare(h->base.el_array[second_child - 1]->data, h->base.el_array[second_child]->data, h->base.context) > 0)
			second_child -= 1;

        heap_set_index(h, h->base.el_array[second_child], hole_index);

		hole_index = second_child;
		second_child = HEAP_SECOND_CHILD_INDEX(hole_index);
	}

	if((last + 1) == second_child)
	{
		second_child -= 1;

		heap_set_index(h, h->base.el_array[second_child], hole_index);

		hole_index = second_child;
	}

	heap_inter_push(h, hole_index, top_index, el_v);
}

/*
*
*������������Ԫ��,�����������δβ
*
*@param
	first:�������������ʼλ��
	last:����������Ľ���λ��
	end:������Ľ���λ��

    0       index_for_pop   last        end
	|---------- | ---------- | --------- |
*
*/
static __INLINE__ void heap_inter_pop(myheap_t * h, size_t index_for_pop)
{
	myvector_element * el_v = NULL;

	assert(h && 
		h->base.compare && 
		h->base.el_array && 
		h->base.el_array_size && 
		index_for_pop < h->base.el_count);

	if(1 >= h->base.el_count)
		return;

	//ͷһ���ڵ�Ϊ�ն�����ͷһ���ڵ��ֵ����β�ڵ�
	el_v = h->base.el_array[h->base.el_count - 1];

	heap_set_index(h, h->base.el_array[index_for_pop], h->base.el_count - 1);

	heap_inter_adjust(h, 0, index_for_pop, h->base.el_count - 2, el_v);
}


/*
*
* ����heap
*
*/
HMYHEAP MyHeapConstruct(HMYMEMPOOL hm, int size, myobj_ops * data_ops, ALG_COMPARE compare)
{
	myheap_t * h = (myheap_t *)MyMemPoolMalloc(hm, sizeof(*h));
	if(NULL == h)
		return NULL;

	memset(h, 0, sizeof(*h));
	h->base.hm = hm;
	h->base.compare = compare;

	if(data_ops)
	{
		h->base.data_ops.construct = data_ops->construct;
		h->base.data_ops.copy = data_ops->copy;
		h->base.data_ops.destruct = data_ops->destruct;
	}

	if(0 != __vector_inter_init_array(&h->base, size))
		goto MyHeapConstruct_err_;


	h->hhs = MyHashTableConstruct(hm, heap_hash, heap_equl, 0);
	if(NULL == h->hhs)
		goto MyHeapConstruct_err_;

	return (HMYHEAP)h;

MyHeapConstruct_err_:
	
	heap_destroy(h);

	return NULL;
}

/*
*
* ����heap
*
*/
void MyHeapDestruct(HMYHEAP h)
{
	if(NULL == h)
		return;

	heap_destroy(h);
}

/*
*
* ���һ���ڵ�
*
*/
HMYHEAP_KEY MyHeapPush(HMYHEAP h, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;
	if(NULL == h || NULL == h->base.el_array || 0 == h->base.el_array_size || NULL == h->base.compare)
		return NULL;

	el = __vector_inter_add(&h->base, data, data_size);
	if(NULL == el)
		return NULL;

	heap_inter_push(h, h->base.el_count - 1, 0, el);

	return (HMYHEAP_KEY)el;
}

/*
*
* �Ӷ��е���һ��Ԫ��
*
*/
int MyHeapPop(HMYHEAP h)
{
	if(NULL == h || NULL == h->base.el_array || 0 == h->base.el_array_size || NULL == h->base.compare)
		return -1;

	heap_inter_pop(h, 0);
	heap_remove_tail(h);

	return 0;
}

/*
*
* ȡ����Ԫ��
*
*/
void * MyHeapFront(HMYHEAP h, size_t * data_size)
{
	if(NULL == h || NULL == h->base.el_array || 0 == h->base.el_array_size || 0 == h->base.el_count)
		return NULL;

	if(data_size)
		*data_size = h->base.el_array[0]->data_size;

	return h->base.el_array[0]->data;
}

/*
*
* ȡ����Ԫ�ص�key
*
*/
HMYHEAP_KEY MyHeapFrontKey(HMYHEAP h)
{
	if(NULL == h || NULL == h->base.el_array || 0 == h->base.el_array_size || 0 == h->base.el_count)
		return NULL;

	return (HMYHEAP_KEY)h->base.el_array[0];
}

/*
*
* ɾ��һ��Ԫ��
*
*/
int MyHeapDel(HMYHEAP h, HMYHEAP_KEY key)
{
	size_t index = (size_t)-1;

	if(NULL == h || NULL == h->base.el_array || 0 == h->base.el_array_size || 0 == h->base.el_count)
		return -1;

	assert(key);

	index = heap_get_index(h, (myvector_element *)key);

	if(index == (size_t)-1)
		return -1;

	assert(index < h->base.el_count);

	//��Ҫɾ����Ԫ�ص��������
	heap_inter_pop(h, index);

	//ɾ��
	heap_remove_tail(h);

	return 0;
}

/*
*
* ����һ��Ԫ��
*
*/
HMYHEAP_KEY MyHeapUpdate(HMYHEAP h, HMYHEAP_KEY key, const void * data, const size_t data_size)
{
	size_t index = (size_t)-1;

	if(NULL == h || NULL == h->base.el_array || 0 == h->base.el_array_size || 0 == h->base.el_count)
		return NULL;

	index = heap_get_index(h, (myvector_element *)key);

	if(index == (size_t)-1)
		return NULL;

	assert(index < h->base.el_count);

	//����index����ֵ,������
	key = (HMYHEAP_KEY)heap_set_index_data(h, index, data, data_size);

	heap_inter_adjust(h, 0, index, h->base.el_count - 1, h->base.el_array[index]);

	return key;
}

/*
*
* ����һ��Ԫ��
*
*/
void * MyHeapSearch(HMYHEAP h, HMYHEAP_KEY key, size_t * data_size)
{
	size_t index = (size_t)-1;

	if(NULL == h || NULL == h->base.el_array || 0 == h->base.el_array_size || 0 == h->base.el_count)
		return NULL;

	index = heap_get_index(h, (myvector_element *)key);
	if(index == (size_t)-1)
		return NULL;

	assert(index < h->base.el_count);

	if(data_size)
		*data_size = h->base.el_array[index]->data_size;

	return h->base.el_array[index]->data;
}

/*
*
* ɾ�����е�Ԫ��
*
*/
void MyHeapClear(HMYHEAP h)
{
	if(NULL == h || NULL == h->base.el_array || 0 == h->base.el_array_size)
		return;

	assert(h->hhs);

	__vector_inter_clear(&h->base);

	MyHashTableClear(h->hhs);
}























