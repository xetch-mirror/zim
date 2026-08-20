/*
 * cmd.c — command execution for zim
 */

#include "zim.h"
#include <string.h>

static void cmd_quit(tvi_t *tvi, int force)
{
    win_t *win = tvi->focus_window;

    if (!force && (win->flags & FLAG_DIRTY)) {
        error(tvi, "No write since last change — :q! (E37)");
        return;
    }

    tvi->flags |= FLAG_QUIT;
    print(tvi, "Quit");
}

static void cmd_edit(tvi_t *tvi, win_t *win, const char *path)
{
    if (!path || !*path) {
        error(tvi, "E32: No file name");
        return;
    }

    if (io_create(path) != 0) {
        error(tvi, "E212: Can't open file for writing");
        return;
    }

    char pathbuf[256];
    strncpy(pathbuf, path, sizeof(pathbuf) - 1);
    pathbuf[sizeof(pathbuf) - 1] = '\0';
    char *files[1] = { pathbuf };

    open_files(win, files, 1);
    win->flags &= ~FLAG_DIRTY;
    win->cursor_x = 0;
    win->cursor_y = 0;
}

static int cmd_write(tvi_t *tvi, win_t *win, const char *path)
{
    const char *target = path;

    if (!target || !*target) {
        if (win->files_count > 0 && win->files)
            target = win->files[0];
        else {
            error(tvi, "E32: No file name");
            return -1;
        }
    }

    if (write_file(tvi, win, target, 0, win->lines_count - 1) != 0) {
        error(tvi, "E212: Error writing file");
        return -1;
    }

    win->flags &= ~FLAG_DIRTY;
    print(tvi, "\"%s\" written", target);
    return 0;
}

int ex_command(tvi_t *tvi, const char *command)
{
    if (!command || !*command)
        return 0;

    win_t *win = tvi->focus_window;
    const char *cmd = command;

    /* Skip leading whitespace */
    while (*cmd == ' ' || *cmd == '\t')
        cmd++;

    if (strncmp(cmd, "wq", 2) == 0) {
        if (cmd_write(tvi, win, NULL) == 0)
            cmd_quit(tvi, 1);
    } else if (strncmp(cmd, "x", 1) == 0) {
        if (cmd_write(tvi, win, NULL) == 0)
            cmd_quit(tvi, 1);
    } else if (strncmp(cmd, "q", 1) == 0) {
        int force = 0;
        cmd++;
        while (*cmd == ' ' || *cmd == '\t')
            cmd++;
        if (*cmd == '!') {
            force = 1;
        }
        cmd_quit(tvi, force);
    } else if (strncmp(cmd, "w", 1) == 0) {
        cmd++;
        while (*cmd == ' ' || *cmd == '\t')
            cmd++;
        cmd_write(tvi, win, *cmd ? cmd : NULL);
    } else if (strncmp(cmd, "e", 1) == 0) {
        cmd++;
        while (*cmd == ' ' || *cmd == '\t')
            cmd++;
        cmd_edit(tvi, win, cmd);
    } else {
        error(tvi, "E492: Unknown command");
    }

    return 0;
}

int ex_main(tvi_t *tvi)
{
    (void)tvi;
    return 0;
}