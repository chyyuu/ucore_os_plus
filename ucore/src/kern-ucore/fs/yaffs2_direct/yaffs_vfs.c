/*
 * =====================================================================================
 *
 *       Filename:  yaffs_vfs.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/10/2012 08:19:14 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include "yaffs_vfs.h"
#include <types.h>
#include <nand_mtd.h>
#include <fs.h>
#include <stat.h>
#include <vfs.h>
#include <inode.h>
#include <iobuf.h>
#include <error.h>

#include "yaffsfs.h"
#include "yaffs_guts.h"
#include "yaffs_trace.h"

#ifdef ARCH_ARM
#include <board.h>
#endif


static const struct inode_ops yaffs_node_dirops;
static const struct inode_ops yaffs_node_fileops;


static const char *mount_partition = "/data";

#define yaffs_inode_to_obj_lv(iptr) (vop_info(node, yaffs2_inode)->obj)
#define yaffs_inode_to_obj(iptr)\
	((struct yaffs_obj *)(yaffs_inode_to_obj_lv(iptr)))
#define yaffs_get_namebuf() (fsop_info((node->in_fs), yaffs2)->yaffs_name_buf)

static int yaffs_vfs_do_sync(struct fs *fs)
{
  int ret;
  kprintf("yaffs_vfs_do_sync: sync disk\n");
  ret = yaffs_sync(mount_partition);
  if(ret)
      kprintf("yaffs_vfs_do_sync: faild to sync %s\n", mount_partition);
  return 0;
}

static const struct inode_ops *
yaffs_get_ops(int type) {
    switch (type ) {
    case YAFFS_OBJECT_TYPE_DIRECTORY:
        return &yaffs_node_dirops;
    case YAFFS_OBJECT_TYPE_FILE:
        return &yaffs_node_fileops;
    }
    panic("invalid file type %d.\n", type);
}

static void yaffs_fill_inode(struct inode *node, struct yaffs_obj* obj)
{
  struct yaffs2_inode *yn = vop_info(node, yaffs2_inode);
  yn->type = obj->variant_type;
  yn->obj = obj;
}

static struct inode *yaffs_get_inode(struct fs *fs, struct yaffs_obj* obj)
{
  struct inode *nnode;
  /*  
  char *namebuf = fsop_info(fs, yaffs2)->yaffs_name_buf; 
  yaffs_get_obj_name(obj, namebuf, FS_MAX_FNAME_LEN); 
  yaffs_trace(YAFFS_TRACE_OS,
      "yaffs_get_inode %d:%s", obj->obj_id, namebuf);
  */
  if ((nnode = alloc_inode(yaffs2_inode)) != NULL) {
    vop_init(nnode, yaffs_get_ops(obj->variant_type), fs);
    yaffs_fill_inode(nnode, obj);
    return nnode;
  }
  return NULL;
}

#if 0
static struct inode *yaffs_iget(struct fs *fs, unsigned long ino)
{
	struct inode *inode;
	struct yaffs_obj *obj;

	yaffs_trace(YAFFS_TRACE_OS, "yaffs_iget for %lu", ino);

	obj = yaffs_find_by_number(fsop_info(fs,yaffs2)->ydev, ino);
  if(!obj)
    return NULL;

  inode = yaffs_get_inode(fs, obj->yst_mode, obj);

	if (!inode)
		return NULL;

	return inode;
}
#endif

struct inode * yaffs_vfs_do_get_root(struct fs *fs)
{
  struct inode *node;
  struct yaffs2_fs *yfs = fsop_info(fs, yaffs2);
  if ((node = alloc_inode(yaffs2_inode)) != NULL) {
    vop_init(node, yaffs_get_ops(YAFFS_OBJECT_TYPE_DIRECTORY), fs);
    node = yaffs_get_inode(fs, yaffs_root(yfs->ydev));
    return node;
  }
  return NULL;
}

static int yaffs_vfs_do_unmount(struct fs *fs)
{
  kprintf("yaffs_vfs_do_unmount:  unmount %s\n", mount_partition);
  int ret = yaffs_unmount(mount_partition);
  if(ret){
    kprintf("yaffs_vfs_do_unmount: faild to unmount %s\n", mount_partition);
    return -1;
  }
  /* release struct fs */
  kfree(fs);
  return 0;
}

