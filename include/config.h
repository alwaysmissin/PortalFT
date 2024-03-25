#ifndef __CONFIG_H__
#define __CONFIG_H__

int config(char *option, char *value);
int config_help();
int config_init();
char *get_config(char *option);

#endif