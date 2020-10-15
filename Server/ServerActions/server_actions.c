#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include <semaphore.h>

#include "../ClientLists/client_lists.h"
#include "server_actions.h"

void sig_exit() {
    server_utilities.server_work = false;
}

void send_message_to_clients(struct cl_info current_thread) {
    unsigned int i;

    for (i = 0; i < all_lists.client_list_len; i++)
        write(all_lists.clients_list[i].write_end_pipe, &current_thread, sizeof(struct cl_info));
}

void send_serv_message(char *user_message, char *nickname) {
    struct cl_info current_thread;

    bzero(&current_thread, sizeof(struct cl_info));
    get_curr_time(current_thread.current_time);
    strcpy((char *)current_thread.message, user_message);
    strcat((char *)current_thread.message, nickname);
    strcpy(current_thread.nickname, "SERVER");
    send_message_to_clients(current_thread);
}

void send_client_message(unsigned char *user_message, unsigned int index) {
    struct cl_info current_thread;

    bzero(&current_thread, sizeof(struct cl_info));
    get_curr_time(current_thread.current_time);
    current_thread.sock_fd = all_lists.sockets_fd_list[index].fd;
    memcpy(current_thread.message, user_message, MES_SIZE);
    memcpy(current_thread.nickname, all_lists.clients_list[index - 1].nickname, 9);
    send_message_to_clients(current_thread);
}

void allocate_structure(struct cl_info **current_thread, struct public_key **pub_key, struct messages **tmp_message) {
    *current_thread = calloc(1, sizeof(struct cl_info));
    *pub_key = calloc(1, sizeof(struct public_key));
    *tmp_message = calloc(1, sizeof(struct messages));
    init_tmp_rsa_struct(*tmp_message);
}

void free_structures(struct cl_info *current_thread, struct public_key *pub_key, struct messages *tmp_message) {
    mpz_clears(pub_key->e, pub_key->n, NULL);
    clear_tmp_rsa_struct(tmp_message);
    free(pub_key);
    free(current_thread);
    free(tmp_message);
}

void copy_date_to_thread(struct pass_info_for_thread *thread_info, struct public_key *pub_key, int *read_end_pipe, int *client_fd) {
    mpz_init_set(pub_key->e, thread_info->pub_key->e);
    mpz_init_set(pub_key->n, thread_info->pub_key->n);
    *read_end_pipe = thread_info->read_end_pipe;
    *client_fd = thread_info->client_fd;
}

void *client_thread(void *thread_info) {
    struct public_key   *pub_key;
    struct messages     *tmp_message;
    struct cl_info      *current_thread;
    int                 read_end_pipe, client_fd;

    allocate_structure(&current_thread, &pub_key, &tmp_message);
    copy_date_to_thread(thread_info, pub_key, &read_end_pipe, &client_fd);
    sem_post(&server_utilities.mutex_for_creating_threads);

    while (read(read_end_pipe, current_thread, sizeof(struct cl_info)) > 0) {
        encrypt_message(pub_key, tmp_message, current_thread, sizeof(struct cl_info));
        write_whole_way(client_fd, tmp_message->swap_buffer, 2 * CRYPT_MES_SIZE);
        bzero(current_thread, sizeof(struct cl_info));
    }

    close(read_end_pipe);
    free_structures(current_thread, pub_key, tmp_message);
    pthread_exit(0);
}

void start_client_thread(int client_fd, const int *to_pipe, pthread_t *thread_id, struct public_key *pub_key) {
    struct pass_info_for_thread     thread_info;
    pthread_attr_t                  thread_attr;

    bzero(&thread_info, sizeof(struct pass_info_for_thread));
    thread_info.read_end_pipe = to_pipe[0];
    thread_info.client_fd = client_fd;
    thread_info.pub_key = pub_key;

    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, 1 * 1024 * 1024);
    pthread_create(thread_id, &thread_attr, client_thread, &thread_info);
    pthread_attr_destroy(&thread_attr);
    sem_wait(&server_utilities.mutex_for_creating_threads);
}

void close_clients_fd(unsigned int iter) {
    close(all_lists.clients_list[iter - 1].write_end_pipe);
    close(all_lists.sockets_fd_list[iter].fd);
}

void disconnect_client(unsigned int index) {
    char tmp_nickname[10];

    pthread_detach(all_lists.clients_list[index - 1].tid);
    close_clients_fd(index);
    memcpy(tmp_nickname,all_lists.clients_list[index - 1].nickname,10);
    remove_client_from_both_lists(index);
    send_serv_message("Disconnected: ", tmp_nickname);
}

void handle_message(unsigned int index, unsigned char * encrypted) {
    decrypt_message(server_utilities.serv_prv_key, server_utilities.tmp_message, encrypted, MES_SIZE);
    send_client_message(server_utilities.tmp_message->swap_buffer, index);
}

void check_new_message() {
    unsigned int i;

    for (i = 1; i < all_lists.sockets_fd_list_len; i++)
        if (all_lists.sockets_fd_list[i].revents & POLLIN) {
            if (read_whole_way(all_lists.sockets_fd_list[i].fd, server_utilities.tmp_message->swap_buffer, 2 * CRYPT_MES_SIZE) <= 0) disconnect_client(i);
            else handle_message(i, server_utilities.tmp_message->swap_buffer);
        }
}

