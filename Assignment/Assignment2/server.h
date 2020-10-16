#ifndef __SERVH
#define __SERVH
#include "unp.h"
#define MAX_CLIENT 5
typedef struct __server
{
    int listenFD; // Listening file descriptor
    int connectFD; // Connection socket file descriptor
    int socketFD; // Temporary socket file descriptor reference
    int clientFDs[MAX_CLIENT]; // Stores client file descriptor access
    int numClients;
    int numReady;
    int gameFlag; // Finished with round?
} SERVER;

SERVER* createServer(int port);
void resetGame(SERVER* server);
void destroyServer(SERVER* server);
#endif
