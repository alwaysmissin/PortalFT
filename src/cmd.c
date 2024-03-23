#include <cmd.h>
#include <common.h>
#include <config.h>
#include <server.h>
#include <client.h>
#include <readline/history.h>
#include <readline/readline.h>

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
    connfd = listen_as_server(get_config("port"));
}

static int cmd_connect(char *args){
    if (args == NULL){
        printf("Usage: connect <ip>\n");
        return 0;
    }
    char *host = strtok(NULL, " ");

    connfd = connect_as_client(host, get_config("port"));
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