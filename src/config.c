#include <common.h>
#include <cmd.h>
#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/*
配置项列表, 包括配置项名称, 配置项描述, 配置项索引
通过索引在config_list中查找对应的值
*/
static struct{
	const char *option;
	const char *description;
	const size_t index;
} config_table[] = {
	{"port", "config the port to listen", PORT},
	{"savepath", "config the path to save the file", SAVEPATH},
	{"threads", "config the number of threads", THREADS},
};

/*
配置项的值列表
*/
static char config_list[NR_CONFIG][128] = {
	"5339",
	".",
	"4"
};

/**
 * 设置参数option对应的值为value
 * 若option不存在, 则打印帮助信息
 * @param option 配置项名称
 * @param value 配置项值
 * @return 成功则返回1, 失败返回0
 */
int config(char *option, char *value)
{
	int i;
	if (strcmp(option, "list") == 0){
		for (i = 0; i < NR_CONFIG; i ++){
			printf("%-10s: %s\n", config_table[i].option, config_list[config_table[i].index]);
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
	config_help();
	return 0;
}

/**
 * 获取参数option对应的值
 * 若目标参数不存在, 则打印帮助信息
 * @param option 配置项名称
 * @return 成功则返回option对应的值, 失败返回NULL
 */
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

/**
 * 打印配置帮助信息
*/
int config_help()
{
	printf("usage: config <option> <value>\n");
	for (int i = 0; i < NR_CONFIG; i++)
	{
		printf("%-10s - %s\n", config_table[i].option, config_table[i].description);
	}
	return 0;
}

/**
 * 从文件config.ini中读取配置, 进行配置初始化
 * @param 无
 * @return 成功则返回0, 失败返回-1
*/
int config_init(char* config_file){
	FILE *fp = fopen(config_file ? config_file : "config.ini", "r");
	if (fp == NULL){
		printf("config.ini not found\n");
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