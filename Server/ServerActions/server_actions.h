
#ifndef P_COMMUNICATOR_SERVER_ACTIONS_H
#define P_COMMUNICATOR_SERVER_ACTIONS_H
#include <semaphore.h>
#include "../../Crypto/rsa.h"


struct serv_utilities {
    struct private_key  *serv_prv_key;
    struct public_key   *serv_pub_key;
    struct messages     *tmp_message;
    sem_t               mutex_for_creating_threads;
    bool                server_work;
} server_utilities;

struct pass_info_for_thread {
    struct public_key   *pub_key;
    int                 client_fd, read_end_pipe;
};

void main_work_server(int server_socket_fd, int max_clients);
void sig_exit();

#endif
