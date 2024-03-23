#include <common.h>
#include <cmd.h>
#include <config.h>


struct{
	const char *option;
	const char *description;
	const size_t index;
} config_table[] = {
	{"port", "config the port to listen", 0},
};


#define NR_CONFIG ARRLEN(config_table)

char config_list[NR_CONFIG][10] = {
	"5339",
};

int config(char *option, char *value)
{
	int i;
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