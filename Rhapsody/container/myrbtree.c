/*
* 
* myrbtree.c 2007-3-28 23:02:55 红黑树
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/

/*
//红黑树
//         红黑树是一种自平衡二叉查找树，是在计算机科学中用到的一种数据结构，典型的用途是实现关联数组。它是在1972年由Rudolf Bayer发明的，
他称之为"对称二叉B树"，它现代的名字是在 Leo J. Guibas 和 Robert Sedgewick 于1978年写的一篇论文中获得的。它是复杂的，
但它的操作有着良好的最坏情况运行时间，并且在实践中是高效的: 它可以在O(log n)时间内做查找，插入和删除，这里的n 是树中元素的数目。
//
//红黑树是一种很有意思的平衡检索树。它的统计性能要好于平衡二叉树(有些书籍根据作者姓名，Adelson-Velskii和Landis，将其称为AVL-树)，
因此，红黑树在很多地方都有应用。在C++ STL中，很多部分(目前包括set, multiset, map, multimap)应用了红黑树的变体(SGI STL中的红黑树有一些变化，
这些修改提供了更好的性能，以及对set操作的支持)。 
//
//
//背景和术语
//
//        红黑树是一种特定类型的二叉树，它是在计算机科学中用来组织数据比如数字的块的一种结构。所有数据块都存储在节点中。
这些节点中的某一个节点总是担当启始位置的功能，它不是任何节点的儿子；我们称之为根节点或根。它有最多两个"儿子"，
都是它连接到的其他节点。所有这些儿子都可以有自己的儿子，以此类推。这样根节点就有了把它连接到在树中任何其他节点的路径。
//
//        如果一个节点没有儿子，我们称之为叶子节点，因为在直觉上它是在树的边缘上。子树是从特定节点可以延伸到的树的某一部分，其自身被当作一个树。
在红黑树中，叶子被假定为 null 或空。
//
//        由于红黑树也是二叉查找树，它们当中每一个节点都的比较值都必须大于或等于在它的左子树中的所有节点，并且小于或等于在它的右子树中的所有节点。
这确保红黑树运作时能够快速的在树中查找给定的值。
//
//用途和好处
//
//        红黑树和AVL树一样都对插入时间、删除时间和查找时间提供了最好可能的最坏情况担保。
这不只是使它们在时间敏感的应用如即时应用(real time application)中有价值，
而且使它们有在提供最坏情况担保的其他数据结构中作为建造板块的价值；例如，在计算几何中使用的很多数据结构都可以基于红黑树。
//
//        红黑树在函数式编程中也特别有用，在这里它们是最常用的持久数据结构之一，它们用来构造关联数组和集合，
在突变之后它们能保持为以前的版本。除了O(log n)的时间之外，红黑树的持久版本对每次插入或删除需要O(log n)的空间。
//
//        红黑树是 2-3-4树的一种等同。换句话说，对于每个 2-3-4 树，都存在至少一个数据元素是同样次序的红黑树。
在 2-3-4 树上的插入和删除操作也等同于在红黑树中颜色翻转和旋转。这使得 2-3-4 树成为理解红黑树背后的逻辑的重要工具，
这也是很多介绍算法的教科书在红黑树之前介绍 2-3-4 树的原因，尽管 2-3-4 树在实践中不经常使用。
//
//属性
//
//        红黑树是每个节点都有颜色特性的二叉查找树，颜色的值是红色或黑色之一。除了二叉查找树带有的一般要求，
我们对任何有效的红黑树加以如下增补要求:
//
//        1.节点是红色或黑色。 
//
//        2.根是黑色。 
//
//        3.所有叶子（外部节点）都是黑色。 
//
//        4.每个红色节点的两个子节点都是黑色。(从每个叶子到根的所有路径上不能有两个连续的红色节点) 
//
//        5.从每个叶子到根的所有路径都包含相同数目的黑色节点。 
//
//        这些约束强制了红黑树的关键属性: 从根到叶子的最长的可能路径不多于最短的可能路径的两倍长。结果是这个树大致上是平衡的。
因为操作比如插入、删除和查找某个值都要求与树的高度成比例的最坏情况时间，这个在高度上的理论上限允许红黑树在最坏情况下都是高效的，
而不同于普通的二叉查找树。
//
//        要知道为什么这些特性确保了这个结果，注意到属性4导致了路径不能有两个毗连的红色节点就足够了。
最短的可能路径都是黑色节点，最长的可能路径有交替的红色和黑色节点。因为根据属性5所有最长的路径都有相同数目的黑色节点，
这就表明了没有路径能多于任何其他路径的两倍长。
//
//        在很多树数据结构的表示中，一个节点有可能只有一个儿子，而叶子节点包含数据。用这种范例表示红黑树是可能的，
但是这会改变一些属性并使算法复杂。为此，本文中我们使用 "nil 叶子" 或"空(null)叶子"，如上图所示，它不包含数据而只充当树在此结束的指示。
这些节点在绘图中经常被省略，导致了这些树好象同上述原则相矛盾，而实际上不是这样。与此有关的结论是所有节点都有两个儿子，
尽管其中的一个或两个可能是空叶子。
//
//操作
//
//        在红黑树上只读操作不需要对用于二叉查找树的操作做出修改，因为它也二叉查找树。
但是，在插入和删除之后，红黑属性可能变得违规。
恢复红黑属性需要少量(O(log n))的颜色变更(这在实践中是非常快速的)并且不超过三次树旋转(对于插入是两次)。
这允许插入和删除保持为 O(log n) 次，但是它导致了非常复杂的操作。 
*/

