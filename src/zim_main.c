/*
 * zim_main.c — главный цикл нормального режима zim: чтение клавиши,
 * применение движения/команды, перерисовка. Режим вставки (insert)
 * реализован как отдельный блокирующий подцикл, входимый по 'i'/'a' —
 * в zim.h нет отдельной константы MODE_INSERT, только MODE_VISUAL и
 * MODE_EX, поэтому обычный (normal) режим — это просто mode == 0.
 */

#include <string.h>
#include "zim.h"

static void clamp_cursor(win_t *win)
{
    if (win->lines_count <= 0) {
        win->cursor_y = 0;
        win->cursor_x = 0;
        return;
    }

    if (win->cursor_y < 0)
        win->cursor_y = 0;
    if (win->cursor_y >= win->lines_count)
        win->cursor_y = win->lines_count - 1;

    int len = 0;
    if (win->text && win->text[win->cursor_y])
        len = (int)strlen(win->text[win->cursor_y]);

    if (win->cursor_x > len)
        win->cursor_x = len;
    if (win->cursor_x < 0)
        win->cursor_x = 0;

    /* прокрутка: держим курсор в видимой части окна
     * (последняя строка окна — статус-бар, не часть текста) */
    if (win->cursor_y < win->scroll)
        win->scroll = win->cursor_y;
    else if (win->cursor_y >= win->scroll + win->height - 1)
        win->scroll = win->cursor_y - win->height + 2;
    if (win->scroll < 0)
        win->scroll = 0;
}

/* Подцикл режима вставки — выходит по Esc */
static void do_insert(tvi_t *tvi, win_t *win)
{
    for (;;) {
        render_all_windows(tvi);
        render_cursor(tvi);
        render_flush(tvi);

        int c = term_get_key();

        if (tvi->interrupted) {
            tvi->interrupted = 0;
            break;
        }
        if (c == 27)
            break;

        if (c == '\r' || c == '\n') {
            text_insert_newline(win, win->cursor_x, win->cursor_y);
            win->cursor_y++;
            win->cursor_x = 0;
        } else if (term_is_delete(c)) {
            if (win->cursor_x > 0) {
                text_delete(win, win->cursor_x - 1, win->cursor_y, 1);
                win->cursor_x--;
            } else if (win->cursor_y > 0) {
                int prev_len = 0;
                if (win->text[win->cursor_y - 1])
                    prev_len = (int)strlen(win->text[win->cursor_y - 1]);
                text_join(win, win->cursor_y - 1, win->cursor_y, 0);
                win->cursor_y--;
                win->cursor_x = prev_len;
            }
        } else if (c == KEY_LEFT) {
            if (win->cursor_x > 0)
                win->cursor_x--;
        } else if (c == KEY_RIGHT) {
            win->cursor_x++;
        } else if (c == KEY_UP) {
            if (win->cursor_y > 0)
                win->cursor_y--;
        } else if (c == KEY_DOWN) {
            if (win->cursor_y + 1 < win->lines_count)
                win->cursor_y++;
        } else if (c >= 32 && c < 127) {
            char ch = (char)c;
            text_insert_buf(win, win->cursor_x, win->cursor_y, &ch, 1);
            win->cursor_x++;
        }

        text_mark_dirty(win);
        clamp_cursor(win);
    }
}

int tvi_main(tvi_t *tvi)
{
    int pending = 0; /* первая половина составной команды: 'd' или 'y' */

    while (!(tvi->flags & FLAG_QUIT)) {
        win_t *win = tvi->focus_window;

        render_all_windows(tvi);
        render_prompt(tvi);
        render_cursor(tvi);
        render_flush(tvi);

        int c = term_get_key();

        if (tvi->interrupted) {
            tvi->interrupted = 0;
            continue;
        }

        if (pending) {
            if (c == pending) {
                if (pending == 'd') {
                    text_yank_lines(tvi, win, win->cursor_y, 1, 0);
                    text_delete_lines(win, win->cursor_y, 1);
                    text_mark_dirty(win);
                } else if (pending == 'y') {
                    text_yank_lines(tvi, win, win->cursor_y, 1, 0);
                }
            }
            pending = 0;
            clamp_cursor(win);
            continue;
        }

        int line_changed = 0;

        switch (c) {
        case 'h': case KEY_LEFT:  win->cursor_x--; break;
        case 'l': case KEY_RIGHT: win->cursor_x++; break;
        case 'k': case KEY_UP:    win->cursor_y--; line_changed = 1; break;
        case 'j': case KEY_DOWN:  win->cursor_y++; line_changed = 1; break;

        case '0': case KEY_START:
            win->cursor_x = 0;
            break;
        case '$': case KEY_END:
            win->cursor_x = (win->text && win->text[win->cursor_y])
                                 ? (int)strlen(win->text[win->cursor_y])
                                 : 0;
            break;

        case 'i':
            do_insert(tvi, win);
            break;
        case 'a':
            win->cursor_x++;
            clamp_cursor(win);
            do_insert(tvi, win);
            break;

        case 'x':
            if (win->text && win->text[win->cursor_y] &&
                win->cursor_x < (int)strlen(win->text[win->cursor_y])) {
                text_delete(win, win->cursor_x, win->cursor_y, 1);
                text_mark_dirty(win);
            }
            break;

        case 'p':
            reg_put_lines(tvi, win, 0, win->cursor_y + 1);
            text_mark_dirty(win);
            break;

        case 'd':
        case 'y':
            pending = c;
            break;

        case ':':
            if (prompt(tvi, "", 0) == 0)
                ex_command(tvi, tvi->prompt);
            break;

        case 27:
        case -1:
        default:
            break;
        }

        clamp_cursor(win);
        if (line_changed)
            cursor_to_non_blank(win);
    }

    return 0;
}
