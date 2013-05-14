/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Windows-specific IO routines.
 */


#ifndef TARGET_WIN
   #error This file should only be compiled as part of the Windows target
#endif


#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>

#define NO_STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>

#include "fed.h"


HWND hwnd = NULL;

HANDLE sleep_object = NULL;

HFONT font = NULL;
HFONT font2 = NULL;

CRITICAL_SECTION crit;

HDC backbuffer_dc = NULL;
HBITMAP backbuffer_bmp = NULL;
HBITMAP prev_backbuffer_bmp = NULL;

int backbuffer_w = 0;
int backbuffer_h = 0;

int font_w = 1;
int font_h = 1;

volatile int window_inited = FALSE;
volatile int window_closed = FALSE;

int focused = FALSE;

char window_title[1024] = "FED";

int screen_w = 80;
int screen_h = 40; 

int new_screen_w = 80;
int new_screen_h = 40;

int got_screen_size = FALSE;

int maximize = FALSE;

int screen_x = 0;
int screen_y = 0;

int got_screen_pos = FALSE;

int term_inited = FALSE;

int dirty = FALSE;

int lock_display = FALSE;

typedef struct SCREEN_CELL
{
   short ch;
   unsigned char col;
} SCREEN_CELL;

SCREEN_CELL **screen;

int x_pos = 0, y_pos = 0;
int _x_pos = 0, _y_pos = 0;

int attrib = 7;
int norm_attrib = 7;

int show_curs = 0;

int m_x = -1;
int m_y = -1;
int m_b = 0;

static volatile int _mouse_x = -256;
static volatile int _mouse_y = -256;
static volatile int _mouse_b = 0;
static volatile int captured = 0;

int mouse_height = 1;

int nosleep = FALSE;

volatile int winched = 0;

#define KEY_BUFFER_SIZE    256

volatile int key_buffer_start = 0;
volatile int key_buffer_end = 0;

typedef struct KEY
{
   int key;
   int mod;
} KEY;

volatile KEY key_buffer[KEY_BUFFER_SIZE];

#define MSG_REDRAW      (WM_USER+0)
#define MSG_SET_TITLE   (WM_USER+1)
#define MSG_CLOSE       (WM_USER+2)
#define MSG_SHOWMOUSE   (WM_USER+3)
#define MSG_HIDEMOUSE   (WM_USER+4)



void refresh_screen()
{
   int x, y;

   EnterCriticalSection(&crit);

   if (winched == 2) {
      disp_exit();

      term_inited = FALSE;

      if (new_screen_h != screen_h) {
	 for (y=new_screen_h; y<screen_h; y++)
	    free(screen[y]);

	 screen = realloc(screen, new_screen_h * sizeof(SCREEN_CELL *));

	 for (y=screen_h; y<new_screen_h; y++) {
	    screen[y] = malloc(new_screen_w * sizeof(SCREEN_CELL));

	    for (x=0; x<new_screen_w; x++) {
	       screen[y][x].ch = ' ';
	       screen[y][x].col = 0;
	    }
	 }
      }

      if (new_screen_w != screen_w) {
	 for (y=0; y<new_screen_h; y++) {
	    screen[y] = realloc(screen[y], new_screen_w * sizeof(SCREEN_CELL));

	    for (x=screen_w; x<new_screen_w; x++) {
	       screen[y][x].ch = ' ';
	       screen[y][x].col = 0;
	    }
	 }
      }

      screen_w = new_screen_w;
      screen_h = new_screen_h;

      term_inited = TRUE;

      disp_init();

      winched = 1;
   }

   LeaveCriticalSection(&crit);

   if (dirty) {
      SendMessage(hwnd, MSG_REDRAW, 0, 0);
      dirty = FALSE;
   }
}



void term_init(int screenheight)
{
   int x, y;

   if (term_inited)
      return;

   EnterCriticalSection(&crit);

   screen = malloc(screen_h * sizeof(SCREEN_CELL *));

   for (y=0; y<screen_h; y++) {
      screen[y] = malloc(screen_w * sizeof(SCREEN_CELL));

      for (x=0; x<screen_w; x++) {
	 screen[y][x].ch = ' ';
	 screen[y][x].col = 0;
      }
   }

   term_inited = TRUE;
   dirty = TRUE;

   errno = 0;

   LeaveCriticalSection(&crit);
}



void term_exit()
{
   int y;

   if (!term_inited)
      return;

   EnterCriticalSection(&crit);

   term_inited = FALSE;

   for (y=0; y<screen_h; y++)
      free(screen[y]);

   free(screen);

   screen = NULL;

   LeaveCriticalSection(&crit);
}



void term_suspend(int newline)
{
}



void term_reinit(int wait) 
{
}



#define VK(k)     (k | 0x8000)



typedef struct KEYINFO
{
   KEY k;
   int c;
} KEYINFO;



