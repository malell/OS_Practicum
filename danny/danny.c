/******************************************************************************/
/* Sistemes Operatius - Pràctica Overlook System - Curs 2020-21               */
/* Marc Legarre Lluch (marc.legarre@students.salle.url.edu)                   */
/* Gerard Pérez Iglesias (gerard.perez@students.salle.url.edu)                */
/******************************************************************************/

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
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>

#include <fcntl.h>

#include "fdreader.h"
#include "utils.h"
#include "network.h"

//Error messages
#define ARG_ERR         "Error: Incorrect arguments number\n"
#define SRV_ERR         "Error: Server returned error message\n"
#define DISCONN_ERR     "Error: Server disconnected\n"
#define IMAGE_ERR       "Error: Image not correctly sent\n"

//System messages
#define START               "\nStarting Danny...\n"
#define STATION             "\n$%s:"
#define TEST                "\nTesting...\n"
#define DISCONNECT          "\nDisconnecting Danny...\n"
#define DISCONNECT_JACK     "\nDisconnecting Jack..."
#define DISCONNECT_WENDY    "\nDisconnecting Wendy..."
#define SENDING             "Sending data...\n"
#define SENT                "Data sent...\n"
#define SENDING_IMAGE       "Sending %s\n"

#define MD5SUM_SIZE 32

Directory directory;
Configuration config;
Text text;

int jackfd;
int wendyfd;

/**
 * Frees dynamic memory from general purpose structures
 */
void freeData(void) {
    //Text
    free(text.date);
    free(text.hour);
    free(text.humidity);
    free(text.precipitation);
    free(text.pressure);
    free(text.temperature);
    //Config
    free(config.station);
    free(config.folder);
    free(config.jack_ip);
    free(config.jack_port);
    free(config.wendy_ip);
    free(config.wendy_port);
}

/**
 * Function to disconnect danny from sockets and free data
 */
void disconnect() {
    write(1,DISCONNECT_JACK,strlen(DISCONNECT_JACK));
    close(jackfd);
    write(1,DISCONNECT_WENDY,strlen(DISCONNECT_WENDY));
    close(wendyfd);

    //Free de totes les possibles estructures
    freeDirStruct(&directory);
    free(directory.fileNames);
    freeData();
    write(1, DISCONNECT, strlen(DISCONNECT));
    exit(EXIT_SUCCESS);
}

/**
 * Function that sends a JPG type file to Wendy
 * @param image Name of the file
 * @param file File path
 */
void sendImage(char* image, char* file){

    int fd[2];
    if(pipe(fd) == -1){
        perror("Pipe creation");
    }

    int md5sum = fork();
    switch(md5sum){
        case -1:
            perror("Fork creation");
            break;
        case 0:
            //FILL
            close(fd[0]);
            dup2(fd[1],1);
            char *args[] = {"md5sum", file, 0};
            execvp(args[0], args);
            break;
        default:
            close(fd[1]);
            wait(&md5sum);
            char* checksum = read_until(fd[0],' ');
            close(fd[0]);

            //Getting file size
            struct stat file_info;
            int fsize;
            if(!stat(file, &file_info)){
                fsize = file_info.st_size;
            }

            //First image frame
            char origin[ORIGIN_BYTES];
            char data[DATA_BYTES];
            char toSend[TOTAL_BYTES];

            memset(origin, '\0', ORIGIN_BYTES);
            memset(data, '\0', DATA_BYTES);
            memset(toSend, '\0', TOTAL_BYTES);

            sprintf(data, "%s#%d#%s", image, fsize, checksum);

            free(checksum);

            sendTo(wendyfd, 'I', data);

            //Printing sending image
            char buff[40];
            bzero(buff, 40);
            int size = sprintf(buff, SENDING_IMAGE, image);
            write(1, buff, size);

            //Binary read of image
            int snd_size = 0;
            int fd = open(file, O_RDONLY);
            if( fd < 0){
                write(1, "Error opening file\n", strlen("Error opening file\n"));
                exit(-1);
            }

            //int fdtest = open("./files/test.jpg", O_CREAT|O_WRONLY, 0600);
            while(snd_size < fsize){

                memset(origin, '\0', ORIGIN_BYTES);
                memset(data, '\0', DATA_BYTES);
                memset(toSend, '\0', TOTAL_BYTES);

                strcpy(origin, "DANNY");

                int n = read(fd, data, sizeof(char)*DATA_BYTES);
                snd_size += n;
                //write(fdtest, data, DATA_BYTES);

                for(int i = 0; i < TOTAL_BYTES; i++){
                    if(i < ORIGIN_BYTES){
                        toSend[i] = origin[i];
                    } else if(i < ORIGIN_BYTES + TYPE_BYTES){
                        toSend[i] = 'F';
                    } else{
                        toSend[i] = data[i-ORIGIN_BYTES-TYPE_BYTES];
                    }
                }
                write(wendyfd, toSend, TOTAL_BYTES);

                usleep(250);

            }
            //close(fdtest);
            close(fd);

            if(!serverSaidOkTo(wendyfd, 'F'))
                write(1, IMAGE_ERR, strlen(IMAGE_ERR));
            break;
    }
}

