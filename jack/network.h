#ifndef NETWORK_H
#define NETWORK_H

#include <arpa/inet.h>
#include "lloyd.h"

typedef struct {
    int fd;
    pthread_t tid;
    char connected;
    char* name;
} Client;

typedef struct {
    int fd;
    Client** dannies;
    int nDannies;
} Server;

//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t *mutexData = &mutex;

Data **datumArray;
Server server;

void startServer(int* serverfd, char* ip, char* port);
void freeSharedMemory(void);
void disconnectDanny(Client* client);
int processMeteoData(char* data, Data* meteo, char* stName);
void sendTo(int fd, char type, char* msg);
void broadenMemory(Server* server, Client client_aux, char* name);


#endif