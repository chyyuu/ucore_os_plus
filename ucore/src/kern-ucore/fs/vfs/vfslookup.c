/*
 * VFS operations relating to pathname translation
 */

#include <types.h>
#include <string.h>
#include <vfs.h>
#include <inode.h>
#include <error.h>
#include <assert.h>

/*
 * Common code to pull the device name, if any, off the front of a
 * path and choose the inode to begin the name lookup relative to.
 */
static int get_device(char *path, char **subpath, struct inode **node_store)
{
	int i, slash = -1, colon = -1;
	/*
	 * Locate the first colon or slash.
	 */
	for (i = 0; path[i] != '\0'; i++) {
		if (path[i] == ':') {
			colon = i;
			break;
		}
		if (path[i] == '/') {
			slash = i;
			break;
		}
	}
	if (colon < 0 && slash != 0) {
		/* *
		 * No colon before a slash, so no device name specified, and the slash isn't leading
		 * or is also absent, so this is a relative path or just a bare filename. Start from
		 * the current directory, and use the whole thing as the subpath.
		 * */
		*subpath = path;
		return vfs_get_curdir(node_store);
	}
	if (colon > 0) {
		/* device:path - get root of device's filesystem */
		path[colon] = '\0';

		/* device:/path - skip slash, treat as device:path */
		while (path[++colon] == '/') ;

		*subpath = path + colon;
		return vfs_get_root(path, node_store);
	}

	/* *
	 * we have either /path or :path
	 * /path is a path relative to the root of the "boot filesystem"
	 * :path is a path relative to the root of the current filesystem
	 * */
	int ret;
	if (*path == '/') {
		if ((ret = vfs_get_bootfs(node_store)) != 0) {
			return ret;
		}
	} else {
		assert(*path == ':');
		struct inode *node;
		if ((ret = vfs_get_curdir(&node)) != 0) {
			return ret;
		}
		/* The current directory may not be a device, so it must have a fs. */
		assert(node->in_fs != NULL);
		*node_store = fsop_get_root(node->in_fs);
		vop_ref_dec(node);
	}

	/* ///... or :/... */
	while (*(++path) == '/') ;
	*subpath = path;
	return 0;
}

/*
 * Name-to-inode translation.
 * (In BSD, both of these are subsumed by namei().)
 */

int vfs_lookup(char *path, struct inode **node_store)
{
	int ret;
	struct inode *node;
	if ((ret = get_device(path, &path, &node)) != 0) {
		return ret;
	}
	if (*path != '\0') {
		ret = vop_lookup(node, path, node_store);
		vop_ref_dec(node);
		return ret;
	}
	*node_store = node;
	return 0;
}

int vfs_lookup_parent(char *path, struct inode **node_store, char **endp)
{
	int ret;
	struct inode *node;
	if ((ret = get_device(path, &path, &node)) != 0) {
		return ret;
	}
	ret =
	    (*path != '\0') ? vop_lookup_parent(node, path, node_store,
						endp) : -E_INVAL;
	vop_ref_dec(node);
	return ret;
}
