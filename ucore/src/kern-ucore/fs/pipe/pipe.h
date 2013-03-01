#ifndef __KERN_FS_PIPE_PIPE_H__
#define __KERN_FS_PIPE_PIPE_H__

#include <types.h>
#include <list.h>
#include <sem.h>

struct fs;
struct inode;
struct pipe_state;

struct pipe_fs {
	struct inode *root;
	semaphore_t pipe_sem;
	list_entry_t pipe_list;
};

void pipe_init(void);
void lock_pipe(struct pipe_fs *pipe);
void unlock_pipe(struct pipe_fs *pipe);

struct pipe_root {
	/* empty */
};

struct pipe_inode {
	enum {
		PIN_RDONLY, PIN_WRONLY,
	} pin_type;
	char *name;
	int reclaim_count;
	struct pipe_state *state;
	list_entry_t pipe_link;
};

#define le2pin(le, member)                          \
    to_struct((le), struct pipe_inode, member)

struct inode *pipe_create_root(struct fs *fs);
struct inode *pipe_create_inode(struct fs *fs, const char *name,
				struct pipe_state *state, bool readonly);
int pipe_open(struct inode **rnode_store, struct inode **wnode_store);

#endif /* !__KERN_FS_PIPE_PIPE_H__ */
