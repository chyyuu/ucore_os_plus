/*
 * =====================================================================================
 *
 *       Filename:  dde_panic.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/28/2012 05:07:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <kio.h>

#include <dde_kit/panic.h>

void dde_kit_panic(const char *fmt, ...)
{
  intr_disable();
  va_list ap;
  va_start(ap, fmt);
  kprintf("kernel panic in DDE:\n    ");
  vkprintf(fmt, ap);
  kprintf("\n");
  va_end(ap);
}

void dde_kit_debug(const char *fmt, ...)
{
  intr_disable();
  va_list ap;
  va_start(ap, fmt);
  kprintf("debug in DDE:\n    ");
  vkprintf(fmt, ap);
  kprintf("\n");
  va_end(ap);
}
