/*
 * =====================================================================================
 *
 *       Filename:  yaffs_vfs.h
 *
 *    Description:  yaffs vfs interface
 *
 *        Version:  1.0
 *        Created:  04/10/2012 08:16:30 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#ifndef __KERN_FS_YAFFS_VFS_H
#define __KERN_FS_YAFFS_VFS_H

#include <types.h>
#include <mmu.h>
#include <sem.h>
#include <atomic.h>
#include <unistd.h>

struct fs;
struct inode;

struct yaffs_obj;
struct yaffs_dev;

/* filesystem for yaffs*/
struct yaffs2_fs {
    struct device *dev;                             /* device mounted on */
    struct yaffs_dev *ydev;
    char yaffs_name_buf[FS_MAX_FNAME_LEN+1];
};

struct yaffs2_inode{
  int type;
  struct yaffs_obj * obj;
};


int yaffs_vfs_mount(const char *devname);

void yaffs_vfs_init(void);

#endif
