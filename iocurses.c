/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Curses-specific IO routines (Linux).
 */


#ifndef TARGET_CURSES
   #error This file should only be compiled as part of the Curses target
#endif


#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/ioctl.h>
#include <sys/kd.h>

#include "fed.h"


int screen_w = 80;
int screen_h = 25; 

int term_inited = FALSE;

int dirty = FALSE;

int x_pos = 0, y_pos = 0;
int _x_pos = 0, _y_pos = 0;

int got_color = FALSE;

int attrib = 7;
int norm_attrib = 7;
int cattrib = 0;

int m_x = -1;
int m_y = -1;
int m_b = 0;

int _mouse_x = -1;
int _mouse_y = -1;
int _mouse_b = 0;

int nosleep = FALSE;

#define MAX_ESCAPE   16

int esc_waiting = 0;
char esc_playback[MAX_ESCAPE];

int esc_count = 0;
char esc_buf[MAX_ESCAPE+1];

int modifier_cache = 0;

char orig_title[256] = "";
char my_title[256] = "";

int winched = 0;



void curs_init(int r)
{
   int f, b, i;

   initscr();
   nocbreak();
   noecho();
   raw();
   nonl();
   keypad(stdscr, FALSE);
   meta(stdscr, TRUE);
   nodelay(stdscr, TRUE);

   if (has_colors()) {
      start_color();

      for (i=1; i<COLOR_PAIRS; i++) {
	 f = (i-1)%COLORS;
	 b = (i-1)/COLORS;

	 /* Linux console pallete isn't the same as in DOS */
	 if (f == 1)
	    f = 4;
	 else if (f == 4)
	    f = 1;
	 else if (f == 3)
	    f = 6;
	 else if (f == 6)
	    f = 3;
	 else
	    f = f;

	 if (b == 1)
	    b = 4;
	 else if (b == 4)
	    b = 1;
	 else if (b == 3)
	    b = 6;
	 else if (b == 6)
	    b = 3;
	 else
	    b = b;

	 init_pair(i, f%COLORS, b%COLORS);
      }

      got_color = TRUE;
   }
   else
      got_color = FALSE;

   if (r)
      refresh();

   screen_w = COLS;
   screen_h = LINES;
}



void refresh_screen()
{
   if (winched == 2) {
      disp_exit();
      endwin();
      curs_init(TRUE);
      disp_init();

      do {
      } while (getch() != ERR);

      winched = 1;
   }

   if (dirty) {
      move(_y_pos, _x_pos);
      refresh();
      dirty = FALSE;
   }
}



void sig_winch()
{
   winched = 2;
}



void term_init(int screenheight)
{
   int c, i;
   char *p;

   if (term_inited)
      return;

   curs_init(screenheight > 0);

   if ((getenv("COLORTERM")) && (!orig_title[0])) {
      printf("\e[?35l\e[?1000h\e[21t");
      fflush(stdout);

      i = 0;

      for (;;) {
	 do {
	    c = getch();
	 } while (c == ERR);

	 orig_title[i] = c;

	 if ((i > 0) && (orig_title[i] == '\\') && (orig_title[i-1] == '\e'))
	    break;

	 i++;
      }

      orig_title[i-1] = 0;

      p = strstr(orig_title, "\e]l");

      if (p) {
	 p += 3;

	 memmove(orig_title, p, strlen(p)+1);

	 for (i=0; orig_title[i]; i++)
	    if (orig_title[i] < ' ')
	       orig_title[i] = ' ';
      }
      else
	 orig_title[0] = 0;

   }

   esc_waiting = 0;
   esc_count = 0;
   modifier_cache = 0;

   signal(SIGWINCH, sig_winch);

   term_inited = TRUE;
   dirty = TRUE;

   errno = 0;
}



void term_exit()
{
   if (orig_title[0]) {
      printf("\e[?35h\e[?1000l\e]0;%s\007", orig_title);
      fflush(stdout);
   }

   goto2(0, screen_h-1);
   cr_scroll();
   cr_scroll();

   clear_keybuf();

   echo();
   cbreak();
   endwin();

   signal(SIGWINCH, SIG_DFL);

   term_inited = FALSE;
}



