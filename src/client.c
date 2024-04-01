#include <client.h>
#include <connect.h>
#include <utils.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

extern SSL_CTX *ctx;
extern SSL *ssl;

#include <fcntl.h>

/**
 * 作为客户端连接服务器
 * @param host 服务器主机名
 * @param port 服务器端口号
 * @return 成功连接时返回客户端文件描述符，否则返回-1
*/
int connect_as_client(char *host, char *port){
    int clientfd;
    clientfd = open_clientfd(host, port);
    
    ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL){
        LogRed("Create CTX error!!!");
    }
    ssl = SSL_new(ctx);
    if (ssl == NULL){
        ERR_print_errors_fp(stderr);
        return -1;
    }
    SSL_set_fd(ssl, clientfd);
    assert(ssl);
    if (SSL_connect(ssl) != 1){
        ERR_print_errors_fp(stderr);
        return -1;
    }
    return clientfd;
}
