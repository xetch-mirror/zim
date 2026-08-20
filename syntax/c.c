#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <zim.h>

static const char *types[] = {
        "char",
        "short",
        "int",
        "long",
        "signed",
        "unsigned",
        "float",
        "double",
        "static",
        "inline",
        "volatile",
        "extern",
        "void",
        "struct",
        "union",
        "enum",
        "const",
        "typedef",

// standard types
        "size_t",
        "ssize_t",
        "int8_t",
        "uint8_t",
        "int16_t",
        "uint16_t",
        "int32_t",
        "uint32_t",
        "int64_t",
        "uint64_t",
        "FILE",
        "DIR",
        "va_list",
};

static const char *keywords[] = {
        "if",
        "else",
        "for",
        "while",
        "do",
        "return",
        "switch",
        "case",
        "default",
        "sizeof",
        "break",
        "continue",
};

static const char *preprocs[] = {
        "#define",
        "#undef",
        "#ifdef",
        "#ifndef",
        "#if",
        "#else",
        "#elif",
        "#endif",
        "#include",
};

static const char *consts[] = {
        "NULL",
        "nullptr",
        "EOF",
        "SEEK_SET",
        "SEEK_CUR",
        "SEEK_END",
        "stdout",
        "stdin",
        "stderr",
        "INT_MAX",
        "INT_MIN",
        "UINT_MAX",
};

#define WORD_TYPES    0
#define WORD_KEYWORDS 1
#define WORD_PREPROCS 2
#define WORD_CONSTS   3
#define arraylen(a) sizeof(a)/sizeof(*a)

static const char **words[] = {
        [WORD_TYPES] = types,
        [WORD_KEYWORDS] = keywords,
        [WORD_PREPROCS]  = preprocs,
        [WORD_CONSTS] = consts,
};

static size_t words_len[] = {
        [WORD_TYPES] = arraylen(types),
        [WORD_KEYWORDS] = arraylen(keywords),
        [WORD_PREPROCS]  = arraylen(preprocs),
        [WORD_CONSTS] = arraylen(consts),
};

static int alpha_sort(const void *e1, const void *e2) {
        const char *const*str1 = e1;
        const char *const*str2 = e2;
        return strcmp(*str1, *str2);
}

static int c_init() {
        for (size_t i=0; i<arraylen(words); i++) {
                qsort(words[i], words_len[i], sizeof(const char *), alpha_sort);
        }
        return 0;
}

static int c_c_is_word_type(const char *word, size_t len, int type) {
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

static int c_word_color(const char *word, size_t size) {
	if (c_c_is_word_type(word, size, WORD_TYPES)) return TERM_ATTR_FG_GREEN;
	if (c_c_is_word_type(word, size, WORD_KEYWORDS)) return TERM_ATTR_FG_YELLOW;
	if (c_c_is_word_type(word, size, WORD_PREPROCS) || isdigit(*word)) return TERM_ATTR_FG_MAGENTA;
}

static int is_word_char(int c) {
        return isalnum(c) || c == '_';
}

static int reach_line_end(const char *line) {
        return !*line || (*line == '/' && line[1] == '/');
}

static const char *skip_backslash(const char *line) {
        if (*line != '\\') return line;
        for (int i=1; i<3; i++) {
                if (!line[i]) {
                        return line + i;
                }
                if (!isdigit(line[i])) {
                        return line + i + 1;
                }
        }
        return line + 4;
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

static void c_print_line(win_t *win, int y, const char *line) {
        // print word by word
        int x = 0;
        while (isblank(*line)) {
                put_at(win, &x, &y, 0, *line);
                line++;
        }
        if (*line == '#') {
                // find lenght of word
                const char *word = line;
                size_t word_len = 1;
                while (is_word_char(word[word_len])) {
                        word_len++;
                }
                if (c_is_word_type(word, word_len, WORD_PREPROCS)) {
                        put_word(win, &x, &y, TERM_ATTR_FG_BLUE, word, word_len);
                        line += word_len;
                }
        }
        while (!reach_line_end(line)) {
                if (*line == '\'') {
                        put_at(win, &x, &y, TERM_ATTR_FG_MAGENTA, *(line++));
                        if (!*line) break;
                        const char *end;
                        if (*line == '\\') {
                                end = skip_backslash(line);
                        } else {
                                end = line + 1;
                        }
                        put_word(win, &x, &y, TERM_ATTR_FG_MAGENTA, line, (int)(end - line));
                        line = end;
                        if (*end == '\'') {
                                put_at(win, &x, &y, TERM_ATTR_FG_MAGENTA, *(line++));
                        }
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
                if (!is_word_char(*line)) {
                        const char *start = line;
                        size_t len = 0;
                        while (!is_word_char(*line) && !reach_line_end(line) && *line != '"' && *line != '\'') {
                                line++;
                                len++;
                        }
                        put_word(win, &x, &y, 0, start, len);
                        continue;
                }
                // find lenght of word
                const char *word = line;
                size_t word_len = 0;
                while (is_word_char(*line)) {
                        word_len++;
                        line++;
                }
                int color = c_word_color(word, word_len);
                put_word(win, &x, &y, color, word, word_len);
                continue;
        }

        if (*line) {
                win_print_at(win, x, y, TERM_ATTR_FG_CYAN, "%s", line);
        }
}