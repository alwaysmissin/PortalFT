#ifndef __PORTALFILE_H__
#define __PORTALFILE_H__
#include <common.h>
#include <stdlib.h>

typedef struct portalfile{
    FILE *fp;
    char filename[128];
    size_t size;
    char md5[33];
    struct portalfile *next;
    size_t *slices;
    FILE **fps;
}file_node;

int add_file(char *filename);
int add_dir(char *dir);
void remove_file(int index);
void list_files();
void release_files();
void send_files(int connfd);
void recv_files(int connfd);
void send_files_ssl(int connfd);
void recv_files_ssl(int connfd);
void calc_md5(FILE *fp, char *md5);
int check_md5(FILE *fp, char *target_md5);
#endif