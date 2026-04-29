/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      watcom-specific IO routines for screen output, reading from the
 *      keyboard and mouse, and simple (faster than stdio) buffered file IO
 */


#ifndef TARGET_WATCOM
   #error This file should only be compiled as part of the watcom target
#endif


#include <dos.h>
#include <i86.h>
#include <time.h>
#include <fcntl.h> 
#include <sys/stat.h>

#include "fed.h"


int screen_w = 80;
int screen_h = 25;               /* screen dimensions */

int x_pos = 0, y_pos = 0;
int attrib = 7;
int norm_attrib = 7;             /* light gray on black */
int saved_lines = 0;
char saved_vmode = 0;
char fed_vmode = 0;
char card_type = 0;
int video_base = 0xb0000;

int mouse_state;
int m_x = -1;
int m_y = -1;
int m_b = 0;
int mouse_height = 1;

int windows_version = 0;
int clipboard_version = 0;

char orig_title[256];

struct term_info {
   char videomode;
   int screenwidth;
   int screenheight;
};

void mouse_init()
{
   union REGS reg;

   if (!mouse_state) {
      reg.w.ax = 0;
      int386(0x33, &reg, &reg);
      mouse_state = reg.w.ax;
   }

   reg.w.ax = 10;
   reg.w.bx = 0;
   reg.w.cx = 0xffff;
   reg.w.dx = 0x7700;
   int386(0x33, &reg, &reg);

   poll_mouse();
   set_mouse_height(1);
}



void display_mouse()
{
   union REGS reg;

   if (mouse_state) {
      reg.w.ax = 1;
      int386(0x33, &reg, &reg);
   }
}



void hide_mouse()
{
   union REGS reg;

   if (mouse_state) {
       reg.w.ax = 2;
       int386(0x33, &reg, &reg);
   }
}



int mouse_changed(int *x, int *y)
{
   union REGS reg;

   if (!mouse_state)
      return FALSE;

   reg.w.ax = 3;
   int386(0x33, &reg, &reg);

   if (x)
      *x = reg.w.cx / 8;

   if (y)
      *y = reg.w.dx / 8;

   return (((reg.w.cx / 8) != m_x) || ((reg.w.dx / 8) != m_y) || 
	   (reg.w.bx != m_b) || (reg.w.bx));
}



int poll_mouse()
{
   static int _x = -1, _y = -1;
   int ret;
   int x, y;
   union REGS reg;

   if (!mouse_state)
      return FALSE;

   reg.w.ax = 3;
   int386(0x33, &reg, &reg);

   m_b = reg.w.bx;
   x = reg.w.cx / 8;
   y = reg.w.dx / 8;

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
   union REGS reg;

   if (!mouse_state)
      return;

   reg.w.ax = 4;
   reg.w.cx = x*8;
   reg.w.dx = y*8;
   int386(0x33, &reg, &reg);

   m_x = x;
   m_y = y;
}



void set_mouse_height(int h)
{
   union REGS reg;

   if (!mouse_state)
      return;

   poll_mouse();

   reg.w.ax = 8;
   reg.w.cx = 0;
   reg.w.dx = screen_h * h * 8 - 1;
   int386(0x33, &reg, &reg);

   reg.w.ax = 15;
   reg.w.cx = 8;
   reg.w.dx = MAX(16/h, 1);
   int386(0x33, &reg, &reg);

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
   union REGS reg;

   reg.w.ax = 0x1003;
   reg.w.bx = 0;
   int386(0x10, &reg, &reg);
}



void set_video_mode(short mode)
{
   union REGS reg;
   
   reg.h.ah = 0;
   reg.h.al = mode;
   int386(0x10, &reg, &reg);
}



void goto2(int x, int y)
{
   union REGS reg;
   
   x_pos = x;
   y_pos = y;
   
   reg.w.ax = 0x0200;
   reg.w.bx = 0;
   reg.h.dh = y_pos;
   reg.h.dl = x_pos;
   int386(0x10, &reg, &reg);
}



void cls()
{
   int i;
   int16_t *p = (int16_t *)(video_base);
   /* Fill the entire screen with spaces */
   for (i = 0; i < screen_h * screen_w; i++) {
      *p++ = (attrib << 8) | ' ';
   }

   goto2(0, 0);
}



void get_term_info(struct term_info *info)
{
   /* Read the video mode from the BIOS area */
   info->videomode = *(char *)( 0x449 );   
   /* Read the screen width from the BIOS area */
   info->screenwidth = *(unsigned char*)( 0x44A );   
   if (card_type < 2) {
      /* MDA and CGA has 25 lines modes only */      
      info->screenheight = 25;
   }  else {      
      /* Only applies to EGA or VGA */
      info->screenheight = *(unsigned char*)( 0x484 ) + 1;
   }
}



