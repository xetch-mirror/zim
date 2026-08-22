#include "zim.h"

#define X_LINE_BUF 512

static char x_out_buf[X_LINE_BUF];

static int x_color_for(char c) {
    switch (c) {
        case '>': case '<': return TERM_ATTR_FG_CYAN;
        case '+': case '-': return TERM_ATTR_FG_GREEN;
        case '.': case ',': return TERM_ATTR_FG_YELLOW;
        case '[': case ']': return TERM_ATTR_FG_MAGENTA;
        default:             return TERM_ATTR_FG_WHITE;
    }
}

static const char *x_print_line(win_t *win, int y, const char *line) {
    (void)win;
    (void)y;

    size_t out = 0;
    int color = -1;
    size_t i = 0;

    while (line[i] != '\0' && out < X_LINE_BUF - 16) {
        int new_color = x_color_for(line[i]);

        if (new_color != color) {
            const char *code = term_get_code(TERM_COLOR_SET_FG);
            size_t code_len = term_get_code_len(TERM_COLOR_SET_FG);
            for (size_t k = 0; k < code_len && out < X_LINE_BUF - 16; k++) {
                x_out_buf[out++] = code[k];
            }
            color = new_color;
        }

        x_out_buf[out++] = line[i];
        i++;
    }

    x_out_buf[out] = '\0';
    return x_out_buf;
}

syntax_t x_syntax = {
    .handle = 0,
    .print_line = x_print_line,
};