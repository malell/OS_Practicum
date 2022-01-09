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
#define START           "\nStarting Jack..."
#define DISCONNECT      "\nDisconnecting Jack...\n"

#define ORIGIN_BYTES    14
#define TYPE_BYTES      1
#define DATA_BYTES      100
#define TOTAL_BYTES     ORIGIN_BYTES + TYPE_BYTES + DATA_BYTES

//Error messages
#define BIND_ERR        "Error: bind\n"

//System messages
#define WAIT            "\nWaiting..."
#define NEW_CONN        "\nNew Connection: %s"
//#define RECEIVE         "\nReceiving data...\n"
#define RECEIVE_TXT     "\nReceiving data...\n%s\n%s"
#define JACK            "\n\n$Jack:"



Configuration config;
pthread_mutex_t mutexData = PTHREAD_MUTEX_INITIALIZER;
//extern pthread_mutex_t mutexData;

int lloyd_pid;
int online = 1;

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
 * Function that run every dedicated server (Threads)
 * @param arg void pointer to client assigned
 * @return NULL
 */
void* dedicatedConn(void* arg) {
    Client** arg_aux = (Client**) arg;
    Client* client = (Client*) *arg_aux;


    //Synchronization initialization
    SEM_constructor_with_name(&s_lloyd,ftok("lloyd.c",0));
    SEM_init(&s_lloyd,0);
    SEM_constructor_with_name(&s_jack,ftok("jack.c",0));
    SEM_init(&s_jack,0);

    //Declaration Shared Memory
    memid = shmget(ftok("lloyd.c",1),sizeof(Data),IPC_EXCL|0600);
    Data *datum = shmat(memid,NULL,0);

    //Memory allocation for shmem pointers array (to dettach on SIGINT)
    if (server.nDannies == 1) {
        datumArray = (Data**) malloc(server.nDannies*sizeof(Data*));
    }
    else {
        datumArray = (Data**) realloc(datumArray, server.nDannies*sizeof(Data*));
    }
    datumArray[server.nDannies-1] = datum;

    char origin[ORIGIN_BYTES];
    char type;
    char data[DATA_BYTES];
    //char received[TOTAL_BYTES];

    //Rutina Jack dedicat
    while(online) {
        //write(1, JACK, strlen(JACK));
        if (!read(client->fd, origin, ORIGIN_BYTES)) {
            disconnectDanny(client);
            return NULL;
        }
        read(client->fd, &type, TYPE_BYTES);
        read(client->fd, data, DATA_BYTES);

        if (strcmp(origin, "DANNY") == 0) {
            switch(type) {
                case 'D':
                    write(1, JACK, strlen(JACK));
                    //Checks data received is correct (size of values)
                    //Converts data "string" for being written in screen
                    //Fills MeteoData structure

                    Data meteo;

                    if (processMeteoData(data, &meteo, client->name)) {
                        //LOCK, SHMEM, SIGNALS, UNLOCK
                        pthread_mutex_lock(&mutexData);

                        strcpy(datum->name, meteo.name);
                        datum->temperature = meteo.temperature;
                        datum->humidity = meteo.humidity;
                        datum->pressure = meteo.pressure;
                        datum->precipitation = meteo.precipitation;
                        SEM_signal(&s_lloyd);
                        SEM_wait(&s_jack);

                        pthread_mutex_unlock(&mutexData);

                        //Answering to Danny
                        sendTo(client->fd, 'B', "DADES OK");

                        char* str;
                        int size = asprintf(&str, RECEIVE_TXT, client->name, data);
                        write(1, str, size);
                        free(str);
                    } else
                        sendTo(client->fd, 'K', "DADES KO");
                    write(1, JACK, strlen(JACK));
                    write(1, WAIT, strlen(WAIT));
                    break;
                case 'Q':
                    write(1, JACK, strlen(JACK));
                    disconnectDanny(client);
                    return NULL;
                    break;
                case 'J':
                    sendTo(client->fd, 'J', "");
                    break;
                default:
                    sendTo(client->fd, 'Z', "ERROR DE TRAMA");
                    break;
            }
        } else {
            sendTo(client->fd, 'Z', "ERROR DE TRAMA");
        }
    }
    return NULL;
}

/**
 * Function to disconnect jack from sockets and free data
 */
void disconnect() {
    kill(lloyd_pid, SIGINT);
    wait(NULL);

    SEM_destructor(&s_lloyd);
    SEM_destructor(&s_jack);

    pthread_mutex_destroy(&mutexData);

    //Dettach of shmem from every thread and destruction of shmem
    if(server.nDannies){
        freeSharedMemory();
        shmctl(memid, IPC_RMID, NULL);
    }

    //Closing all threads
    online = 0;
    for(int i = 0; i < server.nDannies; i++){
        pthread_join(server.dannies[i]->tid, NULL);
    }

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
void startJack(char* file) {
    write(1, START, strlen(START));
    readConfigFile(file, &config);
    startServer(&server.fd, config.ip, config.port);
}

/**
 * General server that accepts danny connection requests and creates a thread to manage it
 */
void Jack(void){
    server.dannies = NULL;
    server.nDannies = 0;

    char origin[ORIGIN_BYTES];
    char type;
    char data[DATA_BYTES];
    //char received[TOTAL_BYTES];
    Client client_aux;
    socklen_t len = sizeof(struct sockaddr_in);
    struct sockaddr_in cli;
    write(1, JACK, strlen(JACK));
    write(1, WAIT, strlen(WAIT));
    while(1) {
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
        }
    }
}

int main(int argc, char const *argv[]) {

    if (argc != 2) {
        write(1, ARG_ERR, strlen(ARG_ERR));
        exit(-1);
    }

    //Synchronization initialization
    SEM_constructor_with_name(&s_lloyd,ftok("lloyd.c",0));
    SEM_init(&s_lloyd,0);
    SEM_constructor_with_name(&s_jack,ftok("jack.c",0));
    SEM_init(&s_jack,0);

    memid = shmget(ftok("lloyd.c",1),sizeof(Data),IPC_CREAT|IPC_EXCL|0600);

    lloyd_pid = fork();
    //Lloyd
    if (lloyd_pid == 0) {
        Lloyd();
    }
    //Jack
    else if (lloyd_pid > 0){
        startJack((char*)argv[1]);
        signal(SIGINT, sighandler);

        //Shared memory creation

        Jack();

    }
    exit(EXIT_SUCCESS);
}