static KEYINFO keyinfo[] =
{
   { { 1,             -1      },    7681     },    /* ctrl+A */
   { { 2,             -1      },    12290    },    /* ctrl+B */
   { { 3,             -1      },    11779    },    /* ctrl+C */
   { { 4,             -1      },    8196     },    /* ctrl+D */
   { { 5,             -1      },    4613     },    /* ctrl+E */
   { { 6,             -1      },    8454     },    /* ctrl+F */
   { { 7,             -1      },    8711     },    /* ctrl+G */
   { { 11,            -1      },    9483     },    /* ctrl+K */
   { { 12,            -1      },    9740     },    /* ctrl+L */
   { { 14,            -1      },    12558    },    /* ctrl+N */
   { { 15,            -1      },    6159     },    /* ctrl+O */
   { { 16,            -1      },    12813    },    /* ctrl+P */
   { { 17,            -1      },    4113     },    /* ctrl+Q */
   { { 18,            -1      },    4882     },    /* ctrl+R */
   { { 19,            -1      },    7955     },    /* ctrl+S */
   { { 20,            -1      },    5140     },    /* ctrl+T */
   { { 21,            -1      },    5653     },    /* ctrl+U */
   { { 22,            -1      },    12054    },    /* ctrl+V */
   { { 23,            -1      },    4375     },    /* ctrl+W */
   { { 24,            -1      },    11544    },    /* ctrl+X */
   { { 25,            -1      },    5401     },    /* ctrl+Y */
   { { 26,            -1      },    11290    },    /* ctrl+Z */
   { { 27,            -1      },    283      },    /* esc */
   { { 8,             -1      },    3592     },    /* backspace */
   { { 9,             -1      },    3849     },    /* tab */
   { { 29,            -1      },    6683     },    /* ctrl+] */
   { { 127,           -1      },    3711     },    /* ctrl+backspace */
   { { VK(VK_UP),     KF_CTRL },    18989    },    /* ctrl+up */
   { { VK(VK_DOWN),   KF_CTRL },    20011    },    /* ctrl+down */
   { { VK(VK_RIGHT),  KF_CTRL },    29696    },    /* ctrl+right */
   { { VK(VK_LEFT),   KF_CTRL },    29440    },    /* ctrl+left */
   { { VK(VK_UP),     -1      },    18432    },    /* up */
   { { VK(VK_DOWN),   -1      },    20480    },    /* down */
   { { VK(VK_RIGHT),  -1      },    19712    },    /* right */
   { { VK(VK_LEFT),   -1      },    19200    },    /* left */
   { { VK(VK_DELETE), -1      },    21248    },    /* delete */
   { { VK(VK_INSERT), KF_CTRL },    5897     },    /* ctrl+insert */
   { { VK(VK_INSERT), -1      },    20992    },    /* insert */
   { { VK(VK_HOME),   KF_CTRL },    30464    },    /* ctrl+home */
   { { VK(VK_HOME),   -1      },    18176    },    /* home */
   { { VK(VK_END),    KF_CTRL },    29952    },    /* ctrl+end */
   { { VK(VK_END),    -1      },    20224    },    /* end */
   { { VK(VK_PRIOR),  -1      },    18688    },    /* pgup */
   { { VK(VK_NEXT),   -1      },    20736    },    /* pgdn */
   { { VK(VK_RETURN), KF_CTRL },    7178     },    /* ctrl+enter */
   { { VK(VK_RETURN), -1      },    7181     },    /* enter */
   { { VK(VK_F1),     KF_CTRL },    24064    },    /* ctrl+F1 */
   { { VK(VK_F1),     -1      },    15104    },    /* F1 */
   { { VK(VK_F2),     KF_CTRL },    24320    },    /* ctrl+F2 */
   { { VK(VK_F2),     -1      },    15360    },    /* F2 */
   { { VK(VK_F3),     KF_CTRL },    24576    },    /* ctrl+F3 */
   { { VK(VK_F3),     -1      },    15616    },    /* F3 */
   { { VK(VK_F4),     KF_CTRL },    24832    },    /* ctrl+F4 */
   { { VK(VK_F4),     -1      },    15872    },    /* F4 */
   { { VK(VK_F5),     KF_CTRL },    25088    },    /* ctrl+F5 */
   { { VK(VK_F5),     -1      },    16128    },    /* F5 */
   { { VK(VK_F6),     KF_CTRL },    25344    },    /* ctrl+F6 */
   { { VK(VK_F6),     -1      },    16384    },    /* F6 */
   { { VK(VK_F7),     KF_CTRL },    25600    },    /* ctrl+F7 */
   { { VK(VK_F7),     -1      },    16640    },    /* F7 */
   { { VK(VK_F8),     KF_CTRL },    25856    },    /* ctrl+F8 */
   { { VK(VK_F8),     -1      },    16896    },    /* F8 */
   { { VK(VK_F9),     KF_CTRL },    26112    },    /* ctrl+F9 */
   { { VK(VK_F9),     -1      },    17152    },    /* F9 */
   { { VK(VK_F10),    KF_CTRL },    26368    },    /* ctrl+F10 */
   { { VK(VK_F10),    -1      },    17408    },    /* F10 */
};



static int syskeys[] =
{
   VK_UP,
   VK_DOWN,
   VK_RIGHT,
   VK_LEFT,
   VK_DELETE,
   VK_INSERT,
   VK_HOME,
   VK_END,
   VK_PRIOR,
   VK_NEXT,
   VK_RETURN,
   VK_F1,
   VK_F2,
   VK_F3,
   VK_F4,
   VK_F5,
   VK_F6,
   VK_F7,
   VK_F8,
   VK_F9,
   VK_F10,
};



static int skipkeys[] =
{
   '\r',
   '\n',
};



