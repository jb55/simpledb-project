/*
 * semaphore.c - a wrapper for linux semaphores
 */

#include "semaphore.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

struct sembuf sem_operations[] =
{
    {0, -1, SEM_UNDO|IPC_NOWAIT},
    {0, 1, SEM_UNDO|IPC_NOWAIT}
};

int semaphore_create(char* path_key, semaphore *sem)
{
    unsigned short init_val[1] = {1};
    key_t key;
    int id;

    key = ftok(path_key, 'S');
    if( key == (key_t)-1 )
        return SEM_CREATE_ERROR;

    id = semget(key, 1, IPC_CREAT | 0666);

    if( id == -1 )
        return SEM_CREATE_ERROR;

    if(semctl(id, 0, SETALL, init_val) == -1)
        return SEM_CREATE_ERROR;

    *sem = id;

    return SEM_OK;
}


int semaphore_lock(semaphore sem)
{
    int res;
    res = semop(sem, &sem_operations[SEM_LOCK], 1);

    if( res < 0 )
        return SEM_LOCKED;

    return SEM_OK;
}


int semaphore_unlock(semaphore sem)
{
    int res;
    res = semop(sem, &sem_operations[SEM_UNLOCK], 1);

    if( res < 0 )
        return SEM_NOT_LOCKED;

    return SEM_OK;
}


int semaphore_destroy(semaphore sem)
{
    int res;
    res = semctl(sem, 0, IPC_RMID, 0);

    if( res < 0 )
        return SEM_DESTROY_ERROR;

    return SEM_OK;
}




