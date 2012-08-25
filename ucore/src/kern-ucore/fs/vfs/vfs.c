#include <types.h>
#include <stdio.h>
#include <string.h>
#include <slab.h>
#include <vfs.h>
#include <inode.h>
#include <sem.h>
#include <error.h>

static semaphore_t bootfs_sem;
static struct inode *bootfs_node = NULL;

extern void vfs_devlist_init(void);

struct fs *
__alloc_fs(int type) {
    struct fs *fs;
    if ((fs = kmalloc(sizeof(struct fs))) != NULL) {
        fs->fs_type = type;
    }
    return fs;
}

void
vfs_init(void) {
    sem_init(&bootfs_sem, 1);
    vfs_devlist_init();
}

static void
lock_bootfs(void) {
    down(&bootfs_sem);
}

static void
unlock_bootfs(void) {
    up(&bootfs_sem);
}

/*
 * Helper function for actually changing bootfs_inode.
 */
static void
change_bootfs(struct inode *node) {
    struct inode *old;
    lock_bootfs();
    {
        old = bootfs_node, bootfs_node = node;
    }
    unlock_bootfs();
    if (old != NULL) {
        vop_ref_dec(old);
    }
}

/*
 * Set bootfs_node.
 *
 * Bootfs_node is the inode used for beginning path translation of
 * pathnames starting with /.
 *
 * It is also incidentally the system's first current directory.
 */
int
vfs_set_bootfs(char *fsname) {
    struct inode *node = NULL;
    if (fsname != NULL) {
        char *s;
        if ((s = strchr(fsname, ':')) == NULL || s[1] != '\0') {
            return -E_INVAL;
        }
        int ret;
        if ((ret = vfs_chdir(fsname)) != 0) {
            return ret;
        }
        if ((ret = vfs_get_curdir(&node)) != 0) {
            return ret;
        }
    }
    change_bootfs(node);
    return 0;
}

/*
 * Get bootfs_inode.
 */
int
vfs_get_bootfs(struct inode **node_store) {
    struct inode *node = NULL;
    if (bootfs_node != NULL) {
        lock_bootfs();
        {
            if ((node = bootfs_node) != NULL) {
                vop_ref_inc(bootfs_node);
            }
        }
        unlock_bootfs();
    }
    if (node == NULL) {
        return -E_NOENT;
    }
    *node_store = node;
    return 0;
}