#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include "myutility.h"
#include "myrbtree.h"


typedef enum __rbtree_colour
{
	rbtree_colour_black,
	rbtree_colour_red,
}rbtree_colour;

typedef struct __myrbtree_node_t
{
	struct __myrbtree_node_t * left;
	struct __myrbtree_node_t * right;
	struct __myrbtree_node_t * parent;

	rbtree_colour colour;

	void * key;
	void * data;
}myrbtree_node_t;

typedef struct __myrbtree_t
{
	myrbtree_node_t * root;

	//内存池
	HMYMEMPOOL hm;

	//比较运算符
	myrbtree_compare compare;
}myrbtree_t;

/*
*
*1 表示 key1 比 key2 大
*0 表示 key1 比 key2 小 
*
*/
static __INLINE__ int rbtree_inter_compare(myrbtree_t * rbtree, const void * key1, const void * key2)
{
	assert(rbtree && rbtree->compare);

	return (*rbtree->compare)(key1, key2);
}

static __INLINE__ myrbtree_node_t * rbtree_inter_create_node(myrbtree_t * rbtree, const void * key, const void * data)
{
	myrbtree_node_t * node_new = NULL;

	assert(rbtree);

	node_new = (myrbtree_node_t *)MyMemPoolMalloc(rbtree->hm, sizeof(*node_new));

	if(NULL == node_new)
		return NULL;

	memset(node_new, 0, sizeof(*node_new));
	node_new->key = (void *)key;
	node_new->data = (void *)data;

	return node_new;
}

static __INLINE__ void rbtree_inter_destroy_node(myrbtree_t * rbtree, myrbtree_node_t * node)
{
	assert(rbtree && node);

	MyMemPoolFree(rbtree->hm, node);
}

/*
*
*左旋
*
*   A                node
*    \              /
*     \    ---->   /
*      node       A
*/
static __INLINE__ void rbtree_inter_rotate_left(/*myrbtree_t * rbtree*/myrbtree_node_t ** root, myrbtree_node_t * node)
{
	myrbtree_node_t * A_node = NULL;

	assert(root && node && node->parent);

	A_node = node->parent;

	node->parent = A_node->parent;
	if(A_node->parent)
	{
		if(A_node == A_node->parent->left)
			A_node->parent->left = node;
		else
			A_node->parent->right = node;
	}
	A_node->parent = node;

	A_node->right = node->left;
	if(node->left)
		node->left->parent = A_node;

	node->left = A_node;

	if(A_node == *root)
		*root = node;

	assert(NULL == *root || NULL == (*root)->parent);
}

/*
*
*右旋
*
*     A        node
*    /          \
*   /    --->    \
*  node           A
*/
static __INLINE__ void rbtree_inter_rotate_right(/*myrbtree_t * rbtree*/myrbtree_node_t ** root, myrbtree_node_t * node)
{
	myrbtree_node_t * A_node = NULL;

	assert(root && node && node->parent);

	A_node = node->parent;
	node->parent = A_node->parent;
	if(A_node->parent)
	{
		if(A_node == A_node->parent->left)
			A_node->parent->left = node;
		else
			A_node->parent->right = node;
	}
	A_node->parent = node;

	A_node->left = node->right;
	if(node->right)
		node->right->parent = A_node;

	node->right = A_node;

	if(A_node == *root)
		*root = node;

	assert(NULL == (*root) || NULL == (*root)->parent);
}

