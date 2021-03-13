/*
*
* myhashtable.c ��ϣ�� 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "myutility.h"
#include "myhashtable.h"


typedef struct __myhash_node_t_
{
	void * key;
	void * data;

	size_t hash_key;

	struct __myhash_node_t_ * next;
}myhash_node_t;

typedef struct __myhashtable_t_
{
	MYHASH_FUN hash_fun;
	MYHASH_KEYEQUAL_FUN keyequal_fun;

	HMYMEMPOOL hm;

	//��ϣ����
	myhash_node_t ** hash_array;
	//��ϣ�����С
	size_t hash_array_size;

	//Ԫ�ؼ���
	size_t element_count;
}myhash_t;


#define __hash_num_primes (sizeof(__hash_prime_list)/sizeof(unsigned long))
#define __hash_array_size(s) ((s) * sizeof(myhash_node_t *))
#define get_next_hash_size(s) ((s) * 2)


static __INLINE__ int hash_inter_need_resize(myhash_t * h)
{
	assert(h);

	if(h->element_count >= h->hash_array_size)
		return 1;

	return 0;
}

static __INLINE__ int hash_inter_equal(myhash_t * h, const void * key1, const void * key2)
{
	assert(h && h->keyequal_fun);

	return (*h->keyequal_fun)(key1, key2);
}

static __INLINE__ size_t hash_inter_calhash(myhash_t * h, const void * key)
{
	assert(h && h->hash_fun);

	return (*h->hash_fun)(key);
}

/*
*
*�ع���ϣ��,�ɹ�0,ʧ��-1;
*
*/
static __INLINE__ int hash_inter_resize(myhash_t * h)
{
	myhash_node_t ** tmp = NULL;
	size_t next_size = 0;
	size_t i = 0;

	assert(h && h->hash_array && h->hash_array_size);

	next_size = get_next_hash_size(h->hash_array_size);

	//�����ϣ����
	tmp = (myhash_node_t **)MyMemPoolMalloc(h->hm, __hash_array_size(next_size));
	memset(tmp, 0, __hash_array_size(next_size));
	if(NULL == tmp)
		return -1;

	//���¼���ÿ��Ԫ�ص�λ��
	for(i = 0; i < h->hash_array_size; i ++)
	{
		myhash_node_t * node = h->hash_array[i];
		while(node)
		{
			h->hash_array[i] = node->next;

			node->hash_key = hash_inter_calhash(h, node->key) % next_size;

			node->next = tmp[node->hash_key];

			tmp[node->hash_key] = node;

			//ָ��������
			node = h->hash_array[i];
		}
	}

	//�ͷ�ԭ�еĹ�ϣ����
	MyMemPoolFree(h->hm, h->hash_array);

	h->hash_array = tmp;
	h->hash_array_size = next_size;

	return 0;
}

static __INLINE__ myhash_node_t * hash_inter_create_node(myhash_t * h, const void * key, const void * data)
{
	myhash_node_t * node = NULL;

	assert(h && h->hash_array && h->hash_array_size);

	node = (myhash_node_t *)MyMemPoolMalloc(h->hm, sizeof(*node));
	if(NULL == node)
		return NULL;

	node->key = (void *)key;
	node->data = (void *)data;

	node->next = NULL;

	return node;
}

/*
*
*���Ԫ��,���ع�
*
*/
static __INLINE__ int hash_inter_add_without_resize(myhash_t * h, myhash_node_t * node)
{
	assert(h && h->hash_array && h->hash_array_size && node);

	//�����ϣ
	node->hash_key = hash_inter_calhash(h, node->key) % h->hash_array_size;

	//�ҽ���ϣͰ
	node->next = h->hash_array[node->hash_key];

	h->hash_array[node->hash_key] = node;

	h->element_count += 1;

	return 0;
}

