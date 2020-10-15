#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <stdlib.h>

#include "../../Utilities/common_func.h"
#include "client_actions.h"

void common_exit(char * prompt) {
    strcpy(client_utilities.prompt, prompt);
    client_utilities.work = false;
}

void client_exit() {
    common_exit("CLIENT");
}

void server_exit() {
    common_exit("SERVER");
}

struct framed_window* create_window(int x_max, int y_max, int pos, char *window_title, char *init_message) {
    struct framed_window    *windows;
    WINDOW                  *body_space, *text_space;

    body_space = newwin(y_max , x_max , pos, 0);
    text_space = subwin(body_space, y_max - 2, x_max - 2 , pos + 1, 1);

    wbkgd(body_space, COLOR_PAIR(DARK_BACKGROUND));
    box(body_space, 0, 0);
    wprintw(body_space, "%s", window_title);
    wrefresh(body_space);

    wbkgd(text_space, COLOR_PAIR(DARK_BACKGROUND));
    scrollok(text_space, TRUE);
    werase(text_space);

    if (init_message) {
        wprintw(text_space, "%s", init_message);
        wrefresh(text_space);
    }

    windows = calloc(1, sizeof(struct framed_window));
    windows->window = text_space;
    windows->frame = body_space;
    return windows;
}

static inline short shuffle_fd(int descriptor) {
    if (descriptor == SERV_MESS) return 0;
    return (short)((descriptor * 64) % 231 + 1);
}

void check_color(int descriptor) {
    short real_var;

    real_var = shuffle_fd(descriptor);
    if (find_pair(real_var, DARK_BACKGROUND) == -1) init_pair(real_var, real_var, DARK_BACKGROUND);
}

void print_message(struct cl_info *client_message) {
    short color;

    color = shuffle_fd(client_message->sock_fd);
    wattron(client_utilities.rx, COLOR_PAIR(color));
    wprintw(client_utilities.rx, "\n[%s|%s]:%s", client_message->current_time, client_message->nickname, client_message->message);
    wrefresh(client_utilities.rx);
    wattroff(client_utilities.rx, COLOR_PAIR(color));
}

void allocate_rx_structures(struct cl_info **client_message, struct pollfd **user_poll_fd, struct messages **tmp_message) {
    *client_message = calloc(1, sizeof(struct cl_info));
    *tmp_message = calloc(1, sizeof(struct messages));
    init_tmp_rsa_struct(*tmp_message);
    *user_poll_fd = calloc(1, sizeof(struct pollfd));
    ((struct pollfd*) *user_poll_fd)->fd = user_info.sock_fd;
    ((struct pollfd*) *user_poll_fd)->events = POLLIN;
}

void free_rx_structure(struct cl_info *client_message, struct pollfd *user_poll_fd, struct messages *tmp_message) {
    clear_tmp_rsa_struct(tmp_message);
    free(client_message);
    free(user_poll_fd);
    free(tmp_message);
}

void unpack_message(struct cl_info *client_message, struct messages *tmp_message) {
    bzero(client_message, sizeof(struct cl_info));
    decrypt_message(client_utilities.prv_key, tmp_message, client_message, sizeof(struct cl_info));
    check_color(client_message->sock_fd);
    print_message(client_message);
}

void last_message() {
    char current_time[9];

    get_curr_time(current_time);
    wprintw(client_utilities.rx, "\n[%s|%s]: exiting", current_time, client_utilities.prompt);
    wrefresh(client_utilities.rx);
    sleep(1);
}

void *communicate_rx() {
    struct messages *tmp_message;
    struct cl_info  *client_message;
    struct pollfd   *user_poll_fd;
    int             ret;

    allocate_rx_structures(&client_message, &user_poll_fd, &tmp_message);
    while (client_utilities.work) {
        ret = poll(user_poll_fd, 1, TIME_OUT);

        if (ret > 0 && (user_poll_fd->revents & POLLIN)) {
            if (read_whole_way(user_info.sock_fd, tmp_message->swap_buffer, 2 * CRYPT_MES_SIZE) <= 0) server_exit();
            else unpack_message(client_message, tmp_message);
        }
    }

    free_rx_structure(client_message,user_poll_fd, tmp_message);
    pthread_exit(0);
}

void update_input() {
    wclear(client_utilities.tx);
    wprintw(client_utilities.tx, "[%s]:%s", user_info.nickname, user_info.message);
}

void try_erase(int *counter) {
    if (*counter > 0) {
        (*counter)--;
        user_info.message[*counter] = 0;
    }
}

void send_message(int *counter){
    if (user_info.message[0] != 0) {
        encrypt_message(client_utilities.pub_key, client_utilities.tmp_message, user_info.message, MES_SIZE);
        write_whole_way(user_info.sock_fd, client_utilities.tmp_message->swap_buffer, 2 * CRYPT_MES_SIZE);
        bzero(user_info.message, MES_SIZE);
        *counter = 0;
    }
}

void append_char(int *counter, int character) {
    if (*counter < MES_SIZE) {
        user_info.message[*counter] = character;
        (*counter)++;
    }
}

void check_overflow(int counter) {
    if (counter == MES_SIZE) wprintw(client_utilities.tx, "[max chars]");
}

