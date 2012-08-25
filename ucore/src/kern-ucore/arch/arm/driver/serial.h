/*
 * =====================================================================================
 *
 *       Filename:  serial.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/17/2012 03:04:14 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef __MACH_SERIAL_H
#define __MACH_SERIAL_H

int serial_check();
void serial_putc(int c);
int serial_proc_data();
void serial_clear();

#endif
