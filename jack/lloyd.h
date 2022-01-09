//
// Created by legar on 07/12/2020.
//

#ifndef C_PROJECT_LLOYD_H
#define C_PROJECT_LLOYD_H


#define _GNU_SOURCE
//#define _POSIX_C_SOURCE 1


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

#include <sys/shm.h>

/*
//Sockets
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 */
//Threads
#include <pthread.h>

#include "semaphore_v2.h"

#define MAX_CHAR    100

//Global variables
semaphore s_lloyd;
semaphore s_jack;
int memid;


typedef struct{
    char name[MAX_CHAR];
    float temperature;
    int humidity;
    float pressure;
    float precipitation;
}Data;

typedef struct{
    char station[MAX_CHAR];
    int lectures;
    float temperature;
    float humidity;
    float pressure;
    float precipitation;
}Statistics;

void Lloyd(void);

#endif //C_PROJECT_LLOYD_H