int gch()
{
   KEY k;
   int i;

   if (term_inited)
      refresh_screen();

   while (key_buffer_start == key_buffer_end) {
      lock_display = FALSE;
      usleep(100);
   }

   k = key_buffer[key_buffer_start];

   if (key_buffer_start < KEY_BUFFER_SIZE-1)
      key_buffer_start++;
   else
      key_buffer_start = 0;

   if (key_buffer_start == key_buffer_end)
      ResetEvent(sleep_object);

   for (i=0; i<(int)(sizeof(keyinfo)/sizeof(KEYINFO)); i++) {
      if ((keyinfo[i].k.key == k.key) &&
	  ((keyinfo[i].k.mod < 0) || (keyinfo[i].k.mod == k.mod)))
	 return keyinfo[i].c;
   }

   return k.key;
}



int keypressed()
{
   if (term_inited)
      refresh_screen();

   if (!auto_input_waiting())
      usleep(10);

   if (key_buffer_start != key_buffer_end)
      return TRUE;

   lock_display = FALSE;
   return FALSE;
}



int modifiers()
{
   int m = 0;

   if (focused) {
      if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	 m |= KF_SHIFT;

      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
	 m |= KF_CTRL;

      if (GetAsyncKeyState(VK_MENU) & 0x8000)
	 m |= KF_ALT;
   }

   return m;
}



int _modifiers()
{
   int m = 0;

   if (GetKeyState(VK_CONTROL) & 0x8000)
      m |= KF_CTRL;

   if (GetKeyState(VK_MENU) & 0x8000)
      m |= KF_ALT;

   return m;
}



void pch(int c)
{
   if ((x_pos >= 0) && (x_pos < screen_w)) {
      screen[y_pos][x_pos].ch = c;
      screen[y_pos][x_pos].col = attrib;

      dirty = TRUE;
   }

   x_pos++;
}



