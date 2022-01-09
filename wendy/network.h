#ifndef NETWORK_H
#define NETWORK_H

#include <arpa/inet.h>

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


Server server;

void startServer(int* serverfd, char* ip, char* port);
void disconnectDanny(Client* client);
void sendTo(int fd, char type, char* msg);
void broadenMemory(Server* server, Client client_aux, char* name);


#endif