int set_screen_lines(int lines)
{
   union REGS r;
   
   if (card_type < 2) {
      /* MDA and CGA have only 80x25 */
      set_video_mode(fed_vmode);
      return 25;
   }
   if (card_type == 2) {
      /* EGA video card */
      r.w.ax = 0x0500;
      int386(0x10, &r, &r); /* Set video page to 0. */
      set_video_mode(fed_vmode);
      if ( lines == 43 ) {
         r.w.ax = 0x1112;
         r.w.bx = 0;
         int386(0x10, &r, &r); /* Set the 8x8 font */
         *((unsigned char*)( 0x487 )) |= 1; /* Cursor emulation enabled */
         return 43;
      } else {
         /* Just 25 lines */
         r.w.ax = 0x1111;
         r.w.bx = 0;
         int386(0x10, &r, &r); /* Set the 8x14 font */
         return 25;
      }
   } else {
      /* VGA video card */
      if ( lines == 43 ) {
         /* Set scan lines for text modes; AL = 1 = 350 scan lines */
         r.w.ax = 0x1201;
         r.w.bx = 0x0030;
         int386(0x10, &r, &r);
      } 
      else {
        /* Set scan lines for text modes; AL = 2 = 400 scan lines */
         r.w.ax = 0x1202;
         r.w.bx = 0x0030;
         int386(0x10, &r, &r);
      }
      set_video_mode(fed_vmode);
      /* Set video page to 0. */
      r.w.ax = 0x0500;
      int386(0x10, &r, &r);      
      if ( lines == 43 || lines == 50 ) {
         /* Set the 8x8 font */
         r.w.ax = 0x1112;
         r.w.bx = 0;
         int386(0x10, &r, &r);
         return lines;
      } else if ( lines == 28 ) {
         /* Set the 8x14 font */
         r.w.ax = 0x1111;
         r.w.bx = 0;
         int386(0x10, &r, &r);
         return 28;
      } else {
         /* Set the 8x16 font */
         r.w.ax = 0x1114;
         r.w.bx = 0;
         int386(0x10, &r, &r);
         return 25;
      }
   }
}



void term_init(int screenheight)
{
   struct term_info info;
   union REGS r;
   
   if (saved_lines <= 0) {
      /* Read the video mode from the BIOS area */
      saved_vmode = *(char *)( 0x449 );
   
      if (saved_vmode == 7) {
         /* Monochrome video card */
         video_base = 0xb0000;
         fed_vmode = 7;
         card_type = 0;    /* MDA */
      } else {
         /* Color video card */
         video_base = 0xb8000;
         fed_vmode = 3;
    
         /* See if is EGA or VGA. */
         /* 0x1a00 is VGA Get Display Combination.  It returns 0x1a in AL if the */
         /* BIOS supports this function, indicating it is a VGA card. */
         r.w.ax = 0x1a00;
         int386(0x10, &r, &r);
         if ( r.h.al == 0x1a ) {
            card_type = 3; /* VGA */
         } else {
            /* If this does not return 0x10 in BL it is an EGA BIOS. */
            r.h.ah = 0x12;
            r.h.bl = 0x10;
            int386(0x10, &r, &r);
            if ( r.h.bl != 0x10 ) {
               card_type = 2; /* EGA */
            } else {
               /* Ok, it is a CGA. */
               card_type = 1;            
            }
         }
      }
      /* Get current screen size */
      get_term_info(&info);
      screen_h = saved_lines = info.screenheight;
      screen_w = info.screenwidth;
   }

   if (screen_h != screenheight || screen_w < 80) {
      screen_h = set_screen_lines(screenheight);
      get_term_info(&info);
      screen_h = info.screenheight;
      screen_w = info.screenwidth;
   }

   set_bright_backgrounds();

   n_vid();
   cls();

   mouse_init();

   errno = 0;
}



void term_exit()                     /* close down the screen */
{
   attrib = norm_attrib;

   if (saved_vmode != fed_vmode) {
      set_video_mode(saved_vmode);
      cls();
   } 
   else if (saved_lines != screen_h) {
      set_screen_lines(saved_lines);
      cls();
   }
   else {
      goto2(0,screen_h-1);
      cr_scroll();
   }

   my_setcursor(_NORMALCURSOR);
   show_c();
}



