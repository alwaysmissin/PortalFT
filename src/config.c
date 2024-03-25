#include <common.h>
#include <cmd.h>
#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


struct{
	const char *option;
	const char *description;
	const size_t index;
} config_table[] = {
	{"port", "config the port to listen", 0},
	{"savepath", "config the path to save the file", 1},
};


#define NR_CONFIG ARRLEN(config_table)

char config_list[NR_CONFIG][128] = {
	"5339",
	""
};

int config(char *option, char *value)
{
	int i;
	if (strcmp(option, "list") == 0){
		for (i = 0; i < NR_CONFIG; i ++){
			printf("%s: %s\n", config_table[i].option, config_list[config_table[i].index]);
		}
		return 0;
	}
	for (i = 0; i < NR_CONFIG; i++)
	{
		if (strcmp(config_table[i].option, option) == 0)
		{
			// printf("hello %s\n", config_table[i].value);
			// memcpy(config_table[i].value, value, strlen(value));
			// return option_func_table[i].handler(value);
			size_t index = config_table[i].index;
			strcpy(config_list[index], value);
			return 1;
		}
	}
	printf("Unknown option '%s'\n", option);
	return 0;
}

char *get_config(char *option){
	int i;
	for (i = 0; i < NR_CONFIG; i ++){
		if (strcmp(config_table[i].option, option) == 0){
			return config_list[config_table[i].index];
		}
	}
	config_help();
	return NULL;
}

int config_help()
{
	/* extract the first argument */
		/* no argument given */
	for (int i = 0; i < NR_CONFIG; i++)
	{
		printf("%s - %s\n", config_table[i].option, config_table[i].description);
	}
	return 0;
}

int config_init(){
	FILE *fp = fopen("config.ini", "r");
	if (fp == NULL){
		return -1;
	}
	char buf[128];
	while(fgets(buf, sizeof(buf), fp) != NULL){
		buf[strcspn(buf, "\n")] = '\0';
		char *option = strtok(buf, ":");
		char *value = strtok(NULL, ":");
		config(option, value);
	}
	return 0;	
}