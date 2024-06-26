#include <client.h>
#include <connect.h>
#include <utils.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <fcntl.h>

/**
 * 作为客户端连接服务器
 * @param host 服务器主机名
 * @param port 服务器端口号
 * @return 成功连接时返回客户端文件描述符，否则返回-1
*/
SSL *connect_as_client_ssl(SSL_CTX *ctx, char *host, char *port){
    int clientfd;
    clientfd = open_clientfd(host, port);
    SSL *ssl = NULL;
    
    if (ctx == NULL){
        ctx = SSL_CTX_new(SSLv23_client_method());
        assert(ctx);
    }
    ssl = SSL_new(ctx);
    if (ssl == NULL){
        ERR_print_errors_fp(stderr);
        return NULL;
    }
    SSL_set_fd(ssl, clientfd);
    assert(ssl);
    if (SSL_connect(ssl) != 1){
        ERR_print_errors_fp(stderr);
        return NULL;
    }
    return ssl;
}

/**
 * 作为客户端连接服务器
 * @param host 服务器主机名
 * @param port 服务器端口号
 * @return 成功连接时返回客户端文件描述符，否则返回-1
*/
int connect_as_client(char *host, char *port){
    int clientfd;
    clientfd = open_clientfd(host, port);
    return clientfd;
}