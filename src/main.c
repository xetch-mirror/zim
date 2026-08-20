/*
 * main.c — точка входа редактора zim
 */

#include "zim.h"

/* Единственное определение глобального tvi (zim.h объявляет его extern) */
tvi_t tvi;

void show_menu(tvi_t *tvi);

int main(int argc, char **argv)
{
    signal_install_handlers();
    term_enable_raw_mode();
    term_enter_fullscreen();
    term_fetch_size();

    tvi.mode = 0;
    tvi.flags = 0;
    tvi.focus_window = win_create(&tvi);
    tvi.first_window = tvi.focus_window;

    if (argc > 1) {
        /* файлы из argv открываются как есть — если какого-то нет,
         * open_files/read_file начнёт с пустого буфера под это имя */
        open_files(tvi.focus_window, &argv[1], (size_t)(argc - 1));
    } else {
        show_menu(&tvi);
    }

    int rc = tvi_main(&tvi);

    term_exit_fullscreen();
    term_quit_raw_mode();
    return rc;
}