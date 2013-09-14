/*
 * =====================================================================================
 *
 *       Filename:  dde_dde_kit.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/29/2012 01:55:03 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <dde_kit/printf.h>
#include <dde_kit/dde_kit.h>

void dde_kit_init(void)
{
  dde_kit_printf("Initializing DDE linux36\n");
  dde_kit_subsys_linux_init();
}
