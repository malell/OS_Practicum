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
//Directory
#include <dirent.h>

#include "fdreader.h"

//Error messages
#define FIL_ERR         "Error: Failure while opening file\n"
#define DIR_ERR         "Error: Directory does not exist\n"
#define SLP_ERR         "Error: Incorrect sleep time value\n"
#define NOT_READED  "Error: No s'ha llegit tot el fitxer"

#define BUFFER_SIZE 1024

struct linux_dirent64 {
   ino64_t        d_ino;    /* 64-bit inode number */
   off64_t        d_off;    /* 64-bit offset to next structure */
   unsigned short d_reclen; /* Size of this dirent */
   unsigned char  d_type;   /* File type */
   char           d_name[]; /* Filename (null-terminated) */
};

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

/**
 * Function that read from file descriptor and returns parsed info with '#'
 * @param fd File descriptor for read
 * @param end Char to finish read
 * @param str String where to concatenate info
 * @param x Number of chars readed in str
 * @return String with concatenated info
 */
char* read_fill_until(int fd, char end, char** str, int* x) {
    int i = 0, size;
    char c = '\0';
    char* string = malloc(sizeof(char));
    while (1) {
        size = read(fd, &c, sizeof(char));
        if (c != end && size > 0) {
            string = (char*)realloc(string, sizeof(char) * (i + 2));
            (*str)[(*x)++] = c;
            string[i++] = c;
        } else {
            i++; //Pel \0
            break;
        }
    }
    (*str)[(*x)++] = '#';
    string[i-1] = '\0';
    return string;
}

/**
 * Function that reads a txt file type
 * @param file Path to the file to be readed
 * @param text Data structure where to save info
 * @param n Number of bytes to be readed. Typically 100
 * @return Returns parsed info into frame type data
 */
char* readTextFile(char* file, Text *text, int n){
    int fd = open(file, O_RDONLY);
    if( fd < 0){
        write(1, FIL_ERR, strlen(FIL_ERR));
        exit(-1);
    }
    int x = 0;
    char* str = (char*) malloc(n*sizeof(char));
    memset(str, '\0', n);

    free(text->date);
    free(text->hour);
    free(text->humidity);
    free(text->precipitation);
    free(text->pressure);
    free(text->temperature);

    text->date = read_fill_until(fd, '\n', &str, &x);
    text->hour = read_fill_until(fd, '\n', &str, &x);
    text->temperature = read_fill_until(fd, '\n', &str, &x);
    text->humidity = read_fill_until(fd, '\n', &str, &x);
    text->pressure = read_fill_until(fd, '\n', &str, &x);
    text->precipitation = read_fill_until(fd, '\n', &str, &x);

    str[x-1] = '\0';

    close(fd);

    return str;
}

/**
 * Function that adapts the folder from configuration file
 * @param fd File descriptor of config file
 * @return return the folder path to be used
 */
char* getFolder(int fd) {
    char* str0 = read_until(fd, '\n');
    if (str0[0] == '.' && str0[1] == '/') {
        if (str0[strlen(str0)-1] == '/')
            str0[strlen(str0)-1] = '\0';
        return str0;
    }
    char* str1;
    if (str0[0] == '/')
        asprintf(&str1, ".%s", str0);
    else if ((str0[0] >= 'a' && str0[0] <= 'z') || (str0[0] >= 'A' && str0[0] <= 'Z'))
        asprintf(&str1, "./%s", str0);
    free(str0);
    if (str1[strlen(str1)-1] == '/')
        str1[strlen(str1)-1] = '\0';
    return str1;
}

/**
 * Function that reads the config file and save it in data structure
 * @param path Path to the config file
 * @param config Data structure where to save info
 * @return If process could be done
 */
int readConfigFile(char* path, Configuration* config) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    write(1, FIL_ERR, strlen(FIL_ERR));
    exit(-1);
  }
  config->station = read_until(fd, '\n');
  config->folder = getFolder(fd);
  char* aux = read_until(fd, '\n');
  config->sleepTime = atoi(aux);
  free(aux);
  if(config->sleepTime <= 0){
      write(1, SLP_ERR, strlen(SLP_ERR));
      close(fd);
      return 1;
  }
  config->jack_ip = read_until(fd, '\n');
  config->jack_port = read_until(fd, '\n');
  config->wendy_ip = read_until(fd, '\n');
  config->wendy_port = read_until(fd, '\n');
  close(fd);
  return 0;
}

/**
 * Function that gets files of a directory
 * @param folder Name of directory
 * @param directory Data structure where to save readed information
 */
void getDirInfo(char* folder, Directory* directory) {
  char buffer[BUFFER_SIZE];
  int fd = open(folder, O_RDONLY | O_DIRECTORY);
  if (fd < 0) {
    write(1, DIR_ERR, strlen(DIR_ERR));
    exit(-1);
  }
  int nread = getdents64(fd, buffer, BUFFER_SIZE);
  struct linux_dirent64* f;
  for (int i = 0; i < nread; ) {
    f = (struct linux_dirent64*) (buffer + i);
    char* filename = f->d_name;
    if (strcmp(filename, ".") != 0 && strcmp(filename, "..") != 0) {
      directory->files++;
      directory->fileNames = realloc(directory->fileNames, directory->files*sizeof(char*));
      //Copy file name
      directory->fileNames[directory->files-1] = malloc(sizeof(char)*(strlen(filename)+1));
      int j;
      for(j = 0; j < (int)strlen(filename); j++) {
        directory->fileNames[directory->files-1][j] = filename[j];
      }
      directory->fileNames[directory->files-1][j] = '\0';
    }
    i += f->d_reclen;
  }
  close(fd);
}

