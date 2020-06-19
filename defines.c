/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "stdio.h"
#include "fcntl.h"
#include "sys/stat.h"
#include "unistd.h"
#include "signal.h"
#include "sys/msg.h"
#include "time.h"
#include "string.h"

#include "defines.h"
#include "semaphore.h"
#include "err_exit.h"
#include "shared_memory.h"
#include "math.h"

#define KRED "\x1B[31m"

pid_t *shm_pointer_scacchiera;
Acknowledgment *shm_pointer_acknowledgements;
pid_t *shm_pointer_listadevice;

int fd_fifo_device;
int fd_fifo;
char path_to_device_fifo[100] = {'\0'};
int fd_posizioni;
int msgq_id;
int id_device;

void eseguiDevice(int n_device, int semid_scacchiera, int semid_acknowledgments, int semid_device, int semid_listadevice, int shmid_scacchiera, int shmid_acknoledgments, int shmid_listadevice, const char *path_file_posizioni)
{
    // mi salvo il numero progressivo di device
    id_device = n_device;

    sigset_t signals_set;
    if (sigfillset(&signals_set) == -1)
    {
        ErrExit("<device> sigfillset fallita");
    }
    sigdelset(&signals_set, SIGTERM);

    if (sigprocmask(SIG_SETMASK, &signals_set, NULL) == -1)
    {
        ErrExit("<device> sigprocmask fallita");
    }

    if (signal(SIGTERM, deviceSigHandler) == SIG_ERR)
    {
        ErrExit("<device> cambio signal handler device fallita");
    }

    shm_pointer_scacchiera = (pid_t *)getSharedMemory(shmid_scacchiera, 0);
    shm_pointer_acknowledgements = (Acknowledgment *)getSharedMemory(shmid_acknoledgments, 0);
    shm_pointer_listadevice = (pid_t *)getSharedMemory(shmid_listadevice, 0);

    semOp(semid_listadevice, 0, -1);
    shm_pointer_listadevice[id_device] = getpid();
    semOp(semid_listadevice, 0, 1);
    // Creo la fifo del device e la apro in lettura non bloccante
    sprintf(path_to_device_fifo, "/tmp/dev_fifo.%d", getpid());
    int res = mkfifo(path_to_device_fifo, S_IRUSR | S_IWUSR);
    if (res == -1)
    {
        ErrExit("<device>Crezione fifo device fallita");
    }
    fd_fifo = open(path_to_device_fifo, O_RDONLY | O_NONBLOCK);
    if (fd_fifo == -1)
    {
        ErrExit("<device>Open fifo device fallita");
    }

    // posizione device
    Posizione posizione = {0, 0};
    char n_line[50];
    Message buffer_mess[10] = {};
    int num_messaggi = 0;
    // Mi prendo le posizioni dei device dal file posizioni
    fd_posizioni = open("input/file_posizioni.txt", O_RDONLY);
    if (fd_posizioni == -1)
    {
        ErrExit("open");
    }
    while (1)
    {
        // Il device aspetta il proprio turno
        semOp(semid_device, (unsigned short)id_device, -1);
        // Il device aspetta che la board sia accessibile
        semOp(semid_scacchiera, 0, 0);

        // Se ha messaggi nel buffer di ricezione prova ad inviarli ai vicini
        invioELetturaMessaggi(semid_listadevice, semid_acknowledgments, buffer_mess, &num_messaggi, &posizione);

        // Si sposta il device
        // Assumo statiche dimensioni matrice scacchiera 10*10
        int old_position_index = posizione.row * 10 + posizione.col;

        // Leggo la prossima riga dal file posizioni
        procProxRiga(n_line);

        // Splitto la lisa con strtok e contando le pipe
        char *row_col = strtok(n_line, "|");
        for (int cont_pipe = 0; cont_pipe < id_device; cont_pipe++)
        {
            row_col = strtok(NULL, "|");
        }

        char *row = strtok(row_col, ",");
        char *col = strtok(NULL, ",");

        posizione.row = atoi(row);
        posizione.col = atoi(col);

        int next_position_index = posizione.row * 10 + posizione.col;

        // se la nuova posizione non è occupata, sposta il device
        if (shm_pointer_scacchiera[next_position_index] == 0)
        {
            // Libero la zona di memoria della scacchiera occupata precedentemente
            shm_pointer_scacchiera[old_position_index] = 0;
            // E occupo quella nuova
            shm_pointer_scacchiera[next_position_index] = getpid();
        }
        else
        {
            // il device resta fermo, e ripristina la sua posizione
            posizione.row = old_position_index / 10;
            posizione.col = old_position_index % 10;
        }

        // Stampo la tabella di debug coda messaggi
        printf("%d %d %d msgs: ", getpid(), posizione.row, posizione.col);
        for (int i = 0; i < num_messaggi; i++)
            printf("%d, ", buffer_mess[i].message_id);
        printf("\n");
        if (id_device == 4)
        {
            printf("####################################################\n");
        }

        // Finisce il turno di quel device
        if (id_device < 4)
        {
            // Sblocca il prossimo device
            semOp(semid_device, (unsigned short)(id_device + 1), 1);
        }
        else
        {
            // Blocca la board
            semOp(semid_scacchiera, 0, 1);
            // Risblocca il primo device
            semOp(semid_device, 0, 1);
        }
    }
}

