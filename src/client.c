#include <client.h>
#include <connect.h>
int connect_as_client(char *host, char *port){
    int clientfd;
    clientfd = open_clientfd(host, port);
    return clientfd;
}
