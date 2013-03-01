/*
 * =====================================================================================
 *
 *       Filename:  module_exporter.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/22/2012 04:14:23 PM
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

#include "uaccess_glue.h"

EXPORT_SYMBOL(__kmalloc);
EXPORT_SYMBOL(krealloc);
EXPORT_SYMBOL(__memzero);
EXPORT_SYMBOL(printk);