void term_suspend(int newline)
{
   if (orig_title[0]) {
      printf("\e[?35h\e[?1000l\e]0;%s\007", orig_title);
      fflush(stdout);
   }

   goto2(0, screen_h-1);

   if (newline)
      cr_scroll();

   clear_keybuf();

   echo();
   cbreak();
   endwin();

   term_inited = FALSE;
}



void term_reinit(int wait) 
{
   term_init(-1);

   if (orig_title[0]) {
      printf("\e[?35l\e[?1000h");
      fflush(stdout);
   }

   if (wait) {
      term_inited = FALSE;
      clear_keybuf();
      gch();
      term_inited = TRUE;
   }

   refresh();

   screen_w = COLS;
   screen_h = LINES;

   if (orig_title[0]) {
      printf("%s", my_title);
      fflush(stdout);
   }
}



static int get_modifiers(int *err)
{
   int arg = 6;
   int mod = 0;

   if (ioctl(fileno(stdin), TIOCLINUX, &arg) != -1) {
      if (arg & 1)
	 mod |= KF_SHIFT;

      if (arg & 4)
	 mod |= KF_CTRL;

      if (arg & 10)
	 mod |= KF_ALT;

      if (err)
	 *err = 0;
   }
   else {
      if (err)
	 *err = 1;
   }

   errno = 0;

   return mod;
}



typedef struct KEYINFO
{
   char *string[4];
   int code[4];
} KEYINFO;



