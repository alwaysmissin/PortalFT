#ifndef __CLIENT_H__
#define __CLIENT_H__
#include <openssl/ssl.h>
int connect_as_client(char *host, char *port);
SSL *connect_as_client_ssl(SSL_CTX *ctx, char *host, char *port);
#endif