#include <common.h>
#include <server.h>
#include <client.h>
#include <readline/readline.h>
#include <readline/history.h>


int main(int argc, char **argv){
    printf("Hello World\n");
    if (argc > 1){
        if (strcmp(argv[1], "s") == 0 || strcmp(argv[1], "server") == 0){
            server_main(argc, argv);
        } else if (strcmp(argv[1], "c") == 0 || strcmp(argv[1], "client") == 0){
            client_main(argc, argv);
        }
    } else {
        fprintf(stderr, "usage: PortalFT s(erver)/c(lient) <port>/n");
        exit(0);
    }
    server_main(argc, argv);

    return 0;
}