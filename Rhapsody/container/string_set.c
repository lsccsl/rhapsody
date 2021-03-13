/*
*
* string_set.h from boost 字符串查找集合字符串查找集合-只需要"一次"字符串比较即可完成从无限个数的字符串集合里提取所需的字符串 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/


#include <string.h>
#include <assert.h>

#include "string_set.h"
#include "myutility.h"

#define INVAL_DATA ((unsigned long)-1)

typedef struct __char_tst_node_
{
	struct __char_tst_node_ * left;
	struct __char_tst_node_ * right;
	struct __char_tst_node_ * middle;

	struct __char_tst_node_ * parent;

	unsigned long data;
	size_t data_size;
	char char_value;

	char Turtle_Rhapsody[3];

}char_tst_node;

typedef struct __string_tst_tree_
{
	myobj_ops data_ops;
	HMYMEMPOOL hm;

	char_tst_node * root;
}string_tst_tree;


static __INLINE__ char_tst_node * string_set_inter_create_node(string_tst_tree * tst_tree)
{
	char_tst_node * node = NULL;

	assert(tst_tree);

	node = (char_tst_node *)MyMemPoolMalloc(tst_tree->hm, sizeof(*node));
	memset(node, 0, sizeof(*node));

	node->data = INVAL_DATA;

	return node;
}

static __INLINE__ void string_set_inter_destroy_node(string_tst_tree * tst_tree, char_tst_node * node)
{
	assert(tst_tree && node);

	MyMemPoolFree(tst_tree->hm, node);
}

static __INLINE__ char_tst_node * stringset_inter_search(string_tst_tree * tst_tree, char * first, const char * last, unsigned long * data, size_t * data_size)
{
	char_tst_node * current = NULL;

	assert(tst_tree && first && last && last != first);

	current = tst_tree->root;

	while(current)
	{
		if((*first) > current->char_value)
		{
			//大于,往右走
			current = current->right;

			continue;
		}
		else if((*first) < current->char_value)
		{
			//小于,往左走
			current = current->left;

			continue;
		}
		else if((*first) == current->char_value)
		{
			//字符串指针下走
			first ++;
		}

		if(last != first && 0 != *first)
		{
			//相等,往中间走,
			current = current->middle;

			continue;
		}

		if(INVAL_DATA == current->data)
			return NULL;

		if(data)
			*data = current->data;

		if(data_size)
			*data_size = current->data_size;

		return current;
	}

	return NULL;
}

static __INLINE__ void stringset_inter_clear(string_tst_tree * tss, char_tst_node * node)
{
	assert(tss && node);

	if(node->left)
	{
		stringset_inter_clear(tss, node->left);
		node->left = NULL;
	}

	if(node->right)
	{
		stringset_inter_clear(tss, node->right);
		node->right = NULL;
	}

	if(node->middle)
	{
		stringset_inter_clear(tss, node->middle);
		node->middle = NULL;
	}

	string_set_inter_destroy_node(tss, node);
}


/*
*
*创建字符串集
*
*/
HSTRING_SET StringSetConstruct(HMYMEMPOOL hm, myobj_ops * data_ops)
{
	string_tst_tree * tst_tree = (string_tst_tree *)MyMemPoolMalloc(hm, sizeof(*tst_tree));

	if(NULL == tst_tree)
		return NULL;

	tst_tree->hm = hm;
	tst_tree->root = NULL;

	if(data_ops)
	{
		tst_tree->data_ops.construct = data_ops->construct;
		tst_tree->data_ops.destruct = data_ops->destruct;
		tst_tree->data_ops.copy = data_ops->copy;
	}

	return (HSTRING_SET)tst_tree;
}

/*
*
*创建字符串集
*
*/
void StringSetDestruct(HSTRING_SET hss)
{
	string_tst_tree * tst_tree = (string_tst_tree *)hss;
	if(NULL == tst_tree)
		return;

	//遍历树,依次释放每个节点
	if(tst_tree->root)
	{
		stringset_inter_clear(tst_tree, tst_tree->root);
		tst_tree->root = NULL;
	}

	MyMemPoolFree(tst_tree->hm, tst_tree);
}