static void yaffs_vfs_do_cleanup(struct fs *fs)
{
  kprintf("yaffs_vfs_do_cleanup: \n");
  return;
}


static int
yaffs_vfs_do_mount(struct device *dev, struct fs **fs_store)
{
  struct fs *fs;
  if ((fs = alloc_fs(yaffs2)) == NULL){
    return -E_NO_MEM;
  }

  struct yaffs2_fs *yfs = fsop_info(fs, yaffs2);
  yfs->dev = dev;

  //TODO mount vfs here
  kprintf("yaffs_vfs_do_mount:  mount %s", mount_partition);
  int ret = yaffs_mount_common(mount_partition,0, 0, &(yfs->ydev));
  if(ret)
    panic("yaffs_vfs_do_mount: faild to mount %s", mount_partition);


  fs->fs_sync =  yaffs_vfs_do_sync;
  fs->fs_get_root = yaffs_vfs_do_get_root;
  fs->fs_unmount = yaffs_vfs_do_unmount;
  fs->fs_cleanup = yaffs_vfs_do_cleanup;

  *fs_store = fs;
  return 0;
}

int yaffs_vfs_mount(const char *devname)
{
  return vfs_mount(devname, yaffs_vfs_do_mount);
}


void
yaffs_vfs_init(void) {
#ifndef HAS_NANDFLASH
  kprintf("yaffs_vfs_init: nandflash not supported, use ramsim\n.");
  int ret;
  if ((ret = yaffs_vfs_mount("disk1")) != 0) {
    panic("failed: yaffs2: yaffs_vfs_mount: %e.\n", ret);
  }

#else
  if(check_nandflash()){
    int ret;
    if ((ret = yaffs_vfs_mount("disk1")) != 0) {
      panic("failed: yaffs2: yaffs_vfs_mount: %e.\n", ret);
    }
  }else{
    kprintf("yaffs_vfs_init: nandflash not found\n");
  }
#endif
}

static int yaffs_vop_reclaim(struct inode *node)
{
  struct yaffs_obj *obj = yaffs_inode_to_obj(node);
  //kprintf("TODO reclaim %d \n", obj->obj_id );
  yaffs_trace(YAFFS_TRACE_OS,
      "yaffs_vop_reclaim: %d", obj->obj_id);
  vop_kill(node); 
  return 0;
}

static char *
yaffs_lookup_subpath(char *path) {
  if ((path = strchr(path, '/')) != NULL) {
    while (*path == '/') {
      *path ++ = '\0';
    }
    if (*path == '\0') {
      return NULL;
    }
  }
  return path;
}

static int yaffs_vop_lookup(struct inode *node, char *path, struct inode **node_store)
{
  yaffs_trace(YAFFS_TRACE_OS,
      "yaffs_lookup for %d:%s",
      yaffs_inode_to_obj(node)->obj_id, path);
#if 0
  struct yaffs_obj *obj = yaffs_inode_to_obj(node);

  obj = yaffs_find_by_name(yaffs_inode_to_obj(node), path);
  obj = yaffs_get_equivalent_obj(obj);

  if (obj) {
    yaffs_trace(YAFFS_TRACE_OS,
        "yaffs_lookup found %d", obj->obj_id);

    struct inode* nnode = yaffs_get_inode(node->in_fs, obj->variant_type, obj);

    if (nnode) {
      yaffs_trace(YAFFS_TRACE_OS, "yaffs_loookup dentry");
      /* return dentry; */
      *node_store = nnode;
      vop_ref_inc(nnode);

      return 0;
    }

  } else {
    yaffs_trace(YAFFS_TRACE_OS, "yaffs_lookup not found");

  }
#endif
  assert(*path != '\0' && *path != '/');
  struct yaffs_obj *obj = yaffs_inode_to_obj(node);
  do {
    obj = yaffs_get_equivalent_obj(obj);
    if (obj->variant_type != YAFFS_OBJECT_TYPE_DIRECTORY) {
      return -E_NOTDIR;
    }

    char *subpath;
next:
    subpath = yaffs_lookup_subpath(path);
    if (strcmp(path, ".") == 0) {
      if ((path = subpath) != NULL) {
        goto next;
      }
      break;
    }

    if (strcmp(path, "..") == 0) {
      //ret = sfs_load_parent(sfs, sin, &subnode);
      if(obj->parent == NULL){
        break;
      }else{
        obj = obj->parent;
      }
    }
    else {
      if (strlen(path) > YAFFS_MAX_NAME_LENGTH) {
        return -E_TOO_BIG;
      }
      obj = yaffs_find_by_name(obj, path);
      if(!obj){
        return -E_NOENT;
      }
    }
    path = subpath;
  } while (path != NULL);

		yaffs_trace(YAFFS_TRACE_OS,
			"yaffs_lookup found %d", obj->obj_id);

		struct inode* nnode = yaffs_get_inode(node->in_fs, obj);

		if (nnode) {
			yaffs_trace(YAFFS_TRACE_OS, "yaffs_lookup dentry");
			/* return dentry; */
      *node_store = nnode;
      //vop_ref_inc(nnode);
      return 0;
    }
    return -E_NO_MEM;
}