/*
*
*���,������Ҫ�ع�
*
*/
static __INLINE__ int hash_inter_add_with_resize(myhash_t * h, myhash_node_t * node)
{
	assert(h && h->hash_array && h->hash_array_size && node);
	
	//�ж��Ƿ���Ҫ�ع���ϣ��
	if(hash_inter_need_resize(h))
	{
		if(-1 == hash_inter_resize(h))
			return -1;
	}

	//���Ԫ�� ���ص�����
	return hash_inter_add_without_resize(h, node);
}

/*
*
*����
*
*/
static __INLINE__ myhash_node_t * hash_inter_search(myhash_t * h, const void * key)
{
	size_t hash_key = 0;
	myhash_node_t * node = NULL;

	assert(h && h->hash_array && h->hash_array_size);

	//�����ϣ
	hash_key = hash_inter_calhash(h, key) % h->hash_array_size;

	node = h->hash_array[hash_key];

	//���Բ���
	while(node)
	{
		if(hash_inter_equal(h, node->key, key))
			break;

		node = node->next;
	}

	return node;
}

/*
*
*���ٽڵ�
*
*/
static __INLINE__ void hash_inter_outlink_node(myhash_t * h, myhash_node_t * prev, myhash_node_t * node, void ** key, void ** data)
{
	assert(h && prev && node && (prev->next == node || prev == node));

	if(key)
		*key = node->key;
	if(data)
		*data = node->data;

	//�ӹ�ϣͰ������
	if(prev != node)
		prev->next = node->next;
	
	//���Ҫɾ������ͷ�ڵ�
	if(node == h->hash_array[node->hash_key])
		h->hash_array[node->hash_key] = node->next;

	h->element_count -= 1;
}

/*
*
*ɾ�����нڵ�
*
*/
static __INLINE__ void hash_inter_del_all(myhash_t * h)
{
	size_t i = 0;

	assert(h->hash_array && h->hash_array_size);

	for(i = 0; i < h->hash_array_size; i ++)
	{
		myhash_node_t * tmp = h->hash_array[i];
		while(tmp)
		{
			myhash_node_t * t = tmp;
			tmp = tmp->next;

			MyMemPoolFree(h->hm, t);
		}
	}

	memset(h->hash_array, 0, __hash_array_size(h->hash_array_size));
	h->element_count = 0;
}

static __INLINE__ int hashtable_inter_construct(myhash_t * hht, HMYMEMPOOL hm, MYHASH_FUN hash_fun, MYHASH_KEYEQUAL_FUN keyequal_fun, size_t hash_size)
{
#define DEF_HASH_SIZE 32

	assert(hht && hash_fun && keyequal_fun);

	hht->element_count = 0;
	hht->hm = hm;
	hht->hash_fun = hash_fun;
	hht->keyequal_fun = keyequal_fun;

	hht->hash_array_size = get_next_hash_size(hash_size);
	if(0 == hht->hash_array_size)
		hht->hash_array_size = DEF_HASH_SIZE;

	hht->hash_array = (myhash_node_t**)MyMemPoolMalloc(hm, __hash_array_size(hht->hash_array_size));	

	if(NULL == hht->hash_array)
		return -1;

	memset(hht->hash_array, 0, __hash_array_size(hht->hash_array_size));

	return 0;

#undef DEF_HASH_SIZE
}


/*
*
*��ϣ����
*
*/
HMYHASHTABLE MyHashTableConstruct(HMYMEMPOOL hm, MYHASH_FUN hash_fun, MYHASH_KEYEQUAL_FUN keyequal_fun, size_t hash_size)
{
	myhash_t * hht = NULL;

	if(NULL == hash_fun || NULL == keyequal_fun)
		return NULL;

	hht = (myhash_t *)MyMemPoolMalloc(hm, sizeof(*hht));
	if(NULL == hht)
		return NULL;

	if(0 == hashtable_inter_construct(hht, hm, hash_fun, keyequal_fun, hash_size))
		return (HMYHASHTABLE)hht;

	if(hht)
		MyMemPoolFree(hm, hht);

	return NULL;
}