void exchange_pub_keys_server(int client_fd, struct public_key *pub_key) {
    char tmp_buffer[RSA_SIZE + 2];

    bzero(tmp_buffer, RSA_SIZE + 2);
    read_whole_way(client_fd, tmp_buffer, RSA_SIZE);
    mpz_set_str(pub_key->e, tmp_buffer, 16);
    bzero(tmp_buffer, RSA_SIZE + 2);
    read_whole_way(client_fd, tmp_buffer, RSA_SIZE);
    mpz_set_str(pub_key->n, tmp_buffer, 16);

    bzero(tmp_buffer, RSA_SIZE + 2);
    mpz_get_str(tmp_buffer, 16, server_utilities.serv_pub_key->e);
    write_whole_way(client_fd, tmp_buffer, RSA_SIZE);
    bzero(tmp_buffer, RSA_SIZE + 2);
    mpz_get_str(tmp_buffer, 16, server_utilities.serv_pub_key->n);
    write_whole_way(client_fd, tmp_buffer, RSA_SIZE);
}

void init_exchange_info(int client_fd, char *client_nickname){
    read_whole_way(client_fd, server_utilities.tmp_message->swap_buffer, 2 * CRYPT_MES_SIZE);
    decrypt_message(server_utilities.serv_prv_key, server_utilities.tmp_message, client_nickname, 9);
    encrypt_message(server_utilities.serv_pub_key, server_utilities.tmp_message, &client_fd, sizeof(int));
    write_whole_way(client_fd, server_utilities.tmp_message->swap_buffer, 2 * CRYPT_MES_SIZE);
}

void add_new_connection(int client_fd, struct in6_addr *ip_address, int *to_pipe) {
    struct public_key   *pub_key;
    pthread_t           thread_id;
    char                client_nickname[10];

    pub_key = calloc(1, sizeof(struct public_key));
    mpz_inits(pub_key->e, pub_key->n, NULL);

    bzero(client_nickname, 10);
    exchange_pub_keys_server(client_fd, pub_key);
    init_exchange_info(client_fd, client_nickname);
    send_serv_message("Connected: ", client_nickname);
    start_client_thread(client_fd,to_pipe, &thread_id, pub_key);
    add_client_to_both_lists(client_fd, ip_address, client_nickname, to_pipe, thread_id);

    mpz_clears(pub_key->e, pub_key->n, NULL);
    free(pub_key);
}

int accept_crt_pipe(int *client_fd, struct sockaddr_in6 *client_addr, int *to_pipe) {
    socklen_t sock_len;

    sock_len = sizeof(struct sockaddr_in6);
    if ((*client_fd = accept(all_lists.sockets_fd_list[0].fd, (struct sockaddr *)client_addr, &sock_len)) == -1) {
        printf("can not create client socket\n");
        return -1;
    }

    if (pipe(to_pipe) == -1) {
        printf("can not create pipe\n");
        return -1;
    }

    return 0;
}

void check_new_connection() {
    struct sockaddr_in6 client_addr;
    int                 to_pipe[2], client_fd;

    if (all_lists.sockets_fd_list[0].revents & POLLIN) {
        if (accept_crt_pipe(&client_fd, &client_addr, to_pipe) == -1) return;
        add_new_connection(client_fd, &client_addr.sin6_addr, to_pipe);
    }
}

void remove_all_clients() {
    while (all_lists.client_list_len > 0) remove_client_from_both_lists(1);
}

void disconnect_all_clients() {
    unsigned int i;

    for (i = 1; i < all_lists.client_list_len + 1; i++){
        close_clients_fd(i);
        pthread_join(all_lists.clients_list[i - 1].tid, NULL);
    }
}

void clear_server_utilities() {
    free(server_utilities.serv_pub_key);
    free(server_utilities.serv_prv_key);
    free(server_utilities.tmp_message);
}

void clear_lists() {
    remove_all_clients();
    free(all_lists.sockets_fd_list);
    free(all_lists.clients_list);
}

void clean_up(int server_socket_fd ) {
    close(server_socket_fd);
    disconnect_all_clients();
    sem_destroy(&server_utilities.mutex_for_creating_threads);
    clear_keys(server_utilities.serv_pub_key, server_utilities.serv_prv_key);
    clear_tmp_rsa_struct(server_utilities.tmp_message);
    clear_lists();
    clear_server_utilities();
}

void malloc_server_utilities() {
    server_utilities.serv_pub_key = calloc(1, sizeof(struct  public_key));
    server_utilities.serv_prv_key = calloc(1, sizeof(struct  private_key));
    server_utilities.tmp_message = calloc(1, sizeof(struct messages));
}

void init_server_utilities() {
    malloc_server_utilities();
    sem_init(&server_utilities.mutex_for_creating_threads, 0, 0);
    init_keys(server_utilities.serv_pub_key, server_utilities.serv_prv_key);
    init_tmp_rsa_struct(server_utilities.tmp_message);
    server_utilities.server_work = true;
}

void exit_actions() {
    printf("Exiting p_server\n");
    sleep(1);
    system("clear");
}

void main_work_server(int server_socket_fd, int max_clients) {
    int ret;

    if (validate_mess_size(sizeof(struct cl_info))) {
        printf("Message size to big!\n");
        return;
    }

    init_server_utilities();
    init_both_lists(server_socket_fd);

    while (server_utilities.server_work) {
        ret = poll(all_lists.sockets_fd_list, all_lists.sockets_fd_list_len, 1000);

        if (ret > 0) {
            check_new_message();
            if (all_lists.client_list_len < max_clients) check_new_connection();
        }

        if (ret == -1) {
            if (errno != EINTR) printf("Poll error!\n");
            break;
        }
    }

    clean_up(server_socket_fd);
    exit_actions();
}