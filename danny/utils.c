//
// Created by legar on 05/11/2020.
//

#define _GNU_SOURCE

//I/O
#include <stdio.h>
//Dinamic memory
#include <stdlib.h>
//STR functions
#include <string.h>
//Signals
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#include "utils.h"

#define NOFILES         "No files available\n"

/**
 * Function that prints found files in screen
 * @param directory Data structure of found files
 */
void printFiles(Directory *directory) {
    char* aux;
    int size;
    if(!directory->files)
        write(1, NOFILES, strlen(NOFILES));
    else {
        size = asprintf(&aux, "%d files found\n", directory->files);
        write(1,aux,size);
        free(aux);

        for(int i = 0; i<directory->files; i++) {
            size = asprintf(&aux, "%s\n", directory->fileNames[i]);
            write(1,aux,size);
            free(aux);
        }
        write(1, "\n", sizeof(char));
    }
}

/**
 * Fucntion that show txt type files information
 * @param text Data structure with txt information
 */
void showTextFile(Text text){
    write(1, text.date, strlen(text.date));
    write(1, "\n", sizeof(char));
    write(1, text.hour, strlen(text.hour));
    write(1, "\n", sizeof(char));
    write(1, text.temperature, strlen(text.temperature));
    write(1, "\n", sizeof(char));
    write(1, text.humidity, strlen(text.humidity));
    write(1, "\n", sizeof(char));
    write(1, text.pressure, strlen(text.pressure));
    write(1, "\n", sizeof(char));
    write(1, text.precipitation, strlen(text.precipitation));
    write(1, "\n\n", sizeof(char)*2);
}

/**
 * Function that checks wether file type is txt or jpg
 * @param filename Full name of the file
 * @return 1 if txt, 0 if jpg, -1 if neither
 */
int fileType(char* filename) {
    int n = strlen(filename);
    char aux[4];
    int j = 0;
    for (int i = n-4; i < n; i++) {
        aux[j] = filename[i];
        j++;
    }
    if (strcmp(aux, ".txt") == 0)
        return TXT;
    else if(strcmp(aux, ".jpg") == 0)
        return JPG;
    else
        return -1;
}

/**
 * Frees dynamic memory allocated in data structure of directory
 * @param directory Directory data structure
 */
void freeDirStruct(Directory *directory) {
    for(int i = 0; i < directory->files; i++) {
        if (directory->fileNames[i] != NULL)
            free(directory->fileNames[i]);
    }
    directory->files = 0;
}