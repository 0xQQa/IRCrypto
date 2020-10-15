
#ifndef P_COMMUNICATOR_CLIENT_LISTS_H
#define P_COMMUNICATOR_CLIENT_LISTS_H
#include "../../Utilities/common_func.h"

struct sr_info {
    pthread_t           tid;
    char                nickname[10], ip_source[INET6_ADDRSTRLEN];      //TODO: wyjebac ip, tylko przekaz do weypiania
    int                 write_end_pipe;
};

struct poll_fd_client_list {
    struct sr_info  *clients_list;
    struct pollfd   *sockets_fd_list;
    unsigned int    client_list_len, sockets_fd_list_len;
} all_lists;

void init_both_lists(int server_socket_fd);
void remove_client_from_both_lists(unsigned int index);
void add_client_to_both_lists(int client_fd, struct in6_addr *ip_address, char *client_nickname, int *to_pipe, pthread_t thread_id);
#endif