static KEYINFO keyinfo[] =
{
   /*  norm      shift     ctrl      c+s          norm   shift  ctrl   c+s   */
   {{ "\001",   "\001",   "\001",   "\001"   }, { 7681,  7681,  7681,  7681  }},   /* ctrl+A */
   {{ "\002",   "\002",   "\002",   "\002"   }, { 12290, 12290, 12290, 12290 }},   /* ctrl+B */
   {{ "\003",   "\003",   "\003",   "\003"   }, { 11779, 11779, 11779, 11779 }},   /* ctrl+C */
   {{ "\004",   "\004",   "\004",   "\004"   }, { 8196,  8196,  8196,  8196  }},   /* ctrl+D */
   {{ "\005",   "\005",   "\005",   "\005"   }, { 4613,  4613,  4613,  4613  }},   /* ctrl+E */
   {{ "\006",   "\006",   "\006",   "\006"   }, { 8454,  8454,  8454,  8454  }},   /* ctrl+F */
   {{ "\007",   "\007",   "\007",   "\007"   }, { 8711,  8711,  8711,  8711  }},   /* ctrl+G */
   {{ "\013",   "\013",   "\013",   "\013"   }, { 9483,  9483,  9483,  9483  }},   /* ctrl+K */
   {{ "\014",   "\014",   "\014",   "\014"   }, { 9740,  9740,  9740,  9740  }},   /* ctrl+L */
   {{ "\016",   "\016",   "\016",   "\016"   }, { 12558, 12558, 12558, 12558 }},   /* ctrl+N */
   {{ "\017",   "\017",   "\017",   "\017"   }, { 6159,  6159,  6159,  6159  }},   /* ctrl+O */
   {{ "\020",   "\020",   "\020",   "\020"   }, { 12813, 12813, 12813, 12813 }},   /* ctrl+P */
   {{ "\021",   "\021",   "\021",   "\021"   }, { 4113,  4113,  4113,  4113  }},   /* ctrl+Q */
   {{ "\022",   "\022",   "\022",   "\022"   }, { 4882,  4882,  4882,  4882  }},   /* ctrl+R */
   {{ "\023",   "\023",   "\023",   "\023"   }, { 7955,  7955,  7955,  7955  }},   /* ctrl+S */
   {{ "\024",   "\024",   "\024",   "\024"   }, { 5140,  5140,  5140,  5140  }},   /* ctrl+T */
   {{ "\025",   "\025",   "\025",   "\025"   }, { 5653,  5653,  5653,  5653  }},   /* ctrl+U */
   {{ "\026",   "\026",   "\026",   "\026"   }, { 12054, 12054, 12054, 12054 }},   /* ctrl+V */
   {{ "\027",   "\027",   "\027",   "\027"   }, { 4375,  4375,  4375,  4375  }},   /* ctrl+W */
   {{ "\030",   "\030",   "\030",   "\030"   }, { 11544, 11544, 11544, 11544 }},   /* ctrl+X */
   {{ "\031",   "\031",   "\031",   "\031"   }, { 5401 , 5401 , 5401 , 5401  }},   /* ctrl+Y */
   {{ "\032",   "\032",   "\032",   "\032"   }, { 11290, 11290, 11290, 11290 }},   /* ctrl+Z */
   {{ "\x7F",   "\010",   "\x1F",   "\x1F"   }, { 3592,  3592,  3711,  3711  }},   /* backspace */
   {{ "\e\010", "\e\010", "\e\010", "\e\010" }, { 3711,  3711,  3711,  3711  }},   /* alt+backspace */
   {{ "\x1C",   "\x1C",   "\x1C",   "\x1C"   }, { 3711,  3711,  3711,  3711  }},   /* ctrl+\ */
   {{ "\011",   "\e[Z",   "\011",   "\011"   }, { 3849,  3849,  3849,  3849  }},   /* tab */
   {{ "\e[A",   "\e[a",   "\eOa",   "\eOA"   }, { 18432, 18432, 18989, 18989 }},   /* up */
   {{ "\e[B",   "\e[b",   "\eOb",   "\eOB"   }, { 20480, 20480, 20011, 20011 }},   /* down */
   {{ "\e[C",   "\e[c",   "\eOc",   "\eOC"   }, { 19712, 19712, 29696, 29696 }},   /* right */
   {{ "\e[D",   "\e[d",   "\eOd",   "\eOD"   }, { 19200, 19200, 29440, 29440 }},   /* left */
   {{ "\eOx",   "\eOx",   "\eOx",   "\eOx"   }, { 18432, 18432, 18989, 18989 }},   /* pad up */
   {{ "\eOw",   "\eOw",   "\eOw",   "\eOw"   }, { 20480, 20480, 20011, 20011 }},   /* pad down */
   {{ "\eOv",   "\eOv",   "\eOv",   "\eOv"   }, { 19712, 19712, 29696, 29696 }},   /* pad right */
   {{ "\eOt",   "\eOt",   "\eOt",   "\eOt"   }, { 19200, 19200, 29440, 29440 }},   /* pad left */
   {{ "\e[3~",  "\e[3$",  "\e[3^",  "\e[3@"  }, { 21248, 21248, 21248, 21248 }},   /* delete */
   {{ "\eOn",   "\eOn",   "\eOn",   "\eOn"   }, { 21248, 21248, 21248, 21248 }},   /* pad  delete */
   {{ "\e[2~",  "\e[2$",  "\e[2^",  "\e[2@"  }, { 20992, 20992, 5897,  5897  }},   /* insert */
   {{ "\e[1~",  "\e[1$",  "\e[1^",  "\e[1@"  }, { 18176, 18176, 30464, 30464 }},   /* home */
   {{ "\e[4~",  "\e[4$",  "\e[4^",  "\e[4@"  }, { 20224, 20224, 29952, 29952 }},   /* end */
   {{ "\e[5~",  "\e[5$",  "\e[5^",  "\e[5@"  }, { 18688, 18688, 18688, 18688 }},   /* pgup */
   {{ "\e[6~",  "\e[6$",  "\e[6^",  "\e[6@"  }, { 20736, 20736, 20736, 20736 }},   /* pgdn */
   {{ "\n",     "\eOM",   "\e\n",   "\e\n"   }, { 7181,  7181,  7178,  7178  }},   /* enter */
   {{ "\x1D",   "\x1D",   "\x1D",   "\e]"    }, { 6683,  6683,  6683,  6683  }},   /* ctrl+] */
   {{ "\e[11~", "\e[23~", "\e[11^", "\e[23^" }, { 15104, 15104, 24064, 24064 }},   /* F1 */
   {{ "\e[12~", "\e[24~", "\e[12^", "\e[24^" }, { 15360, 15360, 24320, 24320 }},   /* F2 */
   {{ "\e[13~", "\e[25~", "\e[13^", "\e[25^" }, { 15616, 15616, 24576, 24576 }},   /* F3 */
   {{ "\e[14~", "\e[26~", "\e[14^", "\e[26^" }, { 15872, 15872, 24832, 24832 }},   /* F4 */
   {{ "\e[15~", "\e[28~", "\e[15^", "\e[28^" }, { 16128, 16128, 25088, 25088 }},   /* F5 */
   {{ "\e[17~", "\e[29~", "\e[17^", "\e[29^" }, { 16384, 16384, 25344, 25344 }},   /* F6 */
   {{ "\e[18~", "\e[31~", "\e[18^", "\e[31^" }, { 16640, 16640, 25600, 25600 }},   /* F7 */
   {{ "\e[19~", "\e[32~", "\e[19^", "\e[32^" }, { 16896, 16896, 25856, 25856 }},   /* F8 */
   {{ "\e[20~", "\e[33~", "\e[20^", "\e[33^" }, { 17152, 17152, 26112, 26112 }},   /* F9 */
   {{ "\e[21~", "\e[34~", "\e[21^", "\e[34^" }, { 17408, 17408, 26368, 26368 }},   /* F10 */
   {{ "\e[[A",  "\e[[A",  "\e[[A",  "\e[[A"  }, { 15104, 15104, 24064, 24064 }},   /* F1 type 2 */
   {{ "\e[[B",  "\e[[B",  "\e[[B",  "\e[[B"  }, { 15360, 15360, 24320, 24320 }},   /* F2 type 2 */
   {{ "\e[[C",  "\e[[C",  "\e[[C",  "\e[[C"  }, { 15616, 15616, 24576, 24576 }},   /* F3 type 2 */
   {{ "\e[[D",  "\e[[D",  "\e[[D",  "\e[[D"  }, { 15872, 15872, 24832, 24832 }},   /* F4 type 2 */
   {{ "\e[[E",  "\e[[E",  "\e[[E",  "\e[[E"  }, { 16128, 16128, 25088, 25088 }},   /* F5 type 2 */
};



