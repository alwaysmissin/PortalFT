#include <common.h>
#include <portalfile.h>
#include <md5.h>
#include <utils.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <config.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUF_SIZE 8192

file_node *file_head = NULL;
file_node *file_tail = NULL;

extern SSL *ssl;

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
    free(filename);
    free(dp);
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
    FILE *fp = NULL;
    if ((fp = fopen(filename, "r")) == NULL){
        perror("open error: please check the file is exist!");
        return -1;
    } else {
        new_file->fp = fp;
    }
    
    // 获取文件大小
    size_t size;

    if (fseek(fp, 0, SEEK_END) < 0){
        perror("fseek error");
        return -1;
    }
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    // 获取文件的md5校验值
    int md5_enable = atoi(get_config("md5"));
    if (md5_enable){
        LogBlue("md5 calculatiing!\n");
        calc_md5(fp, new_file->md5);
    } else {
        // 向md5字段填充0
        sprintf(new_file->md5, "0");
    }

    // 将文件信息添加到发送列表中
    size_t threads = atoi(get_config("threads"));

    // new_file->fps = (FILE **)malloc(sizeof(FILE *) * threads);
    // for (int i = 0; i < threads; i ++){
    //     new_file->fps[i] = fopen(filename, "r");
    // }
    // close(fd);
    strcpy(new_file->filename, filename);
    new_file->size = size;
    // 对文件进行分片, 每个线程发送一个分片
    // new_file->slices = (size_t *)malloc(sizeof(size_t) * (threads + 1));
    // size_t slice_size = size / threads;
    // 为每个线程平均分配文件分片
    // 第一个线程发送的内容为[0, slices[1]]
    // 其余线程发送的内容为[slices[i] + 1, slices[i + 1]]
    // for (int i = 0; i < threads; i ++){
    //     new_file->slices[i] = i * slice_size;
    // }
    // 最后一个线程的分片大小为剩余的文件大小
    // new_file->slices[threads] = size;
    new_file->next = NULL;
    // 将文件信息添加到发送列表中
    if (file_head == NULL){
        file_head = new_file;
        file_tail = new_file;
    } else {
        file_tail->next = new_file;
        file_tail = new_file;
    }
    LogBlue("File \"%s\" added, size: %ld", filename, size);
    return 0;
}

/**
 * 计算文件的md5校验值, 确保MD5中有足够的空间用于存储md5, 否则会出现未知的行为
 * @param fp 文件指针
 * @param md5 用于存储md5校验值的字符串
 * @return void
*/
void calc_md5(FILE *fp, char *md5){
    unsigned char decrypt[16];
    MD5_CTX md5_ctx;
    MD5Init(&md5_ctx);
    do {
        unsigned char encrypt[1024];
        while(!feof(fp)){
            MD5Update(&md5_ctx, encrypt, fread(encrypt, 1, 1024, fp));
        }
        fseek(fp, 0, SEEK_SET);
    } while(0);
    MD5Final(&md5_ctx, decrypt);
    for (int i = 0; i < 16; i ++){
        sprintf(md5 + i * 2, "%02x", decrypt[i]);
    }
}

/**
 * 检查文件的md5校验值
 * @param fp 文件指针
 * @param target_md5 目标md5校验值
 * @return 一致返回1, 不一致返回0
*/
int check_md5(FILE *fp, char *target_md5){
    char md5[33];
    fseek(fp, 0, SEEK_SET);
    calc_md5(fp, md5);
    // printf("md5: %s\n", md5);
    // printf("target_md5: %s\n", target_md5);
    return strcmp(md5, target_md5) == 0 ? 1 : 0;
}
/**
 * 从发送列表中删除文件
 * @param index 文件序号
 * @return void
*/
void remove_file(int index){
    if (file_head == NULL){
        LogRed("No files added\n");
        return;
    }
    int i = 0;
    file_node *prev = NULL;
    file_node *current = file_head;
    int threads = atoi(get_config("threads"));
    for (; current != NULL && i <= index; i ++, current = current -> next){
        if (i == index){
            prev -> next = current -> next;
            fclose(current->fp);
            // for (int i = 0; i < threads; i ++){
            //     fclose(current->fps[i]);
            // }
            // free(current->fps);
            free(current);
            return;
        }
        prev = current;
    }
    LogRed("No such file\n");
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
    LogBlue("No.  : %10s - %s - %s", "Size(B)", "Filename", "MD5");
    for (int i = 0; current != NULL; i ++, current = current -> next){
        LogBlue("No.%2d: %10ldB - %s - %s", i, current->size, current->filename, current->md5);
    }
    // 打印分片信息
    // for (current = file_head; current != NULL; current = current -> next){
    //     LogBlue("File: %s", current->filename);
    //     for (int i = 0; i < atoi(get_config("threads")); i ++){
    //         LogBlue("Slice %d: %ld - %ld", i, current->slices[i], current->slices[i + 1]);
    //     }
    // }
}

