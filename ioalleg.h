/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      IO routines using the Allegro library.
 */


#ifndef IOALLEGRO_H
#define IOALLEGRO_H


#ifndef TARGET_ALLEG
   #error This file should only be compiled as part of the Allegro target
#endif


#define TN     "Allegro"


#include <stdio.h>
#include <unistd.h>
#include <allegro.h>


#define _USE_LFN        1


#define TL_CHAR         0xDA
#define TOP_CHAR        0xC4
#define TR_CHAR         0xBF
#define SIDE_CHAR       0xB3
#define BL_CHAR         0xC0
#define BOTTOM_CHAR     0xC4
#define BR_CHAR         0xD9
#define LJOIN_CHAR      0xC3
#define RJOIN_CHAR      0xB4
#define BJOIN_CHAR      0xC1
#define UP_CHAR         '+'
#define DOWN_CHAR       '+'
#define BAR_CHAR        0xB2


extern int screen_w;
extern int screen_h;
extern int x_pos, y_pos;
extern int attrib;
extern int norm_attrib;


#define ascii(c)        ((c) & 0x00ff) 
#define delay(t)        rest(t) 


void term_suspend(int newline);
void refresh_screen();
int gch();
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


#define refresh_screen()


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


#define FMODE_READ          1
#define FMODE_WRITE         2

typedef FILE MYFILE;

int get_char(MYFILE *f);
int peek_char(MYFILE *f);
void put_char(MYFILE *f, int c);

#define delete_file(p)      unlink(p)
#define file_exists(p)      exists(p)


#endif          /* IOALLEGRO_H */
