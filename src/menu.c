/*
 * menu.c — стартовый экран zim, оформлен по мотивам заставки vim:
 * заголовок, версия и подсказки по командам, отцентрованные
 * по вертикали и горизонтали относительно размера терминала.
 */

#include "zim.h"

#define ZIM_VERSION "0.1"

static void draw_centered(int y, int attr, const char *text)
{
    size_t len = 0;
    while (text[len])
        len++;

    int x = (term_width > (int)len) ? (term_width - (int)len) / 2 : 0;
    term_print_at(x, y, attr, "%s", text);
}

/*
 * show_menu — рисует приветственный экран (как у vim) и ждёт
 * первого действия пользователя. ':' сразу входит в ex-режим —
 * удобно набрать ":e file.c" не глядя. Любая другая клавиша просто
 * закрывает экран; дальнейшую обработку берёт на себя tvi_main().
 */
void show_menu(tvi_t *tvi)
{
    static const char *lines[] = {
        "zim - Z Improved",
        "",
        "version " ZIM_VERSION,
        "",
        "zim is part of the Zythos project",
        "",
        "type  :e {file}<Enter>     to edit or create a file",
        "type  :w<Enter>            to save",
        "type  :wq<Enter>           to save and exit",
        "type  :q<Enter>            to exit",
        "type  :q!<Enter>           to exit without saving",
    };
    int count = (int)(sizeof(lines) / sizeof(lines[0]));

    term_send_code(TERM_CLEAR);

    int start_y = (term_height > count) ? (term_height - count) / 2 : 0;

    for (int i = 0; i < count; i++) {
        int attr = TERM_ATTR_FG_WHITE;
        if (i == 0)
            attr = TERM_ATTR_FG_CYAN | TERM_ATTR_BOLD;
        else if (i == 2)
            attr = TERM_ATTR_FG_CYAN;

        if (lines[i][0])
            draw_centered(start_y + i, attr, lines[i]);
    }

    term_redraw();

    int c = term_get_key();
    if (c == ':') {
        if (prompt(tvi, "", 0) == 0)
            ex_command(tvi, tvi->prompt);
    }
}