int match_escape(int *c)
{
   int i, j;
   int err;

   esc_buf[esc_count] = 0;

   for (i=0; i<(int)(sizeof(keyinfo)/sizeof(KEYINFO)); i++) {
      for (j=0; j<4; j++) {
	 if (strcmp(esc_buf, keyinfo[i].string[j]) == 0) {
	    *c = keyinfo[i].code[j];

	    switch (j) {

	       case 0:
		  /* normal */
		  modifier_cache = get_modifiers(&err);

		  if (!err) {
		     if ((modifier_cache & (KF_SHIFT | KF_CTRL)) == (KF_SHIFT | KF_CTRL)) {
			*c = keyinfo[i].code[3];
			modifier_cache = 0;
		     }
		     else if (modifier_cache & KF_CTRL) {
			if ((*c == 18688) || (*c == 20736)) {
			   /* special bodge to turn ctrl+pgup into shift */
			   modifier_cache = KF_SHIFT;
			}
			else {
			   *c = keyinfo[i].code[2];
			   modifier_cache = 0;
			}
		     }
		     else if (modifier_cache & KF_SHIFT) {
			*c = keyinfo[i].code[1];
			modifier_cache = 0;
		     }
		  }
		  break;

	       case 1:
		  /* shift */
		  modifier_cache = KF_SHIFT;
		  break;

	       case 2:
		  /* ctrl */
		  modifier_cache = KF_CTRL;
		  break;

	       case 3:
		  /* ctrl+shift */
		  modifier_cache = KF_SHIFT | KF_CTRL;
		  break;
	    }

	    return TRUE;
	 }
      }
   }

   return FALSE;
}



