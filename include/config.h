#ifndef __CONFIG_H__
#define __CONFIG_H__

enum{
    PORT,
    SAVEPATH,
    THREADS,
    SSL_ENABLE,
    MD5_ENABLE,
    NR_CONFIG

};
int config(char *option, char *value);
int config_help();
int config_init(char *config_file);
char *get_config(char *option);

#endif