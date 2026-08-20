/*
 * render.c — отрисовка буфера, статусной строки и курсора.
 * Всё содержимое окна рисуется через win_print_at() (уже реализован
 * в win.c), глобальные вещи (командная строка, курсор) — через term_*.
 */

#include <string.h>
#include <stdio.h>
#include "zim.h"

int render_line(tvi_t *tvi, win_t *win, size_t index)
{
    (void)tvi;
    int rel_y = (int)index - win->scroll;

    /* последняя строка окна отдана под статус-бар */
    if (rel_y < 0 || rel_y >= win->height - 1)
        return 0;

    const char *line = "";
    if (win->text && (int)index < win->lines_count && win->text[index])
        line = win->text[index];

    if (win->syntax && win->syntax->print_line) {
        win->syntax->print_line(win, rel_y, line);
    } else {
        win_print_at(win, 0, rel_y, TERM_ATTR_FG_WHITE, "%s", line);
    }
    return 1;
}

void render_text(tvi_t *tvi, win_t *win)
{
    int i;
    for (i = 0; i < win->height - 1; i++)
        render_line(tvi, win, (size_t)(win->scroll + i));
}

void render_status(tvi_t *tvi, win_t *win)
{
    (void)tvi;
    char buf[256];
    const char *name = (win->files_count > 0 && win->files)
                            ? win->files[win->file_index]
                            : "[No Name]";

    snprintf(buf, sizeof(buf), " %s%s  %d,%d",
             name,
             (win->flags & FLAG_DIRTY) ? " [+]" : "",
             win->cursor_y + 1, win->cursor_x + 1);

    win_print_at(win, 0, win->height - 1, TERM_ATTR_INVERSE, "%-*s",
                 win->width, buf);
}

void render_window(tvi_t *tvi, win_t *win)
{
    render_text(tvi, win);
    render_status(tvi, win);
}

void render_all_windows(tvi_t *tvi)
{
    win_t *w = tvi->first_window;
    while (w) {
        render_window(tvi, w);
        w = w->next;
    }
}

void render_prompt(tvi_t *tvi)
{
    if (!(tvi->flags & FLAG_PROMPT))
        return;

    int row = term_height - 1;
    term_clear_line(row);
    term_print_at(0, row, TERM_ATTR_FG_WHITE, ":%s", tvi->prompt);
}

void render_cursor(tvi_t *tvi)
{
    if (tvi->flags & FLAG_PROMPT) {
        term_set_cursor((int)(1 + tvi->prompt_cursor), term_height - 1);
        return;
    }

    win_t *win = tvi->focus_window;
    int screen_x = win->x + win->cursor_x;
    int screen_y = win->y + (win->cursor_y - win->scroll);
    term_set_cursor(screen_x, screen_y);
}

void render_flush(tvi_t *tvi)
{
    (void)tvi;
    term_redraw();
}
