#ifndef SEM_H
#define SEM_H

#include <semaphore.h>

/* 创建命名信号量 */
sem_t *sem_create(void);

/* 打开已经存在的命名信号量 */
sem_t *sem_open_existing(void);

/* P 操作：加锁 */
int sem_lock(sem_t *sem);

/* V 操作：解锁 */
int sem_unlock(sem_t *sem);

/* 关闭当前进程中的信号量句柄 */
int sem_close_handle(sem_t *sem);

/* 删除命名信号量 */
int sem_remove(void);

#endif