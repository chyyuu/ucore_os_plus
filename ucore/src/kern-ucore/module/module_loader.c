/*
 * =====================================================================================
 *
 *       Filename:  module_loader.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/22/2012 06:08:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/err.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/wait.h>

#define __NO_UCORE_TYPE__
#include <module.h>