static __INLINE__ myrbtree_node_t * rbtree_inter_search(myrbtree_t * rbtree, myrbtree_node_t * root, const void * key)
{
	myrbtree_node_t * y = NULL;/* 记录最后一个不大于key的节点 */
	myrbtree_node_t * x = root;

	assert(rbtree);

	while(x)
	{
		if(!rbtree_inter_compare(rbtree, x->key, key))
			y = x, x = x->right;
		else
			x = x->left;
	}

	return (NULL == y || rbtree_inter_compare(rbtree, key, y->key))?NULL:y;
}

static __INLINE__ myrbtree_node_t * rbtree_inter_searchex(myrbtree_t * rbtree, myrbtree_node_t * root, const void * key, myrbtree_node_t ** parent)
{
	myrbtree_node_t * y = NULL;/* 记录最后一个不大于key的节点 */
	myrbtree_node_t * x = root;

	assert(rbtree && parent /*&& rbtree->root == root*/);

	*parent = root;

	while(x)
	{
		*parent = x;

		if(!rbtree_inter_compare(rbtree, x->key, key))
			y = x, x = x->right;
		else
			x = x->left;
	}

	return (NULL == y || rbtree_inter_compare(rbtree, key, y->key))?NULL:y;
}

static __INLINE__ int rbtree_inter_ismynode(const myrbtree_node_t * root, const myrbtree_node_t * node)
{
	int ret = 0;

	if(NULL == root)
		return 0;

	if(node == root)
		return 1;

	if(root->left)
		ret = rbtree_inter_ismynode(root->left, node);
	if(ret)
		return ret;

	if(root->right)
		return rbtree_inter_ismynode(root->right, node);

	return 0;
}

/*
*
*旋转红黑树，使之符合红黑树的规则
*rbtree:需要旋转的红黑树
*node:新加入的节点
*
*/
static __INLINE__ void rbtree_inter_rebalance(/*myrbtree_t * rbtree*/myrbtree_node_t ** root, myrbtree_node_t * node)
{
	assert(root && node && node->parent);

	//新加入节点必为红
	node->colour = rbtree_colour_red;

	//如果父节点为根节点,根据红黑树的定义根节点必为黑
	if(node->parent == *root)
		return;

	//不为根结点，祖父节点必存在
	assert(node->parent->parent);

	//如果父节点不为黑
	while(node != *root && rbtree_colour_red == node->parent->colour)
	{
		//如果父节点是祖父节点的左孩子
		if(node->parent == node->parent->parent->left)
		{
			//如果伯父节点存在，且为红
			if(node->parent->parent->right && rbtree_colour_red == node->parent->parent->right->colour)
			{
				//把父节点与伯父节点涂成黑色
				node->parent->colour = rbtree_colour_black;
				node->parent->parent->right->colour = rbtree_colour_black;

				//把祖父结点涂成红色
				node->parent->parent->colour = rbtree_colour_red;

				//指针往上走
				node = node->parent->parent;
			}
			else
			{
				//如果是外侧插入
				if(node == node->parent->left)
				{
					//node为红
					node->colour = rbtree_colour_red;

					//父节点为黑
					node->parent->colour = rbtree_colour_black;

					//祖父节点为红
					node->parent->parent->colour = rbtree_colour_red;

					//父节点为轴右旋转
					rbtree_inter_rotate_right(root, node->parent);
				}
				else
				{
					myrbtree_node_t * temp = node->parent;

					//node为黑
					node->colour = rbtree_colour_black;

					//父节点为红
					node->parent->colour = rbtree_colour_red;

					//祖父节点为红
					node->parent->parent->colour = rbtree_colour_red;

					//node为轴左旋转
					rbtree_inter_rotate_left(root, node);

					//父节点为轴右旋转右旋转
					rbtree_inter_rotate_right(root, node);

					node = temp;
				}
			}
		}
		//如果父节点是祖父节点的右孩子
		else
		{
			//如果伯父节点存在，且为红
			if(node->parent->parent->left && rbtree_colour_red == node->parent->parent->left->colour)
			{
				//把父节点与伯父节点涂成黑色
				node->parent->colour = rbtree_colour_black;
				node->parent->parent->left->colour = rbtree_colour_black;

				//把祖父结点涂成红色
				node->parent->parent->colour = rbtree_colour_red;

				//指针往上走
				node = node->parent->parent;
			}
			else
			{
				//如果是外侧插入
				if(node == node->parent->right)
				{
					//node为红
					node->colour = rbtree_colour_red;

					//父节点为黑
					node->parent->colour = rbtree_colour_black;

					//祖父节点为红
					node->parent->parent->colour = rbtree_colour_red;

					//父节点为轴左旋
					rbtree_inter_rotate_left(root, node->parent);
				}
				else
				{
					myrbtree_node_t * temp = node->parent;

					//node为黑
					node->colour = rbtree_colour_black;

					//父节点为红
					node->parent->colour = rbtree_colour_red;

					//祖父节点为红
					node->parent->parent->colour = rbtree_colour_red;

					//node为轴右旋转
					rbtree_inter_rotate_right(root, node);

					//父节点为轴左旋
					rbtree_inter_rotate_left(root, node);

					node = temp;
				}
			}
		}
	}

	(*root)->colour = rbtree_colour_black;
}

