/*
 * io.c — file I/O operations for zim
 */

#include "zim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int io_create(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd >= 0) {
        close(fd);
        return 0;
    }
    if (errno == EEXIST)
        return 0;
    return -1;
}

void open_files(win_t *win, char *const*files, size_t files_count)
{
    if (files_count == 0 || !files)
        return;

    win->files = malloc(files_count * sizeof(char*));
    if (!win->files)
        return;

    for (size_t i = 0; i < files_count; i++) {
        win->files[i] = malloc(strlen(files[i]) + 1);
        if (!win->files[i]) {
            free_list(win->files, i);
            win->files = NULL;
            return;
        }
        strcpy(win->files[i], files[i]);
    }

    win->files_count = (int)files_count;
    if (files_count > 0)
        read_file(win, files[0]);
}

void read_file(win_t *win, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return;

    char buf[4096];
    while (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n')
            buf[--len] = '\0';

        char *line = malloc(len + 1);
        if (!line)
            break;
        strcpy(line, buf);

        text_insert_lines(win, win->lines_count, &line, 1);
        free(line);
    }

    fclose(f);
}

int write_file(tvi_t *tvi, win_t *win, const char *path, int first, int last)
{
    (void)tvi;
    FILE *f = fopen(path, "w");
    if (!f)
        return -1;

    for (int i = first; i <= last && i < win->lines_count; i++) {
        const char *line = (win->text && i < win->lines_count) 
                           ? win->text[i] 
                           : "";
        if (fprintf(f, "%s\n", line) < 0) {
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}

void free_list(char **list, size_t count)
{
    if (!list)
        return;
    for (size_t i = 0; i < count; i++)
        free(list[i]);
    free(list);
}
