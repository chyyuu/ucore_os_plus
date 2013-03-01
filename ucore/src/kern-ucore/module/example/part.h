/*
 * =====================================================================================
 *
 *       Filename:  part.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/17/2012 06:44:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef __UCORE_IDE_PART_H
#define __UCORE_IDE_PART_H

#include <ide.h>

/* disk/part_dos.c */
int get_partition_info_dos(struct ide_device *dev_desc, int part,
			   ucore_disk_partition_t * info);
void print_part_dos(struct ide_device *dev_desc);
int test_part_dos(struct ide_device *dev_desc);

#endif