int gch()
{
   clock_t t;
   int c;

   if (esc_waiting) {
      esc_waiting--;
      return esc_playback[esc_waiting];
   }

   if (term_inited)
      refresh_screen();

   modifier_cache = 0;

   esc_count = 0;

   t = clock();

   while ((clock() < t+CLOCKS_PER_SEC/10) || (esc_count <= 0)) {
      c = getch();

      if ((c) && (c != ERR)) {
	 if ((esc_count) && (c == '\e')) {
	    ungetch(c);
	    break;
	 }

	 esc_buf[esc_count++] = c;

	 if (match_escape(&c)) {
	    esc_waiting = 0;
	    return c;
	 }

	 if ((esc_count == 6) && (memcmp(esc_buf, "\e[M", 3) == 0)) {
	    _mouse_x = esc_buf[4] - ' ' - 1;
	    _mouse_y = esc_buf[5] - ' ' - 1;

	    switch (esc_buf[3] - ' ') {
	       case 0: _mouse_b = 1; break;
	       case 1: _mouse_b = 4; break;
	       case 2: _mouse_b = 2; break;
	    } 

	    esc_waiting = 0;
	    return 0;
	 }

	 if (esc_count >= MAX_ESCAPE)
	    break;

	 if ((esc_count == 1) && (esc_buf[0] != '\e'))
	    break;

	 t = clock();
      }
      else if (!nosleep)
	 usleep(10);
   }

   esc_waiting = esc_count-1;

   for (c=0; c<esc_waiting; c++)
      esc_playback[c] = esc_buf[esc_waiting-c];

   if (esc_buf[0] == '\e') {
      if (!esc_waiting) {
	 modifier_cache = KF_ALT;
	 return 0;
      }
      else
	 return '^';
   }

   return esc_buf[0];
}



int keypressed()
{
   int c;

   if (esc_waiting)
      return TRUE;

   if (term_inited)
      refresh_screen();

   do {
      c = getch();
   } while (!c);

   if (c == ERR) {
      if (!nosleep)
	 usleep(10);
      return FALSE;
   }

   ungetch(c);

   return TRUE;
}



int modifiers()
{
   if (modifier_cache)
      return modifier_cache;
   else
      return get_modifiers(NULL);
}



void pch(unsigned char c)
{
   int ch;

   if ((x_pos >= 0) && (x_pos < screen_w)) {
      if ((c < 0x20) || ((c >= 0x80) && (c <= 0xA0)))
	 ch = '^' | cattrib;
      else
	 ch = c | cattrib;

      mvaddch(y_pos, x_pos, ch);

      dirty = TRUE;
   }

   x_pos++;
}



void mywrite(unsigned char *s)
{
   while ((*s) && (x_pos < screen_w)) {
      if (x_pos >= 0) {
	 if (*s < ' ')
	    mvaddch(y_pos, x_pos, '^' | cattrib);
	 else
	    mvaddch(y_pos, x_pos, *s | cattrib);

	 dirty = TRUE;
      }

      s++;
      x_pos++;
   }

   while (*s) {
      s++;
      x_pos++;
   }
}



void del_to_eol()
{
   if (x_pos < screen_w) {
      move(y_pos, MAX(x_pos, 0));

      while (x_pos < screen_w) {
	 addch(' ' | cattrib);
	 x_pos++;
      }

      dirty = TRUE;
   }
}



void cr_scroll()
{
   if (y_pos < screen_h-1) {
      y_pos++;
   }
   else {
      move(0, 0);
      deleteln();
   }

   x_pos = 0;

   _x_pos = x_pos;
   _y_pos = y_pos;

   dirty = TRUE;

   refresh_screen();
}



void cls()
{
   clear();
   goto1(0, 0);

   dirty = TRUE;
}



void home()
{
   x_pos = 0;
   y_pos = 0;
}



