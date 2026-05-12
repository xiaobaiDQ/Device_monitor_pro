#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "common.h"
#include "sem.h"

/* 创建命名信号量，初始值设为 1，表示互斥锁 */
sem_t *sem_create(void)
{
    sem_t *sem;

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED)
    {
        perror("sem_open create failed");
        return NULL;
    }

    return sem;
}

/* 打开已经存在的命名信号量 */
sem_t *sem_open_existing(void)
{
    sem_t *sem;

    sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED)
    {
        perror("sem_open existing failed");
        return NULL;
    }

    return sem;
}

/* P 操作：加锁 */
int sem_lock(sem_t *sem)
{
    if (sem == NULL)
    {
        return -1;
    }

    return sem_wait(sem);
}

/* V 操作：解锁 */
int sem_unlock(sem_t *sem)
{
    if (sem == NULL)
    {
        return -1;
    }

    return sem_post(sem);
}

/* 关闭当前进程中的信号量句柄 */
int sem_close_handle(sem_t *sem)
{
    if (sem == NULL)
    {
        return -1;
    }

    return sem_close(sem);
}

/* 删除命名信号量 */
int sem_remove(void)
{
    return sem_unlink(SEM_NAME);
}