/*
*
*添加一节点
*rbtree:树
*parent:新节点的父节点
*key:新节点的关键字
*
*/
static __INLINE__ void rbtree_inter_insert(myrbtree_t * rbtree, myrbtree_node_t ** root, myrbtree_node_t * node_new, myrbtree_node_t * parent)
{
	assert(rbtree && node_new && root);

	//如果父节点为空，表明添加的是根节点
	if(NULL == parent)
	{
		//根节点必有黑色
		node_new->colour = rbtree_colour_black;
		*root = node_new;

		assert(NULL == *root || NULL == (*root)->parent);

		return;
	}

	node_new->parent = parent;

	//比较节点键值与父节点键值大小
	if(rbtree_inter_compare(rbtree, node_new->key,parent->key))
		parent->right = node_new;
	else
		parent->left = node_new;

	//旋转树，使之符合红黑树的规则
	rbtree_inter_rebalance(root, node_new);
}

/*
*
*从rb树中删除一个节点
*
*删除除节点 z , 则可以用y完全取代z(位置,颜色,用户数据), 删除z演变成删除y
*
* 图1
*      a                         a
*     / \                       / \
*    ?   z                     ?   y
*       / \       ---->           / \ 
*      ?   b                     ?   b
*         / \                       / \
*        y   ?            del ---> z   ?
*         \                         \
*          ?1                        ?1
*
*
*如果z是红色 则?1对应的子树根节点直接挪到z对应的位置,即完成了删除工作，并保证了红黑树的平衡性
*如果z是黑色 并且?1对应的子树根节点x为红色，则把?1挪到z的位置，并把x改成黑色，即完成了删除工作，并保证了红黑树的平衡性
*如果z是黑色 并且?1对应的子树根节点x也为黑色。
*图2
*
*     ?
*    / \
*   ?   x_parent
*      / \
*     x   ?2 = w
*    / \
*   ?   ?
*图2即最终要面临的情况
*x这个分支，由于去除了一个黑色的结点，而x又是黑色，它比 ?2 那个分支相比，"少了一个黑色的结点"
*
*图3
*       x_parent              x_parent->parent
*       /   \                   /           \
*      x     w     -->        x_parent       w_new
*     / \   / \               / \
*    ?   ? ?b  ?b            ?   ?
*假如?b ?b均为"黑" w也为黑 则把 w涂成红  此时x分支与w分支"黑层数"相同 , 
*(x_parent有可能为红，但这不影响结果,因为在下一个循环中,x = x_parent,循环将退出,x即x_parent将被涂成黑,不违反红黑树规则)
*以x = x_parent比 w = w_new这个分支"少了一层" 持续向上迭代 (x = x_parent x_parent = x->parent) 至到根节点为至,即完成了删除工作，并保证了红黑树的平衡
*
*如果w为红,通过旋转，可以演化成图3的情况
*演化步骤如下:
*把w涂成黑色,x_parent涂成红色(根据红黑树定义x_parent必为黑色, w的左右子节点必为黑色)
*以x_parent为轴左旋
*                   w 
*                  / \
*          x_parent    ?b 
*          /    \
*         x     ?b = w_new(必为黑)
*        / \
*       ?   ?
*  x 分支比 ?b分支少了一层
*
* 即，不管w为红还是为黑,最终都可以演化成w为黑的情况
*
*w的子节点中有红色节点的情况
*这种情况可以考虑把w的红色节点转移到x分支，作为x分支的根节点，并涂成黑色，即完成了删除，并保证了红黑树的平衡
*具体变换如下 如果a不为红，可以经过旋转变换为红
*
*     x_parent?
*      /      \
*     x        w黑      
*    / \      / \
*   ?   ?    黑b 红a
*
*              w?
*             /  \
*      x_parent黑 黑a
*     /  \
*    x   黑b
*即完成了删除，并保证了红黑树的平衡性
*
*/
static __INLINE__ myrbtree_node_t * rbtree_inter_rebalance_for_erase(/*myrbtree_t * rbtree*/myrbtree_node_t ** root, myrbtree_node_t * node)
{
	myrbtree_node_t * x = NULL;
	myrbtree_node_t * x_parent = NULL;
	myrbtree_node_t * y = node;

	assert(node && root);
	
	if(NULL == node->left)
		x = node->right;
	else if(NULL == node->right)
		x = node->left;
	else
	{
		//如果node不是叶子结点，则寻找右子树中最"左"边的那个节点替代node
		y = node->right;
		while(y->left)
			y = y->left;

		x = y->right;
	}

	//要删除的节点左右孩子都不为空,y与node 互换
	if(y != node)
	{
		//颜色互换
		rbtree_colour colour = y->colour;
		y->colour = node->colour;
		node->colour = colour;

		if(node->parent)
		{
			if(node == node->parent->left)
				node->parent->left = y;
			else
				node->parent->right = y;
		}
		else if(node == *root)
			*root = y;
		else
			assert(0);//如果走到这一步 bug here

		if(node->left)
			node->left->parent = y;

		//如果y是node的右孩子
		if(y == node->right)
		{
			x_parent = y;
		}
		else
		{
			if(node->right)
				node->right->parent = y;

			assert(y->parent);
			x_parent = y->parent;

			if(y == x_parent->left)
				x_parent->left = x;
			else
				assert(0);//x_parent->right = x; //y不可能是x_parent的右孩子 如果走到这一步,bug

			y->right = node->right;
		}

		y->parent = node->parent;
		y->left = node->left;

		y = node;
	}
	else//y就是要删除的节点，不必做替换，且y必有一孩子节点为空
	{
		//如果要删除的是根节点
		if(y == *root)
		{
			*root = x;
			if(*root)
			{
				(*root)->colour = rbtree_colour_black;
				(*root)->parent = NULL;
			}

			assert(NULL == *root || NULL == (*root)->parent);

			return y;
		}
		else
		{
			assert(y->parent);

			x_parent = y->parent;
			if(y == x_parent->left)
				x_parent->left = x;
			else
				x_parent->right = x;
		}
	}

	assert(x_parent);	

	if(x)
		x->parent = x_parent;

	//转化成
	//*      a                         a
	//*     / \                       / \
	//*    ?   z                     ?   y
	//*       / \       ---->           / \ 
	//*      ?   b                     ?   b
	//*         / \                       / \
	//*        y   ?            del ---> z   ?
	//*         \                         \
	//*          ?1                        ?1

	//如果要删除的节点为红色，函数返回
	if(y->colour == rbtree_colour_red)
	{
		assert(NULL == *root || NULL == (*root)->parent);
		return y;
	}

	while((x != *root) && (x == NULL || rbtree_colour_black == x->colour))
	{
		assert(x_parent);

		if(x == x_parent->left)
		{
			myrbtree_node_t * w = x_parent->right;
			assert(w);//x分支为0/1个黑结点,w分支至少有一个黑结点,所以w分支不可能为null,

			//如果w是红，需要进行转换,
			if(rbtree_colour_red == w->colour)
			{
				//x_parent必定是黑色的,且w的两个子节点必不空，且一定为黑色
				assert(rbtree_colour_black == x_parent->colour);
				assert(w->left && rbtree_colour_black == w->left->colour);
				assert(w->right && rbtree_colour_black == w->right->colour);

				w->colour = rbtree_colour_black;
				x_parent->colour = rbtree_colour_red;

				//旋转
				rbtree_inter_rotate_left(root, w);

				w = x_parent->right;
			}

			//x分支比w分支"少了一层",且x,w均为黑
			assert(w);

			//如果w的左右子节点均为黑
			if( (NULL == w->left || rbtree_colour_black == w->left->colour) &&
				(NULL == w->right || rbtree_colour_black == w->right->colour) )
			{
				//x_parent有可能为红，但这不影响结果,因为在下一个循环中,x = x_parent,循环将退出,x即x_parent将被涂成黑,不违反红黑树规则
				w->colour = rbtree_colour_red;
				x = x_parent;
				x_parent = x->parent;
				continue;
			}

			//如果至少有一个子节点为红, 通过旋转变换,让为红的那个节点始终为右节点
			if(NULL == w->right || rbtree_colour_black == w->right->colour)
			{
				assert(w->left && rbtree_colour_red == w->left->colour);//根据逻辑得出,必为不空,且必为红色
				
				w->left->colour = rbtree_colour_black;
				w->colour = rbtree_colour_red;
				rbtree_inter_rotate_right(root, w->left);
				w = x_parent->right;
			}

			//现在演变成这种情况
			//*     x_parent?
			//*      /      \
			//*     x        w黑      
			//*    / \      / \
			//*   ?   ?    ?b 红a
			assert(w->right && rbtree_colour_red == w->right->colour);//根据逻辑,这个条件必成立

			//旋转,完成了平衡的工作,变换过程如下图
			//*     x_parent?
			//*      /      \
			//*     x        w黑      
			//*    / \      / \
			//*   ?   ?    黑b 红a
			//*
			//*              w?
			//*             /  \
			//*      x_parent黑 黑a
			//*     /  \
			//*    x   黑b
			w->colour = x_parent->colour;
			x_parent->colour = rbtree_colour_black;
			w->right->colour = rbtree_colour_black;
			rbtree_inter_rotate_left(root, w);
			break;
		}
		else//x 是 x_parent的右孩子,操作与x == x_parent->left相同,但左右相反
		{
			myrbtree_node_t * w = x_parent->left;
			assert(w);//x分支为0/1个黑结点,w分支至少有一个黑结点,所以w分支不可能为null,

			//如果w是红，需要进行转换,
			if(rbtree_colour_red == w->colour)
			{
				//x_parent必定是黑色的,且w的两个子节点必不空，且一定为黑色
				assert(rbtree_colour_black == x_parent->colour);
				assert(w->left && rbtree_colour_black == w->left->colour);
				assert(w->right && rbtree_colour_black == w->right->colour);

				w->colour = rbtree_colour_black;
				x_parent->colour = rbtree_colour_red;

				//旋转
				rbtree_inter_rotate_right(root, w);

				w = x_parent->left;
			}

			//x分支比w分支"少了一层",且x,w均为黑
			assert(w);

			//如果w的左右子节点均为黑
			if( (NULL == w->left || rbtree_colour_black == w->left->colour) &&
				(NULL == w->right || rbtree_colour_black == w->right->colour) )
			{
				//x_parent有可能为红，但这不影响结果,因为在下一个循环中,x = x_parent,循环将退出,x即x_parent将被涂成黑,不违反红黑树规则
				w->colour = rbtree_colour_red;
				x = x_parent;
				x_parent = x->parent;
				continue;
			}

			//如果至少有一个子节点为红, 通过旋转变换,让为红的那个节点始终为右节点
			if(NULL == w->left || rbtree_colour_black == w->left->colour)
			{
				assert(w->right && rbtree_colour_red == w->right->colour);//根据逻辑得出,必为不空,且必为红色
				
				w->right->colour = rbtree_colour_black;
				w->colour = rbtree_colour_red;
				rbtree_inter_rotate_left(root, w->right);
				w = x_parent->left;
			}

			assert(w->left && rbtree_colour_red == w->left->colour);//根据逻辑,这个条件必成立

			w->colour = x_parent->colour;
			x_parent->colour = rbtree_colour_black;
			w->left->colour = rbtree_colour_black;
			rbtree_inter_rotate_right(root, w);
			break;
		}
	}

	if(x) 
		x->colour = rbtree_colour_black;

	//assert(!rbtree_inter_ismynode(rbtree->root, y));
	assert(NULL == (*root) || NULL == (*root)->parent);

	return y;
}

