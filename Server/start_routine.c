#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include "../Utilities/common_func.h"
#include "ServerActions/server_actions.h"

int main(int argc, char **argv) {
    struct sockaddr_in6     server_addr;
    unsigned int            port_number;
    char                    *tmp_pointer;
    int                     server_socket_fd, max_queued_clients, max_clients;

    if (argc != 4) err_exit("usage: %s <port> <max clients in queue> <max clients> \n", argv[0]);

    port_number = (unsigned int)strtol(argv[1], &tmp_pointer, 10);
    if (errno == EINVAL || errno == ERANGE) err_exit("can not resolve port number\n", NULL);

    max_queued_clients = (int)strtol(argv[2], &tmp_pointer, 10);
    if (errno == EINVAL || errno == ERANGE) err_exit("can not resolve max queued clients amount\n", NULL);

    max_clients = (int)strtol(argv[3], &tmp_pointer, 10);
    if (errno == EINVAL || errno == ERANGE) err_exit("can not resolve max clients amount\n", NULL);

    if (signal(SIGINT, sig_exit) == (__sighandler_t)-1) err_exit("can not handle SIGINT\n", NULL);

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(port_number);

    if ((server_socket_fd = socket(AF_INET6, SOCK_STREAM, 0)) == -1) err_exit("can not create socket\n", NULL);

    if ((bind(server_socket_fd, (struct sockaddr*) &server_addr, sizeof(server_addr))) != 0) {
        close(server_socket_fd);
        err_exit("can not bind socket\n", NULL);
    }

    if ((listen(server_socket_fd, max_queued_clients)) != 0) {
        close(server_socket_fd);
        err_exit("can not listen\n", NULL);
    }

    main_work_server(server_socket_fd, max_clients);
    return 0;
}
