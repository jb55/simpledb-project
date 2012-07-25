#ifndef _H_SDBSEMAPHORE_
#define _H_SDBSEMAPHORE_

/* linux */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

enum e_semops
{
    SEM_LOCK,
    SEM_UNLOCK
};

enum e_semerr
{
    SEM_OK,
    SEM_CREATE_ERROR,
    SEM_DESTROY_ERROR,
    SEM_LOCKED,
    SEM_NOT_LOCKED
};

typedef int semaphore;
/*struct semaphore
{
    int id;
};*/

int semaphore_create(char* path_key, semaphore *sem);
int semaphore_destroy(semaphore sem);
int semaphore_lock(semaphore sem);
int semaphore_unlock(semaphore sem);

#endif