static __INLINE__ void rbtree_inter_del(myrbtree_t * rbtree, myrbtree_node_t ** root, myrbtree_node_t * node, void ** key, void ** data)
{
	assert(rbtree && node && root);

	//旋转平衡红黑树
	node = rbtree_inter_rebalance_for_erase(root, node);

	if(NULL == node)
		return;

	if(data)
		*data = node->data;

	if(key)
		*key = node->key;

	//销毁node
	rbtree_inter_destroy_node(rbtree, node);
}

static __INLINE__ int rbtree_inter_countlayer(myrbtree_node_t * root, int bmax)
{
	int left = 0;
	int right = 0;

	if(NULL == root)
		return 0;

	left = rbtree_inter_countlayer(root->left, bmax);
	right = rbtree_inter_countlayer(root->right, bmax);

	if(left > right && bmax)
		return left + 1;
	else
		return right + 1;
}

static __INLINE__ myrbtree_node_t * rbtree_inter_begin(myrbtree_t * rbtree)
{
	myrbtree_node_t * node;

	assert(rbtree);

	node = rbtree->root;
	if(NULL == node)
		return NULL;

	while(node->left)
		node = node->left;

	return node;
}

static __INLINE__ myrbtree_node_t * rbtree_inter_end(myrbtree_t * rbtree)
{
	myrbtree_node_t * node;

	assert(rbtree);

	node = rbtree->root;
	if(NULL == node)
		return NULL;

	while(node->right)
		node = node->right;

	return node;
}

