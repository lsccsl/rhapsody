/**
 *
 * @file __avl_tree.c avlƽ����
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "__avl_tree.h"

#include <assert.h>
#include <stdlib.h>


/**
 * @brief �����ڵ�
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
 * @brief ��node��ʼ����,ƽ����Ӱ���·��,�������ƽ�������Ҹ��ڵ�û�г���,ֹͣ����
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
				* ����߶��ޱ仯,���ݵ�������
				* ����߶ȳ��ֱ仯,��Ӧ����
				*/

				size_t new_height = left_height + 1;

				/*
				* �߶ȿ��Բ���
				* ���߳���һ��,���߱䰫һ��
				* ��Ӧ�����������
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

				/* �����߼�,��ʱ��߲�����Ϊ�� */
				assert(node->base.left);

				left_lh = __avltree_left_height((__avltree_node_t *)(node->base.left));
				left_rh = __avltree_right_height((__avltree_node_t *)(node->base.left));

				/* ��֧������avl���� */
				assert(((left_lh + 1) == left_rh) || ((left_rh + 1) == left_lh) || left_lh == left_rh);

				/*
				* �������������������߳���������
				* ��������״�Ĳ�ͬ,����Ӧ����ת,������parent
				*/
				if(left_lh < left_rh)
				{
					/*
					* ���3: ���������ҷ�֧�����֧��1�� (����ת��Ϊ���1)
					*      node               node
					*      / \                / \
					*     L   R              C   R
					*    /\        -->      / \  
					*   A  C               L   D 
					*     / \             / \    
					*    B   D           A   B
					*/

					/* �����߼�,����������Ȼ���� */
					assert(node->base.left && node->base.left->right);

					/* ��һ����ת,���ת������ */
					__rotate_left((bstree_node_t **)root, node->base.left->right);

					/*
					* ���¼���L��C�ĸ߶� ��ͼ����A,B,D������֧�ǲ�����תӰ���
					* L�ĸ߶ȼ�1
					* C�ĸ߶ȼ�1
					*/
					assert(node->base.left);
					assert(node->base.left->left);

					((__avltree_node_t *)(node->base.left->left))->height --;
					((__avltree_node_t *)(node->base.left))->height ++;

					assert(__avltree_left_height((__avltree_node_t *)(node->base.left->left)) + 1 == ((__avltree_node_t *)(node->base.left->left))->height ||
						__avltree_right_height((__avltree_node_t *)(node->base.left->left)) + 1 == ((__avltree_node_t *)(node->base.left->left))->height);
					assert(__avltree_left_height(((__avltree_node_t *)(node->base.left))) + 1 == ((__avltree_node_t *)(node->base.left))->height ||
						__avltree_right_height(((__avltree_node_t *)(node->base.left))) + 1 == ((__avltree_node_t *)(node->base.left))->height);

					/* ���¼���left_lh��left_rh */
					left_lh = __avltree_left_height((__avltree_node_t *)node->base.left);
					left_rh = __avltree_right_height((__avltree_node_t *)node->base.left);
				}

				assert(left_lh >= left_rh);

				/*
				* ���1:�����������֧���ҷ�֧��1��
				*      node               L           
				*      / \               / \
				*     L   R             A   node
				*    /\        -->     / \  / \
				*   A  C              B   D C  R
				*  / \
				* B   D
				*/

				/*
				* ���2: �����������ҷ�֧һ���� ��ͬ���1 ע��L��ʱ������һ��
				*      node                  L
				*      / \                  /  \
				*     L   R                A     node
				*    / \        -->       / \    /  \
				*   A   C                B   D  C    R
				*  / \ / \                     / \
				* B  D E  F                   E   F
				*/

				/* ��һ����ת������ɹ��� */
				__rotate_right((bstree_node_t **)root, node->base.left);

				/* ��ʱnode����ָ��L */
				node = (__avltree_node_t *)node->base.parent;

				assert(node && node->base.right);

				((__avltree_node_t *)(node->base.right))->height = __avltree_cal_height((__avltree_node_t *)node->base.right);
				//if(left_lh == left_rh)
				node->height = __avltree_cal_height(node);
				//else

				/* �����߼��ó�,�������س��� */
				assert(node->height == (__avltree_left_height(node) + 1) ||
					node->height == (__avltree_right_height(node) + 1) );

				assert(__avltree_left_height((__avltree_node_t *)(node->base.right)) + 1 == ((__avltree_node_t *)(node->base.right))->height ||
					__avltree_right_height((__avltree_node_t *)(node->base.right)) + 1 == ((__avltree_node_t *)(node->base.right))->height);
			}
		}
		else/* left_height > right_height �ԳƵ�һ����� */
		{
			if(right_height - left_height <= 1)
			{
				/*
				* ����߶��ޱ仯,���ݵ�������
				* ����߶ȳ��ֱ仯,��Ӧ����
				*/

				size_t new_height = right_height + 1;

				/*
				* �߶ȿ��Բ���
				* ���߳���һ��,���߱䰫һ��
				* ��Ӧ�����������
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

				/* �����߼�,��ʱ��߲�����Ϊ�� */
				assert(node->base.right);

				right_lh = __avltree_left_height((__avltree_node_t *)node->base.right);
				right_rh = __avltree_right_height((__avltree_node_t *)node->base.right);

				/* ��֧������avl���� */
				assert(((right_lh + 1) == right_rh) || ((right_rh + 1) == right_lh) || right_lh == right_rh);

				/*
				* �������������������߳���������
				* ��������״�Ĳ�ͬ,����Ӧ����ת,������parent
				*/
				if(right_rh < right_lh)
				{
					/*
					* ���3: �����������֧���ҷ�֧��1�� (����ת��Ϊ���1)
					*      node               node
					*      / \                / \
					*     L   R              L   A
					*         /\        -->     / \  
					*        A  C               B  R 
					*       / \                   / \    
					*      B   D                 D   C
					*/

					/* �����߼�,����������Ȼ���� */
					assert(node->base.right && node->base.right->left);

					/* ��һ����ת,���ת������ */
					__rotate_right((bstree_node_t **)root, node->base.right->left);

					/*
					* ���¼���L��C�ĸ߶� ��ͼ����C,B,D������֧�ǲ�����תӰ���
					* R�ĸ߶ȼ�1
					* A�ĸ߶ȼ�1
					*/
					assert(node->base.right);
					assert(node->base.right->right);

					((__avltree_node_t *)(node->base.right->right))->height --;
					((__avltree_node_t *)(node->base.right))->height ++;

					assert(__avltree_left_height((__avltree_node_t *)(node->base.right->right)) + 1 == ((__avltree_node_t *)(node->base.right->right))->height ||
						__avltree_right_height((__avltree_node_t *)(node->base.right->right)) + 1 == ((__avltree_node_t *)(node->base.right->right))->height);
					assert(__avltree_left_height(((__avltree_node_t *)(node->base.right))) + 1 == ((__avltree_node_t *)(node->base.right))->height ||
						__avltree_right_height(((__avltree_node_t *)(node->base.right))) + 1 == ((__avltree_node_t *)(node->base.right))->height);

					/* ���¼���left_lh��left_rh */
					right_lh = __avltree_left_height((__avltree_node_t *)node->base.right);
					right_rh = __avltree_right_height((__avltree_node_t *)node->base.right);
				}

				assert(right_rh >= right_lh);

				/*
				* ���1:�����������֧���ҷ�֧��1��
				*      node                     R           
				*      / \                     /  \
				*     L   R                node    C
				*         /\        -->     / \   / \
				*        A  C              L   A  B  D
				*          / \
				*         B   D
				*/

				/*
				* ���2: �����������ҷ�֧һ���� ��ͬ���1 ע��R��ʱ������һ��
				*      node                      R
				*      / \                     /  \
				*     L   R                node     C
				*        / \        -->   / \      /  \
				*       A   C            L   A     E   F
				*      / \ / \              / \
				*     B  D E  F            B   D
				*/

				/* ��һ����ת������ɹ��� */
				__rotate_left((bstree_node_t **)root, node->base.right);

				/* ��ʱnode����ָ��R */
				node = (__avltree_node_t *)node->base.parent;

				assert(node && node->base.left);

				((__avltree_node_t *)(node->base.left))->height = __avltree_cal_height((__avltree_node_t *)node->base.left);
				//if(right_lh == right_rh)
				node->height = __avltree_cal_height(node);
				//else

				/* �����߼��ó�,�������س��� */
				assert(node->height == (__avltree_left_height(node) + 1) ||
					node->height == (__avltree_right_height(node) + 1) );

				assert(__avltree_left_height((__avltree_node_t *)(node->base.left)) + 1 == ((__avltree_node_t *)(node->base.left))->height ||
					__avltree_right_height((__avltree_node_t *)(node->base.left)) + 1 == ((__avltree_node_t *)(node->base.left))->height);
			}
		}

		/*
		* ���¼���ڵ�ĸ߶�,ָ�����
		*/
		node = (__avltree_node_t *)node->base.parent;
	}
}