void mywrite(unsigned char *s)
{
   while ((*s) && (x_pos < screen_w)) {
      if (x_pos >= 0) {
	 screen[y_pos][x_pos].ch = *s;
	 screen[y_pos][x_pos].col = attrib;

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
      x_pos = MAX(x_pos, 0);

      while (x_pos<screen_w) {
	 screen[y_pos][x_pos].ch = ' ';
	 screen[y_pos][x_pos].col = attrib;

	 x_pos++;
      }

      dirty = TRUE;
   }
}



void cr_scroll()
{
   int x, y;

   if (y_pos < screen_h-1) {
      y_pos++;
   }
   else {
      for (y=0; y<screen_h-1; y++) {
	 for (x=0; x<screen_w; x++)
	    screen[y][x] = screen[y+1][x];
      }

      for (x=0; x<screen_w; x++) {
	 screen[screen_h-1][x].ch = ' ';
	 screen[screen_h-1][x].col = norm_attrib;
      }
   }

   x_pos = 0;

   _x_pos = x_pos;
   _y_pos = y_pos;

   dirty = TRUE;

   refresh_screen();
}



void cls()
{
   int x, y;

   for (y=0; y<screen_h; y++) {
      for (x=0; x<screen_w; x++) {
	 screen[y][x].ch = ' ';
	 screen[y][x].col = norm_attrib;
      }
   }

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
   y = MID(0, y, screen_h-1);

   x_pos = x;
   y_pos = y;
}



void goto2(int x, int y)
{
   y = MID(0, y, screen_h-1);

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
   attrib = col;
}



void hide_c()
{
   show_curs = 0;

   dirty = TRUE;
}



void show_c()
{
   show_curs = (config.big_cursor) ? 2 : 1;

   dirty = TRUE;
}



void show_c2(int ovr)
{
   show_curs = (((ovr) ? !config.big_cursor : config.big_cursor) ? 2 : 1);

   dirty = TRUE;
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
   SCREEN_CELL *data;
   int _x, _y;

   w++;
   h++;

   if (x < 0) {
      w += x;
      x = 0;
   }

   if (y < 0) {
      h += y;
      y = 0;
   }

   if (x+w > screen_w)
      w = screen_w-x;

   if (y+h > screen_h)
      h = screen_h-y;

   if ((w <= 0) || (h <= 0))
      return NULL;

   data = malloc(sizeof(SCREEN_CELL)*w*h);
   if (!data)
      return NULL;

   for (_y=0; _y<h; _y++)
      for (_x=0; _x<w; _x++)
	 data[_y*w+_x] = screen[y+_y][x+_x];

   return (char *)data;
}



void restore_screen(int x, int y, int w, int h, char *buf)
{
   SCREEN_CELL *data = (SCREEN_CELL *)buf;
   int _x, _y;

   w++;
   h++;

   if (x < 0) {
      w += x;
      x = 0;
   }

   if (y < 0) {
      h += y;
      y = 0;
   }

   if (x+w > screen_w)
      w = screen_w-x;

   if (y+h > screen_h)
      h = screen_h-y;

   if (buf) {
      for (_y=0; _y<h; _y++)
	 for (_x=0; _x<w; _x++)
	    screen[y+_y][x+_x] = data[_y*w+_x];

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



int file_exist_count;



int count_file_exists(char *filename, int param)
{
   file_exist_count++;
   return 0;
}



int file_exists(char *p)
{
   if ((!strchr(p, '*')) && (!strchr(p, '?')))
      return (file_size(p) >= 0);

   file_exist_count = 0;
   do_for_each_file(p, count_file_exists, 0);
   return (file_exist_count > 0);
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
   int c;
   struct _finddata_t dta;
   long handle;

   errno = 0;

   handle = _findfirst(name, &dta);

   if (errno!=0) {
      for (c=0; name[c]; c++)
	 if ((name[c]=='*') || (name[c]=='?'))
	    goto dont_load;

      errno=0;
      (*call_back)(name,param);

      dont_load:
      return errno;
   }

   do {
      if (!(dta.attrib & (_A_HIDDEN | _A_SUBDIR | _A_SYSTEM))) {
	 strcpy(buf,name);
	 strcpy(get_fname(buf),dta.name);

	 (*call_back)(buf,param);
	 if (errno!=0)
	    break;
      }

   } while (_findnext(handle, &dta)==0);

   _findclose(handle);

   errno=0;
   return errno;
}



int do_for_each_directory(char *name, int (*call_back)(char *, int), int param)
{
   char buf[1024];
   struct _finddata_t dta;
   int handle;

   errno = 0;

   handle = _findfirst(name, &dta);

   if (errno!=0)
      return errno;

   do {
      if ((dta.attrib & _A_SUBDIR) && 
	  (strcmp(dta.name, ".") != 0)) {
	 strcpy(buf,name);
	 strcpy(get_fname(buf),dta.name);
	 strcat(buf,"\\");
	 (*call_back)(buf,param);
	 if (errno!=0)
	    break;
      }
   } while (_findnext(handle, &dta)==0);

   _findclose(handle);

   errno=0;
   return errno;
}



void windows_init()
{
}



int set_window_title(char *title)
{
   sprintf(window_title, "FED - %s", title);

   SendMessage(hwnd, MSG_SET_TITLE, 0, 0);

   return 0;
}



int got_clipboard()
{
   return TRUE;
}



int got_clipboard_data()
{
   return IsClipboardFormatAvailable(CF_TEXT);
}



int set_clipboard_data(char *data, int size)
{
   HGLOBAL mem;
   void *p;

   if (!OpenClipboard(NULL))
      return FALSE;

   mem = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, size+1);
   p = GlobalLock(mem);
   memcpy(p, data, size);
   ((char *)p)[size] = 0;
   GlobalUnlock(mem);

   EmptyClipboard();
   SetClipboardData(CF_TEXT, mem);

   CloseClipboard();

   return TRUE;
}



char *get_clipboard_data(int *size)
{
   HANDLE mem;
   void *p;

   if (!OpenClipboard(NULL))
      return NULL;

   mem = GetClipboardData(CF_TEXT);
   if (!mem)
      return NULL;

   *size = GlobalSize(mem);
   p = malloc(*size);
   memcpy(p, GlobalLock(mem), *size);
   GlobalUnlock(mem);

   CloseClipboard();

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
   static long first = 0;

   long t = timeGetTime();

   if (!first) {
      first = t;
      return 0;
   }
   else {
      t -= first;
      return t;
   }
}



void mouse_init()
{
}



void display_mouse()
{
}



void hide_mouse()
{
}



int mouse_changed(int *x, int *y)
{
   if (x)
      *x = _mouse_x / font_w;

   if (y)
      *y = _mouse_y * mouse_height / font_h;

   return ((_mouse_x / font_w != m_x) ||
	   (_mouse_y * mouse_height / font_h != m_y) || 
	   (_mouse_b != m_b) ||
	   (_mouse_b));
}



int poll_mouse()
{
   static int _x = -1, _y = -1;
   int ret;
   int x, y;

   refresh_screen();

   m_b = _mouse_b;

   x = _mouse_x / font_w;
   y = _mouse_y * mouse_height / font_h;

   ret = ((x != _x) || (y != _y) || (m_b));

   if (recording_macro()) {
      if (ret) {
	 strcpy(message, "Can't record mouse actions in a macro");
	 display_message(0);
      }

      m_b = 0;

      _x = x;
      _y = y;

      return FALSE;
   }

   m_x = _x = x;
   m_y = _y = y;

   return ret;
}



void set_mouse_pos(int x, int y)
{
   POINT p = { x*font_w+font_w/2, (y*font_h+font_h/2+mouse_height/2)/mouse_height };
   ClientToScreen(hwnd, &p);
   SetCursorPos(p.x, p.y);
}



void set_mouse_height(int h)
{
   m_y = m_y*h/mouse_height;
   mouse_height = h;

   SendMessage(hwnd, (mouse_height > 1) ? MSG_HIDEMOUSE : MSG_SHOWMOUSE, 0, 0);
}



int mouse_dclick(int mode)
{
   int ox, oy;
   int start = clock();

   do {
      ox = m_x;
      oy = m_y;

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



void usleep(int msec)
{
   WaitForSingleObjectEx(sleep_object, msec, FALSE);
}



void myprintf(const char *msg, ...)
{
   char tmp[4096];

   va_list ap;

   while (*msg == '\n')
      msg++;

   if (!*msg)
      return;

   va_start(ap, msg);
   vsprintf(tmp, msg, ap);
   va_end(ap);

   MessageBox(hwnd, tmp, "FED", MB_OK);
}



int mysystem(char *cmd)
{
   PROCESS_INFORMATION pinfo;

   STARTUPINFO sinfo =
   {
      sizeof(STARTUPINFO),
      NULL,
      NULL,
      NULL,
      0, 0,
      0, 0,
      0, 0,
      0,
      0,
      0,
      0, 0,
      NULL, NULL, NULL
   };

   DWORD ret;

   if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &sinfo, &pinfo) == 0)
      return 666;

   do {
      Sleep(100);

      if (GetExitCodeProcess(pinfo.hProcess, &ret) == 0)
	 break;
   } while (ret == STILL_ACTIVE);

   return ret;
}



void paint(HDC dc)
{
   static int col[16] =
   {
      0x000000,      /* black */
      0xAA0000,      /* blue */
      0x00AA00,      /* green */
      0xAA5522,      /* cyan */
      0x0000AA,      /* red */
      0xAA00AA,      /* magenta */
      0x00AAAA,      /* brown */
      0xAAAAAA,      /* grey */
      0x444444,      /* dark grey */
      0xFF4444,      /* light blue */
      0x44FF44,      /* light green */
      0xFFFF44,      /* light cyan */
      0x4444FF,      /* light red */
      0xFF44FF,      /* light magenta */
      0x44FFFF,      /* yellow */
      0xFFFFFF,      /* white */
   };

   HFONT prev_font;
   int current_bgc, current_fgc;
   int bgc, fgc;
   int x, y;

   RECT rc;
   GetClientRect(hwnd, &rc);

   if ((!rc.right) || (!rc.bottom))
      return;

   EnterCriticalSection(&crit);

   if ((!term_inited) || (!screen)) {
      FillRect(dc, &rc, GetStockObject(BLACK_BRUSH));
      LeaveCriticalSection(&crit);
      return;
   }

   /* delete old dc if it is the wrong size */
   if ((backbuffer_dc) && ((rc.right != backbuffer_w) || (rc.bottom != backbuffer_h))) {
      SelectObject(backbuffer_dc, prev_backbuffer_bmp);

      DeleteObject(backbuffer_bmp);
      DeleteDC(backbuffer_dc);

      backbuffer_dc = NULL;
   }

   /* create backbuffer DC */
   if (!backbuffer_dc) {
      backbuffer_dc = CreateCompatibleDC(dc);
      backbuffer_bmp = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
      prev_backbuffer_bmp = SelectObject(backbuffer_dc, backbuffer_bmp);
      FillRect(backbuffer_dc, &rc, GetStockObject(BLACK_BRUSH));
   }

   /* set the font */
   prev_font = SelectObject(backbuffer_dc, font);

   SetTextAlign(backbuffer_dc, TA_TOP | TA_LEFT);
   SetBkMode(backbuffer_dc, OPAQUE);

   current_bgc = -1;
   current_fgc = -1;

   /* draw the text */
   for (y=0; y<screen_h; y++) {
      x = 0;

      while (x < screen_w) {
	 char buf[1024];
	 int i = 1;

	 short c = screen[y][x].ch;

	 if (c > 255)
	    c = ' ';

	 buf[0] = c;

	 while ((x+i < screen_w) && (screen[y][x+i].col == screen[y][x].col)) {
	    c = screen[y][x+i].ch;
	    
	    if (c > 255)
	    	c = ' ';

	    buf[i] = c;
	    i++;
	 }

	 buf[i] = 0;

	 fgc = col[screen[y][x].col & 15];
	 bgc = col[screen[y][x].col >> 4];

	 if (fgc != current_fgc) {
	    SetTextColor(backbuffer_dc, fgc);
	    current_fgc = fgc;
	 }

	 if (bgc != current_bgc) {
	    SetBkColor(backbuffer_dc, bgc);
	    current_bgc = bgc;
	 }

	 TextOut(backbuffer_dc, x*font_w, y*font_h, buf, i);

	 x += i;
      }
   }

   /* draw Unicode box chars over the top */
   SelectObject(backbuffer_dc, font2);

   SetBkMode(backbuffer_dc, TRANSPARENT);

   current_fgc = -1;

   for (y=0; y<screen_h; y++) {
      x = 0;

      while (x < screen_w) {
	 short c = screen[y][x].ch;
	 
	 if (c > 255) {
	    fgc = col[screen[y][x].col & 15];
	 
	    if (fgc != current_fgc) {
	       SetTextColor(backbuffer_dc, fgc);
	       current_fgc = fgc;
	    }

            {
	       short buf[2] = { c, 0 };

               TextOutW(backbuffer_dc, x*font_w, y*font_h, buf, 1);
	    }
	 }

	 x++;
      }
   }

   /* draw cursor */
   if (show_curs) {
      RECT rc;

      rc.left = _x_pos * font_w;
      rc.top = _y_pos * font_h;

      rc.right = rc.left + font_w;
      rc.bottom = rc.top + font_h;

      if (show_curs < 2)
	 rc.top = rc.bottom - 4;

      InvertRect(backbuffer_dc, &rc);
   }

   /* cleanup */
   SelectObject(backbuffer_dc, prev_font);

   /* blit to the screen */
   BitBlt(dc, 0, 0, rc.right, rc.bottom, backbuffer_dc, 0, 0, SRCCOPY);

   LeaveCriticalSection(&crit);
}



#define WINDOW_STYLE    \
   WS_OVERLAPPED  |     \
   WS_SYSMENU     |     \
   WS_CAPTION     |     \
   WS_THICKFRAME  |     \
   WS_MINIMIZEBOX |     \
   WS_MAXIMIZEBOX



long FAR PASCAL WndProc(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
   WINDOWPLACEMENT placement;
   PAINTSTRUCT ps;
   HFONT prev_font;
   SIZE sz;
   RECT rc, *prc;
   HDC dc;
   HKEY hkey;
   DWORD dword, type, size;
   int c, w, h;

   switch (message) {

      case WM_CREATE:
	 font = CreateFont(16, 0,
			   0, 0,
			   FW_NORMAL,
			   FALSE, FALSE, FALSE,
			   DEFAULT_CHARSET,
			   OUT_DEFAULT_PRECIS,
			   CLIP_DEFAULT_PRECIS,
			   DEFAULT_QUALITY,
			   FIXED_PITCH,
			   "Courier");

	 font2 = CreateFont(18, 0,
			    0, 0,
			    FW_NORMAL,
			    FALSE, FALSE, FALSE,
			    DEFAULT_CHARSET,
			    OUT_DEFAULT_PRECIS,
			    CLIP_DEFAULT_PRECIS,
			    DEFAULT_QUALITY,
			    FIXED_PITCH,
			    "Courier New");

	 dc = BeginPaint(hwnd, &ps);

	 prev_font = SelectObject(dc, font);

	 GetTextExtentPoint32(dc, " ", 1, &sz);

	 font_w = sz.cx;
	 font_h = sz.cy;

	 SelectObject(dc, prev_font);

	 EndPaint(hwnd, &ps);

	 rc.top = 0;
	 rc.left = 0;
	 rc.right = screen_w * font_w;
	 rc.bottom = screen_h * font_h;

	 AdjustWindowRect(&rc, WINDOW_STYLE, FALSE);

	 rc.right -= rc.left;
	 rc.bottom -= rc.top;

	 c = SWP_NOZORDER | SWP_NOMOVE;

	 if (got_screen_pos) {
	    RECT desktop_rc;
	    HWND desktop_wnd = GetDesktopWindow();

	    GetClientRect(desktop_wnd, &desktop_rc);

	    if (screen_x < 0)
	       screen_x += desktop_rc.right - rc.right + 1;

	    if (screen_y < 0)
	       screen_y += desktop_rc.bottom - rc.bottom + 1;

	    c &= ~SWP_NOMOVE;
	 }

	 if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\FED", 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
	    if (!got_screen_pos) {
	       size = sizeof(dword);
	       if ((RegQueryValueEx(hkey, "x", 0, &type, (char *)&dword, &size) == ERROR_SUCCESS) && (type == REG_DWORD)) {
		  screen_x = dword;
		  c &= ~SWP_NOMOVE;
	       }

	       size = sizeof(dword);
	       if ((RegQueryValueEx(hkey, "y", 0, &type, (char *)&dword, &size) == ERROR_SUCCESS) && (type == REG_DWORD)) {
		  screen_y = dword;
		  c &= ~SWP_NOMOVE;
	       }
	    }

	    if (!got_screen_size) {
	       size = sizeof(dword);
	       if ((RegQueryValueEx(hkey, "maximised", 0, &type, (char *)&dword, &size) == ERROR_SUCCESS) && (type == REG_DWORD))
		  maximize = dword;

	       size = sizeof(dword);
	       if ((RegQueryValueEx(hkey, "w", 0, &type, (char *)&dword, &size) == ERROR_SUCCESS) && (type == REG_DWORD))
		  rc.right = dword;

	       size = sizeof(dword);
	       if ((RegQueryValueEx(hkey, "h", 0, &type, (char *)&dword, &size) == ERROR_SUCCESS) && (type == REG_DWORD))
		  rc.bottom = dword;
	    }

	    RegCloseKey(hkey);
	 }

	 SetWindowPos(hwnd, NULL, screen_x, screen_y, rc.right, rc.bottom, c);

	 DragAcceptFiles(hwnd, TRUE);
	 return 0;

      case WM_PAINT:
	 if (GetUpdateRect(hwnd, &rc, FALSE)) {
	    dc = BeginPaint(hwnd, &ps);

	    paint(dc);

	    EndPaint(hwnd, &ps);
	 }
	 return 0;

      case MSG_REDRAW:
	 if (!lock_display)
	    InvalidateRect(hwnd, NULL, TRUE);
	 return 0;

      case MSG_SET_TITLE:
	 SetWindowText(hwnd, window_title);
	 return 0;

      case WM_SIZING:
	 prc = (LPRECT)lParam;

	 rc = *prc;

	 AdjustWindowRect(&rc, WINDOW_STYLE, FALSE);

	 rc.top    = -rc.top    + 2*prc->top;
	 rc.bottom = -rc.bottom + 2*prc->bottom;
	 rc.left   = -rc.left   + 2*prc->left;
	 rc.right  = -rc.right  + 2*prc->right;

	 w = MAX((rc.right - rc.left + font_w/2) / font_w, 8);
	 h = MAX((rc.bottom - rc.top + font_h/2) / font_h, 8);

	 if ((wParam == WMSZ_RIGHT) || (wParam == WMSZ_BOTTOMRIGHT) || (wParam == WMSZ_TOPRIGHT))
	    rc.right = rc.left + w * font_w;
	 else
	    rc.left = rc.right - w * font_w;

	 if ((wParam == WMSZ_BOTTOM) || (wParam == WMSZ_BOTTOMRIGHT) || (wParam == WMSZ_BOTTOMLEFT))
	    rc.bottom = rc.top + h * font_h;
	 else
	    rc.top = rc.bottom - h * font_h;

	 AdjustWindowRect(&rc, WINDOW_STYLE, FALSE);

	 *prc = rc;
	 return TRUE;

      case WM_SIZE:
	 new_screen_w = MAX(LOWORD(lParam) / font_w, 8);
	 new_screen_h = MAX(HIWORD(lParam) / font_h, 8);

	 winched = 2;
	 return 0;

      case WM_CHAR:
      case WM_SYSCHAR:
	 for (c=0; c<(int)(sizeof(skipkeys)/sizeof(int)); c++) {
	    if (skipkeys[c] == wParam)
	       return 0;
	 }

	 if (key_buffer_end < KEY_BUFFER_SIZE-1)
	    c = key_buffer_end+1;
	 else
	    c = 0;

	 if (c != key_buffer_start) {
	    key_buffer[key_buffer_end].key = wParam;
	    key_buffer[key_buffer_end].mod = _modifiers();
	    key_buffer_end = c;
	 }

	 SetEvent(sleep_object);
	 return 0;

      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
	 for (c=0; c<(int)(sizeof(syskeys)/sizeof(int)); c++) {
	    if (syskeys[c] == wParam) {
	       if (key_buffer_end < KEY_BUFFER_SIZE-1)
		  c = key_buffer_end+1;
	       else
		  c = 0;

	       if (c != key_buffer_start) {
		  key_buffer[key_buffer_end].key = VK(wParam);
		  key_buffer[key_buffer_end].mod = _modifiers();
		  key_buffer_end = c;
	       }

	       SetEvent(sleep_object);
	       break;
	    }
	 }
	 return 0;

      case WM_MOUSEMOVE:
	 if (GetFocus() == hwnd) {
	    _mouse_x = (short)LOWORD(lParam);
	    _mouse_y = (short)HIWORD(lParam);

	    if ((!_mouse_b) &&
		((_mouse_x < 0) || (_mouse_x >= screen_w*font_w) ||
		 (_mouse_y < 0) || (_mouse_y >= screen_h*font_h))) {
	       if (captured) {
		  captured = FALSE;
		  ReleaseCapture();
	       }

	       _mouse_x = -256;
	       _mouse_y = -256;
	    }
	    else {
	       if (!captured) {
		  captured = TRUE;
		  SetCapture(hwnd);
	       }
	    }
	 }
	 else {
	    _mouse_x = -256;
	    _mouse_y = -256;
	 }
	 return 0;

      case WM_LBUTTONDOWN:
	 _mouse_b |= 1;
	 return 0;

      case WM_LBUTTONUP:
	 _mouse_b &= ~1;
	 return 0;

      case WM_RBUTTONDOWN:
	 _mouse_b |= 2;
	 return 0;

      case WM_RBUTTONUP:
	 _mouse_b &= ~2;
	 return 0;

      case WM_CAPTURECHANGED:
	 captured = FALSE;
	 _mouse_b = 0;
	 return 0;

      case MSG_SHOWMOUSE:
	 SetCursor(LoadCursor(NULL, IDC_ARROW));
	 return 0;

      case MSG_HIDEMOUSE:
	 SetCursor(NULL);
	 return 0;

      case WM_SETFOCUS:
	 focused = TRUE;
	 return 0;

      case WM_KILLFOCUS:
	 if ((_mouse_b) || (captured)) {
	    _mouse_b = 0;
	    captured = 0;
	    ReleaseCapture();
	 }
	 focused = FALSE;
	 return 0;

      case WM_CLOSE:
	 for (c=0; c<8; c++)
	    WndProc(hwnd, WM_CHAR, 27, 0);

	 WndProc(hwnd, WM_CHAR, 1+'Q'-'A', 0);
	 return 0;

      case WM_DROPFILES:
	 lock_display = TRUE;

	 c = DragQueryFile((HANDLE)wParam, -1, NULL, 0);

	 for (w=0; w<c; w++) {
	    char buf[256];

	    int n = DragQueryFile((HANDLE)wParam, w, buf, sizeof(buf));

	    if (n > 0) {
	       struct stat s;

	       if ((stat(buf, &s) == 0) && (s.st_mode & _S_IFDIR)) {
		  printf("Set current directory to %s", buf);
		  chdir(buf);
	       }
	       else {
		  for (h=0; h<8; h++)
		     WndProc(hwnd, WM_CHAR, 27, 0);

		  WndProc(hwnd, WM_CHAR, 1+'O'-'A', 0);

		  for (h=0; h<n; h++)
		     WndProc(hwnd, WM_CHAR, buf[h], 0);

		  WndProc(hwnd, WM_KEYDOWN, VK_RETURN, 0);
	       }
	    }
	 }

	 DragFinish((HANDLE)wParam);
	 return 0;

      case MSG_CLOSE:
	 placement.length = sizeof(placement);
	 GetWindowPlacement(hwnd, &placement);

	 if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\FED", 0, NULL, 0, KEY_WRITE, NULL, &hkey, &dword) == ERROR_SUCCESS) {
	    dword = (placement.showCmd == SW_SHOWMAXIMIZED) ? 1 : 0;
	    RegSetValueEx(hkey, "maximised", 0, REG_DWORD, (char *)&dword, sizeof(dword));

	    dword = placement.rcNormalPosition.left;
	    RegSetValueEx(hkey, "x", 0, REG_DWORD, (char *)&dword, sizeof(dword));

	    dword = placement.rcNormalPosition.top;
	    RegSetValueEx(hkey, "y", 0, REG_DWORD, (char *)&dword, sizeof(dword));

	    dword = placement.rcNormalPosition.right - placement.rcNormalPosition.left;
	    RegSetValueEx(hkey, "w", 0, REG_DWORD, (char *)&dword, sizeof(dword));

	    dword = placement.rcNormalPosition.bottom - placement.rcNormalPosition.top;
	    RegSetValueEx(hkey, "h", 0, REG_DWORD, (char *)&dword, sizeof(dword));

	    RegCloseKey(hkey);
	 }

	 DestroyWindow(hwnd);
	 return 0;

      case WM_DESTROY:
	 if (backbuffer_dc) {
	    SelectObject(backbuffer_dc, prev_backbuffer_bmp);

	    DeleteObject(backbuffer_bmp);
	    DeleteDC(backbuffer_dc);

	    backbuffer_dc = NULL;
	 }

	 DeleteObject(font);
	 DeleteObject(font2);

	 hwnd = NULL;

	 PostQuitMessage(0);
	 return 0;
   }

   return DefWindowProc(hwnd, message, wParam, lParam);
}



