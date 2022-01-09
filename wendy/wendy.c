/******************************************************************************/
/* Sistemes Operatius - Pràctica Overlook System - Curs 2020-21               */
/* Marc Legarre Lluch (marc.legarre@students.salle.url.edu)                   */
/* Gerard Pérez Iglesias (gerard.perez@students.salle.url.edu)                */
/******************************************************************************/

#define _GNU_SOURCE
#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 1
//I/O
#include <stdio.h>
//Dinamic memory
#include <stdlib.h>
//STR functions
#include <string.h>
//Modes d'escriptura i lectura (FILEDESCRIPTORS)
#include <fcntl.h>
//Signals
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <wait.h>
/*
//Sockets
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 */
//Threads
#include <pthread.h>

#include <sys/shm.h>

#include "fdreader.h"
#include "network.h"

//Error messages
#define ARG_ERR         "Error: Incorrect arguments number\n"
#define BIND_ERR        "Error: bind\n"

//System messages
#define START           "\nStarting Wendy..."
#define DISCONNECT      "\nDisconnecting Wendy...\n"

#define ORIGIN_BYTES    14
#define TYPE_BYTES      1
#define DATA_BYTES      100
#define TOTAL_BYTES     ORIGIN_BYTES + TYPE_BYTES + DATA_BYTES

//Error messages
#define BIND_ERR        "Error: bind\n"

//System messages
#define WAIT            "\nWaiting..."
#define NEW_CONN        "\nNew Connection: %s"
#define RECEIVE_TXT     "\nReceiving data from %s\n"
#define WENDY            "\n\n$Wendy:"
#define FILE_CHAR   30
#define MD5_SIZE    32

Configuration config;
int online = 1;
int fd;

/**
 * Function that frees allocated memory of server and close sockets
 */
void freeStructs() {
    for (int i = 0; i < server.nDannies; i++) {
        free(server.dannies[i]->name);
        if (server.dannies[i]->connected) {
            close(server.dannies[i]->fd);
        }
        //printf("Faig free de %d\n", i+1);
        free(server.dannies[i]);
    }
    free(server.dannies);
    close(server.fd);

    free(config.ip);
    free(config.port);
}

/**
 * Function to disconnect jack from sockets and free data
 */
void disconnect() {
    online = 0;
    for(int i = 0; i < server.nDannies; i++){
        pthread_join(server.dannies[i]->tid, NULL);
    }

    close(fd);

    freeStructs();
    write(1, DISCONNECT, strlen(DISCONNECT));
    exit(EXIT_SUCCESS);
}

/**
 * RSI for signals
 * @param sig Signal received
 */
void sighandler(int sig){
    switch(sig){
        case SIGINT:
            disconnect();
            exit(0);
            break;
        case SIGPIPE:
            disconnect();
            exit(0);
        default:
            break;
    }
    signal(sig, sighandler);
}

/**
 * Function that reads config file and starts the server
 * @param file Path to configuration file
 */
void startWendy(char* file) {
    write(1, START, strlen(START));
    readConfigFile(file, &config);
    startServer(&server.fd, config.ip, config.port);
}


/**
 * Function that run every dedicated server (Threads)
 * @param arg void pointer to client assigned
 * @return NULL
 */
