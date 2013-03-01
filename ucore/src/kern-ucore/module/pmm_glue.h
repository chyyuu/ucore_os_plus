/*
 * =====================================================================================
 *
 *       Filename:  pmm_glue.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/21/2012 03:36:23 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef PMM_GLUE_H
#define PMM_GLUE_H

#define UCORE_KAP_IO 0x00000001

extern void *ucore_kva_alloc_pages(size_t n, unsigned int flags);

#endif
