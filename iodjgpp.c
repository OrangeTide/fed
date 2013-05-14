/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      djgpp-specific IO routines for screen output, reading from the
 *      keyboard and mouse, and simple (faster than stdio) buffered file IO
 */


#ifndef TARGET_DJGPP
   #error This file should only be compiled as part of the djgpp target
#endif


#include <dos.h>
#include <dir.h>
#include <time.h>
#include <fcntl.h> 
#include <osfcn.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/stat.h>
#include <sys/movedata.h>
#include <sys/segments.h>
#include <sys/farptr.h>
#include <sys/exceptn.h>

#include "fed.h"


int screen_w = 80;
int screen_h = 25;              /* screen dimensions */

int x_pos = 0, y_pos = 0;
int attrib = 7;
int norm_attrib = 7;
int saved_lines = 0;

int dos_seg;

int mouse_state;
int m_x = -1;
int m_y = -1;
int m_b = 0;
int mouse_height = 1;

int windows_version = 0;
int clipboard_version = 0;

char orig_title[256];



void mouse_init()
{
   __dpmi_regs reg;

   if (!mouse_state) {
      reg.x.ax = 0;
      __dpmi_int(0x33, &reg);
      mouse_state = reg.x.ax;
   }

   reg.x.ax = 10;
   reg.x.bx = 0;
   reg.x.cx = 0xffff;
   reg.x.dx = 0x7700;
   __dpmi_int(0x33, &reg);

   poll_mouse();
   set_mouse_height(1);
}



void display_mouse()
{
   __dpmi_regs reg;

   if (mouse_state) {
      reg.x.ax = 1;
      __dpmi_int(0x33, &reg);
   }
}



void hide_mouse()
{
   __dpmi_regs reg;

   if (mouse_state) {
       reg.x.ax = 2;
       __dpmi_int(0x33, &reg);
   }
}



int mouse_changed(int *x, int *y)
{
   __dpmi_regs reg;

   if (!mouse_state)
      return FALSE;

   reg.x.ax = 3;
   __dpmi_int(0x33, &reg);

   if (x)
      *x = reg.x.cx / 8;

   if (y)
      *y = reg.x.dx / 8;

   return (((reg.x.cx / 8) != m_x) || ((reg.x.dx / 8) != m_y) || 
	   (reg.x.bx != m_b) || (reg.x.bx));
}



int poll_mouse()
{
   static int _x = -1, _y = -1;
   int ret;
   int x, y;
   __dpmi_regs reg;

   if (!mouse_state)
      return FALSE;

   reg.x.ax = 3;
   __dpmi_int(0x33, &reg);

   m_b = reg.x.bx;
   x = reg.x.cx / 8;
   y = reg.x.dx / 8;

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
   __dpmi_regs reg;

   if (!mouse_state)
      return;

   reg.x.ax = 4;
   reg.x.cx = x*8;
   reg.x.dx = y*8;
   __dpmi_int(0x33, &reg);

   m_x = x;
   m_y = y;
}



