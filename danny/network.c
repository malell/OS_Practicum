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
#include <arpa/inet.h>
//Threads
#include <pthread.h>

#include "network.h"


//System messages
#define CONN_JACK       "Connecting Jack...\n"
#define CONN_WENDY      "Connecting Wendy...\n"

//Error messages
#define TYPE_ERR        "Error: data type sent is incorrect"


/**
 * Function that sends frames to sockets
 * @param fd File descriptor of socket
 * @param type Type of frame
 * @param msg Frame data content
 * @return True if it was done
 */
int sendTo(int fd, char type, char* msg) {
    char origin[ORIGIN_BYTES];
    char data[DATA_BYTES];
    char toSend[TOTAL_BYTES];

    memset(origin, '\0', ORIGIN_BYTES);
    memset(data, '\0', DATA_BYTES);
    memset(toSend, '\0', TOTAL_BYTES);

    strcpy(origin, "DANNY");
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

    if (write(fd, toSend, TOTAL_BYTES) <= 0)
        return 0;
    return 1;
}

/**
 * Function to check server's response from previous frame
 * @param fd File descriptor of socket
 * @param sent Frame type previously sent
 * @return Return if everything was OK
 */
int serverSaidOkTo(int fd, char sent) {
    char origin[ORIGIN_BYTES];
    char type;
    char data[DATA_BYTES];

    memset(origin, '\0', ORIGIN_BYTES);
    type = '\0';
    memset(data, '\0', DATA_BYTES);


    read(fd, origin, ORIGIN_BYTES);
    read(fd, &type, TYPE_BYTES);
    read(fd, data, DATA_BYTES);

    if (sent == 'D') {
        if (type == 'B')
            return 1;
        else if (type == 'K' || type == 'Z')
            return 0;
        return 0;
    }
    else if (sent == 'I' || sent == 'F') {
        if (type == 'S')
            return 1;
        else if (type == 'R' || type == 'Z')
            return 0;
    }
    //HEARTBEAT JACK
    else if(sent == 'J'){
        if(type == 'J')
            return 1;
    }
    //HEARTBEAT WENDY
    else if(sent == 'W'){
        if(type == 'W')
            return 1;
    }

    write(1, TYPE_ERR, strlen(TYPE_ERR));
    return 0;
}

/**
 * Function that starts socket connection
 * @param ip Ip to connect
 * @param port Port to connect
 * @return File descriptor of socket connection
 */
int startConn(char* ip, char* port) {
    struct sockaddr_in sock;
    sock.sin_family = AF_INET;
    sock.sin_port = htons(atoi(port));
    sock.sin_addr.s_addr = inet_addr(ip);

    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connect(fd, (void*) &sock, sizeof(sock)) < 0) {
        close(fd);
        fd = -1;
    }
    return fd;
}

/**
 * Function that checks if socket connection was successful
 * @param serverName Name of server
 * @param fd File descriptor of socket
 * @return True if connection was correcetly established
 */
int connectedTo(char* serverName, int fd) {
    char origin[ORIGIN_BYTES];
    char type;
    char data[DATA_BYTES];

    if (!read(fd, origin, ORIGIN_BYTES))
        return 0;
    read(fd, &type, TYPE_BYTES);
    read(fd, data, DATA_BYTES);

    if (strcmp(origin, serverName) == 0 && type == 'O')
        return 1;
    if (strcmp(origin, serverName) && type == 'E')
        return 0;

    return 0;
}

/**
 * Function that connect with Jack server
 * @param ip Ip of Jack
 * @param port Port of Jack
 * @param name Meteorologic station name
 * @return File descriptor of socket connection
 */
int connectJack(char* ip, char* port, char* name) {
    write(1, CONN_JACK, strlen(CONN_JACK));
    int fd = startConn(ip, port);
    if (fd >= 0) {
        //Protocol comunicació: Connexió
        sendTo(fd, 'C', name);
        //Resposta
        if (!connectedTo("JACK", fd)) {
            close(fd);
            fd = -1;
        }
    }
    return fd;
}
/**
 * Function that connect with Wendy server
 * @param ip Ip of Wendy
 * @param port Port of Wendy
 * @param name Meteorologic station name
 * @return File descriptor of socket connection
 */
int connectWendy(char* ip, char* port, char* name) {
    write(1, CONN_WENDY, strlen(CONN_WENDY));
    int fd = startConn(ip, port);
    if (fd >= 0) {
        //Protocol comunicació: Connexió
        sendTo(fd, 'C', name);
        //Resposta
        if (!connectedTo("WENDY", fd)) {
            close(fd);
            fd = -1;
        }
    }
    return fd;
}
