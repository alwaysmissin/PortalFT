#ifndef __PORTALFILE_H__
#define __PORTALFILE_H__
#include <common.h>

typedef struct portalfile{
    int fd;
    char filename[128];
    size_t size;
    struct portalfile *next;
}file_node;

int add_file(char *filename);
void list_files();
void send_files(int connfd);
void recv_files(int connfd);
#endif