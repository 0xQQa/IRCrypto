#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "common_func.h"

void err_exit(char *message, char *param){
    printf(message, param);
    exit(1);
}

void get_curr_time(char* current_time){
    struct tm   *curr_time;
    time_t      now;

    time(&now);
    curr_time = localtime(&now);
    sprintf(current_time, "%02d:%02d:%02d", curr_time->tm_hour, curr_time->tm_min, curr_time->tm_sec);
}

int write_whole_way(int fd, const void* buf, size_t size) {
    int result, ret;

    for (ret = 0; ret < size; ret += result){
        result = write(fd, buf + ret, size - ret);
        if (result <= 0 ) return result;
    }

    return ret;
}

int read_whole_way(int fd, void* buf, size_t size) {
    int result, ret;

    for (ret = 0; ret < size; ret += result){
        result = read(fd, buf + ret, size - ret);
        if (result <= 0 ) return result;
    }

    return ret;
}

