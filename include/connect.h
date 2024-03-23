#ifndef __CONNECT_H__
#define __CONNECT_H__
#include <common.h>

int open_clientfd(char *hostname, char *port);
int open_listenfd(char *port);
#endif