#ifndef __CMD_H__
#define __CMD_H__

void portal_cli();
char *rl_gets(char *prompt);

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
#endif