void newline()
{
   if (y_pos < screen_h-1) 
      y_pos++;

   x_pos = 0;
}



void goto1(int x, int y)
{
   x_pos = x;
   y_pos = y;
}



void goto2(int x, int y)
{
   x_pos = _x_pos = x;
   y_pos = _y_pos = y;

   dirty = TRUE;
}



void backspace()
{
   if (x_pos > 0) { 
      x_pos--; 
      pch(' '); 
      x_pos--; 
   } 
}



void linefeed()
{
   if (y_pos < screen_h-1) 
      y_pos++;
}



void cr()
{
   x_pos = 0;
}



void tattr(int col)
{
   int f, b, c;

   attrib = col;

   cattrib = 0;

   if (got_color) {
      f = col&15;
      b = col>>4;
      c = (((f&7) + (b&7)*COLORS) % (COLOR_PAIRS-1)) + 1;

      cattrib |= COLOR_PAIR(c);

      if (f > 7)
	 cattrib |= A_BOLD;

      if (b > 7)
	 cattrib |= A_BLINK;
   }
   else {
      if ((col&0xF0) != (config.norm_col&0xF0))
	 cattrib |= A_REVERSE;

      if ((col&0x0F) != (config.norm_col&0x0F))
	 cattrib |= A_BOLD;
   }
}



void hide_c()
{
   curs_set(0);
}



void show_c()
{
   curs_set((config.big_cursor) ? 2 : 1);
}



void show_c2(int ovr)
{
   curs_set(((ovr) ? !config.big_cursor : config.big_cursor) ? 2 : 1);
}



void print(int c)
{
}



int printer_ready()
{
   return FALSE;
}



char *save_screen(int x, int y, int w, int h)
{
   chtype *data;
   int _x, _y;

   w++;
   h++;

   data = malloc(sizeof(chtype)*w*h);
   if (!data)
      return NULL;

   for (_y=0; _y<h; _y++) {
      for (_x=0; _x<w; _x++) {
	 move(y+_y, x+_x);
	 data[_y*w+_x] = inch();
      }
   }

   return (char *)data;
}



void restore_screen(int x, int y, int w, int h, char *buf)
{
   chtype *data = (chtype *)buf;
   int _x, _y;

   w++;
   h++;

   if (buf) {
      for (_y=0; _y<h; _y++) {
	 for (_x=0; _x<w; _x++) {
	    move(y+_y, x+_x);
	    addch(data[_y*w+_x]);
	 }
      }

      dirty = TRUE;

      free(buf);
   }
   else
      dirty_everything();
}



void clear_keybuf()
{
   while (keypressed())
      gch();
}



MYFILE *open_file(char *name, int mode)
{
   FILE *f;

   if (mode == FMODE_READ)
      f = fopen(name, "rb");
   else if (mode == FMODE_WRITE)
      f = fopen(name, "wb");
   else
      f = NULL;

   if (!f) {
      if (!errno)
	 errno = ENMFILE;
   }
   else
      errno = 0;

   return f;
}



int close_file(MYFILE *f)
{
   if (f) {
      fclose(f);
      return errno;
   }

   return 0;
}



int get_char(MYFILE *f)
{
   int c = fgetc(f);

   if (c == EOF)
      errno = EOF;

   return c;
}



int peek_char(MYFILE *f)
{
   int c;

   c = fgetc(f);
   ungetc(c, f);

   if (c == EOF)
      errno = EOF;

   return c;
}



void put_char(MYFILE *f, int c)
{
   fputc(c, f);
}



int file_exists(char *p)
{
   glob_t g;
   char *s;
   int i;

   for (i=0; p[i]; i++) {
      if ((p[i] == '*') || (p[i] == '?')) {
	 if (glob(p, GLOB_MARK | GLOB_NOESCAPE, NULL, &g) != 0)
	    return FALSE;

	 for (i=0; i<g.gl_pathc; i++) {
	    s = g.gl_pathv[i];

	    if ((s[0]) && (s[strlen(s)-1] != '/')) {
	       globfree(&g);
	       return TRUE;
	    }
	 }

	 globfree(&g);
	 return FALSE;
      }
   }

   return (file_size(p) >= 0);
}



