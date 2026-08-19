#ifndef ZIM_H
#define ZIM_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#else
typedef int sig_atomic_t;
#endif

struct syntax;

typedef struct win {
        struct win *next;
        struct win *prev;
        int x;
        int y;
        int width;
        int height;
        int cursor_x;
        int cursor_y;
        int scroll;
        int flags;
        char **text;
        char **files;
        struct syntax *syntax;
        int lines_count;
        int files_count;
        int file_index;
} win_t;

typedef struct reg {
        char **text;
        size_t lines_count;
        int type;
} reg_t;

#define REG_CHAR 0
#define REG_LINE 1

typedef struct tvi {
        int mode;
        int flags;
        char prompt[256];
        size_t prompt_len;
        size_t prompt_cursor;
        win_t *focus_window;
        win_t *first_window;
        reg_t alpha_regs[26];
        reg_t digit_regs[10];
        reg_t unamed_reg;
        char *old_buffer;
        char *new_buffer;
        volatile sig_atomic_t interrupted;
} tvi_t;

typedef struct syntax {
        void *handle;
        const char *(*print_line)(win_t *win, int y, const char*);
} syntax_t;

typedef struct cell {
        int attr;
        int c;
} cell_t;

typedef struct bound {
        int x;
        int y;
        int width;
        int height;
} bound_t;

const char *term_get_code(int code);
size_t term_get_code_len(int code);
void term_send_code(int code, ...);
int term_enable_raw_mode(void);
void term_quit_raw_mode(void);
int term_get_key(void);
int term_enter_fullscreen(void);
void term_exit_fullscreen(void);
void term_clear_line(int y);
void term_bell(void);
void term_set_cursor(int x, int y);
void term_goto(int x, int y);
int term_is_delete(int c);
void term_redraw(void);
void term_print_at(int x, int y, int attr, const char *fmt, ...);
void term_vprint_at(int x, int y, int attr, const char *fmt, va_list args);
void term_vprint_bound_at(bound_t *bound, int x, int y, int attr, const char *fmt, va_list args);
void term_fetch_size(void);
void term_reset_color(void);
void term_error_color(void);
void render_text(tvi_t *tvi, win_t *win);
int render_line(tvi_t *tvi, win_t *win, size_t index);
void render_status(tvi_t *tvi, win_t *win);
void render_window(tvi_t *tvi, win_t *win);
void render_all_windows(tvi_t *tvi);
void render_prompt(tvi_t *tvi);
void render_cursor(tvi_t *tvi);
void render_flush(tvi_t *tvi);
win_t *win_create(tvi_t *tvi);
void win_free(tvi_t *tvi, win_t *win);
void win_print_at(win_t *win, int x, int y, int attr, const char *fmt, ...);
void text_insert_lines(win_t *win, int addr, char *const*lines, size_t lines_count);
void text_insert_newline(win_t *win, int x, int y);
void text_insert_buf(win_t *win, int x, int y, const char *buf, size_t count);
void text_delete(win_t *win, int x, int y, size_t count);
void text_delete_reg(tvi_t *tvi, win_t *win, int x, int y, size_t count, int reg);
void text_delete_lines(win_t *win, int addr, size_t count);
void text_delete_lines_reg(tvi_t *tvi, win_t *win, int addr, size_t count, int reg);
void text_yank_lines(tvi_t *tvi, win_t *win, int addr, size_t count, int reg);
void text_join(win_t *win, int first, int last, char sep);
void text_mark_dirty(win_t *win);
void reg_write(tvi_t *tvi, int name, char *const*lines, size_t lines_count, int type);
int reg_put(tvi_t *tvi, win_t *win, int name, int x, int y, int after);
int reg_put_lines(tvi_t *tvi, win_t *win, int name, int addr);
syntax_t *syntax_load(const char *name);
void syntax_unload(syntax_t *syntax);
void syntax_print_line(win_t *win, int y, syntax_t *syntax, const char *line);
int ex_command(tvi_t *tvi, const char *command);
void open_files(win_t *win, char *const*files, size_t files_count);
void read_file(win_t *win, const char *path);
int write_file(tvi_t *tvi, win_t *win, const char *path, int first, int last);
void free_list(char **list, size_t count);
int tvi_main(tvi_t *tvi);
int ex_main(tvi_t *tvi);
void error(tvi_t *tvi, const char *fmt, ...);
void print(tvi_t *tvi, const char *fmt, ...);
int prompt(tvi_t *tvi, const char *initial, int newline);
void cursor_to_non_blank(win_t *win);
void signal_install_handlers(void);

