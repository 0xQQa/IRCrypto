#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include "client_lists.h"


void init_server_poll_fd(int server_socket_fd) {
    all_lists.sockets_fd_list = calloc(1, sizeof(struct pollfd));
    all_lists.sockets_fd_list[0].fd = server_socket_fd;
    all_lists.sockets_fd_list[0].events = POLLIN;
}

void init_both_lists(int server_socket_fd) {
    all_lists.client_list_len = 0;
    all_lists.sockets_fd_list_len = 1;
    all_lists.clients_list = NULL;
    init_server_poll_fd(server_socket_fd);
}

void remove_from_sock_fd_list(unsigned int iter) {
    unsigned int i;

    all_lists.sockets_fd_list_len--;
    for (i = iter; i < all_lists.sockets_fd_list_len; i++) all_lists.sockets_fd_list[i] = all_lists.sockets_fd_list[i + 1];
}

static inline int is_sock_fd_list_len_power_of_2(){
    return !((all_lists.sockets_fd_list_len - 1) & all_lists.sockets_fd_list_len);
}

void add_to_sock_fd_list(int socket_fd) {
    struct pollfd *tmp;

    all_lists.sockets_fd_list_len++;
    tmp = is_sock_fd_list_len_power_of_2() ? realloc(all_lists.sockets_fd_list, (all_lists.sockets_fd_list_len * 2) * sizeof(struct pollfd)) : all_lists.sockets_fd_list;
    tmp[all_lists.sockets_fd_list_len - 1].fd = socket_fd;
    tmp[all_lists.sockets_fd_list_len - 1].events = POLLIN;
    all_lists.sockets_fd_list = tmp;
}

void create_client_structure(struct sr_info *client_info, struct in6_addr *ip_address ,char *nickname, int pipe_end, pthread_t tid) {
    inet_ntop(AF_INET6, ip_address, client_info->ip_source, INET6_ADDRSTRLEN);
    memcpy(client_info->nickname, nickname, 10);
    client_info->write_end_pipe = pipe_end;
    client_info->tid = tid;
}

static inline int is_client_list_len_power_of_2(){
    return !((all_lists.client_list_len - 1) & all_lists.client_list_len);
}

void add_to_client_list(struct sr_info new_client){
    struct sr_info  *tmp_client_info;
    char            time[9];

    all_lists.client_list_len++;
    tmp_client_info = is_client_list_len_power_of_2() ? realloc(all_lists.clients_list, (all_lists.client_list_len * 2) * sizeof(struct sr_info)) : all_lists.clients_list;
    tmp_client_info[all_lists.client_list_len - 1] = new_client;
    all_lists.clients_list =  tmp_client_info;  //bylo po printf
    get_curr_time(time);
    printf("[%s| CONN from %s| Named %s| Connected users %u]\n", time, new_client.ip_source, new_client.nickname, all_lists.client_list_len);
}

void remove_from_client_list(unsigned int index){
    unsigned int    i;
    char            time[9];

    all_lists.client_list_len--;
    get_curr_time(time);
    printf("[%s| DISC from %s| Named %s| Connected users %u]\n", time, all_lists.clients_list[index].ip_source, all_lists.clients_list[index].nickname, all_lists.client_list_len);
    for (i = index; i < all_lists.client_list_len; i++) all_lists.clients_list[i] = all_lists.clients_list[i + 1];
}

void remove_client_from_both_lists(unsigned int index){
    remove_from_client_list(index - 1);
    remove_from_sock_fd_list(index);
}

void add_client_to_both_lists(int client_fd, struct in6_addr *ip_address, char *client_nickname, int *to_pipe, pthread_t thread_id) {
    struct sr_info tmp_client_info;

    bzero(&tmp_client_info, sizeof(struct sr_info));
    create_client_structure(&tmp_client_info, ip_address, client_nickname, to_pipe[1], thread_id);
    add_to_client_list(tmp_client_info);
    add_to_sock_fd_list(client_fd);
}
