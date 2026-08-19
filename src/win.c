#include <stdlib.h>
#include <string.h>
#include "zim.h"

win_t *win_create(tvi_t *tvi)
{
        win_t *win = calloc(1, sizeof(win_t));
        if (!win)
                return NULL;

        win->x = 0;
        win->y = 0;
        win->width = term_width;
        win->height = term_height;
        win->cursor_x = 0;
        win->cursor_y = 0;
        win->scroll = 0;
        win->flags = 0;
        win->text = NULL;
        win->files = NULL;
        win->syntax = NULL;
        win->lines_count = 0;
        win->files_count = 0;
        win->file_index = 0;

        if (!tvi->first_window) {
                win->next = NULL;
                win->prev = NULL;
                tvi->first_window = win;
        } else if (!tvi->focus_window) {
                win->next = tvi->first_window;
                win->prev = NULL;
                tvi->first_window->prev = win;
                tvi->first_window = win;
        } else {
                win->prev = tvi->focus_window;
                win->next = tvi->focus_window->next;
                if (tvi->focus_window->next)
                        tvi->focus_window->next->prev = win;
                tvi->focus_window->next = win;
        }

        tvi->focus_window = win;
        return win;
}

void win_free(tvi_t *tvi, win_t *win)
{
        if (!win)
                return;

        if (win->text)
                free_list(win->text, win->lines_count);
        if (win->files)
                free_list(win->files, win->files_count);

        if (win->prev)
                win->prev->next = win->next;
        if (win->next)
                win->next->prev = win->prev;
        if (tvi->first_window == win)
                tvi->first_window = win->next;

        if (tvi->focus_window == win)
                tvi->focus_window = win->next ? win->next : win->prev;

        free(win);
}

void win_print_at(win_t *win, int x, int y, int attr, const char *fmt, ...)
{
        bound_t bound;
        va_list args;

        bound.x = win->x;
        bound.y = win->y;
        bound.width = win->width;
        bound.height = win->height;

        va_start(args, fmt);
        term_vprint_bound_at(&bound, x, y, attr, fm