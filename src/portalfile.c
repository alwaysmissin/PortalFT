#include <common.h>
#include <portalfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <assert.h>

#define BUF_SIZE 8192

file_node *file_head = NULL;
file_node *file_tail = NULL;

int add_file(char *filename){
    file_node *new_file = (file_node *)malloc(sizeof(file_node));
    int fd;
    if ((fd = open(filename, O_RDONLY)) < 0){
        perror("open error: please check the file is exist!");
        return -1;
    };
    size_t size;
    if ((size = lseek(fd, 0, SEEK_END)) < 0){
        perror("lseek error");
        return -1;
    }
    lseek(fd, 0, SEEK_SET);
    new_file->fd = fd;
    strcpy(new_file->filename, filename);
    new_file->size = size;
    new_file->next = NULL;
    if (file_head == NULL){
        file_head = new_file;
        file_tail = new_file;
    } else {
        file_tail->next = new_file;
        file_tail = new_file;
    }
    printf("File %s added, size: %ld\n", filename, size);
    return 0;
}

void list_files(){
    if (file_head == NULL){
        printf("No files added\n");
        return;
    }
    file_node *current = file_head;
    for (int i = 0; current != NULL; i ++, current = current -> next){
        printf("No.%d: filename: %s, size: %ld\n", i, current->filename, current->size);
    }
}

void send_files(int connfd){
    if (file_head == NULL){
        printf("No files added\n");
        return;
    }
    file_node *current = file_head;
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    for (int i = 0; current != NULL; i ++, current = current -> next){
        sprintf(buf, "new %s", current->filename);
        send(connfd, buf, strlen(buf), 0);
        memset(buf, 0, sizeof(buf));
        // 发送文件名后, 等待接收方确认
        printf("waiting for reveiver to commit.\n");
        if (recv(connfd, buf, sizeof(buf), 0) < 0){
            perror("send error!");
            return;
        } else {
            memset(buf, 0, sizeof(buf));
        }
        size_t n;
        while((n = read(current->fd, buf, sizeof(buf))) > 0){
            send(connfd, buf, n, 0);
            memset(buf, 0, n);
            n = recv(connfd, buf, sizeof(buf), 0);
            printf("%s\n", buf);
            memset(buf, 0, n);
        }
        lseek(current->fd, 0, SEEK_SET);
    }
    send(connfd, "fin", 4, 0);
    recv(connfd, buf, sizeof(buf), 0);
    assert(strcmp(buf, "fin") == 0);
    printf("All files sent\n");
}

void recv_files(int connfd){
    int fd;
    size_t recv_size;
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    while((recv_size = recv(connfd, buf, sizeof(buf), 0)) > 0){
        // printf("%s", buf);
        if (strncmp(buf, "new", 3) == 0){
            // close(fd);
            if (fd != 0) close(fd);
            char *filename = strtok(buf, " ");
            filename = strtok(NULL, " ");
            printf("receiving new file: %s\n", filename);
            if ((fd = open(filename, O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH)) < 0){
                perror("failed to create file");
                return ;
            }
            memset(buf, 0, sizeof(buf));
            send(connfd, "ok", 3, 0);
            continue;
        } else if (strcmp(buf, "fin") == 0){
            printf("complete file transfer!\n");
            send(connfd, "fin", 4, 0);
            break;
        }
        write(fd, buf, recv_size);
        memset(buf, 0, sizeof(buf));
        send(connfd, "ok", 3, 0);
    }
    // printf("complete file transfer!\n");
}