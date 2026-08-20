/*
 * util.c — utility functions for zim
 */

#include "zim.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>

void error(tvi_t *tvi, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    term_print_at(0, term_height - 1, TERM_ATTR_FG_RED, "%s", buf);
    term_redraw();
}

void print(tvi_t *tvi, const char *fmt, ...)
{
    (void)tvi;
    va_list args;
    va_start(args, fmt);
    
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    term_print_at(0, term_height - 1, TERM_ATTR_FG_GREEN, "%s", buf);
    term_redraw();
}

void cursor_to_non_blank(win_t *win)
{
    if (!win->text || win->cursor_y < 0 || win->cursor_y >= win->lines_count)
        return;

    const char *line = win->text[win->cursor_y];
    if (!line) {
        win->cursor_x = 0;
        return;
    }

    while (line[win->cursor_x] == ' ' || line[win->cursor_x] == '\t')
        win->cursor_x++;
}

static void signal_handler(int sig)
{
    if (sig == SIGINT) {
        tvi.interrupted = 1;
    }
}

void signal_install_handlers(void)
{
#ifdef HAVE_SIGNAL_H
    signal(SIGINT, signal_handler);
#endif
}

syntax_t *syntax_load(const char *name)
{
    (void)name;
    return NULL;
}

void syntax_unload(syntax_t *syntax)
{
    if (!syntax)
        return;
    free(syntax);
}

void syntax_print_line(win_t *win, int y, syntax_t *syntax, const char *line)
{
    (void)syntax;
    win_print_at(win, 0, y, TERM_ATTR_FG_WHITE, "%s", line);
}
