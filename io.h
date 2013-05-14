/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Include wrapper for the system-specific IO routines.
 */


#ifndef IO_H
#define IO_H


#ifdef TARGET_DJGPP
   #include "iodjgpp.h"
#else
#ifdef TARGET_CURSES
   #include "iocurses.h"
#else
#ifdef TARGET_ALLEG
   #include "ioalleg.h"
#else
#ifdef TARGET_WIN
   #include "iowin.h"
#else
   #error Unknown compile target
#endif
#endif
#endif
#endif


#define BACKSPACE       8
#define TAB             9
#define LF              10
#define FF              12
#define CR              13
#define ESC             27

#define CTRL_C          11779
#define CTRL_G          8711
#define CTRL_R          4882
#define UP_ARROW        18432
#define DOWN_ARROW      20480
#define LEFT_ARROW      19200
#define RIGHT_ARROW     19712
#define PAGE_UP         18688
#define PAGE_DOWN       20736
#define K_HOME          18176
#define K_END           20224
#define K_DELETE        21248
#define ESC_SCANCODE    283


extern int m_x, m_y, m_b;


void mouse_init();
void display_mouse();
void hide_mouse();
int mouse_changed(int *x, int *y);
int poll_mouse();
void set_mouse_pos(int x, int y);
void set_mouse_height(int h);
int mouse_dclick(int mode);
void term_init(int screenheight);
void term_exit();
void term_reinit(int wait);
void my_setcursor(int shape);

#ifdef TARGET_WIN
void pch(int c);
#else
void pch(unsigned char c);
#endif

void mywrite(unsigned char *s);
void del_to_eol();
void cr_scroll();
void screen_block(int s_s, int s_o, int s_g, int d_s, int d_o, int d_g, int w, int h);
char *save_screen(int x, int y, int w, int h);
void restore_screen(int x, int y, int w, int h, char *buf);
void clear_keybuf();
MYFILE *open_file(char *name, int mode);
int close_file(MYFILE *f);
int refill_buffer(MYFILE *f, int peek_flag);
int flush_buffer(MYFILE *f);
long file_time(char *p);
int search_path(char *result, char *prog, char *exts, char *var);
int find_program(char *name, char *ext);
int do_for_each_file(char *name, int (*call_back)(char *, int), int param);
int do_for_each_directory(char *name, int (*call_back)(char *, int), int param);
void windows_init();
int set_window_title(char *title);
int got_clipboard();
int got_clipboard_data();
int set_clipboard_data(char *data, int size);
char *get_clipboard_data(int *size);


#endif          /* IO_H */