/**
 * @brief ɾ��ָ���ڵ�node
 */
extern void __avltree_del(__avltree_node_t ** root, __avltree_node_t * node)
{
	/* ����ƽ�����ʼ�ڵ� */
	__avltree_node_t * node_balance = node;

	/* Ҫ���ٵĽڵ� */
	__avltree_node_t * node_replace = node;

	assert(node && root);
	assert(*root);

	/*
	* ���ɾ������"�ڲ��ڵ�",ͨ��Ѱ�Ҷ�Ӧ����ڵ�,ת��Ϊɾ��"�ⲿ�ڵ�"
	*
	*      root
	*      /  \
	*     ?    del_node --> Ҫɾ���Ľڵ�
	*          / \
	*         ?   ?
	*        / \
	*       ?   x ---> ����ڵ�
	*
	* ɾ��del_node����ڵ�,���տ���ת��Ϊɾ��x�ڵ�
	*/

	if(node->base.left && node->base.right)
	{
		node_replace = (__avltree_node_t *)node->base.left;

		/*
		* Ѱ������������"��"�ߵĽڵ�,��Ϊ����ڵ�
		*/
		while(node_replace->base.right)
			node_replace = (__avltree_node_t *)(node_replace->base.right);
	}

	if(node_replace->base.parent)
		node_balance = (__avltree_node_t *)node_replace->base.parent;
	else
	{
		/*
		* ˵��node�Ǹ��ڵ�
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
	* �����Ҫ������ڵ�
	*/
	if(node_replace != node)
	{
		/*
		* node�ĸ��ڵ�ָ��node_replace
		*/
		if(node->base.parent)
		{
			if((bstree_node_t *)node == node->base.parent->left)
				node->base.parent->left = (bstree_node_t *)node_replace;
			else
				node->base.parent->right = (bstree_node_t *)node_replace;
		}
		else/* ˵��Ҫɾ�����Ǹ��ڵ� */
		{
			*root = node_replace;
		}

		/*
		* node���ӵĸ��ڵ�ָ��ָ��ָ��node_replace(���node_replace����node������)
		*/
		assert(node->base.left);
		if(node_replace != (__avltree_node_t *)node->base.left)
			node->base.left->parent = (bstree_node_t *)node_replace;
		else
			node_balance = node_replace;

		/*
		* node�Һ��ӵĸ��ڵ�ָ��ָ��node_replace
		*/
		assert(node->base.right && node_replace != (__avltree_node_t *)node->base.right);
		node->base.right->parent = (bstree_node_t *)node_replace;

		/*
		* ���node_replace�����Ӵ���,����ָ��node_replace->base.parent(��node_balance)
		*/
		if(node_replace->base.left)
			node_replace->base.left->parent = (bstree_node_t *)node_balance;

		/*
		* node_replace�����ӹҵ�node_replace�ĸ��ڵ���Ӧ��λ��
		*/
		assert(NULL == node_replace->base.right);
		assert(node_replace->base.parent);
		if((bstree_node_t *)node_replace == node_replace->base.parent->left)
			node_replace->base.parent->left = node_replace->base.left;
		else
			node_replace->base.parent->right = node_replace->base.left;

		/*
		* ��node_replace�����Һ����Լ����ڵ����¸�ֵ
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
			else/* ˵��Ҫɾ�����Ǹ��ڵ� */
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
			else/* ˵��Ҫɾ�����Ǹ��ڵ� */
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





























