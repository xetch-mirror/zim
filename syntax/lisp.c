#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <zim.h>

const char *keywords[] = {
        "define",
        "defun",
        "lambda",
        "let",
        "let*",
        "letrec",
        "if",
        "cond",
        "else",
        "case",
        "when",
        "unless",
        "begin",
        "do",
        "and",
        "or",
        "not",
        "set!",
        "quote",
        "quasiquote",
        "unquote",
        "unquote-splicing",
};

const char *builtins[] = {
        "car",
        "cdr",
        "cons",
        "list",
        "append",
        "length",
        "reverse",
        "map",
        "filter",
        "apply",
        "eval",
        "print",
        "display",
        "newline",
        "eq?",
        "eqv?",
        "equal?",
        "null?",
        "pair?",
        "atom?",
        "number?",
        "symbol?",
        "string?",
        "+",
        "-",
        "*",
        "/",
        "=",
        "<",
        ">",
        "<=",
        ">=",
};

const char *consts[] = {
        "nil",
        "t",
        "true",
        "false",
        "#t",
        "#f",
};

#define WORD_KEYWORDS 0
#define WORD_BUILTINS 1
#define WORD_CONSTS   2
#define arraylen(a) sizeof(a)/sizeof(*a)

const char **words[] = {
        [WORD_KEYWORDS] = keywords,
        [WORD_BUILTINS] = builtins,
        [WORD_CONSTS]   = consts,
};

size_t words_len[] = {
        [WORD_KEYWORDS] = arraylen(keywords),
        [WORD_BUILTINS] = arraylen(builtins),
        [WORD_CONSTS]   = arraylen(consts),
};

static int alpha_sort(const void *e1, const void *e2) {
        const char *const*str1 = e1;
        const char *const*str2 = e2;
        return strcmp(*str1, *str2);
}

int init() {
        for (size_t i=0; i<arraylen(words); i++) {
                qsort(words[i], words_len[i], sizeof(const char *), alpha_sort);
        }
        return 0;
}

/* lisp symbols allow a much wider character set than C identifiers:
 * letters, digits, and most punctuation except parens, quote, and
 * whitespace. this covers common cases like set!, <=, ->, list->vector */
static int is_symbol_char(int c) {
        if (isalnum(c)) return 1;
        switch (c) {
        case '(': case ')': case '\'': case '`': case ',':
        case '"': case ';': case ' ': case '\t': case '\0':
                return 0;
        default:
                return 1;
        }
}

int is_word_type(const char *word, size_t len, int type) {
        const char **list = words[type];
        size_t start = 0;
        size_t end  = words_len[type]-1;
        while (end >= start) {
                size_t middle = (start + end) / 2;
                int cmp = strncmp(word, list[middle], len);
                if (cmp == 0) {
                        if (strlen(list[middle]) == len) return 1;
                        else if (strlen(list[middle]) > len) {
                                goto before;
                        } else {
                                goto after;
                        }
                } if (cmp > 0) {
after:
                        start = middle + 1;
                } else {
before:
                        if (middle == 0) return 0;
                        end = middle - 1;
                }
        }
        return 0;
}

int word_color(const char *word, size_t size) {
        if (is_word_type(word, size, WORD_KEYWORDS)) return TERM_ATTR_FG_YELLOW;
        if (is_word_type(word, size, WORD_BUILTINS)) return TERM_ATTR_FG_GREEN;
        if (is_word_type(word, size, WORD_CONSTS)) return TERM_ATTR_FG_MAGENTA;
        if (isdigit(*word) || (*word == '-' && isdigit(word[1]))) return TERM_ATTR_FG_MAGENTA;
        return 0;
}

static int reach_line_end(const char *line) {
        return !*line || (*line == ';');
}

static void put_at(win_t *win, int *x, int *y, int attr, char c) {
        win_print_at(win, (*x)++, *y, attr, "%c", c);
        if (*x >= win->x + win->width) {
                *x = 0;
                (*y)++;
        }
}

static void put_word(win_t *win, int *x, int *y, int attr, const char *str, int len) {
        win_print_at(win, *x, *y, attr, "%.*s", len, str);
        *x += len;
        while (*x >= win->x + win->width) {
                *x -= win->width;
                (*y)++;
        }
}

const char *print_line(win_t *win, int y, const char *line) {
        int x = 0;
        while (isblank(*line)) {
                put_at(win, &x, &y, 0, *line);
                line++;
        }

        while (!reach_line_end(line)) {
                if (*line == '(' || *line == ')') {
                        put_at(win, &x, &y, TERM_ATTR_FG_CYAN, *(line++));
                        continue;
                }
                if (*line == '\'' || *line == '`' || *line == ',') {
                        put_at(win, &x, &y, TERM_ATTR_FG_BLUE, *(line++));
                        continue;
                }
                if (*line == '"') {
                        put_at(win, &x, &y, TERM_ATTR_FG_MAGENTA, *(line++));
                        while (*line != '"' && *line) {
                                put_at(win, &x, &y, TERM_ATTR_FG_MAGENTA, *(line++));
                        }
                        if (*line == '"') {
                                put_at(win, &x, &y, TERM_ATTR_FG_MAGENTA, *(line++));
                        }
                        continue;
                }
                if (isblank(*line)) {
                        const char *start = line;
                        size_t len = 0;
                        while (isblank(*line)) { line++; len++; }
                        put_word(win, &x, &y, 0, start, len);
                        continue;
                }
                if (!is_symbol_char(*line)) {
                        put_at(win, &x, &y, 0, *(line++));
                        continue;
                }

                const char *word = line;
                size_t word_len = 0;
                while (is_symbol_char(*line)) {
                        word_len++;
                        line++;
                }
                int color = word_color(word, word_len);
                put_word(win, &x, &y, color, word, word_len);
        }

        if (*line) {
                win_print_at(win, x, y, TERM_ATTR_FG_CYAN, "%s", line);
        }

        return line;
}