static int yaffs_vop_lookup_parent(struct inode *node, char *path, 
    struct inode **node_store, char **endp)
{
  struct yaffs_obj *d_obj;
  //kprintf("TODO lookup parent %s\n", path);
  d_obj = yaffs_inode_to_obj(node);
  while (1) {
    if (d_obj->variant_type != YAFFS_OBJECT_TYPE_DIRECTORY) {
      return -E_NOTDIR;
    }

    char *subpath;
next:
    if ((subpath = yaffs_lookup_subpath(path)) == NULL) {
      goto found;
      //    *node_store = node, *endp = path;
    }
    if (strcmp(path, ".") == 0) {
      path = subpath;
      goto next;
    }
    if (strcmp(path, "..") == 0) {
      if(d_obj->parent == NULL){
        path = subpath;
        goto next;
      }
      d_obj = d_obj->parent;
    }
    else {
      if (strlen(path) > FS_MAX_FNAME_LEN) {
        return -E_TOO_BIG;
      }
      d_obj = yaffs_find_by_name(d_obj, path);
      if(!d_obj){
        return -E_NOENT;
      }
    }

    path = subpath;

  }
found:
  if (d_obj) {
    yaffs_trace(YAFFS_TRACE_OS,
        "yaffs_vop_lookup_parent: found %d\n", d_obj->obj_id);
    d_obj = yaffs_get_equivalent_obj(d_obj);
    struct inode *inode = yaffs_get_inode(node->in_fs, d_obj);
    if(!inode)
      return -E_NO_MEM;
    //vop_ref_inc(inode);
    *endp = path;
    *node_store = inode;
    return 0;
  }
  return -E_NOENT;
}

int yaffs_vop_gettype(struct inode *node, uint32_t *type_store)
{
  struct yaffs_obj *obj;
  obj = yaffs_inode_to_obj(node);
  assert(obj);
  switch (obj->variant_type) {
    case YAFFS_OBJECT_TYPE_DIRECTORY:
      *type_store = S_IFDIR;
      return 0;
    case YAFFS_OBJECT_TYPE_FILE:
      *type_store = S_IFREG;
      return 0;
    case YAFFS_OBJECT_TYPE_SYMLINK:
      *type_store = S_IFLNK;
      return 0;
    default:
      break;
  } 
  panic("invalid file type %d.\n", obj->variant_type);
  return -E_INVAL;
}


