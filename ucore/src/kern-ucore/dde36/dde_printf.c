/*
 * =====================================================================================
 *
 *       Filename:  dde_printf.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/28/2012 04:32:58 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <kio.h>
#include <dde_kit/printf.h>

void dde_kit_print(const char *msg)
{
  kprintf("%s", msg);
}

int dde_kit_printf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int cnt = vkprintf(fmt, ap);
  va_end(ap);
  return cnt;
}

int dde_kit_vprintf(const char *fmt, va_list va)
{
  return vkprintf(fmt, va);
}