void window_thread(HANDLE hInstance)
{
   WNDCLASS wndclass;
   MSG msg;

   wndclass.style = CS_HREDRAW | CS_VREDRAW;
   wndclass.lpfnWndProc = WndProc;
   wndclass.cbClsExtra = 0;
   wndclass.cbWndExtra = 0;
   wndclass.hInstance = hInstance;
   wndclass.hIcon = LoadIcon(hInstance, "wnd_icon");
   wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground = NULL;
   wndclass.lpszMenuName = NULL;
   wndclass.lpszClassName = "FED";

   RegisterClass(&wndclass);

   hwnd = CreateWindow("FED",                /* window class name */
		       window_title,         /* window caption */
		       WINDOW_STYLE,         /* window style */
		       CW_USEDEFAULT,        /* initial x position */
		       CW_USEDEFAULT,        /* initial y position */
		       CW_USEDEFAULT,        /* initial x size */
		       CW_USEDEFAULT,        /* initial y size */
		       NULL,                 /* parent window handle */
		       NULL,                 /* window menu handle */
		       hInstance,            /* program instance handle */
		       NULL);                /* creation parameters */

   ShowWindow(hwnd, (maximize) ? SW_SHOWMAXIMIZED : SW_SHOW);
   UpdateWindow(hwnd);

   window_inited = TRUE;

   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   window_closed = TRUE;
}



