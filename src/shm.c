#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      /* shm_open 的标志位 */
#include <sys/mman.h>   /* mmap / munmap */
#include <sys/stat.h>   /* mode 常量 */
#include <unistd.h>
#include "shm.h"

/* 创建并初始化共享内存 */
system_status_t *shm_create(void)
{
    int fd;
    system_status_t *ptr;

    /* 打开或创建共享内存对象 */
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        perror("shm_open create failed");
        return NULL;
    }

    /* 设置共享内存大小 */
    if (ftruncate(fd, sizeof(system_status_t)) == -1)
    {
        perror("ftruncate failed");
        close(fd);
        return NULL;
    }

    /* 映射到本进程地址空间 */
    ptr = (system_status_t *)mmap(NULL,
                                  sizeof(system_status_t),
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED,
                                  fd,
                                  0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap failed");
        close(fd);
        return NULL;
    }

    /* fd 已经不再需要，映射建立后可关闭 */
    close(fd);

    /* 初始化共享数据 */
    memset(ptr, 0, sizeof(system_status_t));
    ptr->running = 1;

    return ptr;
}

/* 打开已有共享内存 */
system_status_t *shm_open_existing(void)
{
    int fd;
    system_status_t *ptr;

    /* 打开已经存在的共享内存 */
    fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1)
    {
        perror("shm_open existing failed");
        return NULL;
    }

    /* 映射到当前进程地址空间 */
    ptr = (system_status_t *)mmap(NULL,
                                  sizeof(system_status_t),
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED,
                                  fd,
                                  0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap failed");
        close(fd);
        return NULL;
    }

    close(fd);
    return ptr;
}

/* 解除映射 */
int shm_detach(system_status_t *ptr)
{
    if (ptr == NULL)
    {
        return -1;
    }

    return munmap(ptr, sizeof(system_status_t));
}

/* 删除共享内存对象 */
int shm_remove(void)
{
    return shm_unlink(SHM_NAME);
}