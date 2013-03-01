/*
 * =====================================================================================
 *
 *       Filename:  uaccess_glue.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/21/2012 10:11:46 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef UACCESS_GLUE_H
#define UACCESS_GLUE_H

extern unsigned long __ucore_copy_to_user(void *to, const void *from,
					  unsigned long n);

extern unsigned long __ucore_copy_from_user(void *to, const void *from,
					    unsigned long n);

#ifndef copy_to_user
#define copy_to_user __ucore_copy_to_user

#define copy_from_user __ucore_copy_from_user

#endif

#endif