int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
   char argbuf[4096];
   char *argv[256];
   int argc;
   int i, j, q;
   long thread_handle;
   int ret;

   /* can't use parameter because it doesn't include the executable name */
   strcpy(argbuf, GetCommandLine());

   argc = 0;
   i = 0;

   /* parse commandline into argc/argv format */
   while (argbuf[i]) {
      while ((argbuf[i]) && (isspace(argbuf[i])))
	 i++;

      if (argbuf[i]) {
	 /* extract word */
	 if ((argbuf[i] == '\'') || (argbuf[i] == '"')) {
	    q = argbuf[i++];
	    if (!argbuf[i])
	       break;
	 }
	 else
	    q = 0;

	 argv[argc] = &argbuf[i];

	 while ((argbuf[i]) && ((q) ? (argbuf[i] != q) : (!isspace(argbuf[i]))))
	    i++;

	 if (argbuf[i]) {
	    argbuf[i] = 0;
	    i++;
	 }

	 /* handle -<w>x<h> and -<w>+<h> options */
	 if ((argv[argc][0] == '-') && (((argv[argc][1]>='0') && (argv[argc][1]<='9')) || (argv[argc][1]=='-'))) {
	    for (j=0; argv[argc][j]; j++) {
	       if ((argv[argc][j] == 'x') || (argv[argc][j] == '+'))
		  break;
	    }

	    q = argv[argc][j++];

	    if (q == 'x') {
	       screen_w = atoi(argv[argc]+1);
	       screen_h = atoi(argv[argc]+j);

	       screen_w = MID(8, screen_w, 256);
	       screen_h = MID(8, screen_h, 256);

	       new_screen_w = screen_w;
	       new_screen_h = screen_h;

	       got_screen_size = TRUE;
	    }
	    else if (q == '+') {
	       screen_x = atoi(argv[argc]+1);
	       screen_y = atoi(argv[argc]+j);

	       got_screen_pos = TRUE;
	    }
	 }

	 argc++;
      }
   }

   /* create the worker thread */
   InitializeCriticalSection(&crit);

   sleep_object = CreateEvent(NULL, TRUE, FALSE, NULL);

   thread_handle = _beginthread(window_thread, 0, hInstance);

   do {
   } while (!window_inited);

   /* call the application entry point */
   ret = _main(argc, argv);

   /* shutdown */
   SendMessage(hwnd, MSG_CLOSE, 0, 0);

   do {
   } while (!window_closed);

   CloseHandle(sleep_object);

   return ret;
}

