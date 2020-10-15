#ifndef P_COMMUNICATOR_CLIENT_ACTIONS_H
#define P_COMMUNICATOR_CLIENT_ACTIONS_H

#include "../../Utilities/common_func.h"
#include "../../Crypto/rsa.h"
#include <ncurses.h>
#include <pthread.h>

#define DARK_BACKGROUND 236
#define TIME_OUT 1000
#define SERV_MESS 0

struct cl_info user_info;

struct framed_window {
    WINDOW *frame, *window;
};

struct cli_utilities{
    struct private_key  *prv_key;
    struct public_key   *pub_key;
    struct messages     *tmp_message;
    char                prompt[10];
    WINDOW              *tx, *rx;
    bool                work;
} client_utilities;

struct framed_window* create_window(int x_max, int y_max, int pos, char *window_title, char *init_message);
void server_exit();
void init_gui(int *y_max, int *x_max);
void main_work_client();
void client_exit();

#endif
