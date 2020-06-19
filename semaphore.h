/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once

// definizione di union semun
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

/**
 * semOp: funzione di support per manupolare i valori dei semafori di un set di semafori
 * @param semid: semaphore set identifier
 * @param sem_num: indice del semaforo nel set
 * @param sem_op: operazione da eseguire sul semaforo semn_num
 */
void semOp(int semid, unsigned short sem_num, short sem_op);

/**
 * removeSemaphoreSet: funzione di support per eliminare tutti i semafori di un set di semafori
 * @param semid: semaphore set identifier
 * @param sem_num: indice del semaforo nel set
 * @param sem_op: operazione da eseguire sul semaforo semn_num
 */
void removeSemaphoreSet(int semid_scacchiera, int semid_acknowledgments, int semid_device, int semid_listadevice);

int initSemScacchiera();
int initSemAcknowledgments();
int initSemDevice();
int initSemListaDevice();