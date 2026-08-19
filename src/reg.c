#include <stdlib.h>
#include <string.h>
#include "zim.h"

static reg_t *reg_lookup(tvi_t *tvi, int name)
{
        if (name == 0)
                return &tvi->unamed_reg;
        if (name >= 'a' && name <= 'z')
                return &tvi->alpha_regs[name - 'a'];
        if (name >= '0' && name <= '9')
                return &tvi->digit_regs[name - '0'];
        return NULL;
}

static char *dup_str(const char *s)
{
        size_t len = s ? strlen(s) : 0;
        char *out = malloc(len + 1);
        if (!out)
                return NULL;
        if (len)
                memcpy(out, s, len);
        out[len] = '\0';
        return out;
}

void reg_write(tvi_t *tvi, int name, char *const *lines, size_t lines_count, int type)
{
        reg_t *reg = reg_lookup(tvi, name);
        char **copy;
        size_t i;

        if (!reg)
                return;

        if (reg->text)
                free_list(reg->text, reg->lines_count);
        reg->text = NULL;
        reg->lines_count = 0;

        if (lines_count == 0) {
                reg->type = type;
                return;
        }

        copy = malloc(lines_count * sizeof(char *));
        if (!copy)
                return;

        for (i = 0; i < lines_count; i++)
                copy[i] = dup_str(lines[i]);

        reg->text = copy;
        reg->lines_count = lines_count;
        reg->type = type;
}

int reg_put_lines(tvi_t *tvi, win_t *win, int name, int addr)
{
        reg_t *reg = reg_lookup(tvi, name);

        if (!reg || !reg->text || reg->lines_count == 0)
                return -1;
        if (reg->type != REG_LINE)
                return -1;

        text_insert_lines(win, addr, reg->text, reg->lines_count);
        return 0;
}

int reg_put(tvi_t *tvi, win_t *win, int name, int x, int y, int after)
{
        reg_t *reg = reg_lookup(tvi, name);
        size_t total, i, off;
        char *joined;

        if (!reg || !reg->text || reg->lines_count == 0)
                return -1;

        if (reg->type == REG_LINE)
                return reg_put_lines(tvi, win, name, after ? y + 1 : y);

        total = 0;
        for (i = 0; i < reg->lines_count; i++) {
                total += reg->text[i] ? strlen(reg->text[i]) : 0;
                if (i + 1 < reg->lines_count)
                        total += 1;
        }

        joined = malloc(total + 1);
        if (!joined)
                return -1;

        off = 0;
        for (i = 0; i < reg->lines_count; i++) {
                size_t len = reg->text[i] ? strlen(reg->text[i]) : 0;
                memcpy(joined + off, reg->text[i], len);
                off += len;
                if (i + 1 < reg->lines_count)
                        joined[off++] = '\n';
        }
        joined[off] = '\0';

        text_insert_buf(win, after ? x + 1 : x, y, joined, total);
        free(joined);
        return 0;
}