/**
 * Function that check directory files and operate with each type
 */
void checkFolder(void){
    write(1, TEST, strlen(TEST));
    freeDirStruct(&directory);
    getDirInfo(config.folder, &directory);
    printFiles(&directory);

    //HEARTBEAT JACK
    sendTo(jackfd, 'J', "\0");
    if(!serverSaidOkTo(jackfd,'J'))
        raise(SIGINT);

    //HEARTBEAT WENDY
    sendTo(wendyfd, 'W', "\0");
    if(!serverSaidOkTo(wendyfd,'W'))
        raise(SIGINT);

    if (directory.files) {
        char *path;
        for (int i = 0; i < directory.files; i++) {
            asprintf(&path, "./%s/%s", config.folder, directory.fileNames[i]);   //RELATIVE PATH

            switch(fileType(directory.fileNames[i])) {
                case TXT:
                    write(1, directory.fileNames[i], strlen(directory.fileNames[i]));
                    write(1, "\n", sizeof(char));
                    char* data = readTextFile(path, &text,100);
                    showTextFile(text);
                    write(1, SENDING, strlen(SENDING));
                    //Salta SIGPIPE directament, per la poca fiabilitat de SIGPIPE (comentari
                    // de Malé), fem un raise de SIGINT per si un cas
                    if (!sendTo(jackfd, 'D', data)) {
                        free(path);
                        write(1, DISCONN_ERR, strlen(DISCONN_ERR));
                        raise(SIGINT);
                    }
                    if (!serverSaidOkTo(jackfd, 'D'))
                        write(1, SRV_ERR, strlen(SRV_ERR));
                    else
                        write(1, SENT, strlen(SENT));
                    remove(path);
                    free(data);
                    break;
                case JPG:
                    sendImage(directory.fileNames[i], path);
                    remove(path);
                    break;
                default:
                    break;
            }
            free(path);
        }
    }
}

/**
 * RSI for signals
 * @param sig Signal received
 */
void sigHandler(int sig){
    switch (sig) {
        case SIGALRM:
            checkFolder();
            alarm(config.sleepTime);
            break;
        case SIGPIPE:
        case SIGINT:
            sendTo(jackfd, 'Q', config.station);
            sendTo(wendyfd, 'Q', config.station);
            disconnect();
            break;
        default:
            break;
    }
    signal(sig, sigHandler);
}


/**
 * Starts Danny socket connection with servers
 * @param file File where configuration is found
 * @param config File structure where configuration will be saved
 */
void startDanny(char* file, Configuration* config) {
    write(1, START, strlen(START));
    if(readConfigFile(file, config)){
      //tanca
      disconnect();
    }
    alarm(config->sleepTime);
    signal(SIGALRM, sigHandler);
    signal(SIGINT, sigHandler);
    signal(SIGPIPE, sigHandler);

    //Connection with Jack
    jackfd = connectJack(config->jack_ip, config->jack_port, config->station);
    if (jackfd < 0) {
      write(1, JCONN_ERR, strlen(JCONN_ERR));
      disconnect();
    }

    //Connection with Wendy
    wendyfd = connectWendy(config->wendy_ip, config->wendy_port, config->station);
    if (wendyfd < 0) {
        write(1, WCONN_ERR, strlen(WCONN_ERR));
        disconnect();
    }
}



int main(int argc, char const *argv[]) {
    directory.files = 0;
    directory.fileNames = NULL;

    if (argc != 2) {
      write(1, ARG_ERR, strlen(ARG_ERR));
      exit(-1);
    }
    startDanny((char*)argv[1], &config);

    while(1) {
    char* msg;
    int size = asprintf(&msg, STATION, config.station);
    write(1, msg, size);
    free(msg);
    //Si volem que faci altres coses....
    pause();
    }
    exit(EXIT_SUCCESS);
}
