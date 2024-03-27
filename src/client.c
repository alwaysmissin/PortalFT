#include <client.h>
#include <connect.h>

/**
 * 作为客户端连接服务器
 * @param host 服务器主机名
 * @param port 服务器端口号
 * @return 成功连接时返回客户端文件描述符，否则返回-1
*/
int connect_as_client(char *host, char *port){
    int clientfd;
    clientfd = open_clientfd(host, port);
    return clientfd;
}