/*
*
*��ϣ������
*
*/
void MyHashTableDestruct(HMYHASHTABLE hht)
{
	myhash_t * h = (myhash_t *)hht;
	if(NULL == h)
		return;

	if(NULL != h->hash_array && h->hash_array_size)
		hash_inter_del_all(h);

	MyMemPoolFree(h->hm, h->hash_array);
	MyMemPoolFree(h->hm, h);
}

/*
*
*��ϣ�����һ����¼,�����ظ�
*
*/
HMYHASHTABLE_ITER MyHashTableInsertEqual(HMYHASHTABLE hht, const void * key, const void * data)
{
	myhash_node_t * node = NULL;
	myhash_t * h = (myhash_t *)hht;

	if(NULL == h || NULL == h->hash_fun || NULL == h->keyequal_fun || NULL == h->hash_array)
		return NULL;

	//return (HMYHASHTABLE_ITER)hash_inter_add_with_resize(h, key, data);

	node = hash_inter_create_node(h, key, data);
	if(NULL == node)
		return NULL;

	if(hash_inter_add_with_resize(h, node) == 0)
		return (HMYHASHTABLE_ITER)node;

	MyMemPoolFree(h->hm, node);
	return NULL;
}

/*
*
*����һ����¼,�������ظ�
*
*/
HMYHASHTABLE_ITER MyHashTableInsertUnique(HMYHASHTABLE hht, const void * key, const void * data)
{
	myhash_node_t * node = NULL;
	myhash_t * h = (myhash_t *)hht;

	if(NULL == h)
		return NULL;

	//����,����ҵ�����null
	if(hash_inter_search(h, key))
		return NULL;

	//�Ҳ������
	//return (HMYHASHTABLE_ITER)hash_inter_add_with_resize(h, key, data);

	node = hash_inter_create_node(h, key, data);
	if(NULL == node)
		return NULL;

	if(hash_inter_add_with_resize(h, node) == 0)
		return (HMYHASHTABLE_ITER)node;

	MyMemPoolFree(h->hm, node);
	return NULL;
}

/*
*
*ɾ��һ����¼,����key,
*�ɹ�ɾ������0, ���򷵻�-1
*
*/
int MyHashTableDelKey(HMYHASHTABLE hht, const void * key, void ** key_ret, void ** data)
{
	myhash_t * h = (myhash_t *)hht;
	size_t hash_key = 0;
	myhash_node_t * node = NULL;
	myhash_node_t * prev = NULL;

	if(NULL == h || NULL == h->keyequal_fun || NULL == h->hash_fun || NULL == h->hash_array || 0 == h->hash_array_size)
		return -1;

	hash_key = hash_inter_calhash(h, key) % h->hash_array_size;

	//�����ϣ
	prev = node = h->hash_array[hash_key];

	//���Բ���
	while(node)
	{
		if(hash_inter_equal(h, node->key, key))
			break;

		prev = node;
		node = node->next;
	}

	if(NULL == node)
		return -1;

	hash_inter_outlink_node(h, prev, node, key_ret, data);
	MyMemPoolFree(h->hm, node);

	return 0;
}

/*
*
*ɾ��һ����¼,���ݵ�����
*
*/
int MyHashTableDelIter(HMYHASHTABLE hht, HMYHASHTABLE_ITER iter, void ** key, void ** data)
{
	myhash_t * h = (myhash_t *)hht;
	myhash_node_t * node = (myhash_node_t *)iter;
	myhash_node_t * prev = NULL;
	size_t hash_key = 0;

	if(NULL == h || NULL == node || NULL == h->hash_array || 0 == h->hash_array_size)
		return -1;

	//ȡ��hash key���ж���Ϸ�
	hash_key = node->hash_key;
	if(hash_key >= h->hash_array_size)
		return -1;

	prev = h->hash_array[hash_key];
	while(prev)
	{
		if(node == prev->next)
			break;

		prev = prev->next;
	}

	if(NULL == prev)
		return -1;

	hash_inter_outlink_node(h, prev, node, key, data);
	MyMemPoolFree(h->hm, node);

	return 0;
}