void *communicate_tx() {
    int w_get, counter;

    counter = 0;
    update_input();
    while (client_utilities.work) {
        w_get = wgetch(client_utilities.tx);

        if (w_get != ERR) {
            if (w_get == 0x7F) try_erase(&counter);
            else if (w_get == 0xA) send_message(&counter);
            else append_char(&counter, w_get);

            update_input();
            check_overflow(counter);
        }
    }

    pthread_exit(0);
}

void init_gui(int *y_max, int *x_max) {
    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);
    start_color();
    init_pair(DARK_BACKGROUND, COLOR_WHITE, DARK_BACKGROUND);
    getmaxyx(stdscr, *y_max, *x_max);
    client_utilities.work = true;
}

void clean_gui(struct framed_window *window_rx, struct framed_window *window_tx) {
    int i;

    delwin(window_tx->window);
    delwin(window_tx->frame);
    delwin(window_rx->window);
    delwin(window_rx->frame);
    free(window_tx);
    free(window_rx);
    for (i = 0; i < 230; i++) if (find_pair(i, DARK_BACKGROUND) != -1) free_pair(i);
    free_pair(DARK_BACKGROUND);
}

void free_client_utilities() {
    free(client_utilities.pub_key);
    free(client_utilities.prv_key);
    free(client_utilities.tmp_message);
}

void clean_up(struct framed_window *window_rx, struct framed_window *window_tx) {
    close(user_info.sock_fd);
    clean_gui(window_rx, window_tx);
    clear_keys(client_utilities.pub_key, client_utilities.prv_key);
    clear_tmp_rsa_struct(client_utilities.tmp_message);
    free_client_utilities();
    endwin();
}

void init_exchange_info(int *assigned_sock) {
    encrypt_message(client_utilities.pub_key, client_utilities.tmp_message, user_info.nickname, 10);
    write_whole_way(user_info.sock_fd, client_utilities.tmp_message->swap_buffer, 2 * CRYPT_MES_SIZE);
    read_whole_way(user_info.sock_fd,  client_utilities.tmp_message->swap_buffer, 2 * CRYPT_MES_SIZE);
    decrypt_message(client_utilities.prv_key, client_utilities.tmp_message, assigned_sock, sizeof(int));
}

void exchange_pub_keys_client() {
    char tmp_buffer[RSA_SIZE + 1];

    bzero(tmp_buffer, RSA_SIZE + 1);
    mpz_get_str(tmp_buffer, 16, client_utilities.pub_key->e);
    write_whole_way(user_info.sock_fd, tmp_buffer, RSA_SIZE);
    bzero(tmp_buffer, RSA_SIZE + 1);
    mpz_get_str(tmp_buffer, 16, client_utilities.pub_key->n);
    write_whole_way(user_info.sock_fd, tmp_buffer, RSA_SIZE);

    bzero(tmp_buffer, RSA_SIZE + 1);
    read_whole_way(user_info.sock_fd, tmp_buffer, RSA_SIZE);
    mpz_set_str(client_utilities.pub_key->e, tmp_buffer, 16);
    bzero(tmp_buffer, RSA_SIZE + 1);
    read_whole_way(user_info.sock_fd, tmp_buffer, RSA_SIZE);
    mpz_set_str(client_utilities.pub_key->n, tmp_buffer, 16);
}

void rxtx_threads_work() {
    pthread_attr_t  thread_attr;
    pthread_t       p_tx, p_rx;

    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, 1 * 1024 * 1024);
    pthread_create(&p_rx, &thread_attr, communicate_rx, NULL);
    pthread_create(&p_tx, &thread_attr, communicate_tx, NULL);

    pthread_attr_destroy(&thread_attr);
    pthread_join(p_rx, NULL);
    pthread_join(p_tx, NULL);
}

void malloc_client_utilities() {
    client_utilities.pub_key = calloc(1, sizeof(struct public_key));
    client_utilities.prv_key = calloc(1, sizeof(struct private_key));
    client_utilities.tmp_message = calloc(1, sizeof(struct messages));
}

void init_work(int *assigned_sock) {
    malloc_client_utilities();
    init_keys(client_utilities.pub_key, client_utilities.prv_key);
    init_tmp_rsa_struct(client_utilities.tmp_message);
    exchange_pub_keys_client();
    init_exchange_info(assigned_sock);
    bzero(client_utilities.prompt, 10);
}

void set_gui(struct framed_window **window_rx, struct framed_window **window_tx, int x_max, int y_max, int assigned_sock) {
    *window_rx = create_window(x_max, y_max - 3, 0 ,"Incoming messages:","Welcome to IRCrypto client!");
    *window_tx = create_window(x_max, 3, y_max - 3, "Your message:", NULL);
    check_color(assigned_sock);
    check_color(SERV_MESS);
}

void set_client_rxtx(struct framed_window *window_rx, struct framed_window *window_tx) {
    client_utilities.rx = window_rx->window;
    client_utilities.tx = window_tx->window;
    wtimeout(client_utilities.tx, TIME_OUT);
}

void main_work_client() {
    struct framed_window    *window_rx, *window_tx;
    int                     x_max, y_max, assigned_sock;

    init_work(&assigned_sock);
    init_gui(&y_max, &x_max);
    set_gui(&window_rx, &window_tx, x_max, y_max, assigned_sock);
    set_client_rxtx(window_rx, window_tx);
    rxtx_threads_work();
    last_message();
    clean_up(window_tx, window_rx);
}
