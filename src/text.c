#include <stdlib.h>
#include <string.h>
#include "zim.h"

static char *dup_bytes(const char *s, size_t len)
{
        char *out = malloc(len + 1);
        if (!out)
                return NULL;
        if (len)
                memcpy(out, s, len);
        out[len] = '\0';
        return out;
}

static char *dup_str(const char *s)
{
        return dup_bytes(s, s ? strlen(s) : 0);
}

static int resize_lines(win_t *win, int new_count)
{
        char **grown;

        if (new_count <= 0) {
                free(win->text);
                win->text = NULL;
                win->lines_count = 0;
                return 0;
        }

        grown = realloc(win->text, (size_t)new_count * sizeof(char *));
        if (!grown)
                return -1;

        if (new_count > win->lines_count)
                memset(grown + win->lines_count, 0,
                       (size_t)(new_count - win->lines_count) * sizeof(char *));

        win->text = grown;
        win->lines_count = new_count;
        return 0;
}

void text_insert_lines(win_t *win, int addr, char *const *lines, size_t lines_count)
{
        int old_count = win->lines_count;
        int new_count = old_count + (int)lines_count;
        size_t i;

        if (addr < 0)
                addr = 0;
        if (addr > old_count)
                addr = old_count;

        if (resize_lines(win, new_count) != 0)
                return;

        memmove(win->text + addr + lines_count, win->text + addr,
                (size_t)(old_count - addr) * sizeof(char *));

        for (i = 0; i < lines_count; i++)
                win->text[addr + i] = dup_str(lines[i]);

        text_mark_dirty(win);
}

void text_insert_newline(win_t *win, int x, int y)
{
        char *line, *head, *tail;
        size_t len;

        if (y < 0 || y >= win->lines_count)
                return;

        line = win->text[y];
        len = line ? strlen(line) : 0;
        if (x < 0)
                x = 0;
        if ((size_t)x > len)
                x = (int)len;

        head = dup_bytes(line, (size_t)x);
        tail = dup_bytes(line + x, len - (size_t)x);
        if (!head || !tail) {
                free(head);
                free(tail);
                return;
        }

        free(win->text[y]);
        win->text[y] = head;

        text_insert_lines(win, y + 1, &tail, 1);
        free(tail);
}

void text_insert_buf(win_t *win, int x, int y, const char *buf, size_t count)
{
        size_t start = 0, i;

        if (y < 0 || y >= win->lines_count)
                return;

        for (i = 0; i < count; i++) {
                if (buf[i] != '\n')
                        continue;

                if (i > start) {
                        char *line = win->text[y];
                        size_t len = line ? strlen(line) : 0;
                        char *grown = malloc(len + (i - start) + 1);
                        if (!grown)
                                return;
                        memcpy(grown, line, (size_t)x);
                        memcpy(grown + x, buf + start, i - start);
                        memcpy(grown + x + (i - start), line + x, len - (size_t)x);
                        grown[len + (i - start)] = '\0';
                        free(win->text[y]);
                        win->text[y] = grown;
                        x += (int)(i - start);
                }

                text_insert_newline(win, x, y);
                y++;
                x = 0;
                start = i + 1;
        }

        if (start < count) {
                char *line = win->text[y];
                size_t len = line ? strlen(line) : 0;
                size_t add = count - start;
                char *grown = malloc(len + add + 1);
                if (!grown)
                        return;
                memcpy(grown, line, (size_t)x);
                memcpy(grown + x, buf + start, add);
                memcpy(grown + x + add, line + x, len - (size_t)x);
                grown[len + add] = '\0';
                free(win->text[y]);
                win->text[y] = grown;
        }

        text_mark_dirty(win);
}

static void delete_chars(tvi_t *tvi, win_t *win, int x, int y, size_t count, int reg)
{
        char *line, *removed, *rest;
        size_t len;

        if (y < 0 || y >= win->lines_count)
                return;

        line = win->text[y];
        len = line ? strlen(line) : 0;
        if (x < 0)
                x = 0;
        if ((size_t)x >= len)
                return;
        if (count > len - (size_t)x)
                count = len - (size_t)x;

        if (reg >= 0) {
                removed = dup_bytes(line + x, count);
                if (removed) {
                        char *lines[1] = { removed };
                        reg_write(tvi, reg, lines, 1, REG_CHAR);
                        free(removed);
                }
        }

        rest = dup_bytes(line, (size_t)x);
        if (!rest)
                return;
        {
                size_t head_len = (size_t)x;
                size_t tail_len = len - (size_t)x - count;
                char *joined = malloc(head_len + tail_len + 1);
                if (!joined) {
                        free(rest);
                        return;
                }
                memcpy(joined, line, head_len);
                memcpy(joined + head_len, line + x + count, tail_len);
                joined[head_len + tail_len] = '\0';
                free(rest);
                free(win->text[y]);
                win->text[y] = joined;
        }

        text_mark_dirty(win);
}

void text_delete(win_t *win, int x, int y, size_t count)
{
        delete_chars(NULL, win, x, y, count, -1);
}

void text_delete_reg(tvi_t *tvi, win_t *win, int x, int y, size_t count, int reg)
{
        delete_chars(tvi, win, x, y, count, reg);
}

static void delete_lines(tvi_t *tvi, win_t *win, int addr, size_t count, int reg)
{
        int i;

        if (addr < 0 || addr >= win->lines_count)
                return;
        if (count > (size_t)(win->lines_count - addr))
                count = (size_t)(win->lines_count - addr);
        if (count == 0)
                return;

        if (reg >= 0)
                reg_write(tvi, reg, win->text + addr, count, REG_LINE);

        for (i = 0; i < (int)count; i++)
                free(win->text[addr + i]);

        memmove(win->text + addr, win->text + addr + count,
                (size_t)(win->lines_count - addr - (int)count) * sizeof(char *));

        resize_lines(win, win->lines_count - (int)count);
        text_mark_dirty(win);
}

void text_delete_lines(win_t *win, int addr, size_t count)
{
        delete_lines(NULL, win, addr, count, -1);
}

void text_delete_lines_reg(tvi_t *tvi, win_t *win, int addr, size_t count, int reg)
{
        delete_lines(tvi, win, addr, count, reg);
}

void text_yank_lines(tvi_t *tvi, win_t *win, int addr, size_t count, int reg)
{
        if (addr < 0 || addr >= win->lines_count)
                return;
        if (count > (size_t)(win->lines_count - addr))
                count = (size_t)(win->lines_count - addr);
        if (count == 0)
                return;

        reg_write(tvi, reg, win->text + addr, count, REG_LINE);
}

void text_join(win_t *win, int first, int last, char sep)
{
        size_t total = 0;
        int i;
        char *joined, *p;

        if (first < 0 || first >= win->lines_count)
                return;
        if (last >= win->lines_count)
                last = win->lines_count - 1;
        if (last <= first)
                return;

        for (i = first; i <= last; i++)
                total += win->text[i] ? strlen(win->text[i]) : 0;
        if (sep)
                total += (size_t)(last - first);

        joined = malloc(total + 1);
        if (!joined)
                return;

        p = joined;
        for (i = first; i <= last; i++) {
                size_t len = win->text[i] ? strlen(win->text[i]) : 0;
                if (i > first && sep)
                        *p++ = sep;
                memcpy(p, win->text[i], len);
                p += len;
        }
        *p = '\0';

        free(win->text[first]);
        win->text[first] = joined;

        text_delete_lines(win, first + 1, (size_t)(last - first));
}

void text_mark_dirty(win_t *win)
{
        win->flags |= FLAG_DIRTY;
}