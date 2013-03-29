/*
 * =====================================================================================
 *
 *       Filename:  uaccess_glue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/21/2012 10:14:39 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <vmm.h>
#include <proc.h>

unsigned long __ucore_copy_to_user(void *to, const void *from, unsigned long n)
{
	int ret = 0;
	struct mm_struct *mm = current->mm;
	lock_mm(mm);
	ret = copy_to_user(mm, to, from, n);
	unlock_mm(mm);
	if (ret)
		return 0;
	return n;
}

unsigned long __ucore_copy_from_user(void *to, const void *from,
				     unsigned long n)
{
	int ret = 0;
	struct mm_struct *mm = current->mm;
	lock_mm(mm);
	ret = copy_from_user(mm, to, from, n, 0);
	unlock_mm(mm);
	if (ret)
		return 0;
	return n;
}

int __put_user_1(void *p, unsigned int x)
{
	return __ucore_copy_to_user(p, &x, 1);
}

int __put_user_2(void *p, unsigned int x)
{
	return __ucore_copy_to_user(p, &x, 2);
}

int __put_user_4(void *p, unsigned int x)
{
	return __ucore_copy_to_user(p, &x, 4);
}

int __put_user_8(void *p, unsigned long long x)
{
	return __ucore_copy_to_user(p, &x, 8);
}

int __get_user_1(void *p)
{
	unsigned char v = 0;
	__ucore_copy_from_user(&v, p, 1);
	return v;
}

int __get_user_2(void *p)
{
	unsigned short v = 0;
	__ucore_copy_from_user(&v, p, 2);
	return v;
}

int __get_user_4(void *p)
{
	unsigned int v = 0;
	__ucore_copy_from_user(&v, p, 4);
	return v;
}
