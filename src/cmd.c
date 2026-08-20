/*
 * cmd.c — command execution for zim
 */

#include "zim.h"
#include <string.h>

static void cmd_quit(tvi_t *tvi, int force)
{
    win_t *win = tvi->focus_window;

    if (!force && (win->flags & FLAG_DIRTY)) {
        error(tvi, "нет записи после последнего изменения — :q! (E37)");
        return;
    }

    tvi->flags |= FLAG_QUIT;
    print(tvi, "Выход");
}

int ex_command(tvi_t *tvi, const char *command)
{
    if (!command || !*command)
        return 0;

    const char *cmd = command;
    
    /* Skip leading whitespace */
    while (*cmd == ' ' || *cmd == '\t')
        cmd++;

    if (strncmp(cmd, "q", 1) == 0) {
        int force = 0;
        cmd++;
        while (*cmd == ' ' || *cmd == '\t')
            cmd++;
        if (*cmd == '!') {
            force = 1;
        }
        cmd_quit(tvi, force);
    } else if (strncmp(cmd, "wq", 2) == 0) {
        cmd_quit(tvi, 1);
    } else if (strncmp(cmd, "x", 1) == 0) {
        cmd_quit(tvi, 1);
    }

    return 0;
}

int ex_main(tvi_t *tvi)
{
    (void)tvi;
    return 0;
}