static __INLINE__ void rbtree_inter_erase(myrbtree_t * rbtree, myrbtree_node_t * node)
{
	assert(node);

	while(node)
	{
		myrbtree_node_t * y = node->left;

		if(node->right)
			rbtree_inter_erase(rbtree, node->right);

		rbtree_inter_destroy_node(rbtree, node);

		node = y;
	}
}

static __INLINE__ int rbtree_inter_realcount(myrbtree_node_t * root)
{
	int left = 0;
	int right = 0;

	if(NULL == root)
		return 0;

	if(root->left)
		left = rbtree_inter_realcount(root->left);

	if(root->right)
		right = rbtree_inter_realcount(root->right);

	return left + right + 1;
}

static __INLINE__ int rbtree_inter_examin(myrbtree_t * rbtree, myrbtree_node_t * node)
{
	int left = 0;
	int right = 0;

	if(NULL == node)
		return 0;

	if(node->left)
	{
		assert(rbtree_inter_compare(rbtree, node->key, node->left->key));
		assert(node->left->parent == node);
		left = rbtree_inter_examin(rbtree, node->left);
	}

	if(node->right)
	{
		assert(rbtree_inter_compare(rbtree, node->right->key, node->key));
		assert(node->right->parent == node);
		right = rbtree_inter_examin(rbtree, node->right);
	}

	assert(left == right);

	if(rbtree_colour_black == node->colour)
		return left + 1;
	else
		return left;
}


