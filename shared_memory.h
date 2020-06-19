/// @file shared_memory.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#pragma once
#include <stdlib.h>

/**
 * alloc_shared_memory: crea, se non esiste, un segmento di memoria condivisa di size bytes e chiave shmKey
 * @param shmKey: chiave del segmento di memoria condivisa
 * @param size: grandezza del segmento di memoria
 * @return: shmid se ha successo, altrimenti termina il processo chiamante
 */
int allocSharedMemory(key_t shmKey, size_t size);

/**
 * get_shared_memory: attacca il segmento di memoria condivisa allo spazio virtuale di indirizzamento del processo chiamante
 * @param shmid: id del segmento di memoria condivisa
 * @param shmflg: flags
 * @return: puntatore all'indirizzo al quale la memoria condivisa è stata attaccata con successo,
 * altrimenti termina il processo chiamante
 */
void *getSharedMemory(int shmid, int shmflg);

/**
 * free_shared_memory: DETACH il segmento di memoria condivisa dallo spazio virtuale di indirizzamento del processo chiamante
 * @param prt_sh: puntatore al segmento di memoria
 * Se non ha successo, termina il processo chiamante
 */
void freeSharedMemory(void *prt_sh);

/**
 * remove_shared_memory: rimuove un segmento di memoria condivisa
 * @param shmid: id segmento di memoria
 * Se non ha successo, termina il processo chiamante
 */
void removeSharedMemory(int shmid);