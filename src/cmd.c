#include <cmd.h>
#include <common.h>
#include <string.h>
#include <utils.h>
#include <config.h>
#include <server.h>
#include <client.h>
#include <portalfile.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

int connfd = 0;
int connfd_list[MAXTHREAD] = {0};
int listenfd = 0;

extern SSL_CTX *ctx;
SSL *ssl = NULL;
SSL **ssl_list = NULL;

static int cmd_help(char *args);
static int cmd_quit(char *args);
static int cmd_config(char *args);
static int cmd_listen(char *args);
static int cmd_connect(char *args);
static int cmd_send(char *args);
static int cmd_receive(char *args);
static int cmd_close_conn(char *args);
static int cmd_add(char *args);
static int cmd_remove(char *args);
static int cmd_list(char *args);

/**
 * 获取用户的输入, 并返回用户输入的字符串
 * 使用prompt作为提示符
 * 使用`add_history`将用户输入
 * 字符串添加到历史记录中, 可通过上下键查
 * 历史记录
 * 在输入文件路径时, 可使用tab键自动补全
 * @param prompt 提示符
 * @return 用户输入的字符串
 */
char *rl_gets(char *prompt){
    static char *line_read = NULL;
    if (line_read){
        free(line_read);
        line_read = NULL; 
    }
    line_read = readline(prompt);
    if (line_read && *line_read){
        add_history(line_read);
    }
    return line_read;
}

/**
 * 定义了指令列表, 包括指令名称, 指令描述, 指令处理函数
 * 程序会自动比较用户输入的指令名称, 并调用对应的回调函数进行处理
 * help    : 显示所有支持的指令
 * quit    : 退出程序
 * config  : 设置配置项
 * listen  : 作为服务器监听连接
 * connect : 作为客户端连接服务器
 * send    : 发送文件
 * receive : 接收文件
 * add     : 添加文件到发送列表
 * list    : 列出发送列表中的文件
 * close   : 关闭连接
*/
static struct {
    const char *name;
    const char *description;
    int (*handler)(char *);
} cmd_table[] = {
    {"help"    , "Display information about all supported commands"                   , cmd_help},
    {"quit"    , "Exit"                                                               , cmd_quit},
    {"config"  , "set the target config as the value, usage: config <option> <value>" , cmd_config},
    {"listen"  , "listen connection"                                                  , cmd_listen},
    {"connect" , "connect to the server, usage: connect <host>"                       , cmd_connect},
    {"send"    , "send message or file"                                               , cmd_send},
    {"receive" , "receive message or file"                                            , cmd_receive},
    {"add"     , "add file to the sending list"                                       , cmd_add},
    {"remove"  , "revome file from the sending list"                                  , cmd_remove},
    {"list"    , "list all files in the sending list"                                 , cmd_list},
    {"close"   , "close the connection"                                               , cmd_close_conn},
};

#define NR_CMD ARRLEN(cmd_table)

/**
 * 帮助函数, 显示所有支持的指令
 * @param args 未使用
 * @return 0
 */
