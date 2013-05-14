/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Curses-specific IO routines.
 */


#ifndef IOCURSES_H
#define IOCURSES_H


#ifndef TARGET_CURSES
   #error This file should only be compiled as part of the Curses target
#endif


#define TN     "curses"


#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <curses.h>


#define _USE_LFN        1


#define TL_CHAR         '-'
#define TOP_CHAR        '-'
#define TR_CHAR         '-'
#define SIDE_CHAR       '|'
#define BL_CHAR         '-'
#define BOTTOM_CHAR     '-'
#define BR_CHAR         '-'
#define LJOIN_CHAR      '|'
#define RJOIN_CHAR      '|'
#define BJOIN_CHAR      '-'
#define UP_CHAR         '+'
#define DOWN_CHAR       '+'
#define BAR_CHAR        '#'


extern int screen_w;
extern int screen_h;
extern int x_pos, y_pos;
extern int attrib;
extern int norm_attrib;
extern int nosleep;


#define ascii(c)        ((c) & 0x00ff) 
#define delay(t)        mydelay(t) 


void term_suspend(int newline);
void refresh_screen();
int gch();
int keypressed();
int modifiers();
void print(int c);
int printer_ready();
void cls();
void home();
void newline();
void goto1(int x, int y);
void goto2(int x, int y);
void backspace();
void linefeed();
void cr();
void tattr(int col);
void hide_c();
void show_c();
void show_c2(int ovr);
void mydelay(int t);

extern int winched;


#define ctrl_pressed()  (modifiers() & KF_CTRL)
#define alt_pressed()   (modifiers() & KF_ALT)


#define sel_vid()       tattr(config.sel_col)
#define fold_vid()      tattr(config.fold_col)
#define hi_vid()        tattr(config.hi_col)
#define n_vid()         tattr(config.norm_col)
#define keyword_vid()   tattr(config.keyword_col)
#define comment_vid()   tattr(config.comment_col)
#define string_vid()    tattr(config.string_col)
#define number_vid()    tattr(config.number_col)
#define symbol_vid()    tattr(config.symbol_col)
#define sf_vid()        tattr(((config.sel_col) & 0x0f) |   \
			      ((config.fold_col) & 0xf0))


clock_t myclock();

#define clock()         myclock()

#undef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC  CLK_TCK


#define FMODE_READ          1
#define FMODE_WRITE         2

typedef FILE MYFILE;

int file_size(char *p);
int file_exists(char *p);
int get_char(MYFILE *f);
int peek_char(MYFILE *f);
void put_char(MYFILE *f, int c);

#define delete_file(p)      unlink(p)


#endif          /* IOCURSES_H */
