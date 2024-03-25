#include <cmd.h>
#include <common.h>
#include <config.h>
#include <server.h>
#include <client.h>
#include <portalfile.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

int connfd = 0;

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

static struct {
    const char *name;
    const char *description;
    int (*handler)(char *);
} cmd_table[] = {
    {"help"   , "Display information about all supported commands", cmd_help},
    {"quit"   , "Exit", cmd_quit},
    {"config" , "set the target config as the value, usage: config <option> <value>", cmd_config},
    {"listen" , "listen connection", cmd_listen},
    {"connect", "connect to the server, usage: connect <host>", cmd_connect},
    {"send"   , "send message or file", cmd_send},
    {"receive", "receive message or file", cmd_receive},
    {"close"  , "close the connection", cmd_close_conn},
    {"add"    , "add file to the sending list", cmd_add},
    {"list"   , "list all files in the sending list", cmd_list}

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_quit(char *args){
    close(connfd);
    exit(0);
}

static int cmd_config(char *args){
    if (args == NULL){
        config_help();
        return 0;
    }
    char *option = strtok(NULL, " ");
    char *value = strtok(NULL, " ");
    config(option, value);
    
}

static int cmd_listen(char *args){
    if (connfd){
        printf("Please close the connection first\n");
        return 0;
    }
    listen_as_server(get_config("port"));
}

static int cmd_connect(char *args){
    if (args == NULL){
        printf("Usage: connect <ip>\n");
        return 0;
    }
    if (connfd){
        printf("Please close the connection first\n");
        return 0;
    }
    char *host = strtok(NULL, " ");

    connfd = connect_as_client(host, get_config("port"));
    printf("Connected to %s:%s on fd %d\n", host, get_config("port"), connfd);
}

static int cmd_close_conn(char *args){
    if (connfd == 0){
        printf("Please connect to the server first\n");
        return 0;
    }
    close(connfd);
    connfd = 0;
    printf("Connection closed\n");
}

static int cmd_send(char *args){
    if (connfd == 0){
        printf("Please connect to the server first\n");
        return 0;
    }
    // if (args != NULL){
    //     // write(connfd, args, strlen(args));
    //     send(connfd, args, strlen(args), 0);
    // }
    send_files(connfd);
}

static int cmd_receive(char *args){
    if (connfd == 0){
        printf("Please connect to the server first\n");
        return 0;
    }
    // char buf[1024];
    // // int n = read(connfd, buf, sizeof(buf));
    // int n = recv(connfd, buf, sizeof(buf), 0);
    // if (n > 0){
    //     buf[n] = '\0';
    //     printf("%s\n", buf);
    // }
    recv_files(connfd);
    return 0;
}

static int cmd_add(char *args){
    if (args == NULL){
        printf("Usage: add <filename>\n");
        return 0;
    }
    char *path;
    while((path = strtok(NULL, " ")) != NULL){
        struct stat s_buf;
        stat(path, &s_buf);
        if (S_ISDIR(s_buf.st_mode)){
            add_dir(path);
        } else {
            add_file(path);
        }
    }
}

static int cmd_list(char *args){
    list_files();
    return 0;
}


void portal_cli(){
    char *prompt = "(PortalFT) ";
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
            printf("Unknown command '%s'\n", cmd);
        }
    }
}