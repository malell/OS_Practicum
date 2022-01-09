#ifndef NETWORK_H
#define NETWORK_H

#define ORIGIN_BYTES    14
#define TYPE_BYTES      1
#define DATA_BYTES      100
#define TOTAL_BYTES     ORIGIN_BYTES + TYPE_BYTES + DATA_BYTES

//Error messages
#define JCONN_ERR       "Error: connecting to Jack\n"
#define WCONN_ERR       "Error: connecting to Wendy\n"


int serverSaidOkTo(int fd, char sent);
int sendTo(int fd, char type, char* msg);
int connectServer(char* ip, char* port);
int connectJack(char* ip, char* port, char* name);
int connectWendy(char* ip, char* port, char* name);


#endif