void term_reinit(int wait)             /* fixup after running other progs */
{
   struct term_info info;

   /*  gppconio_init();     Watcom conio and graph do not have init/reset functions */
   get_term_info(&info);

   if (info.screenheight != screen_h || info.videomode != fed_vmode) {
      set_screen_lines(screen_h);
      get_term_info(&info);
      screen_h = info.screenheight;
      screen_w = info.screenwidth;
      mouse_init();
   }

   set_bright_backgrounds();

   if (wait) {
      clear_keybuf();
      gch();
   }

   /* TODO: check if control-C should be disabled also for WCC */
   //  __djgpp_set_ctrl_c(0);
   // setcbrk(0);
}



void my_setcursor(int shape)
{
   union REGS reg;

   reg.h.ah = 1;
   reg.w.cx = shape;
   int386(0x10, &reg, &reg);
}



void pch(unsigned char c)
{
   if ((x_pos >= 0) && (x_pos < screen_w)) {
      uint16_t *ptr = (uint16_t *)SCRN_ADDR(x_pos, y_pos);
      *ptr = (attrib<<8) | c;
   }

   x_pos++;
}



void mywrite(char *s)
{
   uint16_t *p;

   while ((*s) && (x_pos < 0)) {
      x_pos++;
      s++;
   }

   p = (uint16_t *)SCRN_ADDR(x_pos, y_pos);

   while ((*s) && (x_pos < screen_w)) { 
      *p = (attrib<<8) | *s;
      p ++;
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
   uint16_t *p = (uint16_t *)SCRN_ADDR(c, y_pos);

   while (c++ < screen_w) { 
      *p = attrib<<8;
      p ++;
   }
}



void cr_scroll()
{
   union REGS reg;
   
   if (y_pos < screen_h - 1) {
      y_pos++;
   } else {
      reg.w.ax = 0x0601;
      reg.h.bh = attrib;
      reg.w.cx = 0;
      reg.h.dh = screen_h - 1;
      reg.h.dl = screen_w - 1;
      int386(0x10, &reg, &reg);
   }
   
   x_pos = 0;
   
   goto2(x_pos, y_pos);
}



void screen_block(char *s, int s_g, char *d, int d_g, int w, int h)
{
   int y;
   for (y=0; y<h; y++) {
      memcpy(d, s, w);
      s += s_g;
      d += d_g;
   }
}



char *save_screen(int x, int y, int w, int h)
{
   char *b = malloc(++w * ++h * 2);

   if (!b)
      return NULL;

   screen_block((char *)SCRN_ADDR(x,y), 160, b, w*2, w*2, h);
   return b;
}



void restore_screen(int x, int y, int w, int h, char *buf)
{
   w++;
   h++;

   if (buf) {
      screen_block(buf, w*2, (char*)SCRN_ADDR(x,y), 160, w*2, h);
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
   struct find_t dta;

   errno=0;
   if ((f=malloc(sizeof(MYFILE)))==NULL) {
      errno=ENOMEM;
      return NULL;
   }

   f->f_mode = mode;
   f->f_buf_pos = f->f_buf_end = f->f_buf;

   if (mode == FMODE_READ) {
      _dos_findfirst(name, _A_RDONLY | _A_ARCH,  &dta);
      if (errno != 0) {
	 free(f);
	 return NULL;
      }

      f->f_size = dta.size;

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
   struct find_t dta;

   errno = 0;

   if (_dos_findfirst(p, _A_RDONLY | _A_ARCH, &dta)==0)
      return dta.size;
   else
      return -1;
}



long file_time(char *p)
{
   struct find_t dta;
   int ret;

   errno = 0;

   ret = _dos_findfirst(p, _A_RDONLY | _A_ARCH, &dta);

   errno = 0;

   return (ret==0) ? ((long)dta.wr_date << 16) + (long)dta.wr_time : -1;
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
   struct find_t dta;

   errno = 0;

   _dos_findfirst(name, _A_RDONLY | _A_ARCH, &dta);

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
      strcpy(get_fname(buf),dta.name);

      (*call_back)(buf,param);
      if (errno!=0)
	 break;

   } while (_dos_findnext(&dta)==0);

   errno=0;
   return errno;
}



int do_for_each_directory(char *name, int (*call_back)(char *, int), int param)
{
   char buf[256];
   struct find_t dta;

   errno = 0;

   _dos_findfirst(name, _A_SUBDIR, &dta);

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
   } while (_dos_findnext(&dta)==0);

   errno=0;
   return errno;
}



void windows_init()
{
   union REGS r;
   struct SREGS sr;
   unsigned seg;

   r.w.ax = 0x1600; 
   int386(0x2F, &r, &r);

   if ((r.h.al == 0) || (r.h.al == 1) || (r.h.al == 0x80) || (r.h.al == 0xFF)) {
      windows_version = 0;
   } 
   else {
      windows_version = ((int)r.h.al << 8) | (int)r.h.ah;
      
      if (_dos_allocmem((sizeof(orig_title)+15)>>4, &seg) == 0) {
         r.w.ax = 0x168E;
         r.w.dx = 2;
         r.w.cx = sizeof(orig_title)-1;
         segread(&sr);
         sr.es = seg;
         r.w.di = 0;

         int386(0x2F, &r, &r);

         memcpy(orig_title, (char *)(seg*16), sizeof(orig_title));
         
         _dos_freemem(seg);
      }
   }
   
   r.w.ax = 0x1700;
   int386(0x2F, &r, &r);

   if (r.w.ax == 0x1700)
      clipboard_version = 0;
   else
      clipboard_version = r.w.ax;
}



int set_window_title(char *title)
{
   char *buf;
   union REGS r;
   struct SREGS sr;
   unsigned seg;

   if (!windows_version)
      return FALSE;

   if (_dos_allocmem((256+15)>>4, &seg) != 0)
      return FALSE;
      
   buf = (char *)(seg*16);
      
   if (orig_title[0]) {
      strcpy(buf, orig_title);
      strcat(buf, " - ");
      strcat(buf, title);
   }
   else
      strcpy(buf, title);

   buf[79] = 0;

   r.w.ax = 0x168E;
   r.w.dx = 0;
   segread(&sr);
   sr.es = seg;
   r.w.di = 0;

   int386(0x2F, &r, &r);

   return TRUE;
}



int got_clipboard()
{
   return (clipboard_version != 0);
}



int got_clipboard_data()
{
   union REGS r;
   int size;

   if (!clipboard_version)
      return FALSE;

   r.w.ax = 0x1701;
   int386(0x2F, &r, &r);

   if (!r.w.ax)
      return FALSE;

   r.w.ax = 0x1704;
   r.w.dx = 1;
   int386(0x2F, &r, &r);

   size = (r.w.dx<<16) | r.w.ax;

   r.w.ax = 0x1708;
   int386(0x2F, &r, &r);

   return (size > 0);
}



int set_clipboard_data(char *data, int size)
{
   union REGS r;
   struct SREGS sr;
   unsigned seg;
   int ret = TRUE;

   if (!clipboard_version)
      return FALSE;
   
   r.w.ax = 0x1701;
   int386(0x2F, &r, &r);

   if (!r.w.ax)
      return FALSE;

   r.w.ax = 0x1702;
   int386(0x2F, &r, &r);

   if (_dos_allocmem((size+15)>>4, &seg) != 0) {
      ret = FALSE;
   }
   else {
      memcpy((char *)(seg*16), data, size);

      r.w.ax = 0x1703;
      r.w.dx = 1;
      segread(&sr);
      sr.es = seg;
      r.w.bx = 0;
      r.w.si = size>>16;
      r.w.cx = size&0xFFFF;

      int386x(0x2F, &r, &r, &sr);

      if (!r.w.ax)
         ret = FALSE;

      _dos_freemem(seg);
   }

   r.w.ax = 0x1708;
   int386(0x2F, &r, &r);

   return ret;
}



char *get_clipboard_data(int *size)
{
   union REGS r;
   struct SREGS sr;
   unsigned seg;
   void *ret = NULL;

   if (!clipboard_version)
      return NULL;

   r.w.ax = 0x1701;
   int386(0x2F, &r, &r);

   if (!r.w.ax)
      return NULL;

   r.w.ax = 0x1704;
   r.w.dx = 1;
   int386(0x2F, &r, &r);

   *size = (r.w.dx<<16) | r.w.ax;

   if (*size > 0) {
      if (_dos_allocmem((*size+15)>>4, &seg) == 0) {
         r.w.ax = 0x1705;
         r.w.dx = 1;
         segread(&sr);
         sr.es = seg;
         r.w.bx = 0;

         int386x(0x2F, &r, &r, &sr);

         if (r.w.ax) {
            ret = malloc(*size);

         if (ret)
             memcpy(ret, (char*)(seg*16), *size);
         }

         _dos_freemem(seg);
      }
   }

   r.w.ax = 0x1708;
   int386(0x2F, &r, &r);

   return ret;
}


