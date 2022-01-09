

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
//Sockets
#include <sys/socket.h>
#include <netinet/in.h>
//#include <arpa/inet.h> (està en network.h)
//Threads
#include <pthread.h>

#include <sys/shm.h>

#include "network.h"

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


/**
 * Function that starts general server
 * @param serverfd Socket file descriptor for server
 * @param ip Ip of own server
 * @param port Port of own server
 */
void startServer(int* serverfd, char* ip, char* port){
    //Socket
    *serverfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    //Bind
    struct sockaddr_in sock;
    sock.sin_family = AF_INET;
    sock.sin_port = htons(atoi(port));
    sock.sin_addr.s_addr = inet_addr(ip);
    if(bind(*serverfd, (void*)&sock, sizeof(sock))) {
        close(*serverfd);
        write(1, BIND_ERR, strlen(BIND_ERR));
        exit(-1);
    }

    //Listen
    listen(*serverfd, 4);
}


/**
 * Expands memory allocation for general server configuration data
 * @param server Structure where all info is managed
 * @param client_aux New client connection
 * @param name New station name
 */
void broadenMemory(Server* server, Client client_aux, char* name) {
    if (++server->nDannies == 1) {
        server->dannies = (Client**) malloc(server->nDannies*sizeof(Client*));
    }
    else {
        server->dannies = (Client**) realloc(server->dannies,
                                             server->nDannies*sizeof(Client*));
    }
    server->dannies[server->nDannies-1] = (Client*) malloc(sizeof(Client));

    server->dannies[server->nDannies-1]->fd = client_aux.fd;
    server->dannies[server->nDannies-1]->connected = 1;
    server->dannies[server->nDannies-1]->name = (char*) malloc(sizeof(char));
    for (int i = 0; i < (int)strlen(name); i++) {
        server->dannies[server->nDannies-1]->name = realloc(server->dannies[server->nDannies-1]->name,
                                                            sizeof(char)*(i+2));
        server->dannies[server->nDannies-1]->name[i] = name[i];
        server->dannies[server->nDannies-1]->name[i+1] = '\0';
    }

}

/**
 * Function for sending frames to fd
 * @param fd Typically socket fd
 * @param type Frame type
 * @param msg Frame data content
 */
void sendTo(int fd, char type, char* msg) {
    char origin[ORIGIN_BYTES];
    char data[DATA_BYTES];
    char toSend[TOTAL_BYTES];

    memset(origin, '\0', ORIGIN_BYTES);
    memset(data, '\0', DATA_BYTES);
    memset(toSend, '\0', TOTAL_BYTES);

    strcpy(origin, "WENDY");
    strcpy(data, msg);

    for(int i = 0; i < TOTAL_BYTES; i++){
        if(i < ORIGIN_BYTES){
            toSend[i] = origin[i];
        } else if(i < ORIGIN_BYTES + TYPE_BYTES){
            toSend[i] = type;
        } else{
            toSend[i] = data[i-ORIGIN_BYTES-TYPE_BYTES];
        }
    }

    write(fd, toSend, TOTAL_BYTES);
}

/**
 * Disconnects client socket
 * @param client
 */
void disconnectDanny(Client* client) {
    client->connected = 0;
    close(client->fd);
}

