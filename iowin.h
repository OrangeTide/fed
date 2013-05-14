/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Windows-specific IO routines.
 */


#ifndef IOWIN_H
#define IOWIN_H


#ifndef TARGET_WIN
   #error This file should only be compiled as part of the Windows target
#endif


#pragma warning (disable: 4018 4244 4761)


#define TN     "Windows"


#include <stdio.h>
#include <time.h>
#include <direct.h>
#include <io.h>


#define _USE_LFN        1


#define TL_CHAR         0x250C
#define TOP_CHAR        0x2500
#define TR_CHAR         0x2510
#define SIDE_CHAR       0x2502
#define BL_CHAR         0x2514
#define BOTTOM_CHAR     0x2500
#define BR_CHAR         0x2518
#define LJOIN_CHAR      0x251C
#define RJOIN_CHAR      0x2524
#define BJOIN_CHAR      0x2534
#define UP_CHAR         0x25B2
#define DOWN_CHAR       0x25BC
#define BAR_CHAR        0x2592


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

extern volatile int winched;


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
#define CLOCKS_PER_SEC      1000


#define FMODE_READ          1
#define FMODE_WRITE         2

typedef FILE MYFILE;

int file_size(char *p);
int file_exists(char *p);
int get_char(MYFILE *f);
int peek_char(MYFILE *f);
void put_char(MYFILE *f, int c);

void usleep(int msec);

void myprintf(const char *msg, ...);
int mysystem(char *cmd);

#define printf          myprintf
#define system          mysystem
#define delete_file(p)  unlink(p)

#define W_OK            2

#define getdisk()       _getdrive()
#define setdisk(d)      _chdrive(d)

int _main(int argc, char *argv[]);

#define main            _main



#endif          /* IOWIN_H */
