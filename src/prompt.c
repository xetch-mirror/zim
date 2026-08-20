/*
 * prompt.c — интерактивный ввод командной строки (используется
 * ex-режимом и поиском): рисует ":" + текст внизу экрана через
 * render_prompt()/render_cursor() из render.c, читает символы по
 * одному, поддерживает влево/вправо и удаление.
 *
 * Возвращает 0, если пользователь подтвердил ввод (Enter) —
 * результат лежит в tvi->prompt. Возвращает -1, если отменено (Esc).
 */

#include <string.h>
#include "zim.h"

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
        render_prompt(tvi);
        render_cursor(tvi);
        render_flush(tvi);

        int c = term_get_key();

        if (tvi->interrupted) {
            tvi->interrupted = 0;
            tvi->flags &= ~FLAG_PROMPT;
            return -1;
        }

        if (c == '\r' || c == '\n') {
            tvi->flags &= ~FLAG_PROMPT;
            return 0;
        }

        if (c == 27) {
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
            continue;

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
