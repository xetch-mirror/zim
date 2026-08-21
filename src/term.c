/*
 * term.c — терминальный слой zim: посылка ANSI-кодов, чтение клавиш,
 * печать с атрибутами.
 *
 * Примечание: в syscalls.h нет ioctl, поэтому term_fetch_size() не
 * может использовать TIOCGWINSZ — вместо этого запрашивает позицию
 * курсора через "\033[999;999H\033[6n" и парсит ответ терминала.
 *
 * "Сырой" режим (raw mode) в классическом смысле (termios) тоже
 * недоступен — но в этом и нет нужды: sys_read() у нас и так отдаёт
 * символы прямо из кольцевого буфера IRQ1 без построчной буферизации
 * ядра, так что term_enable_raw_mode()/term_quit_raw_mode() ниже —
 * заглушки, оставленные ради совместимости интерфейса.
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "zim.h"

int term_width;
int term_height;

static const char *g_code_table[TERM_CODES_COUNT] = {
    [TERM_GOTO]           = "\033[%d;%dH",
    [TERM_COLOR_RESET]    = "\033[0m",
    [TERM_COLOR_INVERSE]  = "\033[7m",
    [TERM_COLOR_BOLD]     = "\033[1m",
    [TERM_COLOR_SET_FG]   = "\033[3%dm",
    [TERM_COLOR_SET_BG]   = "\033[4%dm",
    [TERM_CLEAR_END_LINE] = "\033[K",
    [TERM_INSERT]         = "\033[4h",
    [TERM_DELETE]         = "\033[P",
    [TERM_CLEAR]          = "\033[2J\033[H",
    [TERM_INSERT_LINE]    = "\033[L",
    [TERM_DELETE_LINE]    = "\033[M",
};

static void term_write(const char *buf, size_t len)
{
    write(STDOUT_FILENO, buf, len);
}

static void term_write_str(const char *s)
{
    term_write(s, strlen(s));
}

const char *term_get_code(int code)
{
    if (code < 0 || code >= TERM_CODES_COUNT)
        return "";
    return g_code_table[code];
}

size_t term_get_code_len(int code)
{
    return strlen(term_get_code(code));
}

void term_send_code(int code, ...)
{
    char buf[64];
    const char *tmpl = term_get_code(code);
    va_list args;

    va_start(args, code);

    switch (code) {
    case TERM_GOTO: {
        int y = va_arg(args, int);
        int x = va_arg(args, int);
        snprintf(buf, sizeof(buf), tmpl, y, x);
        term_write_str(buf);
        break;
    }
    case TERM_COLOR_SET_FG:
    case TERM_COLOR_SET_BG: {
        int color = va_arg(args, int);
        snprintf(buf, sizeof(buf), tmpl, color);
        term_write_str(buf);
        break;
    }
    default:
        term_write_str(tmpl);
        break;
    }

    va_end(args);
}

int term_enable_raw_mode(void)
{
    return 0;
}

void term_quit_raw_mode(void)
{
}

int term_enter_fullscreen(void)
{
    term_write_str("\033[?1049h");
    return 0;
}

void term_exit_fullscreen(void)
{
    term_write_str("\033[?1049l");
}

void term_clear_line(int y)
{
    term_send_code(TERM_GOTO, y, 0);
    term_send_code(TERM_CLEAR_END_LINE);
}

void term_bell(void)
{
    term_write("\a", 1);
}

void term_goto(int x, int y)
{
    term_send_code(TERM_GOTO, y + 1, x + 1);
}

void term_set_cursor(int x, int y)
{
    term_goto(x, y);
}

int term_is_delete(int c)
{
    return c == 127 || c == 8;
}

void term_reset_color(void)
{
    term_send_code(TERM_COLOR_RESET);
}

void term_error_color(void)
{
    term_send_code(TERM_COLOR_SET_FG, TERM_ATTR_RED);
    term_send_code(TERM_COLOR_BOLD);
}

static void apply_attr(int attr)
{
    term_reset_color();

    if (attr & TERM_ATTR_FG)
        term_send_code(TERM_COLOR_SET_FG, attr & TERM_ATTR_FG_MASK);
    if (attr & TERM_ATTR_BG)
        term_send_code(TERM_COLOR_SET_BG,
                       (attr & TERM_ATTR_BG_MASK) >> TERM_ATTR_BG_SHIFT);
    if (attr & TERM_ATTR_BOLD)
        term_send_code(TERM_COLOR_BOLD);
    if (attr & TERM_ATTR_INVERSE)
        term_send_code(TERM_COLOR_INVERSE);
}

void term_vprint_bound_at(bound_t *bound, int x, int y, int attr,
                           const char *fmt, va_list args)
{
    char buf[512];
    int max_len;

    vsnprintf(buf, sizeof(buf), fmt, args);

    max_len = bound->x + bound->width - x;
    if (max_len < 0)
        return;
    if ((int)strlen(buf) > max_len)
        buf[max_len] = '\0';

    if (y < bound->y || y >= bound->y + bound->height)
        return;

    term_goto(x, y);
    apply_attr(attr);
    term_write_str(buf);
    term_send_code(TERM_CLEAR_END_LINE);
    term_reset_color();
}

void term_print_at(int x, int y, int attr, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    term_vprint_at(x, y, attr, fmt, args);
    va_end(args);
}

void term_redraw(void)
{
    /* пока без явной буферизации; позже можно добавить
     * настоящий back-buffer + один write() за кадр */
}

void term_fetch_size(void)
{
    char resp[32];
    size_t i = 0;
    char c;

    term_write_str("\033[999;999H\033[6n");

    do {
        if (read(STDIN_FILENO, &c, 1) != 1)
            goto fallback;
    } while (c != '\033');

    if (read(STDIN_FILENO, &c, 1) != 1 || c != '[')
        goto fallback;

    while (i < sizeof(resp) - 1) {
        if (read(STDIN_FILENO, &c, 1) != 1)
            goto fallback;
        if (c == 'R')
            break;
        resp[i++] = c;
    }
    resp[i] = '\0';

    {
        int rows = 0, cols = 0;
        char *sep = strchr(resp, ';');
        if (!sep)
            goto fallback;
        *sep = '\0';
        rows = atoi(resp);
        cols = atoi(sep + 1);
        if (rows > 0 && cols > 0) {
            term_height = rows;
            term_width = cols;
            return;
        }
    }

fallback:
    term_width = 80;
    term_height = 24;
}

int term_get_key(void)
{
    char c;

    if (read(STDIN_FILENO, &c, 1) != 1)
        return -1;

    if ((unsigned char)c != 27)
        return (unsigned char)c;

    char c2;
    if (read(STDIN_FILENO, &c2, 1) != 1)
        return 27;

    if (c2 != '[' && c2 != 'O')
        return 27;

    char c3;
    if (read(STDIN_FILENO, &c3, 1) != 1)
        return 27;

    switch (c3) {
    case 'A': return KEY_UP;
    case 'B': return KEY_DOWN;
    case 'C': return KEY_RIGHT;
    case 'D': return KEY_LEFT;
    case 'H': return KEY_START;
    case 'F': return KEY_END;
    default:  return 27;
    }
}