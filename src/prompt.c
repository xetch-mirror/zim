/*
 * prompt.c — интерактивный ввод командной строки (используется
 * ex-режимом и поиском): рисует ":" + текст внизу экрана, читает
 * символы по одному, поддерживает влево/вправо и удаление.
 *
 * Возвращает 0, если пользователь подтвердил ввод (Enter) —
 * результат лежит в tvi->prompt. Возвращает -1, если отменено (Esc).
 */

#include "zim.h"
#include <string.h>

static void render_prompt_line(tvi_t *tvi)
{
    int row = term_height - 1;
    term_clear_line(row);
    term_print_at(0, row, TERM_ATTR_FG_WHITE, ":%s", tvi->prompt);
    term_set_cursor((int)(1 + tvi->prompt_cursor), row);
}

int prompt(tvi_t *tvi, const char *initial, int newline)
{
    (void)newline;

    size_t len = initial ? strlen(initial) : 0;
    if (len >= sizeof(tvi->prompt))
        len = sizeof(tvi->prompt) - 1;

    memcpy(tvi->prompt, initial ? initial : "", len);
    tvi->prompt[len] = '\0';
    tvi->prompt_len = len;
    tvi->prompt_cursor = len;
    tvi->flags |= FLAG_PROMPT;

    for (;;) {
        render_prompt_line(tvi);

        int c = term_get_key();

        if (c == '\r' || c == '\n') {
            tvi->flags &= ~FLAG_PROMPT;
            return 0;
        }

        if (c == 27) { /* Esc отменяет ввод */
            tvi->flags &= ~FLAG_PROMPT;
            tvi->prompt[0] = '\0';
            tvi->prompt_len = 0;
            tvi->prompt_cursor = 0;
            return -1;
        }

        if (term_is_delete(c)) {
            if (tvi->prompt_cursor > 0) {
                size_t i = --tvi->prompt_cursor;
                memmove(&tvi->prompt[i], &tvi->prompt[i + 1],
                        tvi->prompt_len - i);
                tvi->prompt_len--;
            }
            continue;
        }

        if (c == KEY_LEFT) {
            if (tvi->prompt_cursor > 0)
                tvi->prompt_cursor--;
            continue;
        }
        if (c == KEY_RIGHT) {
            if (tvi->prompt_cursor < tvi->prompt_len)
                tvi->prompt_cursor++;
            continue;
        }

        if (c < 0 || c < 32)
            continue; /* спецклавиши/управляющие символы игнорируем */

        if (tvi->prompt_len + 1 < sizeof(tvi->prompt)) {
            size_t i = tvi->prompt_cursor;
            memmove(&tvi->prompt[i + 1], &tvi->prompt[i],
                    tvi->prompt_len - i + 1);
            tvi->prompt[i] = (char)c;
            tvi->prompt_len++;
            tvi->prompt_cursor++;
        }
    }
}