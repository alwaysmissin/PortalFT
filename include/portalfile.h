#ifndef __PORTALFILE_H__
#define __PORTALFILE_H__
#include <common.h>
#include <stdlib.h>
#include <openssl/ssl.h>

typedef struct portalfile{
    FILE *fp;
    char filename[128];
    size_t size;
    char md5[33];
    struct portalfile *next;
    // size_t *slices;
    // FILE **fps;
}file_node;

typedef struct{
    char magic[5];
    char type;
    char filename[128];
    off_t offset;
    size_t size;
    char md5[33];
} CTRLINFO;

enum {NEW, OK_TO_RECEIVE, FIN, FINALL, RESUME};

int add_file(char *filename);
int add_dir(char *dir);
void remove_file(int index);
void list_files();
void release_files();
void send_files(int connfd);
void recv_files(int connfd);
void send_files_ssl(SSL *ssl);
void recv_files_ssl(SSL *ssl);
void calc_md5(FILE *fp, char *md5);
int check_md5(FILE *fp, char *target_md5);
#endif