int file_size(char *p)
{
   struct stat entry;

   if (stat(p, &entry) != 0)
      return -1;

   if (!(entry.st_mode & S_IFREG))
      return -1;

   return entry.st_size;
}



long file_time(char *p)
{
   struct stat entry;

   if (stat(p, &entry) != 0)
      return 0;

   if (!(entry.st_mode & S_IFREG))
      return 0;

   return entry.st_mtime;
}



int search_path(char *result, char *prog, char *exts, char *var)
{
   /* search for a file on the disk, searching PATH if needed */

   char *p;
   int pos;
   strcpy(result,prog);

   if (find_program(result,exts)) {       /* look in current directory */
      return TRUE;
   }

   p=getenv(var);
   if (p!=NULL) {
      while(*p) {
	 while ((*p==' ') || (*p==',') || (*p==';') || (*p==':'))
	    p++;
	 if(*p) {
	    pos=0;
	    result[0]=0;
	    while ((*p) && (*p!=' ') && (*p!=',') && (*p!=';') && (*p!=':')) {
	       result[pos++]=*(p++);
	       result[pos]=0;
	    }
	    append_backslash(result);
	    strcat(result,prog);
	    if (find_program(result,exts))
	       return TRUE;
	 }
      }
   }

   return FALSE;
}



int find_program(char *name, char *ext)
{
   /* look for a file, checking the possible extensions */

   char *p;
   int pos;

   if ((ext) && (*ext) && (*(find_extension(name))==0)) {
      p = ext;
      while(*p) {
	 while ((*p==' ') || (*p==',') || (*p==';'))
	    p++;
	 if(*p) {
	    pos=strlen(name);
	    name[pos++]='.';
	    name[pos]=0;
	    while ((*p) && (*p!=' ') && (*p!=',') && (*p!=';')) {
	       name[pos++]=*(p++);
	       name[pos]=0;
	    }
	    if (file_exists(name))
	       return TRUE;
	    remove_extension(name);
	 }
      }
   }
   else
      return file_exists(name);

   return FALSE;
}



int do_for_each_file(char *name, int (*call_back)(char *, int), int param)
{
   char buf[1024];
   glob_t g;
   int i, ret;
   int matched = FALSE;

   ret = glob(name, GLOB_MARK | GLOB_NOESCAPE, NULL, &g);

   if (ret == 0) {
      errno = 0;

      for (i=0; i<g.gl_pathc; i++) {
	 strcpy(buf, g.gl_pathv[i]);

	 if ((buf[0]) && (buf[strlen(buf)-1] != '/')) {
	    matched = TRUE;

	    (*call_back)(buf, param);

	    if (errno)
	       break;
	 }
      }

      globfree(&g);
   }

   if ((ret == GLOB_NOMATCH) || (!matched)) {
      for (i=0; name[i]; i++) {
	 if ((name[i]=='*') || (name[i]=='?') || (name[i]=='[')) {
	    errno = ENMFILE;
	    return errno;
	 }
      }

      errno = 0;
      (*call_back)(name, param);
      return errno;
   }

   errno = 0;
   return 0;
}



int do_for_each_directory(char *name, int (*call_back)(char *, int), int param)
{
   char buf[1024];
   DIR *dir;
   glob_t g;
   int i;

   strcpy(buf, name);
   buf[strlen(buf)-1] = 0;

   dir = opendir(buf);

   if (!dir) {
      errno = ENMFILE;
      return errno;
   }

   closedir(dir);

   strcat(buf, "../");
   (*call_back)(buf, param);

   if (glob(name, GLOB_MARK | GLOB_NOESCAPE, NULL, &g) == 0) {
      errno = 0;

      if (!errno) {
	 for (i=0; i<g.gl_pathc; i++) {
	    strcpy(buf, g.gl_pathv[i]);

	    if ((buf[0]) && (buf[strlen(buf)-1] == '/')) {
	       (*call_back)(buf, param);

	       if (errno)
		  break;
	    }
	 }
      }

      globfree(&g);
   }

   errno = 0;
   return 0;
}



