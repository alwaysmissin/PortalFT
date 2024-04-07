#include <server.h>
#include <fcntl.h>
#include <connect.h>
#include <pthread.h>
#include <cmd.h>
#include <utils.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

// int listen_as_server(char *port){
//     int listenfd, connfd;
//     socklen_t clientlen;
//     struct sockaddr_storage clientaddr;
//     char client_hostname[MAXLINE], client_port[MAXLINE];
//     printf("listening on port: %s\n", port);
//     listenfd = open_listenfd(port);
//     // 设置为非阻塞
//     int flags = fcntl(listenfd, F_GETFL, 0);
//     fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);
//     while(1){
//         fd_set readfds;
//         FD_ZERO(&readfds);
//         FD_SET(listenfd, &readfds);

//         if (select(listenfd + 1, &readfds, NULL, NULL, NULL) < 0){
//             perror("select error");
//             continue;
//         }

//         if (FD_ISSET(listenfd, &readfds)){
//             clientlen = sizeof(struct sockaddr_storage);
//             connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

//             // 检查是否成功接受连接
//             if (connfd >= 0) {
//                 getnameinfo((struct sockaddr *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
//                 printf("Connected to (%s, %s)\n", client_hostname, client_port);
//                 return connfd;
//             }
//         }
//         // clientlen = sizeof(struct sockaddr_storage);
//         // connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
//         // getnameinfo((struct sockaddr *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
//         // printf("Connected to (%s, %s)\n", client_hostname, client_port);
//         // return connfd;
//         return 0;
//     }
// }

extern int connfd;
extern int listenfd;
extern SSL_CTX *ctx;
extern SSL *ssl;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *handle_connection(void *arg){
    // int *listenfd = (int *)arg;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    fd_set readfds, readyfds;
    FD_ZERO(&readfds);
    FD_SET(listenfd, &readfds);

    while(1){
        readyfds = readfds;
        if (select(listenfd + 1, &readyfds, NULL, NULL, NULL) < 0){
            perror("select error");
            continue;
        }

        if (FD_ISSET(listenfd, &readyfds)){
            clientlen = sizeof(struct sockaddr_storage);
            int new_connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

            if (new_connfd >= 0){
                getnameinfo((struct sockaddr *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
                LogGreen("Connected to (%s, %s)\n", client_hostname, client_port);

                // pthread_mutex_lock(&lock);
                connfd = new_connfd;
                ssl = SSL_new(ctx);
                SSL_set_fd(ssl, connfd);
                if(SSL_accept(ssl) == -1){
                    perror("accept");
                    close(connfd);
                    // break;
                }
                // pthread_cond_signal(&cond);
                // pthread_mutex_unlock(&lock);
                break;
            }
        }
    }
                // printf("hello\n");
    pthread_exit(NULL);
    // write(STDOUT_FILENO, "\n", 1);
}

void listen_as_server(char *port){
    printf("listenint on port: %s\n", port);
    ctx = SSL_CTX_new(SSLv23_server_method());
    if (ctx == NULL){
        LogRed("Create CTX error!!!");
    }

    SSL_CTX_use_certificate_file(ctx, "./cacert.pem", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, "./privkey.pem", SSL_FILETYPE_PEM);
    SSL_CTX_check_private_key(ctx);

    listenfd = open_listenfd(port);
    int flags = fcntl(listenfd, F_GETFL, 0);
    fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, handle_connection, NULL);

    return ;
}