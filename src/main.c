#include <common.h>
#include <cmd.h>
#include <config.h>
#include <assert.h>
// #include <server.h>
// #include <client.h>
// #include <readline/readline.h>
// #include <readline/history.h>

// 主函数, 完成基本的初始化工作, 并启动Portal终端
int main(int argc, char **argv){
    // while(1){
    //     char *line = rl_gets("(Portal) ");
    //     printf("%s\n", line);
    // }
    assert(config_init() == 0);
    portal_cli();

    return 0;
}