void set_mouse_height(int h)
{
   __dpmi_regs reg;

   if (!mouse_state)
      return;

   poll_mouse();

   reg.x.ax = 8;
   reg.x.cx = 0;
   reg.x.dx = screen_h * h * 8 - 1;
   __dpmi_int(0x33, &reg);

   reg.x.ax = 15;
   reg.x.cx = 8;
   reg.x.dx = MAX(16/h, 1);
   __dpmi_int(0x33, &reg);

   set_mouse_pos(m_x, m_y*h/mouse_height);
   mouse_height = h;
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



void set_bright_backgrounds()
{
   __dpmi_regs reg;

   reg.x.ax = 0x1003;
   reg.x.bx = 0;
   __dpmi_int(0x10, &reg);
}



void term_init(int screenheight)
{
   struct text_info dat;

   if (saved_lines <= 0) {
      gettextinfo(&dat);
      screen_h = saved_lines = dat.screenheight;
      screen_w = dat.screenwidth;
      norm_attrib = dat.normattr;
   }

   if (screen_h != screenheight) {
      _set_screen_lines(screenheight);
      gettextinfo(&dat);
      screen_h = dat.screenheight;
      screen_w = dat.screenwidth;
   }

   set_bright_backgrounds();

   n_vid();
   textattr(attrib);
   cls();

   mouse_init();

   errno = 0;
}



void term_exit()                     /* close down the screen */
{
   textattr(norm_attrib);

   if (saved_lines != screen_h) {
      _set_screen_lines(saved_lines);
      cls();
   }
   else {
      goto2(0,screen_h-1);
      putch('\n');
   }

   _setcursortype(_NORMALCURSOR);
   show_c();
}



void term_reinit(int wait)             /* fixup after running other progs */
{
   struct text_info dat;

   gppconio_init();
   gettextinfo(&dat);

   if (dat.screenheight != screen_h) {
      _set_screen_lines(screen_h);
      gettextinfo(&dat);
      screen_h = dat.screenheight;
      screen_w = dat.screenwidth;
      mouse_init();
   }

   set_bright_backgrounds();

   if (wait) {
      clear_keybuf();
      gch();
   }

   __djgpp_set_ctrl_c(0);
   setcbrk(0);
}



void my_setcursor(int shape)
{
   __dpmi_regs reg;

   reg.h.ah = 1;
   reg.x.cx = shape;
   __dpmi_int(0x10, &reg);
}



void pch(unsigned char c)
{
   if ((x_pos >= 0) && (x_pos < screen_w))
      _farpokew(dos_seg, SCRN_ADDR(x_pos, y_pos), (attrib<<8)|c);

   x_pos++;
}



void mywrite(unsigned char *s)
{
   int p;

   while ((*s) && (x_pos < 0)) {
      x_pos++;
      s++;
   }

   p = SCRN_ADDR(x_pos, y_pos);

   _farsetsel(dos_seg);

   while ((*s) && (x_pos < screen_w)) { 
      _farnspokew(p, (attrib<<8) | *s);
      p += 2;
      x_pos++;
      s++;
   }

   while (*s) {
      x_pos++;
      s++;
   }
}



void del_to_eol()
{
   int c = MAX(x_pos, 0);
   int p = SCRN_ADDR(c, y_pos);

   _farsetsel(dos_seg);

   while (c++ < screen_w) { 
      _farnspokew(p, attrib<<8);
      p += 2;
   }
}



void cr_scroll()
{
   putchar('\n');
   fflush(stdout);
}



void screen_block(int s_s, int s_o, int s_g, int d_s, int d_o, int d_g, int w, int h)
{
   int y;
   for (y=0; y<h; y++) {
      movedata(s_s, s_o, d_s, d_o, w);
      s_o += s_g;
      d_o += d_g;
   }
}



char *save_screen(int x, int y, int w, int h)
{
   char *b = malloc(++w * ++h * 2);

   if (!b)
      return NULL;

   screen_block(dos_seg, SCRN_ADDR(x,y), 160, _my_ds(), (int)b, w*2, w*2, h);
   return b;
}



void restore_screen(int x, int y, int w, int h, char *buf)
{
   w++;
   h++;

   if (buf) {
      screen_block(_my_ds(), (int)buf, w*2,
		   dos_seg, SCRN_ADDR(x,y), 160, w*2, h);
      free(buf);
   }
   else
      dirty_everything();
}



void clear_keybuf()
{
   while(keypressed())
      gch();
}



MYFILE *open_file(char *name, int mode)
{
   /* opens a file, returning a pointer to a MYFILE struct, or NULL on error */

   MYFILE *f;
   struct ffblk dta;

   errno=0;
   if ((f=malloc(sizeof(MYFILE)))==NULL) {
      errno=ENOMEM;
      return NULL;
   }

   f->f_mode = mode;
   f->f_buf_pos = f->f_buf_end = f->f_buf;

   if (mode == FMODE_READ) {
      findfirst(name, &dta, FA_RDONLY | FA_ARCH);
      if (errno != 0) {
	 free(f);
	 return NULL;
      }

      f->f_size = dta.ff_fsize;

      f->f_hndl = open(name, O_RDONLY|O_BINARY, S_IRUSR|S_IWUSR);
      if (f->f_hndl < 0) {
	 free(f);
	 return NULL;
      }
      errno = 0;
   }
   else if (mode == FMODE_WRITE) { 
      f->f_hndl = open(name, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
      if (f->f_hndl < 0) {
	 free(f);
	 return NULL;
      }
      f->f_buf_end=f->f_buf+BUFFER_SIZE;
      errno = 0;
   }

   return f;
}



int close_file(MYFILE *f)
{
   /* closes a file, flushing buffers and freeing the file struct */

   if (f) {
      if (f->f_mode==FMODE_WRITE)
	 flush_buffer(f);

      close(f->f_hndl);
      free(f);
      return errno;
   }

   return 0;
}



int refill_buffer(MYFILE *f, int peek_flag)
{
   /* refills the read buffer, for use by the get_char and peek_char macros.
      The file MUST have been opened in read mode, and the buffer must be
      empty. If peek_flag is set the character is examined without moving
      the file pointer onwards */

   int s = (int)(f->f_buf_pos - f->f_buf);

   if (s >= f->f_size) {                /* EOF */
      errno = EOF;
      return EOF;
   }
   else {                               /* refill the buffer */
      f->f_size -= s;
      s = (int)MIN(BUFFER_SIZE, f->f_size);
      f->f_buf_end = f->f_buf + s;
      f->f_buf_pos = f->f_buf;
      if (read(f->f_hndl, f->f_buf, s) != s)
	 return EOF;
      if (peek_flag)
	 return *(f->f_buf_pos);
      else
	 return *(f->f_buf_pos++);
   }
}



int flush_buffer(MYFILE *f)
{
   /* flushes a file buffer to the disk */

   int s = (int)(f->f_buf_pos - f->f_buf);

   if ((s>0) && (errno==0))
      if (write(f->f_hndl, f->f_buf, s) == s)
	 f->f_buf_pos=f->f_buf;

   return errno;
}



int file_size(char *p)
{
   struct ffblk dta;

   errno = 0;

   if (findfirst(p, &dta, FA_RDONLY | FA_ARCH)==0)
      return dta.ff_fsize;
   else
      return -1;
}



long file_time(char *p)
{
   struct ffblk dta;
   int ret;

   errno = 0;

   ret = findfirst(p, &dta, FA_RDONLY | FA_ARCH);

   errno = 0;

   return (ret==0) ? ((long)dta.ff_fdate << 16) + (long)dta.ff_ftime : -1;
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
	 while ((*p==' ') || (*p==',') || (*p==';'))
	    p++;
	 if(*p) {
	    pos=0;
	    result[0]=0;
	    while ((*p) && (*p!=' ') && (*p!=',') && (*p!=';')) {
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
   char buf[256];
   int c;
   struct ffblk dta;

   errno = 0;

   findfirst(name, &dta, FA_RDONLY | FA_ARCH);

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
      strcpy(buf,name);
      strcpy(get_fname(buf),dta.ff_name);

      (*call_back)(buf,param);
      if (errno!=0)
	 break;

   } while (findnext(&dta)==0);

   errno=0;
   return errno;
}



int do_for_each_directory(char *name, int (*call_back)(char *, int), int param)
{
   char buf[256];
   struct ffblk dta;

   errno = 0;

   findfirst(name, &dta, FA_DIREC);

   if (errno!=0)
      return errno;

   do {
      if ((dta.ff_attrib & FA_DIREC) && 
	  (strcmp(dta.ff_name, ".") != 0)) {
	 strcpy(buf,name);
	 strcpy(get_fname(buf),dta.ff_name);
	 strcat(buf,"\\");
	 (*call_back)(buf,param);
	 if (errno!=0)
	    break;
      }
   } while (findnext(&dta)==0);

   errno=0;
   return errno;
}



#define MASK_LINEAR(addr)     (addr & 0x000FFFFF)
#define RM_OFFSET(addr)       (addr & 0xF)
#define RM_SEGMENT(addr)      ((addr >> 4) & 0xFFFF)



void windows_init()
{
   __dpmi_regs r;

   r.x.ax = 0x1600; 
   __dpmi_int(0x2F, &r);

   if ((r.h.al == 0) || (r.h.al == 1) || (r.h.al == 0x80) || (r.h.al == 0xFF)) {
      windows_version = 0;
      clipboard_version = 0;
      return;
   }

   windows_version = ((int)r.h.al << 8) | (int)r.h.ah;

   r.x.ax = 0x168E;
   r.x.dx = 2;
   r.x.cx = sizeof(orig_title)-1;
   r.x.es = RM_SEGMENT(__tb);
   r.x.di = RM_OFFSET(__tb);

   __dpmi_int(0x2F, &r);

   dosmemget(MASK_LINEAR(__tb), sizeof(orig_title), orig_title);

   r.x.ax = 0x1700;
   __dpmi_int(0x2F, &r);

   if (r.x.ax == 0x1700)
      clipboard_version = 0;
   else
      clipboard_version = r.x.ax;
}



int set_window_title(char *title)
{
   char buf[256];
   __dpmi_regs r;

   if (!windows_version)
      return FALSE;

   if (orig_title[0]) {
      strcpy(buf, orig_title);
      strcat(buf, " - ");
      strcat(buf, title);
   }
   else
      strcpy(buf, title);

   buf[79] = 0;

   dosmemput(buf, strlen(buf)+1, MASK_LINEAR(__tb));

   r.x.ax = 0x168E;
   r.x.dx = 0;
   r.x.es = RM_SEGMENT(__tb);
   r.x.di = RM_OFFSET(__tb);

   __dpmi_int(0x2F, &r);

   return TRUE;
}



int got_clipboard()
{
   return (clipboard_version != 0);
}



int got_clipboard_data()
{
   __dpmi_regs r;
   int size;

   if (!clipboard_version)
      return FALSE;

   r.x.ax = 0x1701;
   __dpmi_int(0x2F, &r);

   if (!r.x.ax)
      return FALSE;

   r.x.ax = 0x1704;
   r.x.dx = 1;
   __dpmi_int(0x2F, &r);

   size = (r.x.dx<<16) | r.x.ax;

   r.x.ax = 0x1708;
   __dpmi_int(0x2F, &r);

   return (size > 0);
}



int set_clipboard_data(char *data, int size)
{
   __dpmi_regs r;
   int seg, sel;
   int ret = TRUE;

   if (!clipboard_version)
      return FALSE;

   r.x.ax = 0x1701;
   __dpmi_int(0x2F, &r);

   if (!r.x.ax)
      return FALSE;

   r.x.ax = 0x1702;
   __dpmi_int(0x2F, &r);

   seg = __dpmi_allocate_dos_memory((size+15)>>4, &sel);

   if (seg < 0) {
      ret = FALSE;
   }
   else {
      dosmemput(data, size, seg*16);

      r.x.ax = 0x1703;
      r.x.dx = 1;
      r.x.es = seg;
      r.x.bx = 0;
      r.x.si = size>>16;
      r.x.cx = size&0xFFFF;

      __dpmi_int(0x2F, &r);

      if (!r.x.ax)
	 ret = FALSE;

      __dpmi_free_dos_memory(sel);
   }

   r.x.ax = 0x1708;
   __dpmi_int(0x2F, &r);

   return ret;
}



char *get_clipboard_data(int *size)
{
   __dpmi_regs r;
   int seg, sel;
   void *ret = NULL;

   if (!clipboard_version)
      return FALSE;

   r.x.ax = 0x1701;
   __dpmi_int(0x2F, &r);

   if (!r.x.ax)
      return NULL;

   r.x.ax = 0x1704;
   r.x.dx = 1;
   __dpmi_int(0x2F, &r);

   *size = (r.x.dx<<16) | r.x.ax;

   if (*size > 0) {
      seg = __dpmi_allocate_dos_memory((*size+15)>>4, &sel);

      if (seg > 0) {
	 r.x.ax = 0x1705;
	 r.x.dx = 1;
	 r.x.es = seg;
	 r.x.bx = 0;

	 __dpmi_int(0x2F, &r);

	 if (r.x.ax) {
	    ret = malloc(*size);

	    if (ret)
	       dosmemget(seg*16, *size, ret);
	 }

	 __dpmi_free_dos_memory(sel);
      }
   }

   r.x.ax = 0x1708;
   __dpmi_int(0x2F, &r);

   return ret;
}