extern int term_width;
extern int term_height;
extern tvi_t tvi;

#define ESC "\033"
#define CRTL(c) (c - 'A' + 1)
#define MODE_VISUAL   1
#define MODE_EX       2
#define FLAG_PROMPT   0x01 // when prompt is enabled
#define FLAG_QUIT     0x02
#define FLAG_DIRTY    0x04


#define KEY_UP    -2
#define KEY_DOWN  -3
#define KEY_RIGHT -4
#define KEY_LEFT  -5
#define KEY_START -6
#define KEY_END   -7

#define TERM_GOTO            0
#define TERM_COLOR_RESET     1
#define TERM_COLOR_INVERSE   2
#define TERM_COLOR_BOLD      3
#define TERM_COLOR_SET_FG    4
#define TERM_COLOR_SET_BG    5
#define TERM_CLEAR_END_LINE  6
#define TERM_INSERT          7
#define TERM_DELETE          8
#define TERM_CLEAR           9
#define TERM_INSERT_LINE    10
#define TERM_DELETE_LINE    11
#define TERM_CODES_COUNT    12

#define TERM_ATTR_BLACK   0
#define TERM_ATTR_RED     1
#define TERM_ATTR_GREEN   2
#define TERM_ATTR_YELLOW  3
#define TERM_ATTR_BLUE    4
#define TERM_ATTR_MAGENTA 5
#define TERM_ATTR_CYAN    6
#define TERM_ATTR_WHITE   7

#define TERM_ATTR_FG            0x008
#define TERM_ATTR_FG_MASK       0x007
#define TERM_ATTR_FG_BLACK      TERM_ATTR_FG | TERM_ATTR_BLACK
#define TERM_ATTR_FG_RED        TERM_ATTR_FG | TERM_ATTR_RED
#define TERM_ATTR_FG_GREEN      TERM_ATTR_FG | TERM_ATTR_GREEN
#define TERM_ATTR_FG_YELLOW     TERM_ATTR_FG | TERM_ATTR_YELLOW
#define TERM_ATTR_FG_BLUE       TERM_ATTR_FG | TERM_ATTR_BLUE
#define TERM_ATTR_FG_MAGENTA    TERM_ATTR_FG | TERM_ATTR_MAGENTA
#define TERM_ATTR_FG_CYAN       TERM_ATTR_FG | TERM_ATTR_CYAN
#define TERM_ATTR_FG_WHITE      TERM_ATTR_FG | TERM_ATTR_WHITE
#define TERM_ATTR_BG            0x080
#define TERM_ATTR_BG_MASK       0x070
#define TERM_ATTR_BG_SHIFT          8
#define TERM_ATTR_BG_BLACK      TERM_ATTR_BG | (TERM_ATTR_BLACK << TERM_ATTR_BG_SHIFT)
#define TERM_ATTR_BG_RED        TERM_ATTR_BG | (TERM_ATTR_RED << TERM_ATTR_BG_SHIFT)
#define TERM_ATTR_BG_GREEN      TERM_ATTR_BG | (TERM_ATTR_GREEN << TERM_ATTR_BG_SHIFT)
#define TERM_ATTR_BG_YELLOW     TERM_ATTR_BG | (TERM_ATTR_YELLOW << TERM_ATTR_BG_SHIFT)
#define TERM_ATTR_BG_BLUE       TERM_ATTR_BG | (TERM_ATTR_BLUE << TERM_ATTR_BG_SHIFT)
#define TERM_ATTR_BG_MAGENTA    TERM_ATTR_BG | (TERM_ATTR_MAGENTA << TERM_ATTR_BG_SHIFT)
#define TERM_ATTR_BG_CYAN       TERM_ATTR_BG | (TERM_ATTR_CYAN << TERM_ATTR_BG_SHIFT)
#define TERM_ATTR_BG_WHITE      TERM_ATTR_BG | (TERM_ATTR_WHITE << TERM_ATTR_BG_SHIFT)
#define TERM_ATTR_INVERSE       0x100
#define TERM_ATTR_BOLD          0x200

#endif