int yaffs_vop_namefile(struct inode *node, struct iobuf *iob)
{
  //struct yaffs2_fs *yfs = fsop_info(vop_fs(node), yaffs2);
  //struct yaffs2_inode *yin = vop_info(node, yaffs2_inode);

  char *ptr = iob->io_base + iob->io_resid;
  size_t alen, resid = iob->io_resid - 2;

  struct yaffs_obj *d_obj = yaffs_inode_to_obj(node);
	//struct yaffs_obj *parent_obj;
  if(iob->io_resid <= 2)
     return -E_NO_MEM;

  while (d_obj->obj_id != YAFFS_OBJECTID_ROOT) {
      
    int name_len = yaffs_get_obj_name(d_obj,yaffs_get_namebuf(), FS_MAX_FNAME_LEN );

    if ((alen = name_len + 1) > resid) {
      return -E_NO_MEM;
    }
    resid -= alen, ptr -= alen;
    memcpy(ptr, yaffs_get_namebuf(), alen - 1);
    ptr[alen - 1] = '/';

    d_obj = d_obj->parent;
  }

  alen = iob->io_resid - resid - 2;
  ptr = memmove(iob->io_base + 1, ptr, alen);
  ptr[-1] = '/', ptr[alen] = '\0';
  iobuf_skip(iob, alen);

  return 0;
  //kprintf("TODO vop_namefile\n");
}

static int
yaffs_vop_opendir(struct inode *node, uint32_t open_flags) {
    switch (open_flags & O_ACCMODE) {
    case O_RDONLY:
        break;
    case O_WRONLY:
    case O_RDWR:
    default:
        return -E_ISDIR;
    }
    if (open_flags & O_APPEND) {
        return -E_ISDIR;
    }
    return 0;
}

static int
yaffs_vop_close(struct inode *node) {
    return vop_fsync(node);
}



static int
yaffs_vop_fsync(struct inode *node)
{
  yaffs_trace(YAFFS_TRACE_OS | YAFFS_TRACE_SYNC, "yaffs_sync_object");
  struct yaffs_obj *obj = yaffs_inode_to_obj(node);
	int ret = yaffs_flush_file(obj, 1, 1);
  return (ret==YAFFS_OK)?0:-E_INVAL;
}

static int
yaffs_vop_fstat(struct inode *node, struct stat *stat) {
    int retVal = -1;
    memset(stat, 0, sizeof(struct stat));
    struct yaffs_obj *obj = yaffs_inode_to_obj(node);
    obj = yaffs_get_equivalent_obj(obj);

    if ((retVal = vop_gettype(node, &(stat->st_mode))) != 0) {
        return retVal;
    }
    stat->st_nlinks = yaffs_get_obj_link_count(obj);
    unsigned int blksize  = obj->my_dev->data_bytes_per_chunk;
	  stat->st_size = yaffs_get_obj_length(obj);
	  stat->st_blocks = (stat->st_size + blksize -1)/blksize;

    return 0;
}




static int
yaffs_vop_getdirentry(struct inode *node, struct iobuf *iob) 
{
  off_t offset = iob->io_offset;
  struct yaffs_obj *obj = yaffs_inode_to_obj(node);
  assert(obj);
  if(obj->variant_type != YAFFS_OBJECT_TYPE_DIRECTORY){
    return -E_NOTDIR;
  }
  int ret, slot = offset / (FS_MAX_FNAME_LEN);
  switch (slot) {
    case 0:
      strcpy(yaffs_get_namebuf(), ".");
      break;
    case 1:
      strcpy(yaffs_get_namebuf(), "..");
      break;
    default:
      {
        struct yaffs_obj *item = yaffs_helper_get_nth_direntry(obj, slot-2);
        if(!item) return -E_NOENT;
        yaffs_get_obj_name(item, yaffs_get_namebuf(), FS_MAX_FNAME_LEN);
      }
  }
  ret = iobuf_move(iob, yaffs_get_namebuf(), FS_MAX_FNAME_LEN+1, 1, NULL);
  return ret;
}

static int
yaffs_vop_rename(struct inode *node, const char *name, struct inode *new_node, const char *new_name) {
  if (strlen(name) > FS_MAX_FNAME_LEN || strlen(new_name) > FS_MAX_FNAME_LEN ) {
    return -E_TOO_BIG;
  }
  if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
    return -E_INVAL;
  }
  if (strcmp(new_name, ".") == 0 || strcmp(new_name, "..") == 0) {
    return -E_EXISTS;
  }
  struct yaffs_obj *olddir = yaffs_inode_to_obj(node);
  struct yaffs_obj *newdir = yaffs_inode_to_obj(new_node);
  if(olddir->variant_type != YAFFS_OBJECT_TYPE_DIRECTORY 
      || newdir->variant_type != YAFFS_OBJECT_TYPE_DIRECTORY)
    return -E_NOTDIR;

  int ret = yaffs_rename_obj(olddir, name, newdir, new_name);

  return (ret==YAFFS_OK)?0:-E_INVAL;
}