/*
*
*添加字符串
*
*/
int StringSetAdd(const HSTRING_SET hss, char * first, const char * last, unsigned long data, size_t data_size)
{
	string_tst_tree * tst_tree = (string_tst_tree *)hss;
	char_tst_node ** current = NULL;
	char_tst_node * current_parent = NULL;

	if(NULL == first || NULL == last || last == first || 0 == *first || NULL == tst_tree)
		return -1;

	current = &(tst_tree->root);

	for(;;)
	{
		//当前指针为空,赋值,源字符串下走,
		if(NULL == *current)
		{
			*current = string_set_inter_create_node(tst_tree);
			//*current = MyMemPoolMalloc(tst_tree->hm, sizeof(**current));
			//memset(*current, 0, sizeof(**current));

			(*current)->char_value = *first;
			(*current)->parent = current_parent;

			//字符串指针往下走
			first ++;
		}
		else if(*first > (*current)->char_value)	
		{
			current_parent = (*current);

			//源字符串大于当前指针的值,往右
			current = &((*current)->right);
			continue;
		}
		else if(*first < (*current)->char_value)
		{
			current_parent = (*current);

			//源字符串小于当前指针的值,往左
			current = &((*current)->left);
			continue;
		}
		else
		{
			//字符串指针往下走
			first ++;
		}

		if(last != first && 0 != *first)
		{
			current_parent = (*current);

			//源字符串等于当前指针的值,往中间走
			current = &((*current)->middle);

			continue;
		}

		//如果已经存在这个字符串序列
		if(INVAL_DATA != (*current)->data)
			return -1;

		(*current)->data = data;
		(*current)->data_size = data_size;

		return 0;
	}
}

/*
*
*添加字符串
*
*/
int StringSetSearch(const HSTRING_SET hss, char * first, const char * last, unsigned long * data, size_t * data_size)
{
	string_tst_tree * tst_tree = (string_tst_tree *)hss;

	if(NULL == tst_tree || NULL == first || NULL == last || last == first)
		return -1;

	if(stringset_inter_search(tst_tree, first, last, data, data_size))
		return 0;

	return -1;
}

/*
*
*删除一个指定序列
*
*/
int StringSetDel(const HSTRING_SET hss, char * first, const char * last, unsigned long * data, size_t * data_size)
{
	string_tst_tree * tst_tree = (string_tst_tree *)hss;
	char_tst_node * current = NULL;

	if(NULL == tst_tree || NULL == first || NULL == last || last == first)
		return -1;

	current = stringset_inter_search(tst_tree, first, last, data, data_size);

	if(NULL == current)
		return -1;

	current->data = INVAL_DATA;

	while(current && (NULL == current->middle) && (current->data == INVAL_DATA))
	{
		//如果current没有兄弟(左右)节点,释放current,current = current -> parent,
		if(NULL == current->left && NULL == current->right && current->parent && current->parent->middle == current)
		{
			char_tst_node * parent = current->parent;
			parent->middle = NULL;
			string_set_inter_destroy_node(tst_tree, current);
			current = parent;
			continue;
		}

		//如果current有兄弟(左右)节点,取其上做为其父节点的middle/left/right,并break;
		if(current->left)
		{
			char_tst_node * temp = current->left;

			if(current->parent)
			{
				if(current->parent->left == current)
					current->parent->left = temp;
				else if(current->parent->right == current)
					current->parent->right = temp;
				else
					current->parent->middle = temp;
				temp->parent = current->parent;
			}

			if(current->right)
			{
				while(temp->right)
					temp = temp->right;

				temp->right = current->right;
				current->right->parent = temp;
			}
		}
		else if(current->right)
		{
			char_tst_node * temp = current->right;

			if(current->parent)
			{
				if(current->parent->left == current)
					current->parent->left = temp;
				else if(current->parent->right == current)
					current->parent->right = temp;
				else
					current->parent->middle = temp;
				temp->parent = current->parent;
			}

			if(current->left)
			{
				while(temp->left)
					temp = temp->left;

				temp->left = current->left;
				current->left->parent = temp;
			}
		}
		else if(current->parent) 
		{
			if(current->parent->left == current)
				current->parent->left = NULL;				
			else if(current->parent->right == current)
				current->parent->right = NULL;				
			else
				current->parent->middle = NULL;
		}

		if(NULL == current->parent)
		{
			if(current->left)
				tst_tree->root = current->left;
			else if(current->right)
				tst_tree->root = current->right;
			else if(current->middle)
				tst_tree->root = NULL;

			if(tst_tree->root)
				tst_tree->root->parent = NULL;
		}

		if(current == tst_tree->root)
			tst_tree->root = NULL;

		string_set_inter_destroy_node(tst_tree, current);

		break;
	}

	return 0;
}



















