#include "server.h"

SERVER* createServer(int port)
{

    int lfd;
    if ( (lfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("setupServer() socket error\n");
        exit(EXIT_FAILURE);
    }

    // Define the address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Setup the server
    if ((bind(lfd, (SA*) &serv_addr, sizeof(serv_addr))) < 0) {
        perror("setupServer() bind error\n");
        exit(EXIT_FAILURE);
    }
    if ((listen(lfd, MAX_CLIENT)) < 0) {
        perror("setupServer() listen error\n");
        exit(EXIT_FAILURE);
    }
    SERVER* serv = (SERVER*) malloc(sizeof(SERVER));
    serv->listenFD = lfd;
    serv->connectFD = -1;
    serv->socketFD = -1;
    memset(serv->clientFDs,0,MAX_CLIENT);
    serv->numClients = 0;
    serv->numReady = 0;
    serv->gameFlag = 0;
    return serv;
}

void resetGame(SERVER* server)
{
    server->numClients = 0;
    server->numReady = 0;
    server->gameFlag = 0;
}
void destroyServer(SERVER* server)
{
    close(server->listenFD);
}