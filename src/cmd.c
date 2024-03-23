#include <cmd.h>
#include <common.h>
#include <readline/history.h>
#include <readline/readline.h>

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