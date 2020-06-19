/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

#include <sys/types.h>

typedef struct
{
    pid_t pid_sender;
    pid_t pid_receiver;
    int message_id;
    char message[256];
    double max_distance;
} Message;

typedef struct
{
    pid_t pid_sender;
    pid_t pid_receiver;
    int message_id;
    time_t timestamp;
} Acknowledgment;

typedef struct
{
    long mtype;
    Acknowledgment ack[5];
} Response;

typedef struct {
    int row;
    int col;
} Posizione;

void eseguiDevice(int n_device, int semid_scacchiera, int semid_acknowledgments, int semid_device, int semid_listadevice, int shmid_scacchiera, int shmid_acknoledgments, int shmid_listadevice, const char *path_file_posizioni);
void eseguiAckmanager(int shmid_acknoledgments, int msgq_key, int semid_acknowledgments);
void deviceSigHandler(int sig);
void ackmanagerSigHandler(int sig);
void response2client(int message_id);
int compareAcks(const void *a, const void *b);
void invioELetturaMessaggi(int semid_listadevice, int semid_acknowledgments, Message *buffer, int *num_messaggi, Posizione *posizione);
int ackListContains(int id, Acknowledgment *shm_pointer_acknowledgements, pid_t pid_destinatario);
void procProxRiga(char *next_line);