static int cmd_help(char *args) {
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
    printf("%-10s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%-10s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

/**
 * 退出函数, 关闭连接后退出程序
 */
static int cmd_quit(char *args){
    close(connfd);
    SSL_CTX_free(ctx);
    release_files();
    return -1;
}

/**
 * 配置函数, 用于设置配置项
 * 可配置的选项包括:
 * port     : 服务器监听的端口
 * savepath : 文件保存的路径
 * 
 */
static int cmd_config(char *args){
    char *option = strtok(NULL, " ");
    if (option == NULL){
        config_help();
        return 0;
    } else if (strcmp(option, "help") == 0){
        config_help();
        return 0;
    }
    char *value = strtok(NULL, " ");
    if (value == NULL){
        printf(ANSI_FG_BLUE "Usage: config <option> <value>\n" ANSI_NONE);
        return 0;
    }
    return config(option, value);
}

/**
 * 作为服务器监听连接
 * 不需要参数, 监听的端口使用config命令进行配置
 */
static int cmd_listen(char *args){
    int ssl_enable = atoi(get_config("ssl"));
    if (ssl_enable){
        listen_as_server_ssl(get_config("port"));
    } else {
        if (connfd){
            printf(ANSI_FG_RED "Please close the connection first\n" ANSI_NONE);
            return 0;
        }
        listen_as_server(get_config("port"));
    }
    // 监听端口, 并设置连接的文件描述符
    return 0;
}

/**
 * 连接到服务器
 * 需要参数, 参数为服务器的ip地址或url
 */
static int cmd_connect(char *args){
    if (args == NULL){
        printf(ANSI_FG_BLUE "Usage: connect <ip>\n" ANSI_NONE);
        return 0;
    }
    char *host = strtok(NULL, " ");
    int ssl_enable = atoi(get_config("ssl"));
    if (ssl_enable){
        int num_threads = atoi(get_config("threads"));
        ssl_list = malloc(sizeof(SSL *) * num_threads);
        for (int i = 0; i < num_threads; i++) {
            ssl_list[i] = connect_as_client_ssl(ctx, host, get_config("port"));
        }
    } else {
        if (connfd){
            printf(ANSI_FG_RED "Please close the connection first\n" ANSI_NONE);
            return 0;
        }
        connfd = connect_as_client(host, get_config("port"));
    }
    printf(ANSI_FG_GREEN "Connected to %s:%s\n" ANSI_NONE, host, get_config("port"));
    return 0;
}

/**
 * 关闭连接
 */
static int cmd_close_conn(char *args){
    if (connfd == 0){
        printf("Please connect to the server first\n");
        return 0;
    }
    close(connfd);
    connfd = 0;
    printf("Connection closed\n");
    return 0;
}

void *send_files_thread(void *arg){
    int ssl_index = *(int *)arg;
    send_files_ssl(ssl_list[ssl_index]);
    return NULL;
}

void *recv_files_thread(void *arg){
    int ssl_index = *(int *)arg;
    recv_files_ssl(ssl_list[ssl_index], ssl_index);
    return NULL;
}

/**
 * 在建立连接后, 向对方发送文件列表中的文件
 * 在对方确认接收之前, 会阻塞, 直到对方确认接收后, 开始向对方发送文件
 */
static int cmd_send(char *args){
    int ssl_enable = atoi(get_config("ssl"));
    if (ssl_enable){
        if (ssl_list == NULL) {
            printf("Please connect to the server first\n");
            return 0;
        }
        int num_threads = atoi(get_config("threads"));
        pthread_t *tids = malloc(sizeof(pthread_t) * num_threads);
        int *indexs = malloc(sizeof(int) * num_threads);
        for (int i = 0; i < num_threads; i++){
            indexs[i] = i;
            pthread_create(&tids[i], NULL, send_files_thread, indexs + i);
        }
        for (int i = 0;i < num_threads; i++){
            pthread_join(tids[i], NULL);
        }
        free(tids);
        free(indexs);
        // send_files_ssl(ssl);
    } else {
        if (connfd == 0){
            printf("Please connect to the server first\n");
            return 0;
        }
        send_files(connfd);
    }
    return 0;
}

/**
 * 在建立连接后, 接收对方发送的文件
 * 在对方确认发送之前, 会阻塞, 直到对方开始发送文件
 * 接收的文件将会保存在配置的路径中
 */
static int cmd_receive(char *args){
    int ssl_enable = atoi(get_config("ssl"));
    if (ssl_enable){
        if (ssl_list == NULL){
            printf(ANSI_FG_RED "Please connect to the server first\n" ANSI_NONE);
            return 0;
        }
        int num_threads = atoi(get_config("threads"));
        printf("thread num: %d\n", num_threads);
        pthread_t *tids = malloc(sizeof(pthread_t) * num_threads);
        int *indexs = malloc(sizeof(int) * num_threads);
        start_recv();
        for (int i = 0; i < num_threads; i++){
            indexs[i] = i;
            pthread_create(&tids[i], NULL, recv_files_thread, indexs + i);
        }
        // 创建一个线程用于计算接收速度
        pthread_t speed_tid;
        pthread_create(&speed_tid, NULL, recv_speed_calc, NULL);

        for (int i = 0;i < num_threads; i++){
            pthread_join(tids[i], NULL);
        }
        // 令接收速度线程退出
        recv_over();
        pthread_join(speed_tid, NULL);
        free(tids);
        free(indexs);
        free_size_record();
    } else {
        if (connfd == 0){
            printf(ANSI_FG_RED "Please connect to the server first\n" ANSI_NONE);
            return 0;
        }
        recv_files(connfd);
    }
    return 0;
}

/**
 * 向发送列表中添加文件
 * 参数可以为多个文件路径
 * 若文件路径为目录, 则会将目录下的所有文件添加到发送列表中
 * 若文件路径为普通文件, 则直接将文件添加到发送列表中
 */
static int cmd_add(char *args){
    if (args == NULL){
        printf(ANSI_FG_BLUE "Usage: add <filename>\n" ANSI_NONE);
        return 0;
    }
    char *path;
    while((path = strtok(NULL, " ")) != NULL){
        struct stat s_buf;
        stat(path, &s_buf);
        // 判断文件类型, 若为目录则添加目录下的所有文件
        if (S_ISDIR(s_buf.st_mode)){
            add_dir(path);
        } else {
            add_file(path);
        }
    }
    return 0;
}

static int cmd_remove(char *args){
    if (args == NULL){
        printf(ANSI_FG_BLUE "Usage: remove <filename>\n" ANSI_NONE);
        return 0;
    }
    char *NO_s = strtok(NULL, " ");
    int NO = atoi(NO_s);
    if (NO == 0){
        printf(ANSI_FG_RED "Please input the correct number\n" ANSI_NONE);
        return 0;
    }
    remove_file(NO);
    return 0;
}

/**
 * 列出发送列表中的所有文件
 */
static int cmd_list(char *args){
    list_files();
    return 0;
}

/**
 * 用于自动补全用户输入的指令
 * @param text 用户输入的字符串
 * @param state 状态
 * @return 自动补全的字符串
*/
char *command_generator(const char *text, int state){
    // char *name = NULL;
    if (!state){
        for (int i = 0; i < NR_CMD; i ++){
            if (!strncmp(cmd_table[i].name, text, strlen(text))){
                return strdup(cmd_table[i].name);
            }
        }
    }
    return NULL;
}

/**
 * 用于自动补全用户输入的指令
 * @param text 用户输入的字符串
 * @param start 开始位置
 * @param end 结束位置
 * @return 自动补全的字符串列表
*/
char **command_complete(const char *text, int start, int end){
    char **matches = NULL;
    if (!start){
        matches = rl_completion_matches(text, command_generator);
    }
    return matches;
}

/**
 * 主循环, 不断获取用户输入, 并调用对应的处理函数
 * 若输入未知指令, 则显示帮助信息
 */
void portal_cli(){
    char *prompt = "(PortalFT) ";
    // 设置自动补全函数
    rl_attempted_completion_function = command_complete;
    for (char *str; (str = rl_gets(prompt)) != NULL; ){
        char *str_end = str + strlen(str);

        char *cmd = strtok(str, " ");
        if (cmd == NULL){
            continue;
        }

        char *args = cmd + strlen(cmd) + 1;
        if (args >= str_end){
            args = NULL;
        }
        int i;
        for (i = 0; i < NR_CMD; i ++){
            if (strcmp(cmd, cmd_table[i].name) == 0){
                if (cmd_table[i].handler(args) < 0){
                    return;
                }
                break;
            }
        }

        if (i == NR_CMD){
            printf(ANSI_FG_RED "Unknown command '%s'\n" ANSI_NONE, cmd);
            cmd_help(NULL);
        }
    }
}