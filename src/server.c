#include <server.h>
#include <connect.h>

int listen_as_server(char *port){
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    printf("listening on port: %s\n", port);
    listenfd = open_listenfd(port);
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        getnameinfo((struct sockaddr *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        return connfd;
    }
}