/**
 *
 * @file __avl_tree.c avl平衡树
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "__avl_tree.h"

#include <assert.h>
#include <stdlib.h>


/**
 * @brief 创建节点
 */
size_t __avltree_cal_height(__avltree_node_t * node)
{
	assert(node);

	{
		size_t l = __avltree_left_height(node);
		size_t r = __avltree_right_height(node);

		assert(l == r || (l + 1)== r || (r + 1) == l);

		return (l > r)?(l + 1):(r + 1);
	}
}

/**
 * @brief 从node开始回溯,平衡受影响的路径,如果满足平衡条件且父节点没有长高,停止回溯
 */
void __avltree_balance(__avltree_node_t ** root, __avltree_node_t * node)
{
	assert(root && node);
	assert(*root);

	while(node)
	{
		size_t left_height = __avltree_left_height(node);
		size_t right_height = __avltree_right_height(node);

		if(left_height > right_height)
		{
			if(left_height - right_height <= 1)
			{
				/*
				* 如果高度无变化,回溯调整结束
				* 如果高度出现变化,则应继续
				*/

				size_t new_height = left_height + 1;

				/*
				* 高度可以不变
				* 或者长高一层,或者变矮一层
				* 不应出现其它情况
				*/
				assert(node->height == new_height || 1 + new_height == node->height ||
					new_height - 1 == node->height);


				if(new_height == node->height)
				{
					node->height = new_height;
					break;
				}
				else 
					node->height = new_height;
			}
			else
			{
				size_t left_lh = 0;
				size_t left_rh = 0;

				assert(left_height - right_height == 2);

				/* 根据逻辑,此时左边不可能为空 */
				assert(node->base.left);

				left_lh = __avltree_left_height((__avltree_node_t *)(node->base.left));
				left_rh = __avltree_right_height((__avltree_node_t *)(node->base.left));

				/* 分支必满足avl规则 */
				assert(((left_lh + 1) == left_rh) || ((left_rh + 1) == left_lh) || left_lh == left_rh);

				/*
				* 处理左子树比右子树高出两层的情况
				* 根据树形状的不同,做相应的旋转,并更改parent
				*/
				if(left_lh < left_rh)
				{
					/*
					* 情况3: 左子树的右分支比左分支高1层 (可以转化为情况1)
					*      node               node
					*      / \                / \
					*     L   R              C   R
					*    /\        -->      / \  
					*   A  C               L   D 
					*     / \             / \    
					*    B   D           A   B
					*/

					/* 根据逻辑,以下条件必然成立 */
					assert(node->base.left && node->base.left->right);

					/* 作一次旋转,完成转化工作 */
					__rotate_left((bstree_node_t **)root, node->base.left->right);

					/*
					* 重新计算L与C的高度 从图看出A,B,D这三分支是不受旋转影响的
					* L的高度减1
					* C的高度加1
					*/
					assert(node->base.left);
					assert(node->base.left->left);

					((__avltree_node_t *)(node->base.left->left))->height --;
					((__avltree_node_t *)(node->base.left))->height ++;

					assert(__avltree_left_height((__avltree_node_t *)(node->base.left->left)) + 1 == ((__avltree_node_t *)(node->base.left->left))->height ||
						__avltree_right_height((__avltree_node_t *)(node->base.left->left)) + 1 == ((__avltree_node_t *)(node->base.left->left))->height);
					assert(__avltree_left_height(((__avltree_node_t *)(node->base.left))) + 1 == ((__avltree_node_t *)(node->base.left))->height ||
						__avltree_right_height(((__avltree_node_t *)(node->base.left))) + 1 == ((__avltree_node_t *)(node->base.left))->height);

					/* 重新计算left_lh与left_rh */
					left_lh = __avltree_left_height((__avltree_node_t *)node->base.left);
					left_rh = __avltree_right_height((__avltree_node_t *)node->base.left);
				}

				assert(left_lh >= left_rh);

				/*
				* 情况1:左子树的左分支比右分支高1层
				*      node               L           
				*      / \               / \
				*     L   R             A   node
				*    /\        -->     / \  / \
				*   A  C              B   D C  R
				*  / \
				* B   D
				*/

				/*
				* 情况2: 左子树的左右分支一样高 视同情况1 注意L此时长高了一层
				*      node                  L
				*      / \                  /  \
				*     L   R                A     node
				*    / \        -->       / \    /  \
				*   A   C                B   D  C    R
				*  / \ / \                     / \
				* B  D E  F                   E   F
				*/

				/* 做一次旋转即可完成工作 */
				__rotate_right((bstree_node_t **)root, node->base.left);

				/* 此时node重新指向L */
				node = (__avltree_node_t *)node->base.parent;

				assert(node && node->base.right);

				((__avltree_node_t *)(node->base.right))->height = __avltree_cal_height((__avltree_node_t *)node->base.right);
				//if(left_lh == left_rh)
				node->height = __avltree_cal_height(node);
				//else

				/* 根据逻辑得出,此条件必成立 */
				assert(node->height == (__avltree_left_height(node) + 1) ||
					node->height == (__avltree_right_height(node) + 1) );

				assert(__avltree_left_height((__avltree_node_t *)(node->base.right)) + 1 == ((__avltree_node_t *)(node->base.right))->height ||
					__avltree_right_height((__avltree_node_t *)(node->base.right)) + 1 == ((__avltree_node_t *)(node->base.right))->height);
			}
		}
		else/* left_height > right_height 对称的一种情况 */
		{
			if(right_height - left_height <= 1)
			{
				/*
				* 如果高度无变化,回溯调整结束
				* 如果高度出现变化,则应继续
				*/

				size_t new_height = right_height + 1;

				/*
				* 高度可以不变
				* 或者长高一层,或者变矮一层
				* 不应出现其它情况
				*/
				assert(node->height == new_height || 1 + new_height == node->height ||
					new_height - 1 == node->height);


				if(new_height == node->height)
				{
					node->height = new_height;
					break;
				}
				else
					node->height = new_height;
			}
			else
			{
				size_t right_lh = 0;
				size_t right_rh = 0;

				assert(right_height - left_height == 2);

				/* 根据逻辑,此时左边不可能为空 */
				assert(node->base.right);

				right_lh = __avltree_left_height((__avltree_node_t *)node->base.right);
				right_rh = __avltree_right_height((__avltree_node_t *)node->base.right);

				/* 分支必满足avl规则 */
				assert(((right_lh + 1) == right_rh) || ((right_rh + 1) == right_lh) || right_lh == right_rh);

				/*
				* 处理左子树比右子树高出两层的情况
				* 根据树形状的不同,做相应的旋转,并更改parent
				*/
				if(right_rh < right_lh)
				{
					/*
					* 情况3: 右子树的左分支比右分支高1层 (可以转化为情况1)
					*      node               node
					*      / \                / \
					*     L   R              L   A
					*         /\        -->     / \  
					*        A  C               B  R 
					*       / \                   / \    
					*      B   D                 D   C
					*/

					/* 根据逻辑,以下条件必然成立 */
					assert(node->base.right && node->base.right->left);

					/* 作一次旋转,完成转化工作 */
					__rotate_right((bstree_node_t **)root, node->base.right->left);

					/*
					* 重新计算L与C的高度 从图看出C,B,D这三分支是不受旋转影响的
					* R的高度减1
					* A的高度加1
					*/
					assert(node->base.right);
					assert(node->base.right->right);

					((__avltree_node_t *)(node->base.right->right))->height --;
					((__avltree_node_t *)(node->base.right))->height ++;

					assert(__avltree_left_height((__avltree_node_t *)(node->base.right->right)) + 1 == ((__avltree_node_t *)(node->base.right->right))->height ||
						__avltree_right_height((__avltree_node_t *)(node->base.right->right)) + 1 == ((__avltree_node_t *)(node->base.right->right))->height);
					assert(__avltree_left_height(((__avltree_node_t *)(node->base.right))) + 1 == ((__avltree_node_t *)(node->base.right))->height ||
						__avltree_right_height(((__avltree_node_t *)(node->base.right))) + 1 == ((__avltree_node_t *)(node->base.right))->height);

					/* 重新计算left_lh与left_rh */
					right_lh = __avltree_left_height((__avltree_node_t *)node->base.right);
					right_rh = __avltree_right_height((__avltree_node_t *)node->base.right);
				}

				assert(right_rh >= right_lh);

				/*
				* 情况1:左子树的左分支比右分支高1层
				*      node                     R           
				*      / \                     /  \
				*     L   R                node    C
				*         /\        -->     / \   / \
				*        A  C              L   A  B  D
				*          / \
				*         B   D
				*/

				/*
				* 情况2: 左子树的左右分支一样高 视同情况1 注意R此时长高了一层
				*      node                      R
				*      / \                     /  \
				*     L   R                node     C
				*        / \        -->   / \      /  \
				*       A   C            L   A     E   F
				*      / \ / \              / \
				*     B  D E  F            B   D
				*/

				/* 做一次旋转即可完成工作 */
				__rotate_left((bstree_node_t **)root, node->base.right);

				/* 此时node重新指向R */
				node = (__avltree_node_t *)node->base.parent;

				assert(node && node->base.left);

				((__avltree_node_t *)(node->base.left))->height = __avltree_cal_height((__avltree_node_t *)node->base.left);
				//if(right_lh == right_rh)
				node->height = __avltree_cal_height(node);
				//else

				/* 根据逻辑得出,此条件必成立 */
				assert(node->height == (__avltree_left_height(node) + 1) ||
					node->height == (__avltree_right_height(node) + 1) );

				assert(__avltree_left_height((__avltree_node_t *)(node->base.left)) + 1 == ((__avltree_node_t *)(node->base.left))->height ||
					__avltree_right_height((__avltree_node_t *)(node->base.left)) + 1 == ((__avltree_node_t *)(node->base.left))->height);
			}
		}

		/*
		* 重新计算节点的高度,指针回溯
		*/
		node = (__avltree_node_t *)node->base.parent;
	}
}

