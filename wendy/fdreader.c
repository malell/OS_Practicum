#define _GNU_SOURCE
//I/O
#include <stdio.h>
//STR functions
#include <string.h>
//Dinamic memory
#include <stdlib.h>

#include <unistd.h>
//Modes d'escriptura i lectura (FILEDESCRIPTORS)
#include <fcntl.h>

#include "fdreader.h"

//Error messages
#define FIL_ERR         "Error: Failure while opening file\n"

/**
 * Magical function that read from a file descriptor
 * @param fd File descriptor
 * @param end Char to finish read
 * @return String with readed chars
 */
char* read_until(int fd, char end) {
    int i = 0, size;
    char c = '\0';
    char* string = malloc(sizeof(char));
    while (1) {
        size = read(fd, &c, sizeof(char));
        if (c != end && size > 0) {
            string = (char*)realloc(string, sizeof(char) * (i + 2));
            string[i++] = c;
        } else {
            i++; //Pel \0
            break;
        }
    }
    string[i-1] = '\0';
    return string;
}
/*
char* read_str_until(char* str, char end) {
    int i = 0;
    char c;
    char* string = malloc(sizeof(char));
    while (1) {
        c = str[i];
        if (c != end) {
            string = (char*)realloc(string, sizeof(char) * (i + 2));
            string[i++] = c;
        } else {
            i++; //Pel \0
            break;
        }
    }
    string[i-1] = '\0';
    return string;
}*/

/**
 * Function that reads the config file and save it in data structure
 * @param path Path to the config file
 * @param config Data structure where to save info
 * @return If process could be done
 */
void readConfigFile(char* path, Configuration* config) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        write(1, FIL_ERR, strlen(FIL_ERR));
        exit(-1);
    }
    config->ip = read_until(fd, '\n');
    config->port = read_until(fd, '\n');
    close(fd);
}