/*
*
*创建rb树
*
*/
HMYRBTREE MyRBTreeConstruct(HMYMEMPOOL hm, myrbtree_compare compare)
{
	myrbtree_t * rbtree = NULL;

	rbtree = (myrbtree_t *)MyMemPoolMalloc(hm, sizeof(*rbtree));

	if(NULL == rbtree)
		return NULL;

	rbtree->compare = compare;
	rbtree->hm = hm;
	rbtree->root = NULL;

	return (HMYRBTREE)rbtree;
}

/*
*
*销毁rb树
*
*/
void MyRBTreeDestruct(HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return;

	//遍历树,释放每个节点
	if(rbtree->root)
		rbtree_inter_erase(rbtree, rbtree->root);

	MyMemPoolFree(rbtree->hm, rbtree);
}

/*
*
*删除所有节点
*
*/
void MyRBTreeClear(HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return;

	if(NULL == rbtree->root)
		return;

	//遍历树,释放每个节点
	rbtree_inter_erase(rbtree, rbtree->root);
	rbtree->root = NULL;
}

/*
*
*往rb树中插入一个节点
*
*/
HMYRBTREE_ITER MyRBTreeInsertEqual(HMYRBTREE htree, const void * key, const void * userdata)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;
	myrbtree_node_t * node_new = NULL;
	myrbtree_node_t * parent = NULL;

	if(NULL == rbtree)
        return NULL;

	rbtree_inter_searchex(rbtree, rbtree->root, key, &parent);

	node_new = rbtree_inter_create_node(rbtree, key, userdata);
	if(NULL == node_new)
		return NULL;

	rbtree_inter_insert(rbtree, &rbtree->root, node_new, parent);

	return (HMYRBTREE_ITER)node_new;
}

/*
*
*往rb树中插入一个节点
*
*/
HMYRBTREE_ITER MyRBTreeInsertUnique(HMYRBTREE htree, const void * key, const void * userdata)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	myrbtree_node_t * parent = NULL;
	myrbtree_node_t * node_new = NULL;

	if(NULL == rbtree)
        return NULL;
	
	node_new = rbtree_inter_searchex(rbtree, rbtree->root, key, &parent);
	if(node_new != NULL)
		return (HMYRBTREE_ITER)node_new;

	node_new = rbtree_inter_create_node(rbtree, key, userdata);
	if(NULL == node_new)
		return NULL;

	rbtree_inter_insert(rbtree, &rbtree->root, node_new, parent);

	return (HMYRBTREE_ITER)node_new;
}

