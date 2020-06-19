/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "stdio.h"
#include "fcntl.h"
#include "err_exit.h"
#include "semaphore.h"
#include "sys/stat.h"
#include "sys/sem.h"

void semOp(int semid, unsigned short sem_num, short sem_op)
{
    struct sembuf semaphore_op = {
        .sem_num = sem_num,
        .sem_op = sem_op,
        .sem_flg = 0
    };

    if (semop(semid, &semaphore_op, 1) == -1) {
        ErrExit("semop fallita");
    }
}

void removeSemaphoreSet(int semid_scacchiera, int semid_acknowledgments, int semid_device, int semid_listadevice)
{
    if (semctl(semid_scacchiera, 0, IPC_RMID, NULL) == -1)
    {
        ErrExit("semctl IPC_RMID semaforo accesso scacchiera fallito");
    }
    if (semctl(semid_acknowledgments, 0, IPC_RMID, NULL) == -1)
    {
        ErrExit("semctl IPC_RMID semaforo lista acknowledgments fallito");
    }
    if (semctl(semid_device, 0, IPC_RMID, NULL) == -1)
    {
        ErrExit("semctl IPC_RMID semaforo device fallito");
    }
    if (semctl(semid_listadevice, 0, IPC_RMID, NULL) == -1)
    {
        ErrExit("semctl IPC_RMID semaforo listadevice fallito");
    }
}

// 1 -> bloccato, 0 -> sbloccato
int initSemScacchiera(){
    printf("<server>\tCreazione e inizializazione semaforo scacchiera...\t\t");
    int semid_scacchiera = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if (semid_scacchiera == -1)
    {
        ErrExit("<server> semget semid_scacchiera fallita");
    }

    unsigned short semInitVal_scacchiera[] = {1};
    union semun arg_scacchiera;
    arg_scacchiera.array = semInitVal_scacchiera;
    if (semctl(semid_scacchiera, 0, SETALL, arg_scacchiera) == -1)
    {
        ErrExit("<server>semctl SETALL semid_scacchiera fallita");
    }
    printf("OK.\n");
    return semid_scacchiera;
}

// 1 -> bloccato, 0 -> sbloccato
int initSemAcknowledgments(){
    printf("<server>\tCreazione e inizializazione semaforo acknowledgments...\t");
    int semid_acknowledgments = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if (semid_acknowledgments == -1)
    {
        ErrExit("<server> semget semid_acknowledgments fallita");
    }
    // Semaforo ack inizialmente bloccato
    unsigned short semInitVal_ack[] = {1};
    union semun arg_ack;
    arg_ack.array = semInitVal_ack;
    if (semctl(semid_acknowledgments, 0, SETALL, arg_ack) == -1)
    {
        ErrExit("<server> semctl SETALL semid_acknowledgments fallita");
    }
    printf("OK.\n");
    return semid_acknowledgments;
}

int initSemDevice(){
    printf("<server>\tCreazione e inizializazione 5 semafori device...\t\t");
    int semid_device = semget(IPC_PRIVATE, 5, S_IRUSR | S_IWUSR);
    if (semid_device == -1)
    {
        ErrExit("<server> semget semid_device fallita");
    }
    // Device inizialmente tutti sbloccati tranne il primo
    unsigned short semInitVal_device[] = {1, 0, 0, 0, 0};
    union semun arg_device;
    arg_device.array = semInitVal_device;
    if (semctl(semid_device, 0, SETALL, arg_device) == -1)
    {
        ErrExit("<server> semctl SETALL semid_device fallita");
    }
    printf("OK.\n");
    return semid_device;
}

// 1 -> bloccato, 0 -> sbloccato
int initSemListaDevice(){
    printf("<server>\tCreazione e inizializazione semaforo listadevice...\t\t");
    int semid_listadevice = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if (semid_listadevice == -1)
    {
        ErrExit("<server> semget semid_listadevice fallita");
    }
    unsigned short semInitVal_listadevice[] = {1};
    union semun arg_listadevice;
    arg_listadevice.array = semInitVal_listadevice;
    if (semctl(semid_listadevice, 0, SETALL, arg_listadevice) == -1)
    {
        ErrExit("<server> semctl SETALL semid_listadevice fallita");
    }
    printf("OK.\n");
    return semid_listadevice;
}