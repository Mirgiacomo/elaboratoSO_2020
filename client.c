/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
//#include "sys/types.h"
#include "sys/msg.h"
#include "sys/stat.h"
#include "unistd.h"
#include "fcntl.h"
#include "errno.h"
#include "signal.h"
#include "time.h"

#include "err_exit.h"
#include "defines.h"

#define PATH_LENGTH 50
#define KGRN "\x1B[32m"

int main(int argc, char *argv[])
{
    // Controllo che il numero dei parametri passati siano corretti
    if (argc != 2)
    {
        printf("Usage: %s msg_queue_key \n", argv[0]);
        exit(1);
    }

    int msq_key = atoi(argv[1]);
    if (msq_key <= 0)
    {
        printf("La chiave della message queue deve essere un numero maggiore di zero!\n");
        exit(1);
    }

    int msq_id = msgget(msq_key, S_IRUSR | S_IWUSR);
    if (msq_id == -1)
    {
        ErrExit("msgget fallita");
    }

    // Costruisco il messaggio
    Message message;
    // Input pid device destinazione
    printf("Inserire pid device a cui inviare il messaggio:\t");
    scanf(" %d", &message.pid_receiver);
    // Input body messaggio
    printf("Inserire id messaggio:\t");
    scanf(" %d", &message.message_id);
    int i;
    do
    {
        i = getchar();
    } while (i != EOF && i != '\n');
    printf("Inserire il messaggio:\t");
    fgets(message.message, sizeof(message.message), stdin);

    // Input max distance con controllo
    do
    {
        printf("Inserire massima distanza comunicazione per il messaggio:\t");
        scanf(" %lf", &message.max_distance);
    } while (message.max_distance < 0);

    // Apro la fifo del relativo device
    char fname[PATH_LENGTH] = {'\0'};
    sprintf(fname, "/tmp/dev_fifo.%d", message.pid_receiver);
    int fd_device = open(fname, O_WRONLY);
    if (fd_device == -1)
    {
        ErrExit("open file fifo device fallita");
    }
    message.pid_sender = getpid();

    // Scrivo sulla fifo
    if (write(fd_device, &message, sizeof(Message)) != sizeof(Message))
    {
        ErrExit("write message fallita");
    }
    //printf("Messaggio %d mandato al device %d.\n", message.message_id, message.pid_receiver);

    // Chiudo al fifo
    if (close(fd_device))
    {
        ErrExit("close device fifo fallita");
    }

    Response response;
    size_t size = sizeof(Response) - sizeof(long);

    if (msgrcv(msq_id, &response, size, message.message_id, 0) == -1)
    {
        ErrExit("msgrcv fallita");
    }

    if (mkdir("out/", S_IRUSR | S_IWUSR | S_IXUSR) == -1 && errno != EEXIST)
    {
        ErrExit("mkdir fallita");
    }

    char fname_out[PATH_LENGTH] = {'\0'};
    sprintf(fname_out, "out/out_%d.txt", message.message_id);
    int fd_fileout = open(fname_out, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd_fileout == -1)
    {
        ErrExit("open file out fallita");
    }

    char buffer_header[sizeof(message.message) + 30];
    sprintf(buffer_header, "%s %d: %s\n", "Messaggio", message.message_id, message.message);
    if (write(fd_fileout, buffer_header, strlen(buffer_header)) == -1)
        ErrExit("write fallita");

    char buffer[100] = "Lista acknowledgements:\n";
    if (write(fd_fileout, buffer, strlen(buffer)) == -1)
        ErrExit("write failed");
    for (int i = 0; i < 5; i++)
    {
        memset(buffer, 0, sizeof(buffer));
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&response.ack[i].timestamp));
        sprintf(buffer, "%d, %d, %s\n", response.ack[i].pid_sender, response.ack[i].pid_receiver, timestamp);

        //printf("%s", buffer);
        if (write(fd_fileout, buffer, strlen(buffer)) == -1)
            ErrExit("write fallita");
    }
    printf("%s-----------------------------INVIATO-----------------------------\n", KGRN);

    if (close(fd_fileout) == -1)
    {
        ErrExit("close file out fallita");
    }

    return 0;
}