/**
 * @brief 删除指定节点node
 */
extern void __avltree_del(__avltree_node_t ** root, __avltree_node_t * node)
{
	/* 回溯平衡的起始节点 */
	__avltree_node_t * node_balance = node;

	/* 要销毁的节点 */
	__avltree_node_t * node_replace = node;

	assert(node && root);
	assert(*root);

	/*
	* 如果删除的是"内部节点",通过寻找对应替代节点,转化为删除"外部节点"
	*
	*      root
	*      /  \
	*     ?    del_node --> 要删除的节点
	*          / \
	*         ?   ?
	*        / \
	*       ?   x ---> 替代节点
	*
	* 删除del_node这个节点,最终可以转化为删除x节点
	*/

	if(node->base.left && node->base.right)
	{
		node_replace = (__avltree_node_t *)node->base.left;

		/*
		* 寻找左子树的最"右"边的节点,作为替代节点
		*/
		while(node_replace->base.right)
			node_replace = (__avltree_node_t *)(node_replace->base.right);
	}

	if(node_replace->base.parent)
		node_balance = (__avltree_node_t *)node_replace->base.parent;
	else
	{
		/*
		* 说明node是根节点
		*/
		assert(node == *root);
		assert(NULL == node->base.left || NULL == node->base.right);

		if(node->base.left)
			*root = (__avltree_node_t *)node->base.left;
		else
			*root = (__avltree_node_t *)node->base.right;

		if(*root)
			(*root)->base.parent = NULL;
		return;
	}

	/*
	* 如果需要用替代节点
	*/
	if(node_replace != node)
	{
		/*
		* node的父节点指向node_replace
		*/
		if(node->base.parent)
		{
			if((bstree_node_t *)node == node->base.parent->left)
				node->base.parent->left = (bstree_node_t *)node_replace;
			else
				node->base.parent->right = (bstree_node_t *)node_replace;
		}
		else/* 说明要删除的是根节点 */
		{
			*root = node_replace;
		}

		/*
		* node左孩子的父节点指针指向指向node_replace(如果node_replace不是node的左孩子)
		*/
		assert(node->base.left);
		if(node_replace != (__avltree_node_t *)node->base.left)
			node->base.left->parent = (bstree_node_t *)node_replace;
		else
			node_balance = node_replace;

		/*
		* node右孩子的父节点指向指向node_replace
		*/
		assert(node->base.right && node_replace != (__avltree_node_t *)node->base.right);
		node->base.right->parent = (bstree_node_t *)node_replace;

		/*
		* 如果node_replace的左孩子存在,令它指向node_replace->base.parent(即node_balance)
		*/
		if(node_replace->base.left)
			node_replace->base.left->parent = (bstree_node_t *)node_balance;

		/*
		* node_replace的左孩子挂到node_replace的父节点相应的位置
		*/
		assert(NULL == node_replace->base.right);
		assert(node_replace->base.parent);
		if((bstree_node_t *)node_replace == node_replace->base.parent->left)
			node_replace->base.parent->left = node_replace->base.left;
		else
			node_replace->base.parent->right = node_replace->base.left;

		/*
		* 给node_replace的左右孩子以及父节点重新赋值
		*/
		node_replace->base.left = node->base.left;
		node_replace->base.right = node->base.right;
		node_replace->base.parent = node->base.parent;

		node_replace->height = node->height;
	}
	else
	{
		if(node->base.left)
		{
			if(node->base.parent)
			{
				if((bstree_node_t *)node == node->base.parent->left)
					node->base.parent->left = node->base.left;
				else
					node->base.parent->right = node->base.left;
			}
			else/* 说明要删除的是根节点 */
			{
				*root = (__avltree_node_t *)node->base.left;
			}

			node->base.left->parent = node->base.parent;
		}
		else if(node->base.right)
		{
			if(node->base.parent)
			{
				if((bstree_node_t *)node == node->base.parent->left)
					node->base.parent->left = node->base.right;
				else
					node->base.parent->right = node->base.right;
			}
			else/* 说明要删除的是根节点 */
			{
				*root = (__avltree_node_t *)node->base.right;
			}

			node->base.right->parent = node->base.parent;
		}
		else if(node->base.parent)
		{
			if((bstree_node_t *)node == node->base.parent->left)
				node->base.parent->left = NULL;
			else
				node->base.parent->right = NULL;
		}
	}

	assert(node_balance != node);
	assert(node_balance);

	__avltree_balance(root, node_balance);
}





