/*
*
*从rb树中删除一个节点 iter失效
*
*/
void MyRBTreeDelIter(HMYRBTREE htree, HMYRBTREE_ITER iter, void ** key, void ** data)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;
	myrbtree_node_t * node = (myrbtree_node_t *)iter;

	if(NULL == rbtree || NULL == node)
		return;

	assert(rbtree_inter_search(rbtree, rbtree->root, node->key));

	rbtree_inter_del(rbtree, &(rbtree->root), node, key, data);
}

/*
*
*根据键值删除一个节点
*成功删除返回0, 否则返回-1
*
*/
int MyRBTreeDelKey(HMYRBTREE htree, const void * key, void ** key_ret, void ** data_ret)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;
	myrbtree_node_t * node = NULL;

	if(NULL == rbtree)
		return -1;

	node = rbtree_inter_search(rbtree, rbtree->root, key);

	if(NULL == node)
		return -1;

	rbtree_inter_del(rbtree, &(rbtree->root), node, key_ret, data_ret);

	return 0;
}

/*
*
*获取节点的用户数据
*
*/
void * MyRBTreeGetIterData(const HMYRBTREE_ITER iter)
{
	myrbtree_node_t * node = (myrbtree_node_t *)iter;

	if(NULL == node)
		return NULL;

	return node->data;
}

/*
*
*获取节点的键
*
*/
const void * MyRBTreeGetIterKey(const HMYRBTREE_ITER iter)
{
	myrbtree_node_t * node = (myrbtree_node_t *)iter;

	if(NULL == node)
		return NULL;

	return node->key;
}

/*
*
*查找节点
*
*/
HMYRBTREE_ITER MyRBTreeSearch(const HMYRBTREE htree, const void * key)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;
	myrbtree_node_t * node = NULL;

	if(NULL == rbtree || NULL == rbtree->compare)
		return NULL;

	node = rbtree_inter_search(rbtree, rbtree->root, key);

	return (HMYRBTREE_ITER)node;
}

/*
*
*计算最大层数
*
*/
int MyRBTreeLayer(const HMYRBTREE htree, int bmax)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return 0;

	return rbtree_inter_countlayer(rbtree->root, bmax);
}

/*
*
*"获取第一个节点"
*
*/
HMYRBTREE_ITER MyRBTreeBegin(const HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return 0;

	return (HMYRBTREE_ITER)rbtree_inter_begin(rbtree);
}

/*
*
*"获取最后一个节点"
*
*/
HMYRBTREE_ITER MyRBTreeEnd(const HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return 0;

	return (HMYRBTREE_ITER)rbtree_inter_end(rbtree);
}

/*
*
*获取下一个节点
*
*/
HMYRBTREE_ITER MyRBTreeGetNext(const HMYRBTREE_ITER it)
{
	myrbtree_node_t * node = (myrbtree_node_t *)it;

	if(NULL == node)
		return NULL;

	if(NULL == node->right)
	{
		while(node->parent && node == node->parent->right)
			node = node->parent;

		if(node->parent && node == node->parent->left)
			return (HMYRBTREE_ITER)node->parent;
		else
			return NULL;
	}
	else
	{
		node = node->right;
		while(node->left)
			node = node->left;

		return (HMYRBTREE_ITER)node;
	}
}

/*
*
*获取上一个节点
*
*/
HMYRBTREE_ITER MyRBTreeGetPrev(const HMYRBTREE_ITER it)
{
	myrbtree_node_t * node = (myrbtree_node_t *)it;

	if(node->left)
	{
		node = node->left;
		while(node->right)
			node = node->right;
		return (HMYRBTREE_ITER)node;
	}
	else
	{
		while(node->parent && node == node->parent->left)
			node = node->parent;

		if(node->parent && node == node->parent->right)
			return (HMYRBTREE_ITER)node->parent;
		else
			return NULL;
	}		
}

/*
*
*检查红黑树是否合法
*
*/
int MyRBTreeExamin(const HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return 0;

	return rbtree_inter_examin(rbtree, rbtree->root);
}

/*
*
*获取个数
*
*/
int MyRBTreeGetRealCount(const HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return 0;

	return rbtree_inter_realcount(rbtree->root);
}

#include "mymap.c"

#ifdef WIN32
	#include "MyStringSetEx.c"
#endif




























