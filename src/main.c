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
    // SSL库初始化
    SSL_library_init();
    // 载入所有的SSL算法
    OpenSSL_add_all_algorithms();
    // 载入所有的SSL错误信息
    SSL_load_error_strings();
}

// 主函数, 完成基本的初始化工作, 并启动Portal终端
int main(int argc, char **argv){
    // 解析输入参数
    parse_args(argc, argv);
    // 初始化配置
    config_init(config_file);
    // 初始化ssl连接
    ssl_init();
    // 初始化log输出
    init_log(log_file);

    // 启动portal终端, 开始操作
    portal_cli();

    return 0;
}