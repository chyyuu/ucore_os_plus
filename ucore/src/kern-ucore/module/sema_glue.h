/*
 * =====================================================================================
 *
 *       Filename:  sema_glue.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/21/2012 12:43:50 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef SEMA_GLUE_H
#define SEMA_GLUE_H

/*TODO dirty hack, sizeof(sem) in ucore */
struct semaphore {
	volatile int __dummy[5];
};

extern void sem_init(struct semaphore *sem, int val);
#define sema_init sem_init

#define init_MUTEX(sem)		sema_init(sem, 1)
#define init_MUTEX_LOCKED(sem)	sema_init(sem, 0)

extern void down(struct semaphore *sem);
extern int __must_check down_interruptible(struct semaphore *sem);
extern int __must_check down_killable(struct semaphore *sem);
extern int __must_check down_trylock(struct semaphore *sem);
extern int __must_check down_timeout(struct semaphore *sem, long jiffies);
extern void up(struct semaphore *sem);

#endif
