#include <common.h>
#include <portalfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <config.h>

#define BUF_SIZE 8192

file_node *file_head = NULL;
file_node *file_tail = NULL;

/**
 * 将目录dir下的所有文件添加到发送列表中
 * @param dir 目录名
 * @return 成功返回0, 失败返回-1
*/
int add_dir(char *dir){
    struct dirent *filename;
    struct stat s_buf;
    DIR *dp = opendir(dir);
    while(filename = readdir(dp)){
        // 将目录名和文件名拼接成完整的路径
        char file_path[200];
        memset(file_path, 0, sizeof(file_path));
        strcat(file_path, dir);
        strcat(file_path, "/");
        strcat(file_path, filename->d_name);
        
        // 获取文件信息
        stat(file_path, &s_buf);
        
        // 如果是目录, 递归添加
        // 如果是文件, 则直接添加
        if (S_ISDIR(s_buf.st_mode)){
            if (strcmp(filename->d_name, "..") == 0 || strcmp(filename->d_name, ".") == 0)
                continue;
            add_dir(file_path);
        } else {
            add_file(file_path);
        }
    } 
    return 0;
}

/**
 * 将文件添加到发送列表中
 * @param filename 文件名
 * @return 成功返回0, 失败返回-1
*/
int add_file(char *filename){
    // 打开文件, 并检测文件是否存在
    file_node *new_file = (file_node *)malloc(sizeof(file_node));
    int fd;
    if ((fd = open(filename, O_RDONLY)) < 0){
        perror("open error: please check the file is exist!");
        return -1;
    };
    // 获取文件大小
    size_t size;
    if ((size = lseek(fd, 0, SEEK_END)) < 0){
        perror("lseek error");
        return -1;
    }
    lseek(fd, 0, SEEK_SET);

    // 将文件信息添加到发送列表中
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

/**
 * 列出发送列表中的所有文件
 * @return void
*/
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

/**
 * 发送发送列表中的所有文件
 * @param connfd 连接描述符
*/
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
        // 如果文件大小小于0x7fff0000, 则使用sendfile发送文件
        // 否则使用read-send的方式发送文件
        if (current -> size < 0x7fff0000){
            sendfile(connfd, current->fd, NULL, current->size);
        } else {
            printf("The file to send is bigger than limitation, using read-send to send file instead of sendfile\n");
            while((n = read(current->fd, buf, sizeof(buf))) > 0){
                send(connfd, buf, n, 0);
                memset(buf, 0, n);
            }
        }
        // 当一个文件发送完毕后, 向接收方发送当前文件传输结束信号
        send(connfd, "fin", 4, 0);
        recv(connfd, buf, sizeof(buf), 0);
        assert(strcmp(buf, "fin") == 0);
        lseek(current->fd, 0, SEEK_SET);
    }

    // 完成所有文件的发送后, 向接收方发送结束信号
    send(connfd, "finall", 7, 0);

    printf("All files sent\n");
}

/**
 * 接收文件
 * @param connfd 连接描述符
 * @return void
*/
void recv_files(int connfd){
    int fd;
    size_t recv_size;
    char buf[BUF_SIZE];
    char filepath[128];
    memset(buf, 0, sizeof(buf));
    while((recv_size = recv(connfd, buf, sizeof(buf), 0)) > 0){
        // printf("%s", buf);
        if (strncmp(buf, "new", 3) == 0){
            if (fd != 0) close(fd);
            // 从 buf 中分割出文件名
            char *savepath = get_config("savepath");
            char *filename = strtok(buf, " ");
            filename = buf + strlen(filename) + 1;
            filename = basename(filename);
            printf("receiving new file: %s\n", filename);
            sprintf(filepath, "%s/%s", savepath, filename);
            // 创建文件, 并设置权限
            if ((fd = open(filepath, O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH)) < 0){
                perror("failed to create file");
                return ;
            }
            // 向发送方发送确认信息, 通知发送方可以开始发送文件内容
            memset(buf, 0, sizeof(buf));
            send(connfd, "ok", 3, 0);
            continue;
        } else if (strcmp(buf + recv_size - 4, "fin") == 0){
            // 接收到发送方的结束信号, 关闭文件描述符
            // 准备接收下一个文件
            send(connfd, "fin", 4, 0);
            write(fd, buf, recv_size - 4);
            memset(buf, 0, recv_size);
            printf("saved to %s\n", filepath);
            continue;
        } else if (strcmp(buf + recv_size - 7, "finall") == 0){
            write(fd, buf, recv_size - 7);
            memset(buf, 0, recv_size);
            break;
        }
        write(fd, buf, recv_size);
        memset(buf, 0, sizeof(buf));
    }
    if (fd != 0) close(fd);
}