void eseguiAckmanager(int shmid_acknoledgments, int msgq_key, int semid_acknowledgments)
{
    sigset_t signals_set;
    if (sigfillset(&signals_set) == -1)
    {
        ErrExit("<ackmanager> sigfillset fallita");
    }

    // rimuove SIGTERM
    sigdelset(&signals_set, SIGTERM);

    // blocca tutti i segnali, tranne SIGTERM
    if (sigprocmask(SIG_SETMASK, &signals_set, NULL) == -1)
    {
        ErrExit("<ackmanager> sigprocmask fallita");
    }
    if (signal(SIGTERM, ackmanagerSigHandler) == SIG_ERR)
    {
        ErrExit("<ackmanager> change signal handler fallita");
    }

    shm_pointer_acknowledgements = (Acknowledgment *)getSharedMemory(shmid_acknoledgments, 0);

    msgq_id = msgget(msgq_key, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (msgq_id == -1)
    {
        ErrExit("<ackmanager> get message queue fallita");
    }

    while (1)
    {
        sleep(5);

        // Blocco il semaforo relativo all'ackmanager
        semOp(semid_acknowledgments, 0, -1);

        for (int i = 0; i < 100; i++)
        {
            if (shm_pointer_acknowledgements[i].message_id != 0)
            {
                Acknowledgment ack = shm_pointer_acknowledgements[i];
                char buff[20];
                strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", localtime(&ack.timestamp));

                printf("<ack %d> %d %d %d %s\n", i, ack.pid_sender, ack.pid_receiver, ack.message_id, buff);
            }
        }

        // Eseguo la ackmanager routine
        int ack_vuoti = 0;

        for (int i = 0; i < 100; i++)
        {
            if (shm_pointer_acknowledgements[i].message_id != 0)
            {
                // Conto gli ack nella lista degli Acknowledgment relativi ad un mess id
                int result = 0;
                for (int j = 0; j < 100; j++)
                {
                    if (shm_pointer_acknowledgements[j].message_id == shm_pointer_acknowledgements[i].message_id)
                        result++;
                }
                if (result == 5)
                {
                    // Se ci sono tutti e 5 manda la risposta degli ack al client
                    response2client(shm_pointer_acknowledgements[i].message_id);
                    ack_vuoti += 5;
                }
            } 
            else {
                ack_vuoti++;
            }
        }

        // Se la lista degli ack è piena invio un segnale SIGTERM al server (senno continuerebbe all'infinito)
        if (ack_vuoti == 0)
        {
            printf("%s-----------------------------LISTA ACK PIENA-----------------------------\n", KRED);
            kill(getppid(), SIGTERM);
        }
        semOp(semid_acknowledgments, 0, 1);
    }
}

void deviceSigHandler(int sig)
{
    if (sig == SIGTERM)
    {
        printf("%s<device pid: %d>\t\tRimozione risorse...\t", KRED, getpid());
        freeSharedMemory(shm_pointer_scacchiera);
        freeSharedMemory(shm_pointer_acknowledgements);
        freeSharedMemory(shm_pointer_listadevice);

        if (close(fd_fifo_device) == -1)
        {
            ErrExit("<device> close device fifo fallita");
        }
        if (unlink(path_to_device_fifo) == -1)
        {
            ErrExit("<device> unlink fifo fallita");
        }
        if (close(fd_posizioni) == -1)
        {
            ErrExit("<device> close position file fallita");
        }
        printf("OK\n");
        exit(0);
    }
}

void ackmanagerSigHandler(int sig)
{
    if (sig == SIGTERM)
    {
        printf("%s<ackmanager pid: %d>\tRimozione risorse...\t", KRED, getpid());
        if (msgctl(msgq_id, IPC_RMID, NULL) == -1)
        {
            ErrExit("<ackmanager> remove message queue fallita");
        }
        freeSharedMemory(shm_pointer_acknowledgements);
        printf("  OK\n");
        exit(0);
    }
}

void response2client(int message_id)
{
    int id_acknowledgements = 0;
    Acknowledgment ack_null = {0};

    Response response;
    response.mtype = message_id;
    for (int i = 0; i < 100; i++)
    {
        if (shm_pointer_acknowledgements[i].message_id == message_id)
        {
            response.ack[id_acknowledgements++] = shm_pointer_acknowledgements[i];
            shm_pointer_acknowledgements[i] = ack_null;
        }
    }

    qsort(response.ack, 5, sizeof(Acknowledgment), compareAcks);

    size_t size = sizeof(Response) - sizeof(long);
    if (msgsnd(msgq_id, &response, size, 0) == -1)
    {
        ErrExit("<ackmanager> msgsnd response2client fallita");
    }
}

int compareAcks(const void *a, const void *b)
{
    Acknowledgment *primo_ack = (Acknowledgment *)a;
    Acknowledgment *secondo_ack = (Acknowledgment *)b;

    return (primo_ack->timestamp - secondo_ack->timestamp);
}

void invioELetturaMessaggi(int semid_listadevice, int semid_acknowledgments, Message *buffer_mess, int *num_messaggi, Posizione *posizione)
{
    pid_t pid_prox_device = 0;

    for (int i = 0; i < *num_messaggi; i++)
    {
        semOp(semid_acknowledgments, 0, -1);

        // Controllo se nella shared memory c'è posto per aggiungere un altro ack
        int result = 0;
        for (int i = 0; i < 100 && result == 0; i++)
        {
            if (shm_pointer_acknowledgements[i].message_id == 0)
            {
                result = 1;
            }
        }
        // Se trova uno slot libero, cerca un device a cui inviare il messaggio
        if (result)
        {
            pid_t tmp = 0;
            double distanza_device_pitagora = 0;
            int row_min = 0;
            int col_min = 0;
            int row_max = 0;
            int col_max = 0;

            if((posizione->row - buffer_mess[i].max_distance) > 0){
                row_min = posizione->row - buffer_mess[i].max_distance;
            } 
            else {
                row_min = 0;
            }

            if((posizione->col - buffer_mess[i].max_distance) > 0){
                col_min = posizione->col - buffer_mess[i].max_distance;
            } 
            else {
                col_min = 0;
            }

            if((posizione->row + buffer_mess[i].max_distance + 1) < 10){
                row_max = posizione->col + buffer_mess[i].max_distance + 1;
            } 
            else {
                row_max = 10;
            }

            if((posizione->col + buffer_mess[i].max_distance + 1) < 10){
                col_max = posizione->col + buffer_mess[i].max_distance + 1;
            } 
            else {
                col_max = 10;
            }

            for (int row = row_min; row < row_max && tmp == 0; row++)
            {
                for (int col = col_min; col < col_max && tmp == 0; col++)
                {
                    int offset = row * 10 + col;
                    // Check con distanza_device_pitagora usando pitgora
                    distanza_device_pitagora = (double)((posizione->row - row)*(posizione->row - row)) + ((posizione->col - col)*(posizione->col - col));
                    // Controllo se nella lista degli ack esiste già un certo ack
                    if (shm_pointer_scacchiera[offset] > 0 && shm_pointer_scacchiera[offset] != getpid() && !ackListContains(buffer_mess[i].message_id, shm_pointer_acknowledgements, shm_pointer_scacchiera[offset]) && (sqrt(distanza_device_pitagora) <= buffer_mess[i].max_distance))
                    {
                        tmp = shm_pointer_scacchiera[offset];
                    }
                }
            }
            pid_prox_device = tmp;
        }

        semOp(semid_acknowledgments, 0, 1);

        if (pid_prox_device != 0)
        {
            char fname_fifo_out[100] = {'\0'};
            sprintf(fname_fifo_out, "/tmp/dev_fifo.%d", pid_prox_device);
            int fd_fifo_out = open(fname_fifo_out, O_WRONLY);
            if (fd_fifo_out == -1)
            {
                ErrExit("<device> Open fifo out device fallita");
            }

            // Mi salvo il nuovo messaggio preso dal buffer
            Message messaggio = buffer_mess[i];
            messaggio.pid_sender = getpid();
            messaggio.pid_receiver = pid_prox_device;

            int res = write(fd_fifo_out, &messaggio, sizeof(Message));
            if (res == -1 || res != sizeof(Message))
            {
                ErrExit("write message to fifo fallito");
            }

            // shifto fino a buffer_mess-1 per togliere il messaggio appena inviato
            for (int j = i; j < *num_messaggi && j < 9; j++)
            {
                buffer_mess[j] = buffer_mess[j + 1];
            }
            (*num_messaggi)--;

            if (close(fd_fifo_out) == -1)
            {
                ErrExit("close fifo fallita");
            }
        }
    }

    // Parte di read: se ci sono messaggi, li carica nel buffer di ricezione
    Message messaggio;
    int buffer_reader;
    do
    {
        buffer_reader = -1;
        // blocco la lista contenente gli ack
        semOp(semid_acknowledgments, 0, -1);

        // dimensione massima buffer messaggi impostata staticamente a 100
        int available = 0;
        for (int i = 0; i < 100 && available == 0; i++)
        {
            if (shm_pointer_acknowledgements[i].message_id == 0)
            {
                available = 1;
            }
        }

        // sblocco la lista contenente gli ack
        semOp(semid_acknowledgments, 0, 1);

        if (available)
        {
            buffer_reader = read(fd_fifo, &messaggio, sizeof(Message));
            if (buffer_reader == -1)
            {
                ErrExit("<device> read fifo fallita");
            }
            else if (buffer_reader == sizeof(Message))
            {
                semOp(semid_listadevice, 0, -1); // blocca la lista dei device

                // Controlla se il pid passato è di un device
                int dev = 0;
                for (int i = 0; i < 5 && !dev; i++)
                {
                    if (messaggio.pid_sender != 0 && shm_pointer_listadevice[i] == messaggio.pid_sender)
                    {
                        dev = 1;
                    }
                }
                semOp(semid_listadevice, 0, 1); // sblocca la lista dei device

                semOp(semid_acknowledgments, 0, -1); // blocca la ack list
                if (!dev)
                {
                    // Se il client manda un messaggio che però è già presente nell'ack list, invia un segnale di errore al client
                    int ack_count = 0;
                    for (int i = 0; i < 100; i++)
                    {
                        if (shm_pointer_acknowledgements[i].message_id == messaggio.message_id)
                        {
                            ack_count++;
                        }
                    }
                    if (ack_count > 0)
                    {
                        kill(messaggio.pid_sender, SIGUSR1);

                        semOp(semid_acknowledgments, 0, 1);
                        continue;
                    }
                }

                // Altrimenti aggiunge un nuovo ack relativo al msg alla lista degli ack
                Acknowledgment *new_ack = NULL;
                for (int i = 0; 100 && new_ack == NULL; i++)
                {
                    if (shm_pointer_acknowledgements[i].message_id == 0)
                    {
                        new_ack = &shm_pointer_acknowledgements[i];
                        new_ack->pid_sender = messaggio.pid_sender;
                        new_ack->pid_receiver = messaggio.pid_receiver;
                        new_ack->message_id = messaggio.message_id;
                        new_ack->timestamp = time(NULL);
                        break;
                    }
                }

                // Se non è l'ultimo device, lo salva per inviarlo al prossimo giro, altrimenti inserisce solo l'ack
                // 100 -> size lista ack statico
                int n_ack = 0;
                for (int i = 0; i < 100; i++)
                {
                    if (shm_pointer_acknowledgements[i].message_id == messaggio.message_id)
                    {
                        n_ack++;
                    }
                }
                if (n_ack < 5)
                {
                    buffer_mess[*num_messaggi] = messaggio;
                    (*num_messaggi)++;
                }

                semOp(semid_acknowledgments, 0, 1);
            }
        }
    } while (buffer_reader > 0);
}

int ackListContains(int id, Acknowledgment *shm_pointer_acknowledgements, pid_t pid_destinatario)
{
    int result = 0;
    for (int i = 0; i < 100 && !result; i++)
    {
        if (shm_pointer_acknowledgements[i].pid_receiver == pid_destinatario && shm_pointer_acknowledgements[i].message_id == id)
        {
            result++;
        }
    }
    return result;
}

void procProxRiga(char *next_line)
{
    // contiene la prox riga
    char row[100] = {0};
    char buffer[2] = {0};

    while (read(fd_posizioni, buffer, 1) != 0 && strcmp(buffer, "\n") != 0)
    {
        strcat(row, buffer);
    }

    // Se è finito il file, ricomincia da capo
    if (strlen(row) == 0)
    {
        lseek(fd_posizioni, 0, SEEK_SET);
        procProxRiga(next_line);
    }
    else
    {
        memcpy(next_line, row, strlen(row) + 1);
    }
}