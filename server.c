/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include "stdio.h"
//#include "stdlib.h"
#include "sys/sem.h"
//#include "sys/types.h"
#include "sys/wait.h"
//#include "string.h"
//#include "sys/stat.h"
#include "unistd.h"
//#include "fcntl.h"

#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "err_exit.h"

#define KGRN "\x1B[32m"
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
//#define ROW_BYTE 20

int semid_scacchiera = -1;
int semid_acknowledgments = -1;
int semid_device = -1;
int semid_listadevice = -1;

int shmid_scacchiera;
int shmid_acknoledgments;
int shmid_listadevice;

// The signal handler that will be used when the signal SIGALRM
void sigHandler(int sig)
{
    if (sig == SIGTERM)
    {
        if (killpg(getpgrp(), SIGTERM) == -1)
        {
            ErrExit("kill failed");
        }

        // Lato server rimuovo tutti i semafori e le memorie condivise
        printf("%s<server pid: %d>\t\tChiusura e rimozione semafori...\t\t", KRED, getpid());
        removeSemaphoreSet(semid_scacchiera, semid_acknowledgments, semid_device, semid_listadevice);
        printf("OK.\n");
        printf("<server pid: %d>\t\tChiusura e rimozione shared memory...\t", getpid());
        removeSharedMemory(shmid_scacchiera);
        removeSharedMemory(shmid_acknoledgments);
        removeSharedMemory(shmid_listadevice);
        printf("OK.\n");
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    // Controllo che il numero dei parametri passati siano corretti
    if (argc != 3)
    {
        printf("Uso: %s msg_queue_key file_posizioni\n", argv[0]);
        exit(1);
    }

    char *path_file_posizioni = argv[0];
    int msgq_key = atoi(argv[1]);
    if (msgq_key <= 0)
    {
        printf("La chiave della message queue deve essere un numero maggiore di zero!\n");
        exit(1);
    }

    // Cambio la mask del signalHandler in modo da eseguire sigHandler
    printf("%s\n------------------------------------------------------------------------------\n", KGRN);
    printf("<server>\tCambio signalhandler...\t\t\t\t\t\t\t\t\t");
    sigset_t signals_set;
    if (sigfillset(&signals_set) == -1)
    {
        ErrExit("<server> sigfillset fallita");
    }

    sigdelset(&signals_set, SIGTERM);

    //blocca tutto tranne SIGTERM
    if (sigprocmask(SIG_SETMASK, &signals_set, NULL) == -1)
    {
        ErrExit("<server> sigprocmask fallito");
    }

    if (signal(SIGTERM, sigHandler) == SIG_ERR)
    {
        ErrExit("<server> change signalhandler fallito");
    }
    printf("OK.\n");

    // Creo e inizializzo il semaforo per la scacchiera
    semid_scacchiera = initSemScacchiera();

    // Creo e inizializzo il semaforo per gli ack
    semid_acknowledgments = initSemAcknowledgments();

    // Creo e inizializzo il semaforo per i devices
    semid_device = initSemDevice();

    // Creo e inizializzo il semaforo per la lista dei devie
    semid_listadevice = initSemListaDevice();

    // Inizializzo la memoria condivisa
    printf("<server>\tInizializzazione shared memory...\t\t\t\t\t\t");
    shmid_scacchiera = allocSharedMemory(IPC_PRIVATE, sizeof(pid_t) * 10 * 10);
    shmid_acknoledgments = allocSharedMemory(IPC_PRIVATE, sizeof(Acknowledgment) * 100);
    shmid_listadevice = allocSharedMemory(IPC_PRIVATE, sizeof(pid_t) * 5);
    printf("OK.\n");
    printf("------------------------------------------------------------------------------\n\n");

    // Creo i 5 processi per i device
    pid_t pid;
    for (int n_device = 0; n_device < 5; ++n_device)
    {
        pid = fork();
        if (pid == -1)
        {
            printf("Device %d non creato!", n_device);
            exit(0);
        }
        else if (pid == 0)
        {
            // Se il processo è il figlio, inizializzo il device
            eseguiDevice(n_device, semid_scacchiera, semid_acknowledgments, semid_device, semid_listadevice, shmid_scacchiera, shmid_acknoledgments, shmid_listadevice, path_file_posizioni);
        }
    }

    // Creo un nuovo processo per l'ackmanager
    pid_t pid_ackmanager = fork();

    if (pid_ackmanager == -1)
    {
        ErrExit("Ack manager non creato!");
    }
    else if (pid_ackmanager == 0)
    {
        // Se il processo è il figlio, inizializzo l'ackmanager
        eseguiAckmanager(shmid_acknoledgments, msgq_key, semid_acknowledgments);
    }

    int i = 0;
    while (1)
    {
        // Sleep, incremento e stampa, altrimenti il print viene sbagliato
        sleep(2);
        i++;
        // Sblocco la board
        semOp(semid_scacchiera, 0, -1);
        printf("%s\n# Step %d: device positions ########################\n", KNRM, i);
    }
}