void* dedicatedConn(void* arg) {
    Client** arg_aux = (Client**) arg;
    Client* client = (Client*) *arg_aux;

    char origin[ORIGIN_BYTES];
    char type;
    char data[DATA_BYTES];
    //char received[TOTAL_BYTES];

    char *name;
    int size = 0;
    char *md5sum;

    bzero(data, DATA_BYTES);

    int rcv_size = 0;
    int readed = 0;

    //Rutina Wendy dedicat
    while(online) {

        if(!readed){
            if (!read(client->fd, origin, ORIGIN_BYTES)) {
                disconnectDanny(client);
                return NULL;
            }
            read(client->fd, &type, TYPE_BYTES);
        }
        readed = 0;

        if (strcmp(origin, "DANNY") == 0) {
            switch(type) {
                case 'I':
                    rcv_size = 0;
                    size = 0;
                    name = read_until(client->fd, '#');
                    //write(1, name, strlen(name));
                    char *aux = read_until(client->fd, '#');
                    size = atoi(aux);
                    //write(1, aux, strlen(aux));
                    md5sum = read_until(client->fd, '\0');

                    //Cleaning buffer for next frame
                    read(client->fd, data, DATA_BYTES - (strlen(name) + strlen(aux) + strlen(md5sum) + 2));

                    free(aux);
                    //write(1, md5sum, strlen(md5sum));
                    write(1, WENDY, strlen(WENDY));
                    char* buff = malloc(sizeof(char)*(strlen(RECEIVE_TXT)+strlen(client->name)));
                    int s = sprintf(buff, RECEIVE_TXT, client->name);
                    write(1, buff, s);
                    free(buff);
                    //write(1, "\nVaig a llegir F!\n", strlen("\nVaig a llegir F!\n"));
                    break;
                case 'F':
                    //write(1, "\nHe llegit F!\n", strlen("\nHe llegit F!\n"));
                    //printf("Size: %d\n", size);

                    memset(data, '\0', DATA_BYTES);

                    int n = read(client->fd, data, DATA_BYTES);

                    char* PATH = malloc(sizeof(char)*(strlen(PATH_FILE)+strlen(name)));
                    sprintf(PATH, PATH_FILE, name);
                    fd = open(PATH, O_CREAT|O_WRONLY, 0600);
                    free(PATH);

                    if(rcv_size + n > size){
                        write(fd, data, size-rcv_size);
                        rcv_size += size-rcv_size;
                    }else{
                        write(fd, data, n);
                        rcv_size += n;
                    }

                    char buffer[15];
                    while(rcv_size < size){
                        memset(data, '\0', DATA_BYTES);
                        read(client->fd, buffer, 15);
                        type = buffer[14];
                        if(type != 'F'){
                            readed = 1;
                            break;
                        }
                        read(client->fd, data, DATA_BYTES);
                        if(rcv_size + n > size){
                            write(fd, data, size-rcv_size);
                            rcv_size += size-rcv_size;
                        }else{
                            write(fd, data, n);
                            rcv_size += n;
                        }
                        //printf("Size rebut: %d\n", rcv_size);
                    }

                    close(fd);


                    if(rcv_size == size){
                        //printf("He rebut fitxer!!!\n");
                        /*char nom[30];
                        memset(nom, '\0', 30);
                        strcpy(nom, name);*/
                        //saveImage(file_rcv, name, rcv_size);

                        int fd[2];
                        if(pipe(fd) == -1){
                            //ERROR FENT PIPE
                            perror("Pipe creation");
                        }
                        int f = -1;
                        f = fork();
                        switch(f){
                            case -1:
                                //ERROR
                                perror("Fork creation");
                                break;
                            case 0: //FILL
                                close(fd[0]);
                                dup2(fd[1], 1);
                                char PATH[100];
                                sprintf(PATH, PATH_FILE, name);
                                char *args[] = {"md5sum", PATH, 0};
                                execvp(args[0], args);
                                break;
                            default: //PARE
                                close(fd[1]);
                                wait(&f);
                                char* checksum = read_until(fd[0], ' ');
                                close(fd[0]);

                                if(!strcmp(md5sum,checksum)){
                                    write(1, name, strlen(name));
                                    write(1, "\n", strlen("\n"));
                                    sendTo(client->fd, 'S', "IMATGE OK");

                                }else{
                                    sendTo(client->fd, 'R', "IMATGE KO");
                                }
                                free(name);
                                free(md5sum);
                                free(checksum);
                                rcv_size = 0;
                                write(1, WENDY, strlen(WENDY));
                                write(1, WAIT, strlen(WAIT));
                                break;
                        }

                    }
                    break;
                case 'Q':
                    read(client->fd, data, DATA_BYTES);
                    write(1, WENDY, strlen(WENDY));
                    disconnectDanny(client);
                    return NULL;
                    break;
                case 'W':
                    read(client->fd, data, DATA_BYTES);
                    sendTo(client->fd, 'W', "\0");
                    break;
                default:
                    sendTo(client->fd, 'Z', "ERROR DE TRAMA");
                    break;
            }
        }else{
            sendTo(client->fd, 'Z', "ERROR DE TRAMA");
        }
    }
    return NULL;
}


/**
 * General server that accepts danny connection requests and creates a thread to manage it
 */
void Wendy(void) {
    server.dannies = NULL;
    server.nDannies = 0;

    char origin[ORIGIN_BYTES];
    char type;
    char data[DATA_BYTES];

    //char received[TOTAL_BYTES];
    Client client_aux;
    socklen_t len = sizeof(struct sockaddr_in);
    struct sockaddr_in cli;
    while(1) {
        write(1, WENDY, strlen(WENDY));
        write(1, WAIT, strlen(WAIT));
        client_aux.fd = accept(server.fd, (void*)&cli, &len);
        //Protocol connexió amb Danny
        int bytes = 0;

        bytes += read(client_aux.fd, origin, ORIGIN_BYTES);
        bytes += read(client_aux.fd, &type, TYPE_BYTES);
        bytes += read(client_aux.fd, data, DATA_BYTES);
        /*bytes = read(client_aux.fd, received, TOTAL_BYTES);
        strncpy(origin, received, ORIGIN_BYTES+1);
        origin[ORIGIN_BYTES] = '\0';
        strncpy(&type, received + ORIGIN_BYTES, TYPE_BYTES);
        strncpy(data, received + ORIGIN_BYTES + TYPE_BYTES, DATA_BYTES+1);
        data[DATA_BYTES] = '\0';*/
        if (bytes != TOTAL_BYTES) {
            //Enviar trama ERROR
            sendTo(client_aux.fd, 'E', "ERROR");
        }
        else if (strcmp(origin, "DANNY") == 0 && type == 'C'){
            //Connectar amb DANNY (afegir-lo a estructura)
            broadenMemory(&server, client_aux, data);

            sendTo(server.dannies[server.nDannies-1]->fd, 'O', "CONNEXIO OK");

            char* str;
            int size = asprintf(&str, NEW_CONN, server.dannies[server.nDannies-1]->name);
            write(1, str, size);
            free(str);

            pthread_create (&server.dannies[server.nDannies-1]->tid,
                            NULL,
                            dedicatedConn,
                            (void*) &server.dannies[server.nDannies-1]);
        } else{
            sendTo(client_aux.fd, 'Z', "ERROR DE TRAMA");
        }
    }
}

int main(int argc, char const *argv[]) {

    if (argc != 2) {
        write(1, ARG_ERR, strlen(ARG_ERR));
        exit(-1);
    }

    startWendy((char*)argv[1]);
    signal(SIGINT, sighandler);

    Wendy();

    exit(EXIT_SUCCESS);
}