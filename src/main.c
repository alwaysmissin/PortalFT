#include <common.h>
#include <cmd.h>
#include <config.h>
#include <getopt.h>
#include <utils.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
// #include <server.h>
// #include <client.h>
// #include <readline/readline.h>
// #include <readline/history.h>
static char* config_file = NULL;
static char* log_file = NULL;

SSL_CTX *ctx;

static int parse_args(int argc, char *argv[]){
    const struct option table[] = {
        {"init", required_argument, NULL, 'i'},
        {"log" , required_argument, NULL, 'l'},
        {0     , 0                , 0   , 0}
    };
    int o;
    while((o = getopt_long(argc, argv, "i:l:", table, NULL)) != -1){
        switch(o){
            case 'i':
                config_file = optarg;
                break;
            case 'l':
                log_file = optarg;
                break;
            default:
                exit(0);
        }
    }
    return 0;
}

void ssl_init(){
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
}

// 主函数, 完成基本的初始化工作, 并启动Portal终端
int main(int argc, char **argv){
    // while(1){
    //     char *line = rl_gets("(Portal) ");
    //     printf("%s\n", line);
    // }
    parse_args(argc, argv);
    config_init(config_file);
    ssl_init();
    init_log(log_file);
    portal_cli();

    return 0;
}