static int
yaffs_vop_mkdir(struct inode *node, const char *name) {
    if (strlen(name) > FS_MAX_FNAME_LEN) {
        return -E_TOO_BIG;
    }
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return -E_EXISTS;
    }
    struct yaffs_obj *parent = yaffs_inode_to_obj(node);
		struct yaffs_obj * dir = yaffs_create_dir(parent,name,S_IWRITE|S_IREAD|S_IEXEC,0,0);
		if(dir)
      return 0;
		else if (yaffs_find_by_name(parent,name))
			return -E_EXISTS; /* the name already exists */
		else
			return -E_NO_MEM; /* just assume no space */

}

static int
yaffs_vop_create(struct inode *node, const char *name, bool excl, struct inode **node_store) {
    if (strlen(name) > FS_MAX_FNAME_LEN) {
        return -E_TOO_BIG;
    }
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return -E_EXISTS;
    }
    struct yaffs_obj *parent = yaffs_inode_to_obj(node);
    struct yaffs_obj *obj;
    parent = yaffs_get_equivalent_obj(parent);
    if ( parent->variant_type != YAFFS_OBJECT_TYPE_DIRECTORY)
      return -E_NOTDIR;
    if ( (obj = yaffs_find_by_name(parent, name)) != NULL){
      if (!excl) {
        if (obj->variant_type == YAFFS_OBJECT_TYPE_FILE) {
          goto out;
        }
      }
      return -E_EXISTS;
    }else{ //crate a file
      obj = yaffs_create_file(parent, name, S_IWRITE|S_IREAD|S_IEXEC,0,0);
      if(!obj)
        return -E_EXISTS;
    }
out:
    *node_store =  yaffs_get_inode(node->in_fs, obj);
    return 0;
}

static int
yaffs_vop_unlink(struct inode *node, const char *name) {
    if (strlen(name) > FS_MAX_FNAME_LEN) {
        return -E_TOO_BIG;
    }
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return -E_ISDIR;
    }
    struct yaffs_obj *parent = yaffs_inode_to_obj(node);
    parent = yaffs_get_equivalent_obj(parent);
    if ( parent->variant_type != YAFFS_OBJECT_TYPE_DIRECTORY)
      return -E_NOTDIR;

    if(!yaffs_find_by_name(parent,name))
      return -E_NOENT;
    int ret = yaffs_unlinker(parent,name);

    return (ret==YAFFS_OK)?0:-E_NOTEMPTY;
}

/* file operators */

static int
yaffs_vop_openfile(struct inode *node, uint32_t open_flags) {
    return 0;
}

static int
yaffs_vop_truncfile(struct inode *node, off_t len)
{
  if(len>=YAFFS_MAX_FILE_SIZE || len<0)
    return -E_INVAL;
  struct yaffs_obj *obj = yaffs_inode_to_obj(node);
  if(obj->variant_type != YAFFS_OBJECT_TYPE_FILE)
    return -E_INVAL;
  int ret = yaffs_resize_file(obj, len);
  return (ret==YAFFS_OK)?0:-E_INVAL;
}

static int
yaffs_vop_tryseek(struct inode *node, off_t pos) {
    if (pos < 0 || pos>=YAFFS_MAX_FILE_SIZE) {
        return -E_INVAL;
    }
    struct yaffs_obj *obj = yaffs_inode_to_obj(node);
    if (pos > yaffs_get_obj_length(obj)) {
        return vop_truncate(node, pos);
    }
    return 0;
}