/**
 * 释放发送列表中的所有文件
*/
void release_files(){
    file_node *current = file_head;
    file_node *next;
    size_t threads = atoi(get_config("threads"));
    while(current != NULL){
        next = current -> next;
        // free(current->slices);
        fclose(current->fp);
        // for (int i = 0; i < threads; i ++){
        //     fclose(current->fps[i]);
        // }
        // free(current->fps);
        free(current);
        current = next;
    }
    file_head = NULL;
    file_tail = NULL;
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
        // sprintf(buf, "new %s %s", current->md5, current->filename);
        // 构造发送的控制信息
        CTRLINFO ctrlinfo = {
            .magic = "ctrl", .type = NEW, 
            .offset = 0, .size = current->size};
        strcpy(ctrlinfo.filename, current->filename);
        strcpy(ctrlinfo.md5, current->md5);
        memcpy(buf, &ctrlinfo, sizeof(ctrlinfo));
        // 发送控制信息, 该控制信息是一个新文件, 发送的内容包括了文件名, 大小和md5校验值
        write(connfd, buf, sizeof(CTRLINFO));
        memset(buf, 0, sizeof(buf));
        // 发送文件名后, 等待接收方确认
        LogBlue("Sending file: %s", current->filename);
        size_t nbytes_left;
        if (read(connfd, buf, sizeof(buf)) < 0){
            perror("send error!");
            return;
        } else {
            // char *ok = strtok(buf, " ");
            // size_t end_off = atol(buf + strlen(ok) + 1);
            // assert(strcmp(ok, "ok") == 0);
            CTRLINFO *ctrlinfo_ptr = (CTRLINFO *)buf;
            assert(strncmp(ctrlinfo_ptr->magic, "ctrl", 4) == 0);
            assert(ctrlinfo_ptr->type == OK_TO_RECEIVE);
            size_t end_off = ctrlinfo_ptr->offset;
            nbytes_left = ctrlinfo_ptr->size;
            // lseek(current->fd, end_off, SEEK_SET);
            if (end_off == current->size){
                LogBlue("File already exists on the receiver side, skip sending");
                continue;
            } else {
                LogBlue("Start to send file: %s from the offset %ld with %ld bytes", current->filename, end_off, nbytes_left);
                // lseek(current->fd, end_off, SEEK_SET);
                fseek(current->fp, end_off, SEEK_SET);
            }
            memset(buf, 0, sizeof(buf));
        }
        size_t n;
        // 否则使用read-send的方式发送文件
        int ret, err;
        while(!feof(current->fp)){
            n = fread(buf, sizeof(char), nbytes_left > BUF_SIZE ? nbytes_left : BUF_SIZE, current->fp);
            nbytes_left -= n;
            if((ret = write(connfd, buf, n)) < 0){
                printf("write error: %d\n", err);
            }
            memset(buf, 0, n);
        }
        // 当一个文件发送完毕后, 向接收方发送当前文件传输结束信号
        CTRLINFO fin_ctrl = {
            .magic = "ctrl", .type = FIN
        };
        write(connfd, &fin_ctrl, sizeof(CTRLINFO));
        read(connfd, &fin_ctrl, sizeof(CTRLINFO));
        assert(strncmp(fin_ctrl.magic, "ctrl", 4) == 0);
        assert(fin_ctrl.type == FIN);
        // assert(strcmp(buf, "fin") == 0);
        fseek(current->fp, 0, SEEK_SET);
    }

    // 完成所有文件的发送后, 向接收方发送结束信号
    CTRLINFO finall_ctrl = {
        .magic = "ctrl", .type = FINALL
    };
    write(connfd, &finall_ctrl, sizeof(CTRLINFO));

    printf("All files sent\n");
}

