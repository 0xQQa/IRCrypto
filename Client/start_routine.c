#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <signal.h>
#include "ClientActions/client_actions.h"

void set_ip_v6(void *struct_ip, int port_number, char *addr) {
    ((struct sockaddr_in6*)struct_ip)->sin6_family = AF_INET6;
    ((struct sockaddr_in6*)struct_ip)->sin6_port = htons(port_number);
    if (inet_pton(AF_INET6, addr, &((struct sockaddr_in6*)struct_ip)->sin6_addr) != 1) err_exit("can not resolve IPv6 address\n", NULL);
}

void set_ip_v4(void *struct_ip, int port_number, char *addr) {
    ((struct sockaddr_in*)struct_ip)->sin_family = AF_INET;
    ((struct sockaddr_in*)struct_ip)->sin_port = htons(port_number);
    if (inet_pton(AF_INET, addr, &((struct sockaddr_in*)struct_ip)->sin_addr) != 1) err_exit("can not resolve IPv4 address\n", NULL);
}

int main(int argc, char **argv) {
    size_t  size;
    void    *sockaddr_struct;
    char    *tmp_pointer;
    int     port_number, type;

    if (argc != 5) err_exit("usage: %s <IP type> <IP addr> <port> <nickname>\n", argv[0]);

    if (strlen(argv[4]) > 9) err_exit("max nickname length < 10\n", NULL);

    bzero(user_info.nickname, 10);
    strcpy(user_info.nickname, argv[4]);

    port_number = (int)strtol(argv[3], &tmp_pointer, 10);
    if (errno == EINVAL || errno == ERANGE) err_exit("can not resolve port number\n", NULL);

    if (signal(SIGINT, client_exit) == (__sighandler_t)-1) err_exit("can not handle SIGINT\n", NULL);

    if (strcmp(argv[1], "-v4") == 0) {
        type = AF_INET;
        size = sizeof(struct sockaddr_in);
        sockaddr_struct = calloc(1, size);
        set_ip_v4(sockaddr_struct, port_number, argv[2]);
    }else if (strcmp(argv[1], "-v6") == 0) {
        type = AF_INET6;
        size = sizeof(struct sockaddr_in6);
        sockaddr_struct = calloc(1, size);
        set_ip_v6(sockaddr_struct, port_number, argv[2]);
    } else err_exit("Unknown IP type, choose [-v4|-v6]\n", NULL);

    if ((user_info.sock_fd = socket(type, SOCK_STREAM, 0)) == -1) {
        free(sockaddr_struct);
        err_exit("can not create socket\n", NULL);
    }

    if (connect(user_info.sock_fd, (struct sockaddr *)sockaddr_struct, size) != 0) {
        close(user_info.sock_fd );
        free(sockaddr_struct);
        err_exit("can not connect to server\n", NULL);
    }

    free(sockaddr_struct);
    main_work_client();
    return 0;
}
