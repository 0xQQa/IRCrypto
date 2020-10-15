#ifndef P_COMMUNICATOR_COMMON_FUNC_H
#define P_COMMUNICATOR_COMMON_FUNC_H

#include <netinet/in.h>
#include <stdbool.h>

#define MES_SIZE 400

struct cl_info{
    char                nickname[10];
    unsigned char       message[MES_SIZE + 1];
    int                 sock_fd;
    char                current_time[9];
};

void get_curr_time(char* current_time);
void err_exit(char *message, char *param);
int write_whole_way(int fd, const void* buf, size_t size);
int read_whole_way(int fd, void* buf, size_t size);

#endif //P_COMMUNICATOR_COMMON_FUNC_H
