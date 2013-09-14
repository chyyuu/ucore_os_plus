/*
 * =====================================================================================
 *
 *       Filename:  dde_timer.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/30/2012 10:12:23 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <dde_kit/types.h>
#include <dde_kit/timer.h>

dde_kit_uint64_t jiffies_64;
unsigned long volatile jiffies;

