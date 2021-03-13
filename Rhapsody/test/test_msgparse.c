#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include <winsock2.h>
void gettimeofday(struct timeval *ptv, void *tzp);
#else
#include <ctype.h>
#include <sys/time.h>
#endif
#include <time.h>
#include <string.h>


void getcallid(char * callid, size_t callid_len);


#define MAXPTRLEN	32

typedef void (*MessageHandler)();

#define LAST_HANDLER_TYPE 8
/* 
 * Message table structure 
 */
struct Message
{
  const char *cmd;
  unsigned int count;      /* number of times command used */
  unsigned int rcount;     /* number of times command used by server */
  unsigned int parameters; /* at least this many args must be passed
                             * or an error will be sent to the user 
                             * before the m_func is even called 
                             */
  unsigned int maxpara;    /* maximum permitted parameters */
  unsigned int flags;      /* bit 0 set means that this command is allowed
			     * to be used only on the average of once per 2
			     * seconds -SRB
			     */
  unsigned long bytes;  /* bytes received for this message */
  /*
   * client_p = Connected client ptr
   * source_p = Source client ptr
   * parc = parameter count
   * parv = parameter variable array
   */
  /* handlers:
   * UNREGISTERED, CLIENT, SERVER, ENCAP, OPER, DUMMY, LAST
   */
  MessageHandler handlers[LAST_HANDLER_TYPE];
};

struct MessageTree
{
  int links; /* Count of all pointers (including msg) at this node 
	      * used as reference count for deletion of _this_ node.
	      */
  struct Message *msg;
  struct MessageTree *pointers[MAXPTRLEN];
};

static struct MessageTree msg_tree;

/* clear_tree_parse()
 *
 * inputs       - NONE
 * output       - NONE
 * side effects - MUST MUST be called at startup ONCE before
 *                any other keyword routine is used.
 */
void
clear_tree_parse(void)
{
  memset(&msg_tree, 0, sizeof(msg_tree));
}

/* add_msg_element()
 *
 * inputs	- pointer to MessageTree
 *		- pointer to Message to add for given command
 *		- pointer to current portion of command being added
 * output	- NONE
 * side effects	- recursively build the Message Tree ;-)
 */
/*
 * How this works.
 *
 * The code first checks to see if its reached the end of the command
 * If so, that struct MessageTree has a msg pointer updated and the links
 * count incremented, since a msg pointer is a reference.
 * Then the code descends recursively, building the trie.
 * If a pointer index inside the struct MessageTree is NULL a new
 * child struct MessageTree has to be allocated.
 * The links (reference count) is incremented as they are created
 * in the parent.
 */
static void
add_msg_element(struct MessageTree *mtree_p, 
		struct Message *msg_p, const char *cmd)
{
  struct MessageTree *ntree_p;

  if (*cmd == '\0')
  {
    mtree_p->msg = msg_p;
    mtree_p->links++;		/* Have msg pointer, so up ref count */
  }
  else
  {
    /* *cmd & (MAXPTRLEN-1) 
     * convert the char pointed to at *cmd from ASCII to an integer
     * between 0 and MAXPTRLEN.
     * Thus 'A' -> 0x1 'B' -> 0x2 'c' -> 0x3 etc.
     */

    if ((ntree_p = mtree_p->pointers[*cmd & (MAXPTRLEN-1)]) == NULL)
    {
      ntree_p = (struct MessageTree *)malloc(sizeof(struct MessageTree));
memset(ntree_p, 0, sizeof(msg_tree));
      mtree_p->pointers[*cmd & (MAXPTRLEN-1)] = ntree_p;

      mtree_p->links++;		/* Have new pointer, so up ref count */
    }
    add_msg_element(ntree_p, msg_p, cmd+1);
  }
}

/* del_msg_element()
 *
 * inputs	- Pointer to MessageTree to delete from
 *		- pointer to command name to delete
 * output	- NONE
 * side effects	- recursively deletes a token from the Message Tree ;-)
 */
