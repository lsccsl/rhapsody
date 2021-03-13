///**
// *
// * @file mySkipList.c 跳跃表
// *
// * @brief:
// *
// *		跳跃表如下所示
// *
// *				 新节点
// *				   |
// *		| = 0      |
// *		| ---------|-> | = 0
// *		| ---> | --|-> | ---> | = 0
// *
// *		类似于黄页, 在不同的层次记录关键值.达到快速检索的目的
// *
// *		添加节点时,通过rand随机数来确定新增节点所在的层数
// *		如果新增节点长高(大于当前的最大层数),则修改第一个节点相应增高层数的指针
// *
// *		新节点应替换链表指针链接
// *
// *		删除节点时,则将所删节点从相应的链表层中断开
// *		并且要重新计算链表的层数
// *
// * 1-2-3确定性跳表:
// *    定义1:如果至少存在一个指针从一个元素指向另一个元素,则两个元素称为是链接的(linked)
// *    定义2:两个高度为h链接的元不间的间隙容量(gap size)等于它们之间高度为h-1的元素的个数
// *    1-2-3确定性跳表:每一个间隙(除在头和尾之间可能的零间隙外)的容量为1,2或3.
// *    添加时,自顶向下的过程中,如遇到间隙容量为3的间隙,应使中间项高度长1(分裂)
// *    删除时,当遇到容量为1的间隙,则应放大它,可以向临间隙借,如果无法借(容量均为1),则可以合并(因为保证了上一层的间隙大于1,所以不会造成空间隙)
// *    用链表的形式表示层数节点
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
//	/* 节点关键字 */
//	void * key;
//
//	/* 用户数据 */
//	void * data;
//
//	/* 下层链接表 */
//	struct __myskl_node_t_ * down;
//
//	/* 同层右节点与前向节点指针 */
//	struct __myskl_node_t_ * right;
//	struct __myskl_node_t_ * prev;
//
//	/* 是否为尾结点 */
//	int btail;
//}myskl_node_t;
//
//typedef struct __myskl_t_
//{
//	/* 内存池句柄 */
//	HMYMEMPOOL hm;
//
//	/* 跳表的头元素,即第一串链表的尾元素 */
//	myskl_node_t * first_tail;
//
//	/* 比较运算子 */
//	ALG_COMPARE compare;
//
//	/* 当前的层数 */
//	size_t current_level;
//}myskl_t;
//
//
///**
// * @brief 比较两个节点的key
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
// * @brief 创建一个节点
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
// * @brief 重新分区,使递归下降时,所处的间隙一定大于等于2
// */
//static __INLINE__  void skl_repartion(myskl_t * skl, myskl_node_t * up_left,
//									  myskl_node_t * head, myskl_node_t * tail)
//{
//	assert(skl && up_left && head && tail);
//	????
//}
//
///**
// * @brief 给一个间隙重选分割点
// * @param up_layer_head:上层间隙的"起始节点",亦即要被除去的节点
// * @param up_layer_tail:上层间隙的"尾节点"
// * @param head:当前间隙的起始节点
// * @param tail:当前间隙的结束节点
// * @param up:更上层的节点,可以为null
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
//		* 说明原间隙容量为 左>= 2 右>=1
//		* [? a b] [del c ?]
//		* 分法: 左1 中3 右?
//		* [!] [! ! !] [?]     !:表示一定有, ?:表示有可能不存在
//		* 此时要删除的节点在中间那个间隙
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
//		* 说明原间隙容量为 左1 右3
//		* [ a ] [del b c]
//		* 分法: 左2 右2
//		* [a del] [b c] 
//		* 此时要删除的节点在第一个间隙的第二个节点
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
// * @brief 创建一个节点
// */
//static __INLINE__ void skl_del_at_first_layer(myskl_t * skl, const void * key)
//{
//	///* 跳表头节点 */
//	//myskl_node_t * first_tail = NULL;
//
//	///* 跳表头节点的down指针 */
//	//myskl_node_t * first_tail_down = NULL;
//
//	///* 当前节点 */
//	//myskl_node_t * current = NULL;
//
//	///* current在右节点 */
//	//myskl_node_t * right = NULL;
//
//	///* right的down节点 */
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
//	///* 如果在第一层找到,直接删除 */
//	//if(0 == ret)
//	//{
//	//	/* current的链表前节点 */
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
//	//	* 如果第一层空了,说明一定有删除动作发生
//	//	*/
//	//	myskl_node_t * second_layer_head = first_tail_down->down;
//
//	//	assert(second_layer_head == right);
//
//	//	/*
//	//	* 如果合并后的下层间隙小于等于3,整个跳表高度降低一层
//	//	*/
//	//	if(second_layer_head->right->right->btail)
//	//	{
//	//		first_tail->down = second_layer_head;
//	//		skl_destroy_node(skl, first_tail_down);
//	//	}
//	//	else
//	//	{
//	//		/*
//	//		* 如果大于等于4,选出新的分割点
//	//		*/
//
//	//		myskl_node_t * right_down_prev = right->down->prev;
//
//	//		assert(right_down_prev);
//	//		assert(skl->compare(key, right->down->key, NULL) == 0);
//
//	//		/*
//	//		* 选取合适的分割点,这里注意别把下层要删除的节点选为分割点
//	//		*/
//	//		if(right_down_prev->prev)
//	//		{
//	//			/*
//	//			* 说明原间隙容量为 左>= 2 右>=1
//	//			* [? a b] [del c ?]
//	//			* 分法: 左1 中3 右?
//	//			* [!] [! ! !] [?]     !:表示一定有, ?:表示有可能不存在
//	//			* 此时要删除的节点在中间那个间隙
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
//	//			/* 根据需要创建第二个分割点 */
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
//	//			* 说明原间隙容量为 左1 右3
//	//			* [ a ] [del b c]
//	//			* 分法: 左2 右2
//	//			* [a del] [b c] 
//	//			* 此时要删除的节点在第一个间隙的第二个节点
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
//	///* 如果总和大于等于4,重新分割,保证即将递归的子间隙容量为2 */
//}
//
///**
// * @brief 创建一个节点
// */
//static void skl_destroy_node(myskl_t * skl, myskl_node_t * node)
//{
//	assert(skl && node);
//
//	MyMemPoolFree(skl->hm, node);
//}
//
///**
// * @brief 销毁skl
// */
//static __INLINE__ void skl_destroy(myskl_t * skl)
//{
//	assert(0);
//}
//
//
///**
// * @brief 创建跳跃表
// * @param max_level:跳表的层数
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
// * @brief 销毁跳跃表
// */
//HMYSKL mySkipListDestruct(HMYSKL hskl);
//
///**
// * @brief 往跳表里添加一条记录
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
//		* 比较大小,如果key大,往后跳,key小停止,如果到了该层链表的结尾,也停止
//		*/
//		while(hskl->compare(key, current->key, NULL) > 0 && !(current->btail))
//			current = current->right;
//
//		if(NULL == current->down)
//			break;
//
//		/*
//		* 看看当前的间隙容量是否等于3,如果为3,则需要分裂间隙 
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
//		/* 长高一层 */
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
//		* 新节点关键字赋值
//		*/
//		new_node->key = current->key;
//		new_node->data = current->data;
//		new_node->btail = current->btail;
//
//		/*
//		* 新节点链接关系
//		*/
//		new_node->right = current->right;
//		new_node->down = current->down->right;
//		new_node->prev = current;
//
//		/*
//		* 更新当前节点
//		*/
//		current->key = current->down->right->key;
//		current->data = current->down->right->data;
//
//		/*
//		* 当前节点链接关系更新
//		*/
//		current->right = new_node;
//		current->btail = 0;
//
//		/* 指针不必下走 */
//	}
//
//	/* 直接链入即可 */
//	new_node = skl_create_node(hskl);
//	if(NULL == new_node)
//		return -1;
//
//	/*
//	* 新节点取代current
//	*/
//	new_node->key = current->key;
//	new_node->data = current->data;
//	new_node->btail = current->btail;
//
//	new_node->right = current->right;
//
//	/*
//	* current赋为添加的键与值,更改right链接
//	*/
//	current->key = key;
//	current->data = data;
//	current->btail = 0;
//
//	current->right = new_node;
//}
//
///**
// * @brief 从跳表里删除一条记录
// */
//int mySkipListDel(HMYSKL hskl, const void * key, void ** key_ount, void ** data_out)
//{
//	/*
//	* 在每一层链表里查找,依次删除
//	*
//	* 如果要删除的节点在第一层出现,直接删除了它,如果第一层为空,寻找新的分割节点,或者整个跳表下降一层(次层节点不足4的情况下)
//	* 如果要删除的节点不在第一层,并出现了"总和为4"的问题,则应考虑切换分割节点,即让要删除的区间间隙变成2
//	* 
//	* 非顶层节点处理情况,如果要删除的节点在当前间隙
//	*  如果检查删除后合并下一层的新间隙容量会达到 4~6 选出新的节点分割间隙
//	*  如果删除后当前间隙只剩下一个节点,应选出新的分割点,将当前间隙容量补到2
//	*  检查将要递归到下一层的子间隙,如果它的容量为1,则考虑借,或合并,保证将要递归的新间隙的容量大于等于2
//	*/
//
//	//if(NULL == hskl)
//	//	return -1;
//
//	//while(1)
//	//{
//	//	/*
//	//	* 比较大小,如果key大,往后跳,key小停止,如果到了该层链表的结尾,也停止
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
//	//		* 在递归向下的过程中已确保,当前间隙容量大于等于2
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
// * @brief 查询一条记录
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
