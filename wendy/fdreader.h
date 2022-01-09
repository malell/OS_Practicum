#ifndef FDREADER_H
#define FDREADER_H

typedef struct config {
    char* ip;
    char* port;
} Configuration;

#define PATH_FILE       "./Barry/%s"

char* read_until(int fd, char end);
//char* read_str_until(char* str, char end);
void readConfigFile(char* path, Configuration* config);
void buildImage(char *file_rcv, const char *binData, int *rcv_size, const int size);
void saveImage(char *binData, char *file_name, int fsize);

#endif