static int
yaffs_vop_read(struct inode *node, struct iobuf *iob) {
  size_t alen = iob->io_resid;
  off_t  offset = iob->io_offset;
  if(alen > YAFFS_MAX_FILE_SIZE){
    return -E_INVAL;
  }
  struct yaffs_obj *obj = yaffs_inode_to_obj(node);
  
  loff_t maxRead ; 
  if(yaffs_get_obj_length(obj) > offset)
    maxRead = yaffs_get_obj_length(obj) - offset;
  else
    maxRead = 0;
  if(alen > maxRead)
			alen = maxRead;
  int nRead = yaffs_file_rd(obj, iob->io_base, offset, alen);
  if (nRead > 0) {
    iobuf_skip(iob, nRead);
  } 
  return 0;
}

static int
yaffs_vop_write(struct inode *node, struct iobuf *iob) {
  size_t alen = iob->io_resid;
  off_t  offset = iob->io_offset;
  if(alen > YAFFS_MAX_FILE_SIZE){
    return -E_INVAL;
  }
  if( offset<0 || offset > YAFFS_MAX_FILE_SIZE)
    return -E_INVAL;
  struct yaffs_obj *obj = yaffs_inode_to_obj(node);
  int nWritten = yaffs_wr_file(obj, iob->io_base, offset , alen,0);
  if(nWritten > 0)
    iobuf_skip(iob, nWritten);
  return 0;
}

static const struct inode_ops yaffs_node_dirops = {
  .vop_magic                      = VOP_MAGIC,
  .vop_open                       = yaffs_vop_opendir,
  .vop_close                      = yaffs_vop_close,
  .vop_read                       = NULL_VOP_ISDIR,
  .vop_write                      = NULL_VOP_ISDIR,
  .vop_fstat                      = yaffs_vop_fstat,
  .vop_fsync                      = yaffs_vop_fsync,
  .vop_mkdir                      = yaffs_vop_mkdir,
  .vop_link                       = NULL_VOP_UNIMP,
  .vop_rename                     = yaffs_vop_rename,
  .vop_readlink                   = NULL_VOP_ISDIR,
  .vop_symlink                    = NULL_VOP_UNIMP,
  .vop_namefile                   = yaffs_vop_namefile,
  .vop_getdirentry                = yaffs_vop_getdirentry,
  .vop_reclaim                    = yaffs_vop_reclaim,
  .vop_ioctl                      = NULL_VOP_INVAL,
  .vop_gettype                    = yaffs_vop_gettype,
  .vop_tryseek                    = NULL_VOP_ISDIR,
  .vop_truncate                   = NULL_VOP_ISDIR,
  .vop_create                     = yaffs_vop_create,
  .vop_unlink                     = yaffs_vop_unlink,
  .vop_lookup                     = yaffs_vop_lookup,
  .vop_lookup_parent              = yaffs_vop_lookup_parent,
};

static const struct inode_ops yaffs_node_fileops = {
  .vop_magic                      = VOP_MAGIC,
  .vop_open                       = yaffs_vop_openfile,
  .vop_close                      = yaffs_vop_close,
  .vop_read                       = yaffs_vop_read,
  .vop_write                      = yaffs_vop_write,
  .vop_fstat                      = yaffs_vop_fstat,
  .vop_fsync                      = yaffs_vop_fsync,
  .vop_mkdir                      = NULL_VOP_NOTDIR,
  .vop_link                       = NULL_VOP_NOTDIR,
  .vop_rename                     = yaffs_vop_rename,
  .vop_readlink                   = NULL_VOP_NOTDIR,
  .vop_symlink                    = NULL_VOP_NOTDIR,
  .vop_namefile                   = NULL_VOP_NOTDIR,
  .vop_getdirentry                = NULL_VOP_NOTDIR,
  .vop_reclaim                    = yaffs_vop_reclaim,
  .vop_ioctl                      = NULL_VOP_INVAL,
  .vop_gettype                    = yaffs_vop_gettype,
  .vop_tryseek                    = yaffs_vop_tryseek,
  .vop_truncate                   = yaffs_vop_truncfile,
  .vop_create                     = NULL_VOP_NOTDIR,
  .vop_unlink                     = NULL_VOP_NOTDIR,
  .vop_lookup                     = NULL_VOP_NOTDIR,
  .vop_lookup_parent              = NULL_VOP_NOTDIR,
};
