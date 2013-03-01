/*
 * =====================================================================================
 *
 *       Filename:  ramdisk.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/26/2012 03:35:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <board.h>
#include <ide.h>

/* defined in ldscript */
extern char initrd_begin[], initrd_end[];

bool check_initrd();

#define CHECK_INITRD_EXIST() (initrd_end != initrd_begin)
#define INITRD_SIZE() (initrd_end-initrd_begin)

void ramdisk_init_struct(struct ide_device *dev);
