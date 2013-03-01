#include <types.h>
#include <string.h>
#include <vfs.h>
#include <inode.h>
#include <pipe.h>
#include <pipe_state.h>
#include <error.h>
#include <assert.h>

static void
lookup_pipe_nolock(struct pipe_fs *pipe, const char *name,
		   struct inode **rnode_store, struct inode **wnode_store)
{
	list_entry_t *list = &(pipe->pipe_list), *le = list;
	*rnode_store = *wnode_store = NULL;
	while ((le = list_next(le)) != list) {
		struct pipe_inode *pin = le2pin(le, pipe_link);
		if (strcmp(pin->name, name) == 0) {
			struct inode *node = info2node(pin, pipe_inode);
			switch (pin->pin_type) {
			case PIN_RDONLY:
				assert(*rnode_store == NULL);
				*rnode_store = node;
				break;
			case PIN_WRONLY:
				assert(*wnode_store == NULL);
				*wnode_store = node;
				break;
			default:
				panic("unknown pipe_inode type %d.\n",
				      pin->pin_type);
			}
			if (vop_ref_inc(node) == 1) {
				pin->reclaim_count++;
			}
		}
	}
}

static int
pipe_root_create(struct inode *__node, const char *name, bool excl,
		 struct inode **node_store)
{
	assert((name[0] == 'r' || name[0] == 'w') && name[1] == '_');
	int ret = 0;
	bool readonly = (name[0] == 'r');
	name += 2;

	struct inode *node[2];
	struct fs *fs = vop_fs(__node);
	struct pipe_fs *pipe = fsop_info(fs, pipe);

	lock_pipe(pipe);

	lookup_pipe_nolock(pipe, name, &node[0], &node[1]);
	if (!readonly) {
		struct inode *tmp = node[0];
		node[0] = node[1], node[1] = tmp;
	}

	if (node[0] != NULL) {
		if (excl) {
			ret = -E_EXISTS, vop_ref_dec(node[0]);
			goto out;
		}
		*node_store = node[0];
		goto out;
	}
	ret = -E_NO_MEM;

	struct pipe_state *state;
	if (node[1] == NULL) {
		if ((state = pipe_state_create()) == NULL) {
			goto out;
		}
	} else {
		state = vop_info(node[1], pipe_inode)->state;
		pipe_state_acquire(state);
	}

	struct inode *new_node;
	if ((new_node = pipe_create_inode(fs, name, state, readonly)) == NULL) {
		pipe_state_release(state);
		goto out;
	}

	list_add(&(pipe->pipe_list),
		 &(vop_info(new_node, pipe_inode)->pipe_link));
	ret = 0, *node_store = new_node;

out:
	unlock_pipe(pipe);

	if (node[1] != NULL) {
		vop_ref_dec(node[1]);
	}
	return ret;
}

static int
pipe_root_lookup(struct inode *__node, char *path, struct inode **node_store)
{
	assert((path[0] == 'r' || path[0] == 'w') && path[1] == '_');
	struct inode *node[2];
	struct pipe_fs *pipe = fsop_info(vop_fs(__node), pipe);
	lock_pipe(pipe);
	{
		lookup_pipe_nolock(pipe, path + 2, &node[0], &node[1]);
	}
	unlock_pipe(pipe);

	if (path[0] != 'r') {
		struct inode *tmp = node[0];
		node[0] = node[1], node[1] = tmp;
	}

	if (node[1] != NULL) {
		vop_ref_dec(node[1]);
	}
	if (node[0] == NULL) {
		return -E_NOENT;
	}
	*node_store = node[0];
	return 0;
}

static int
pipe_root_lookup_parent(struct inode *node, char *path,
			struct inode **node_store, char **endp)
{
	assert((path[0] == 'r' || path[0] == 'w') && path[1] == '_');
	*node_store = node, *endp = path;
	vop_ref_inc(node);
	return 0;
}

static const struct inode_ops pipe_root_ops = {
	.vop_magic = VOP_MAGIC,
	.vop_open = NULL_VOP_INVAL,
	.vop_close = NULL_VOP_INVAL,
	.vop_read = NULL_VOP_INVAL,
	.vop_write = NULL_VOP_INVAL,
	.vop_fstat = NULL_VOP_INVAL,
	.vop_fsync = NULL_VOP_INVAL,
	.vop_mkdir = NULL_VOP_INVAL,
	.vop_link = NULL_VOP_INVAL,
	.vop_rename = NULL_VOP_INVAL,
	.vop_readlink = NULL_VOP_INVAL,
	.vop_symlink = NULL_VOP_INVAL,
	.vop_namefile = NULL_VOP_INVAL,
	.vop_getdirentry = NULL_VOP_INVAL,
	.vop_reclaim = NULL_VOP_INVAL,
	.vop_ioctl = NULL_VOP_INVAL,
	.vop_gettype = NULL_VOP_INVAL,
	.vop_tryseek = NULL_VOP_INVAL,
	.vop_truncate = NULL_VOP_INVAL,
	.vop_create = pipe_root_create,
	.vop_unlink = NULL_VOP_INVAL,
	.vop_lookup = pipe_root_lookup,
	.vop_lookup_parent = pipe_root_lookup_parent,
};

struct inode *pipe_create_root(struct fs *fs)
{
	struct inode *node;
	if ((node = alloc_inode(pipe_root)) != NULL) {
		vop_init(node, &pipe_root_ops, fs);
	}
	return node;
}