/**
 * 接收文件
 * @param connfd 连接描述符
 * @return void
*/
void recv_files(int connfd){
    FILE *fp = NULL;
    size_t recv_size;
    char buf[BUF_SIZE];
    char filepath[128];
    memset(buf, 0, sizeof(buf));
    struct timeval tv;    
    time_t secs = 0;
    size_t bytes_received = 0;
    char *md5;
    while((recv_size = read(connfd, buf, sizeof(buf))) > 0){
        CTRLINFO *ctrlinfo_ptr = (CTRLINFO *)(buf + recv_size - sizeof(CTRLINFO));
        // printf("%s", buf);
        if (strncmp(ctrlinfo_ptr->magic, "ctrl", 4) == 0){
            if (ctrlinfo_ptr->type == NEW){
                if (fp != NULL) fclose(fp);
                // 从 buf 中分割出md5值
                strtok(buf, " ");
                char *md5_buf = strtok(NULL, " ");
                strcpy(md5, md5_buf);

                // 从 buf 中分割出文件名
                char *savepath = get_config("savepath");
                char *filename = strtok(NULL, " ");
                // filename = buf + strlen(filename) + 1;
                filename = basename(filename);
                printf("receiving new file: %s\n", filename);
                sprintf(filepath, "%s/%s", savepath, filename);
                // 创建文件, 并设置权限
                fp = fopen(filepath, "r+");
                if (fp == NULL){
                    fp = fopen(filepath, "w+");
                    if (fp == NULL){
                        perror("failed to create file");
                        return ;
                    }
                }
                // 向发送方发送确认信息, 通知发送方可以开始发送文件内容
                // 且发送文件的末尾位置
                // size_t end_off = lseek(fd, 0, SEEK_END);
                if (fseek(fp, 0, SEEK_END) < 0){
                    perror("fseek error");
                    return ;
                }
                size_t end_off = ftell(fp);
                memset(buf, 0, sizeof(buf));
                int len = sprintf(buf, "ok %ld\n", end_off);
                printf("%s\n", buf);
                assert(len != -1);
                // send(connfd, "ok", 3, 0);
                CTRLINFO ok_ctrl = {
                    .magic = "ctrl", .type = OK_TO_RECEIVE,
                    .offset = end_off, .size = ctrlinfo_ptr->size
                };
                write(connfd, &ok_ctrl, sizeof(CTRLINFO));
                continue;
            } else if (ctrlinfo_ptr->type == FIN){
                // 接收到发送方的结束信号, 关闭文件描述符
                // 确保我方开启了md5校验且对方发送了正确的md5值
                if (atoi(get_config("md5")) && strcmp(md5, "0") != 0){
                    LogBlue("md5 checking!!");
                    if (check_md5(fp, md5)){
                        printf("md5 check passed\n");
                    } else {
                        printf("md5 check failed\n");
                    }
                }
                // 准备接收下一个文件
                // send(connfd, "fin", 4, 0);
                // send(connfd, "fin", 4, 0);
                CTRLINFO fin_ctrl = {
                    .magic = "ctrl", .type = FIN
                };
                write(connfd, &fin_ctrl, sizeof(CTRLINFO));
                // write(fd, buf, recv_size - 4);
                fwrite(buf, recv_size - 4, 1, fp);
                fflush(fp);
                memset(buf, 0, recv_size);
                printf("\nsaved to %s\n", filepath);
                continue;
            } else if (ctrlinfo_ptr->type == FINALL){
                // write(fd, buf, recv_size - 7);
                fwrite(buf, recv_size - sizeof(CTRLINFO), 1, fp);
                fflush(fp);
                memset(buf, 0, recv_size);
                break;
            }
        } else {
            // write(fd, buf, recv_size);
            fwrite(buf, recv_size, 1, fp);
            fflush(fp);
            if (gettimeofday(&tv, NULL) == 0){
                bytes_received += recv_size;
                if (tv.tv_sec != secs){
                    secs = tv.tv_sec;
                    printf("\rreceiving speed: %ld MB/s                ", bytes_received >> 20);
                    fflush(stdout);
                    bytes_received = 0;
                }
            }
        }
        memset(buf, 0, sizeof(buf));
    }
    // if (fd != 0) close(fd);
    if (fp != NULL) fclose(fp);
}
/**
 * 发送发送列表中的所有文件
 * @param connfd 连接描述符
*/
void send_files_ssl(int connfd){
    if (file_head == NULL){
        printf("No files added\n");
        return;
    }
    file_node *current = file_head;
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    for (int i = 0; current != NULL; i ++, current = current -> next){
        // sprintf(buf, "new %s %s", current->md5, current->filename);
        // 构造发送的控制信息
        CTRLINFO ctrlinfo = {
            .magic = "ctrl", .type = NEW, 
            .offset = 0, .size = current->size};
        // printf("%x, %x, %x, %x\n", &ctrlinfo, ctrlinfo.magic, ctrlinfo.filename, ctrlinfo.md5);
        strcpy(ctrlinfo.filename, current->filename);
        strcpy(ctrlinfo.md5, current->md5);
        memcpy(buf, &ctrlinfo, sizeof(CTRLINFO));
        // 发送控制信息, 该控制信息是一个新文件, 发送的内容包括了文件名, 大小和md5校验值
        // send(connfd, buf, strlen(buf), 0);
        SSL_write(ssl, &ctrlinfo, sizeof(CTRLINFO));
        memset(buf, 0, sizeof(buf));
        // 发送文件名后, 等待接收方确认
        LogBlue("Sending file: %s", current->filename);
        // if (recv(connfd, buf, sizeof(buf), 0) < 0){
        size_t nbytes_left;
        if (SSL_read(ssl, buf, sizeof(buf)) < 0){
            perror("send error!");
            return;
        } else {
            // char *ok = strtok(buf, " ");
            // size_t end_off = atol(buf + strlen(ok) + 1);
            // assert(strcmp(ok, "ok") == 0);
            CTRLINFO *ctrlinfo_ptr = (CTRLINFO *)buf;
            // printf("ctrlinfo_ptr->magic: %s\n", ctrlinfo_ptr->magic);
            assert(strncmp(ctrlinfo_ptr->magic, "ctrl", 4) == 0);
            assert(ctrlinfo_ptr->type == OK_TO_RECEIVE);
            size_t end_off = ctrlinfo_ptr->offset;
            nbytes_left = ctrlinfo_ptr->size;
            // lseek(current->fd, end_off, SEEK_SET);
            if (end_off == current->size){
                LogBlue("File already exists on the receiver side, skip sending");
                continue;
            } else {
                LogBlue("Start to send file: %s from the offset %ld with %ld bytes", current->filename, end_off, nbytes_left);
                // lseek(current->fd, end_off, SEEK_SET);
                fseek(current->fp, end_off, SEEK_SET);
            }
            memset(buf, 0, sizeof(buf));
        }
        
        size_t n;
        // 否则使用read-send的方式发送文件
        int ret, err;
        while(!feof(current->fp) && nbytes_left > 0){
            n = fread(buf, sizeof(char), nbytes_left < BUF_SIZE ? nbytes_left : BUF_SIZE, current->fp);
            nbytes_left -= n;
            if((ret = SSL_write(ssl, buf, n)) < 0){
                err = SSL_get_error(ssl, ret);
                printf("SSL_write error: %d\n", err);
            }
            memset(buf, 0, n);
        }

        LogBlue("Complete transfer %s", current->filename);
        CTRLINFO fin_ctrl = {
            .magic = "ctrl", .type = FIN
        };
        SSL_write(ssl, &fin_ctrl, sizeof(CTRLINFO));
        SSL_read(ssl, &fin_ctrl, sizeof(CTRLINFO));
        assert(strncmp(fin_ctrl.magic, "ctrl", 4) == 0);
        assert(fin_ctrl.type == FIN);
        // assert(strcmp(buf, "fin") == 0);
        fseek(current->fp, 0, SEEK_SET);
    }

    // 完成所有文件的发送后, 向接收方发送结束信号
    // send(connfd, "finall", 7, 0);
    LogBlue("All files sent");
    CTRLINFO finall_ctrl = {
        .magic = "ctrl", .type = FINALL
    };
    SSL_write(ssl, &finall_ctrl, sizeof(CTRLINFO));

    printf("All files sent\n");
}

