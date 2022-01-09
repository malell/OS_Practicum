#ifndef FDREADER_H
#define FDREADER_H

typedef struct config {
    char* ip;
    char* port;
} Configuration;

char* read_until(int fd, char end);
void readConfigFile(char* path, Configuration* config);

#endif