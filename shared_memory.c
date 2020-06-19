/// @file shared_memory.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#include "err_exit.h"
#include "shared_memory.h"
#include "sys/shm.h"

int allocSharedMemory(key_t shm_key, size_t size)
{
    int shmid = shmget(shm_key, size, IPC_CREAT | 0666);
    if (shmid == -1)
    {
        ErrExit("shmget failed");
    }
    return shmid;
}

void *getSharedMemory(int shmid, int shmflg)
{
    void *ptr_sh = shmat(shmid, NULL, shmflg);
    if (ptr_sh == (void *)-1)
    {
        ErrExit("shmat failed");
    }
    return ptr_sh;
}

void freeSharedMemory(void *ptr_sh)
{
    if (shmdt(ptr_sh) == -1)
    {
        ErrExit("shmdt failed");
    }
}

void removeSharedMemory(int shmid)
{
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        ErrExit("shmctl failed");
}