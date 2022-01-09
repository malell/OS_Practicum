#ifndef FDREADER_H
#define FDREADER_H


typedef struct config {
  char*   station;
  char*   folder;
  int     sleepTime;
  char*   jack_ip;
  char*   jack_port;
  char*   wendy_ip;
  char*   wendy_port;
} Configuration;

typedef struct dir {
  char**  fileNames;
  int     files;
} Directory;


typedef struct txt {
    char*   date;
    char*   hour;
    char*   temperature;
    char*   humidity;
    char*   pressure;
    char*   precipitation;
} Text;

int readConfigFile(char* path, Configuration* config);
void getDirInfo(char* folder, Directory* directory);
char* readTextFile(char* file, Text *text, int n);
char* read_until(int fd, char end);

#endif
