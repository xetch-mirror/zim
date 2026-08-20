/*
 * ex.c — разбор ex-команд для zim, в духе командной строки vim:
 * поддерживает сокращения (:e/:ed/:edit, :w/:wr/:write, :q/:qu/:quit),
 * принудительный флаг '!' (:q!, :w!, :e!), а также :wq и :x.
 */

#include "zim.h"
#include "io.h"
#include <string.h>

static const char *skip_ws(const char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    return s;
}

/* Длина слова команды: буквы до пробела, '!' или конца строки */
static size_t word_len(const char *p)
{
    size_t n = 0;
    while (p[n] && p[n] != ' ' && p[n] != '\t' && p[n] != '!')
        n++;
    return n;
}

/* word — сокращение full, если совпадает как префикс (":w" -> "write") */
static int is_abbrev(const char *word, size_t len, const char *full)
{
    if (len == 0 || len > strlen(full))
        return 0;
    return strncmp(word, full, len) == 0;
}

/*
 * Генерирует имя вида newN.txt для :e без аргумента — обычный
 * текстовый черновик, а не .c, раз имя явно не задано.
 */
static void gen_untitled_name(char *out, unsigned long out_size)
{
    static int counter = 0;
    int n = counter++;
    unsigned long i = 0;

    out[i++] = 'n'; out[i++] = 'e'; out[i++] = 'w';
    if (n > 0) {
        char digits[8];
        int d = 0;
        int v = n;
        while (v > 0 && d < 8) {
            digits[d++] = (char)('0' + (v % 10));
            v /= 10;
        }
        while (d > 0 && i < out_size - 5)
            out[i++] = digits[--d];
    }
    out[i++] = '.'; out[i++] = 't'; out[i++] = 'x'; out[i++] = 't';
    out[i] = '\0';
}

/*
 * :e[dit][!] [filename] — открыть/создать файл.
 * Без имени -> newN.txt. Незаписанные изменения блокируют переход,
 * пока не указан '!' (как E37 в vim).
 */
static void cmd_edit(tvi_t *tvi, const char *arg, int force)
{
    win_t *win = tvi->focus_window;
    char name_buf[256];
    const char *name = skip_ws(arg);

    if (!force && (win->flags & FLAG_DIRTY)) {
        error(tvi, "буфер изменён — используйте :e! (E37)");
        return;
    }

    if (*name == '\0') {
        gen_untitled_name(name_buf, sizeof(name_buf));
        name = name_buf;
    }

    if (io_create(name) != 0) {
        error(tvi, "не удалось создать файл: %s", name);
        return;
    }

    char *files[1];
    files[0] = (char *)name;
    open_files(win, files, 1);
    win->flags &= ~FLAG_DIRTY;

    print(tvi, "\"%s\"", name);
}

/* :w[rite][!] [filename] — записать буфер целиком */
static int cmd_write(tvi_t *tvi, const char *arg, int force)
{
    (void)force;
    win_t *win = tvi->focus_window;
    const char *path = skip_ws(arg);
    const char *target;

    if (*path) {
        target = path;
    } else if (win->files_count > 0) {
        target = win->files[win->file_index];
    } else {
        error(tvi, "нет имени файла (E32)");
        return -1;
    }

    if (write_file(tvi, win, target, 0, win->lines_count - 1) != 0) {
        error(tvi, "ошибка записи файла: %s", target);
        return -1;
    }

    win->flags &= ~FLAG_DIRTY;
    print(tvi, "\"%s\" записано", target);
    return 0;
}

/* :q[uit][!] — выйти; отказывает при несохранённых правках без '!' */
static void cmd_quit(tvi_t *tvi, int force)
{
    win_t *win = tvi->focus_window;

    if (!force && (win->flags & FLAG_DIRTY)) {
        error(tvi, "нет записи после последнего изменения — :q! (E37)");