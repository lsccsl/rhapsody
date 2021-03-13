///**
// *
// * @file mySkipList.c ��Ծ��
// *
// * @brief:
// *
// *		��Ծ��������ʾ
// *
// *				 �½ڵ�
// *				   |
// *		| = 0      |
// *		| ---------|-> | = 0
// *		| ---> | --|-> | ---> | = 0
// *
// *		�����ڻ�ҳ, �ڲ�ͬ�Ĳ�μ�¼�ؼ�ֵ.�ﵽ���ټ�����Ŀ��
// *
// *		��ӽڵ�ʱ,ͨ��rand�������ȷ�������ڵ����ڵĲ���
// *		��������ڵ㳤��(���ڵ�ǰ��������),���޸ĵ�һ���ڵ���Ӧ���߲�����ָ��
// *
// *		�½ڵ�Ӧ�滻����ָ������
// *
// *		ɾ���ڵ�ʱ,����ɾ�ڵ����Ӧ��������жϿ�
// *		����Ҫ���¼�������Ĳ���
// *
// * 1-2-3ȷ��������:
// *    ����1:������ٴ���һ��ָ���һ��Ԫ��ָ����һ��Ԫ��,������Ԫ�س�Ϊ�����ӵ�(linked)
// *    ����2:�����߶�Ϊh���ӵ�Ԫ����ļ�϶����(gap size)��������֮��߶�Ϊh-1��Ԫ�صĸ���
// *    1-2-3ȷ��������:ÿһ����϶(����ͷ��β֮����ܵ����϶��)������Ϊ1,2��3.
// *    ���ʱ,�Զ����µĹ�����,��������϶����Ϊ3�ļ�϶,Ӧʹ�м���߶ȳ�1(����)
// *    ɾ��ʱ,����������Ϊ1�ļ�϶,��Ӧ�Ŵ���,�������ټ�϶��,����޷���(������Ϊ1),����Ժϲ�(��Ϊ��֤����һ��ļ�϶����1,���Բ�����ɿռ�϶)
// *    ���������ʽ��ʾ�����ڵ�
// *
// * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
// *
// */
//#include "mySkipList.h"
//
//#include "myconfig.h"
//
//
//typedef struct __myskl_node_t_
//{
//	/* �ڵ�ؼ��� */
//	void * key;
//
//	/* �û����� */
//	void * data;
//
//	/* �²����ӱ� */
//	struct __myskl_node_t_ * down;
//
//	/* ͬ���ҽڵ���ǰ��ڵ�ָ�� */
//	struct __myskl_node_t_ * right;
//	struct __myskl_node_t_ * prev;
//
//	/* �Ƿ�Ϊβ��� */
//	int btail;
//}myskl_node_t;
//
//typedef struct __myskl_t_
//{
//	/* �ڴ�ؾ�� */
//	HMYMEMPOOL hm;
//
//	/* �����ͷԪ��,����һ�������βԪ�� */
//	myskl_node_t * first_tail;
//
//	/* �Ƚ������� */
//	ALG_COMPARE compare;
//
//	/* ��ǰ�Ĳ��� */
//	size_t current_level;
//}myskl_t;
//
//
///**
// * @brief �Ƚ������ڵ��key
// */
//static __INLINE__ int skl_compare(myskl_t * skl, myskl_node_t * node1, myskl_node_t * node2)
//{
//	assert(skl && node1 && node2);
//
//	if(node2->btail)
//		return -1;
//
//	if(node1->btail)
//		return 1;
//
//	return skl->compare(node1->key, node2->key, NULL);
//
//	return 0;
//}
//
///**
// * @brief ����һ���ڵ�
// */
//static __INLINE__ myskl_node_t * skl_create_node(myskl_t * skl)
//{
//	myskl_node_t * node = NULL;
//
//	assert(skl);
//
//	node = MyMemPoolMalloc(skl->hm,  sizeof(*node));
//	if(NULL == node)
//		return NULL;
//
//	memset(node, 0, sizeof(*node));
//
//	return node;
//}
//
///**
// * @brief ���·���,ʹ�ݹ��½�ʱ,�����ļ�϶һ�����ڵ���2
// */
//static __INLINE__  void skl_repartion(myskl_t * skl, myskl_node_t * up_left,
//									  myskl_node_t * head, myskl_node_t * tail)
//{
//	assert(skl && up_left && head && tail);
//	????
//}
//
///**
// * @brief ��һ����϶��ѡ�ָ��
// * @param up_layer_head:�ϲ��϶��"��ʼ�ڵ�",�༴Ҫ����ȥ�Ľڵ�
// * @param up_layer_tail:�ϲ��϶��"β�ڵ�"
// * @param head:��ǰ��϶����ʼ�ڵ�
// * @param tail:��ǰ��϶�Ľ����ڵ�
// * @param up:���ϲ�Ľڵ�,����Ϊnull
// *
// *     up_layer_head     up_layer_tail
// *          |
// *         head  ....         tail
// */
//static __INLINE__ myskl_node_t * skl_vote_partion(myskl_t * skl, myskl_node_t * up_layer_head, myskl_node_t * up_layer_tail, 
//							 myskl_node_t * head, myskl_node_t * tail,
//							 myskl_node_t * up)
//{
//	assert(skl && up_layer_head && up_layer_tail && head && tail && node_del);
//
//	assert(up_layer_head->right == up_layer_tail);
//	assert(skl_compare(up_layer_tail->down, up_layer_head) == 0);
//	assert(up_layer_head->down == head);
//
//	if(head->right->right != tail)
//	{
//		if(up)
//			up->down = up_layer_tail;
//
//		up_layer_tail->down = head;
//
//		return up_layer_tail;
//	}
//
//	if(up_layer_tail->down->prev != head)
//	{
//		/*
//		* ˵��ԭ��϶����Ϊ ��>= 2 ��>=1
//		* [? a b] [del c ?]
//		* �ַ�: ��1 ��3 ��?
//		* [!] [! ! !] [?]     !:��ʾһ����, ?:��ʾ�п��ܲ�����
//		* ��ʱҪɾ���Ľڵ����м��Ǹ���϶
//		*/
//
//		myskl_node_t * node_new1 = skl_create_node(skl);
//		myskl_node_t * node_new2 = NULL;
//		myskl_node_t * node_partion = head->right->right->right;
//
//		if(NULL == node_new1)
//			return NULL;
//
//		node_new1->key = head->right->key;
//		node_new1->data = head->right->data;
//
//		if(up_layer_head->prev)
//		{
//			node_new1->prev = up_layer_head->prev;
//			up_layer_head->prev->right = node_new1;
//		}
//		else
//			node_new1->prev = NULL;
//
//		node_new1->right = up_layer_tail;
//		up_layer_tail->prev = node_new1;
//
//		node_new1->down = head;
//
//		if(up)
//			up->down = up_layer_tail;
//
//		if(node_partion == tail)
//		{
//			up_layer_tail->down = node_partion;
//			return node_new1;
//		}
//
//		node_new2 = skl_create_node(skl);
//		if(NULL == node_new2)
//		{
//			skl_destroy_node(skl, node_new1);
//			return NULL;
//		}
//
//		node_new2->key = node_partion->key;
//		node_new2->data = node_partion->data;
//
//		node_new2->prev = node_new1;
//
//		node_new2->right = node_new1->right;
//		node_new1->prev = node_new2;
//
//		node_new2->down = node_partion;
//
//		assert(0 == node_partion->right->btail);
//		up_layer_tail->down = node_partion->right;
//
//		return node_new1;
//	}
//	else
//	{
//		/* 
//		* ˵��ԭ��϶����Ϊ ��1 ��3
//		* [ a ] [del b c]
//		* �ַ�: ��2 ��2
//		* [a del] [b c] 
//		* ��ʱҪɾ���Ľڵ��ڵ�һ����϶�ĵڶ����ڵ�
//		*/
//
//		myskl_node_t * right = up_layer_tail->down->right;
//		myskl_node_t * node_new1 = skl_create_node(skl);
//
//		assert(0 == right->btail);
//
//		if(NULL == node_new1)
//			return NULL;
//
//		if(up)
//			up->down = up_layer_tail;
//
//		node_new1->key = right->key;
//		node_new1->data = right->data;
//
//		if(up_layer_head->prev)
//		{
//			node_new1->prev = up_layer_head->prev;
//			up_layer_head->prev->right = node_new1;
//		}
//		else
//			node_new1->prev = NULL;
//
//		node_new1->right = up_layer_tail;
//		up_layer_tail->prev = node_new1;
//
//		node_new1->down = head;
//
//		up_layer_tail->down = right;
//
//		return node_new1;
//	}
//}
//
///**
// * @brief ����һ���ڵ�
// */
//static __INLINE__ void skl_del_at_first_layer(myskl_t * skl, const void * key)
//{
//	///* ����ͷ�ڵ� */
//	//myskl_node_t * first_tail = NULL;
//
//	///* ����ͷ�ڵ��downָ�� */
//	//myskl_node_t * first_tail_down = NULL;
//
//	///* ��ǰ�ڵ� */
//	//myskl_node_t * current = NULL;
//
//	///* current���ҽڵ� */
//	//myskl_node_t * right = NULL;
//
//	///* right��down�ڵ� */
//	//myskl_node_t * right_down = NULL;
//
//	//assert(skl);
// //   
//	//first_tail = skl->first_tail;
//	//current = first_tail->down;
//
//	//int ret = skl->compare(key, current->key, NULL);
//	//while(ret > 0 && !(current->btail))
//	//{
//	//	current = current->right;
//	//	ret = skl->compare(key, current->key, NULL);
//	//}
//
//	//right = current->right;
//	//right_down = right->down;
//
//	///* ����ڵ�һ���ҵ�,ֱ��ɾ�� */
//	//if(0 == ret)
//	//{
//	//	/* current������ǰ�ڵ� */
//	//	myskl_node_t * prev = current->prev;
//	//	if(prev)
//	//	{
//	//		prev->right = right;
//	//		right->prev = prev;
//	//	}
//	//	else
//	//		right->prev = NULL;
//
//	//	if(current == first_tail->down)
//	//		first_tail->down = right;
//
//	//	skl_destroy_node(skl, current);
//	//}
//
//	//first_tail_down = first->down;
//
//	//if(first_tail_down->btail)
//	//{
//	//	/*
//	//	* �����һ�����,˵��һ����ɾ����������
//	//	*/
//	//	myskl_node_t * second_layer_head = first_tail_down->down;
//
//	//	assert(second_layer_head == right);
//
//	//	/*
//	//	* ����ϲ�����²��϶С�ڵ���3,��������߶Ƚ���һ��
//	//	*/
//	//	if(second_layer_head->right->right->btail)
//	//	{
//	//		first_tail->down = second_layer_head;
//	//		skl_destroy_node(skl, first_tail_down);
//	//	}
//	//	else
//	//	{
//	//		/*
//	//		* ������ڵ���4,ѡ���µķָ��
//	//		*/
//
//	//		myskl_node_t * right_down_prev = right->down->prev;
//
//	//		assert(right_down_prev);
//	//		assert(skl->compare(key, right->down->key, NULL) == 0);
//
//	//		/*
//	//		* ѡȡ���ʵķָ��,����ע�����²�Ҫɾ���Ľڵ�ѡΪ�ָ��
//	//		*/
//	//		if(right_down_prev->prev)
//	//		{
//	//			/*
//	//			* ˵��ԭ��϶����Ϊ ��>= 2 ��>=1
//	//			* [? a b] [del c ?]
//	//			* �ַ�: ��1 ��3 ��?
//	//			* [!] [! ! !] [?]     !:��ʾһ����, ?:��ʾ�п��ܲ�����
//	//			* ��ʱҪɾ���Ľڵ����м��Ǹ���϶
//	//			*/
//	//			myskl_node_t * node_new1 = skl_create_node(skl);
//	//			myskl_node_t * node_partion = second_layer_head->right->right->right;
//	//			assert(node_new1);//bug here;
//
//	//			node_new1->key = second_layer_head->right->key;
//	//			node_new1->data = second_layer_head->right->data;
//
//	//			node_new1->prev = NULL;
//	//			node_new1->right = first_tail_down;
//	//			node_new1->down = second_layer_head;
//
//	//			first_tail->down = first_tail_down = node_new1;
//
//	//			/* ������Ҫ�����ڶ����ָ�� */
//	//			if(!node_partion->btail)
//	//			{
//	//				myskl_node_t * node_new2 = skl_create_node(skl);
//	//				assert(node_new2);//bug here;
//
//	//				node_new2->key = node_partion->key;
//	//				node_new2->data = node_partion->data;
//
//	//				node_new2->prev = node_new1;
//	//				node_new2->right = first_tail_down;
//	//				node_new2->down = second_layer_head->right;
//
//	//				node_new1->right = node_new2;
//
//	//				first_tail_down->down = node_partion;
//	//			}
//	//			else
//	//			{
//	//				first_tail_down->down = second_layer_head->right;
//	//			}
//	//		}
//	//		else 
//	//		{
//	//			/* 
//	//			* ˵��ԭ��϶����Ϊ ��1 ��3
//	//			* [ a ] [del b c]
//	//			* �ַ�: ��2 ��2
//	//			* [a del] [b c] 
//	//			* ��ʱҪɾ���Ľڵ��ڵ�һ����϶�ĵڶ����ڵ�
//	//			*/
//
//	//			myskl_node_t * node_new = skl_create_node(skl);
//	//			myskl_node_t * node_partion = right->down->right;
//	//			assert(node_new);//bug here
//
//	//			node_new->key = node_partion->key;
//	//			node_new->data = node_partion->data;
//
//	//			node_new->prev = NULL;
//	//			node_new->right = first_tail_down;
//	//			node_new->down = second_layer_head;
//
//	//			first_tail_down->down = node_partion;
//	//		}
//	//	}
//	//}
//	//else
//	//{
//	//}
//
//	///* ����ܺʹ��ڵ���4,���·ָ�,��֤�����ݹ���Ӽ�϶����Ϊ2 */
//}
//
///**
// * @brief ����һ���ڵ�
// */
//static void skl_destroy_node(myskl_t * skl, myskl_node_t * node)
//{
//	assert(skl && node);
//
//	MyMemPoolFree(skl->hm, node);
//}
//
///**
// * @brief ����skl
// */
//static __INLINE__ void skl_destroy(myskl_t * skl)
//{
//	assert(0);
//}
//
//
///**
// * @brief ������Ծ��
// * @param max_level:����Ĳ���
// */
//HMYSKL mySkipListConstruct(HMYMEMPOOL hm, ALG_COMPARE compare)
//{
//	myskl_t * skl = MyMemPoolMalloc(hm, sizeof(*skl));
//	if(NULL == skl)
//		return NULL;
//
//	skl->hm = hm;
//	skl->compare = compare;
//	skl->current_level = 0;
//
//	skl->first_tail = skl_create_node(skl);
//
//	skl->first_tail->right = skl->first_tail;
//	skl->first_tail->prev = NULL;
//	skl->first_tail->btail = 1;
//
//	return skl;
//
//mySkipListConstruct_end_:
//
//	skl_destroy(skl);
//
//	return NULL;
//}
//
///**
// * @brief ������Ծ��
// */
//HMYSKL mySkipListDestruct(HMYSKL hskl);
//
///**
// * @brief �����������һ����¼
// */
//int mySkipListAdd(HMYSKL hskl, const void * key, const void * data)
//{
//	myskl_node_t * new_node = NULL;
//	myskl_node_t * current = NULL;
//
//	if(NULL == hskl)
//		return -1;
//
//	current = hskl->first_tail;
//
//	while(1)
//	{
//		/*
//		* �Ƚϴ�С,���key��,������,keyСֹͣ,������˸ò�����Ľ�β,Ҳֹͣ
//		*/
//		while(hskl->compare(key, current->key, NULL) > 0 && !(current->btail))
//			current = current->right;
//
//		if(NULL == current->down)
//			break;
//
//		/*
//		* ������ǰ�ļ�϶�����Ƿ����3,���Ϊ3,����Ҫ���Ѽ�϶ 
//		*/
//		if(skl_compare(hskl, current, current->down->right->right) <= 0)
//		{
//			current = current->down;
//			continue;
//		}
//
//		new_node = skl_create_node(hskl);
//		if(NULL == new_node)
//			return -1;
//
//		/* ����һ�� */
//		if(current == hskl->first_tail)
//		{	
//			myskl_node_t * new_list_tail = skl_create_node(hskl);
//			new_list_tail = 1;
//			if(NULL == new_list_tail)
//			{
//				skl_destroy_node(new_node);
//				return -1;
//			}
//
//			new_list_tail->down = current->down;
//			current->down = new_list_tail;
//			new_list_tail->right = new_list_tail;
//			new_list_tail->prev = NULL;
//
//			current = new_list_tail;
//		}
//
//		/*
//		* �½ڵ�ؼ��ָ�ֵ
//		*/
//		new_node->key = current->key;
//		new_node->data = current->data;
//		new_node->btail = current->btail;
//
//		/*
//		* �½ڵ����ӹ�ϵ
//		*/
//		new_node->right = current->right;
//		new_node->down = current->down->right;
//		new_node->prev = current;
//
//		/*
//		* ���µ�ǰ�ڵ�
//		*/
//		current->key = current->down->right->key;
//		current->data = current->down->right->data;
//
//		/*
//		* ��ǰ�ڵ����ӹ�ϵ����
//		*/
//		current->right = new_node;
//		current->btail = 0;
//
//		/* ָ�벻������ */
//	}
//
//	/* ֱ�����뼴�� */
//	new_node = skl_create_node(hskl);
//	if(NULL == new_node)
//		return -1;
//
//	/*
//	* �½ڵ�ȡ��current
//	*/
//	new_node->key = current->key;
//	new_node->data = current->data;
//	new_node->btail = current->btail;
//
//	new_node->right = current->right;
//
//	/*
//	* current��Ϊ��ӵļ���ֵ,����right����
//	*/
//	current->key = key;
//	current->data = data;
//	current->btail = 0;
//
//	current->right = new_node;
//}
//
///**
// * @brief ��������ɾ��һ����¼
// */
//int mySkipListDel(HMYSKL hskl, const void * key, void ** key_ount, void ** data_out)
//{
//	/*
//	* ��ÿһ�����������,����ɾ��
//	*
//	* ���Ҫɾ���Ľڵ��ڵ�һ�����,ֱ��ɾ������,�����һ��Ϊ��,Ѱ���µķָ�ڵ�,�������������½�һ��(�β�ڵ㲻��4�������)
//	* ���Ҫɾ���Ľڵ㲻�ڵ�һ��,��������"�ܺ�Ϊ4"������,��Ӧ�����л��ָ�ڵ�,����Ҫɾ���������϶���2
//	* 
//	* �Ƕ���ڵ㴦�����,���Ҫɾ���Ľڵ��ڵ�ǰ��϶
//	*  ������ɾ����ϲ���һ����¼�϶������ﵽ 4~6 ѡ���µĽڵ�ָ��϶
//	*  ���ɾ����ǰ��϶ֻʣ��һ���ڵ�,Ӧѡ���µķָ��,����ǰ��϶��������2
//	*  ��齫Ҫ�ݹ鵽��һ����Ӽ�϶,�����������Ϊ1,���ǽ�,��ϲ�,��֤��Ҫ�ݹ���¼�϶���������ڵ���2
//	*/
//
//	//if(NULL == hskl)
//	//	return -1;
//
//	//while(1)
//	//{
//	//	/*
//	//	* �Ƚϴ�С,���key��,������,keyСֹͣ,������˸ò�����Ľ�β,Ҳֹͣ
//	//	*/
//	//	int ret = hskl->compare(key, current->key, NULL);
//	//	while(ret > 0 && !(current->btail))
//	//	{
//	//		current = current->right;
//	//		prev = current;
//	//		ret = hskl->compare(key, current->key, NULL);
//	//	}
//
//	//	if(0 == ret)
//	//	{
//	//		/*
//	//		* �ڵݹ����µĹ�������ȷ��,��ǰ��϶�������ڵ���2
//	//		*/
//	//		if(prev)
//	//			prev->right;
//	//	}
//
//	//	if(NULL == current->down)
//	//		break;
//	//}
//
//	//return 0;
//}
//
///**
// * @brief ��ѯһ����¼
// */
//int mySkipListSearch(HMYSKL hskl, const void * key, void ** data_out)
//{}
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