/*
*
*����һ����¼
*
*/
HMYHASHTABLE_ITER MyHashTableSearch(const HMYHASHTABLE hht, const void * key)
{
	myhash_t * h = (myhash_t *)hht;
	if(NULL == h)
		return NULL;

	return (HMYHASHTABLE_ITER)hash_inter_search(h, key);
}

/*
*
*��ȡ��������ֵ��
*
*/
void * MyHashTableGetIterData(const HMYHASHTABLE_ITER it)
{
	myhash_node_t * node = (myhash_node_t *)it;
	if(NULL == node)
		return NULL;

	return node->data;
}

/*
*
*���õ�������ֵ��
*
*/
void MyHashTableSetIterData(const HMYHASHTABLE_ITER it, const void * data)
{
	myhash_node_t * node = (myhash_node_t *)it;
	if(NULL == node)
		return;

	node->data = (void *)data;
}

/*
*
*��ȡ�������ļ�ֵ
*
*/
const void * MyHashTableGetIterKey(HMYHASHTABLE_ITER it)
{
	myhash_node_t * node = (myhash_node_t *)it;
	if(NULL == node)
		return NULL;

	return node->key;
}

/*
*
*��ȡ�������ļ�ֵ
*
*/
HMYHASHTABLE_ITER MyHashTableBegin(HMYHASHTABLE hht)
{
	size_t i = 0;
	myhash_t * h = (myhash_t *)hht;
	if(NULL == h || NULL == h->hash_array)
		return NULL;

	for(i = 0; i < h->hash_array_size; i ++)
	{
		if(NULL == h->hash_array[i])
			continue;

		return (HMYHASHTABLE_ITER)h->hash_array[i];
	}

	return NULL;
}

/*
*
*��ȡһ�µ�����
*
*/
HMYHASHTABLE_ITER MyHashTableGetNext(HMYHASHTABLE hht, HMYHASHTABLE_ITER it)
{
	size_t i = 0;
	myhash_node_t * node = (myhash_node_t *)it;
	myhash_t * h = (myhash_t *)hht;
	if(NULL == h || NULL == h->hash_array)
		return NULL;

	if(node && node->next)
		return (HMYHASHTABLE_ITER)node->next;

	for(i = node->hash_key + 1; i < h->hash_array_size; i ++)
	{
		if(NULL == h->hash_array[i])
			continue;

		return (HMYHASHTABLE_ITER)h->hash_array[i];
	}

	return NULL;
}

/*
*
*��ȡԪ�ظ���
*
*/
size_t MyHashTableGetElementCount(const HMYHASHTABLE hht)
{
	myhash_t * h = (myhash_t *)hht;
	if(NULL == h)
		return 0;

	return h->element_count;
}

/*
*
*��ȡһ�µ�����
*
*/
void MyHashTableClear(HMYHASHTABLE hht)
{

	myhash_t * h = (myhash_t *)hht;
	if(NULL == h)
		return;

	assert(h->hash_array && h->hash_array_size);

	hash_inter_del_all(h);
}

/*
*
*��ӡ��ϣ��
*
*/
void MyHashTablePrint(const HMYHASHTABLE hht)
{
	size_t i = 0;
	myhash_t * h = (myhash_t *)hht;
	assert(h);

	printf("\n");
	for(i = 0; i < h->hash_array_size; i ++)
	{
		myhash_node_t * tmp = h->hash_array[i];
		if(h->hash_array[i])
			printf("\n");

		printf("%d:",i);
		while(tmp)
		{
#ifdef _MBCSV6
			printf("[%d-%d]", tmp->key, tmp->data);
#else
#ifdef WIN32
			printf("[%d-%d]", (long long)tmp->key, (long long)tmp->data);
#else
			printf("[%d-%d]", tmp->key, tmp->data);
#endif
#endif
			tmp = tmp->next;
		}
	}
	printf("\n");
}

#include "myhashmap.c"

