/*
 * How this works.
 *
 * Well, first off, the code recursively descends into the trie
 * until it finds the terminating letter of the command being removed.
 * Once it has done that, it marks the msg pointer as NULL then
 * reduces the reference count on that allocated struct MessageTree
 * since a command counts as a reference.
 *
 * Then it pops up the recurse stack. As it comes back up the recurse
 * The code checks to see if the child now has no pointers or msg
 * i.e. the links count has gone to zero. If its no longer used, the
 * child struct MessageTree can be deleted. The parent reference
 * to this child is then removed and the parents link count goes down.
 * Thus, we continue to go back up removing all unused MessageTree(s)
 */
static void
del_msg_element(struct MessageTree *mtree_p, const char *cmd)
{
  struct MessageTree *ntree_p;

  /* In case this is called for a nonexistent command
   * check that there is a msg pointer here, else links-- goes -ve
   * -db
   */

  if ((*cmd == '\0') && (mtree_p->msg != NULL))
  {
    mtree_p->msg = NULL;
    mtree_p->links--;
  }
  else
  {
    if ((ntree_p = mtree_p->pointers[*cmd & (MAXPTRLEN-1)]) != NULL)
    {
      del_msg_element(ntree_p, cmd+1);
      if (ntree_p->links == 0)
      {
	mtree_p->pointers[*cmd & (MAXPTRLEN-1)] = NULL;
	mtree_p->links--;
	free(ntree_p);
      }
    }
  }
}

/* msg_tree_parse()
 *
 * inputs	- Pointer to command to find
 *		- Pointer to MessageTree root
 * output	- Find given command returning Message * if found NULL if not
 * side effects	- none
 */
static struct Message *
msg_tree_parse(const char *cmd, struct MessageTree *root)
{
  struct MessageTree *mtree = root;
  assert(cmd && *cmd);

  while (isalpha(*cmd) && (mtree = mtree->pointers[*cmd & (MAXPTRLEN - 1)]))
    if (*++cmd == '\0')
      return mtree->msg;

  return NULL;
}

/* mod_add_cmd()
 *
 * inputs	- pointer to struct Message
 * output	- none
 * side effects - load this one command name
 *		  msg->count msg->bytes is modified in place, in
 *		  modules address space. Might not want to do that...
 */
void
mod_add_cmd(struct Message *msg)
{
  struct Message *found_msg;

  if (msg == NULL)
    return;

  /* someone loaded a module with a bad messagetab */
  assert(msg->cmd != NULL);

  /* command already added? */
  if ((found_msg = msg_tree_parse(msg->cmd, &msg_tree)) != NULL)
    return;

  add_msg_element(&msg_tree, msg, msg->cmd);
  msg->count = msg->rcount = msg->bytes = 0;
}

/* mod_del_cmd()
 *
 * inputs	- pointer to struct Message
 * output	- none
 * side effects - unload this one command name
 */
void
mod_del_cmd(struct Message *msg)
{
  assert(msg != NULL);

  if (msg == NULL)
    return;

  del_msg_element(&msg_tree, msg->cmd);
}

/* find_command()
 *
 * inputs	- command name
 * output	- pointer to struct Message
 * side effects - none
 */
struct Message *
find_command(const char *cmd)
{
  return(msg_tree_parse(cmd, &msg_tree));
}


void test_msgparse()
{
	int i = 0;
	struct timeval tv1;
	struct timeval tv2;
	struct Message m;

	clear_tree_parse();

	srand(0);
	for(i = 0; i<100000;i++)
	{
		char buf[32];
		getcallid(buf, sizeof(buf) - 1);
		m.cmd = buf;
		mod_add_cmd(&m);
	}

	srand(0);
	gettimeofday(&tv1, 0);
	for(i = 0; i<100000;i++)
	{
		struct Message * temp = NULL;
		char buf[32] = {0};
		getcallid(buf, sizeof(buf) - 1);
		temp = msg_tree_parse(buf , &msg_tree);
		assert(temp);
	}
	gettimeofday(&tv2, 0);
	printf("msgparse²éÕÒÓÃÊ±:%fÃë\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);
}



















