#ifndef SHM_H
#define SHM_H

#include "common.h"

system_status_t *shm_create();
system_status_t *shm_open_existing();
int shm_detach(system_status_t *ptr);
int shm_remove(void);

#endif