void windows_init()
{
}



int set_window_title(char *title)
{
   if (orig_title[0]) {
      char *t = getenv("TERMID");

      if (!t)
	 t = "";

      sprintf(my_title, "\e]0;%sFED - %s\007", t, title);
      printf("%s", my_title);
      fflush(stdout);
   }

   return 0;
}



int got_clipboard()
{
   return TRUE;
}



char *clipboard_file()
{
   static char buf[1024] = "";
   char *p;

   if (!buf[0]) {
      p = getenv("HOME");

      if (p) {
	 strcpy(buf, p);
	 append_backslash(buf);
      }
      else
	 buf[0] = 0;

      strcat(buf, CLIP_FILE);
   }

   return buf;
}



int got_clipboard_data()
{
   return (access(clipboard_file(), R_OK) == 0);
}



int set_clipboard_data(char *data, int size)
{
   FILE *f = fopen(clipboard_file(), "w");

   if (!f)
      return FALSE;

   fwrite(data, 1, size, f);
   fclose(f);

   return TRUE;
}



char *get_clipboard_data(int *size)
{
   FILE *f;
   char *p;

   *size = file_size(clipboard_file());
   if (*size <= 0) 
      return NULL;

   p = malloc(*size);
   if (!p)
      return NULL;

   f = fopen(clipboard_file(), "r");
   if (!f) {
      free(p);
      return NULL;
   }

   fread(p, 1, *size, f);
   fclose(f);

   return p;
}



void mydelay(int t)
{
   int c = clock() + t*(float)CLOCKS_PER_SEC/1000.0;

   refresh_screen();

   do {
      usleep(10);
   } while (clock() < c);
}



clock_t myclock()
{
   static clock_t first = 0;

   struct tms buf;

   clock_t t = times(&buf);

   if (!first) {
      first = t;
      return 0;
   }
   else {
      t -= first;
      return (double)t * (double)CLOCKS_PER_SEC / (double)CLK_TCK;
   }
}



void mouse_init()
{
}



void display_mouse()
{
   refresh_screen();
}



void hide_mouse()
{
}



int mouse_changed(int *x, int *y)
{
   if (x)
      *x = _mouse_x;

   if (y)
      *y = _mouse_y;

   return ((_mouse_x != m_x) || (_mouse_y != m_y) || 
	   (_mouse_b != m_b) || (_mouse_b));
}



int poll_mouse()
{
   static int _x = -1, _y = -1;
   static clock_t c = 0;
   int ret;

   if (_mouse_b) {
      m_b = _mouse_b;
      _mouse_b = 0;
      c = clock();
   }
   else if (clock() > c+CLOCKS_PER_SEC/10) {
      m_b = 0;
   }

   ret = ((_mouse_x != _x) || (_mouse_y != _y) || (m_b));

   if (recording_macro()) {
      if (ret) {
	 strcpy(message, "Can't record mouse actions in a macro");
	 display_message(0);
      }

      m_b = 0;

      _x = _mouse_x;
      _y = _mouse_y;

      return FALSE;
   }

   m_x = _x = _mouse_x;
   m_y = _y = _mouse_y;

   return ret;
}



void set_mouse_pos(int x, int y)
{
}



void set_mouse_height(int h)
{
}



int mouse_dclick(int mode)
{
   int ox, oy;
   int start = clock();

   do {
      ox = m_x;
      oy = m_y;

      clear_keybuf();

      poll_mouse();

      if (!mode)
	 if ((ox != m_x) || (oy != m_y))
	    return TRUE;

      if (m_b & 1) {
	 if (mode) 
	    return TRUE;
      }
      else
	 if (!mode)
	    return FALSE;

   } while ((clock() - start) < CLOCKS_PER_SEC / 4);

   return !mode;
}