/**
 * 接收文件
 * @param connfd 连接描述符
 * @return void
*/
void recv_files_ssl(int connfd){
    FILE *fp = NULL;
    size_t recv_size;
    char buf[BUF_SIZE];
    char filepath[128];
    memset(buf, 0, sizeof(buf));
    struct timeval tv;    
    time_t secs = 0;
    size_t bytes_received = 0;
    char md5[33];
    // while((recv_size = recv(connfd, buf, sizeof(buf), 0)) > 0){
    while((recv_size = SSL_read(ssl, buf, sizeof(buf))) > 0){
        CTRLINFO *ctrlinfo_ptr = (CTRLINFO *)(buf + recv_size - sizeof(CTRLINFO));
        // printf("recv_size: %ld\n", recv_size);
        // printf("recv_size: %ld buf: %s\n", recv_size, buf);
        if (strncmp(ctrlinfo_ptr->magic, "ctrl", 4) == 0){
            if (ctrlinfo_ptr->type == NEW){
                if (fp != NULL) fclose(fp);
                // 从 buf 中分割出md5值
                char *md5_buf = ctrlinfo_ptr->md5;
                strcpy(md5, md5_buf);

                // 从 buf 中分割出文件名
                char *savepath = get_config("savepath");
                char *filename = ctrlinfo_ptr->filename;
                // filename = buf + strlen(filename) + 1;
                filename = basename(filename);
                printf("receiving new file: %s\n", filename);
                sprintf(filepath, "%s/%s", savepath, filename);
                // 创建文件, 并设置权限
                fp = fopen(filepath, "r+");
                if (fp == NULL){
                    fp = fopen(filepath, "w+");
                    if (fp == NULL){
                        perror("failed to create file");
                        return ;
                    }
                }
                // 向发送方发送确认信息, 通知发送方可以开始发送文件内容
                // 且发送文件的末尾位置
                // size_t end_off = lseek(fd, 0, SEEK_END);
                if (fseek(fp, 0, SEEK_END) < 0){
                    perror("fseek error");
                    return ;
                }
                size_t end_off = ftell(fp);
                // memset(buf, 0, sizeof(buf));
                // int len = sprintf(buf, "ok %ld\n", end_off);
                // printf("%s\n", buf);
                // assert(len != -1);
                CTRLINFO ok_ctrl = {
                    .magic = "ctrl", .type = OK_TO_RECEIVE,
                    .offset = end_off, .size = ctrlinfo_ptr->size - end_off
                };
                // printf("ctrl->size: %ld\n", ctrlinfo_ptr->size);
                SSL_write(ssl, &ok_ctrl, sizeof(CTRLINFO));
                memset(buf, 0, sizeof(buf));
                // send(connfd, "ok", 3, 0);
                // SSL_write(ssl, buf, len);
                continue;
            } else if (ctrlinfo_ptr->type == FIN){
                // 接收到发送方的结束信号, 关闭文件描述符
                // 确保我方开启了md5校验且对方发送了正确的md5值
                if (atoi(get_config("md5")) && strcmp(md5, "0") != 0){
                    LogBlue("md5 checking!!");
                    if (check_md5(fp, md5)){
                        printf("md5 check passed\n");
                    } else {
                        printf("md5 check failed\n");
                    }
                }
                // 准备接收下一个文件
                // send(connfd, "fin", 4, 0);
                CTRLINFO fin_ctrl = {
                    .magic = "ctrl", .type = FIN
                };
                SSL_write(ssl, &fin_ctrl, sizeof(CTRLINFO));
                // write(fd, buf, recv_size - 4);
                fwrite(buf, recv_size - sizeof(CTRLINFO), 1, fp);
                fflush(fp);
                memset(buf, 0, recv_size);
                printf("\nsaved to %s\n", filepath);
                continue;
            } else if (ctrlinfo_ptr->type == FINALL){
                // write(fd, buf, recv_size - 7);
                fwrite(buf, recv_size - sizeof(CTRLINFO), 1, fp);
                fflush(fp);
                memset(buf, 0, recv_size);
                break;
            }
        } else {
            fwrite(buf, sizeof(char), recv_size, fp);
            fflush(fp);
            if (gettimeofday(&tv, NULL) == 0){
                bytes_received += recv_size;
                if (tv.tv_sec != secs){
                    secs = tv.tv_sec;
                    printf("\rreceiving speed: %ld MB/s                ", bytes_received >> 20);
                    fflush(stdout);
                    bytes_received = 0;
                }
            }
        }
        // write(fd, buf, recv_size);
        // printf("\n");
        memset(buf, 0, sizeof(buf));
    }
    if (fp != NULL) fclose(fp);
}
