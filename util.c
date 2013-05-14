/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Various utility functions, used all over the place
 */


#include <ctype.h>

#ifdef DJGPP
#include <sys/stat.h>
#endif

#include "fed.h"



char *prepare_popup(char *t, int *_x, int *_y, int *_w)
{
   char *b;

   if (*_w > screen_w - 4)
      *_w = screen_w - 4;

   *_x = (screen_w - *_w) / 2 - 1;
   *_y = (screen_h - 5) / 2;
   b = save_screen(*_x, *_y, *_w, 5);

   draw_box(t, *_x, *_y, *_w, 5, TRUE);

   goto1(*_x+5, *_y+2);
   hi_vid();

   return b;
}



void end_popup(char *b, int *_x, int *_y, int *_w)
{
   n_vid(); 
   restore_screen(*_x, *_y, *_w, 5, b);
}



void alert_box(char *s)        /* alert the user to an error */
{
   char *b;
   int x, y, w;

   w = strlen(s) + 20;
   b = prepare_popup("Error", &x, &y, &w);
   hide_c();
   mywrite(s);
   clear_keybuf();
   while (m_b)
      poll_mouse();
   while ((!input_waiting()) && (!m_b))
      poll_mouse();
   if (input_waiting())
      input_char();
   else
      while (m_b)
	 poll_mouse();
   show_c();
   end_popup(b, &x, &y, &w);
}



char *err()
{
   switch (errno) {
      case EACCES:         return "Permission denied";
      case EAGAIN:         return "Resource temporarily unavailable";
      case EBADF:          return "Bad file descriptor";
      case EBUSY:          return "Resource busy";
      case EDEADLK:        return "Resource deadlock avoided";
      case EEXIST:         return "File exists";
      case EFAULT:         return "Bad address";
      case EFBIG:          return "File too big";
      case EINTR:          return "Interrupted system call";
      case EINVAL:         return "Invalid argument";
      case EIO:            return "Input or output error";
      case EISDIR:         return "Is a directory";
      case EMFILE:         return "Too many open files";
      case EMLINK:         return "Too many links";
      case ENAMETOOLONG:   return "File name too long";
      case ENFILE:         return "Too many open files in system";
      case ENODEV:         return "No such device";
      case ENOENT:         return "No such file or directory";
      case ENOEXEC:        return "Unable to execute file";
      case ENOLCK:         return "No locks available";
      case ENOMEM:         return "Not enough memory";
      case ENOSPC:         return "No space left on drive";
      case ENOSYS:         return "Function not implemented";
      case ENOTDIR:        return "Not a directory";
      case ENOTEMPTY:      return "Directory not empty";
      case ENOTTY:         return "Inappropriate I/O control operation";
      case ENXIO:          return "No such device or address";
      case EPERM:          return "Operation not permitted";
      case EPIPE:          return "Broken pipe";
      case EROFS:          return "Read-only file system";
      case ESPIPE:         return "Invalid seek";
      case ESRCH:          return "No such process";
      case EXDEV:          return "Improper link";
#if ENMFILE != ENOENT
      case ENMFILE:        return "No such file or directory";
#endif
      default:             return "Error";
   }
}



int myatoi(char *s)
{
   int x;      /* the number */
   int base;   /* number base */
   int neg = FALSE;

   #define IS_DIGIT(c)  (((c >= '0') && (c <= ((base==8) ? '7' : '9'))) || \
			 ((base==16) && (c >= 'a') && (c <= 'f')))

   if (*s=='-') {
      neg = TRUE;
      s++;
   }

   if (*s=='0') {
      s++;
      if (*s=='x') {    /* 0x<hex number> */
	 s++;
	 base = 16;
      }
      else
	 base = 8;      /* 0<octal number> */
   }
   else
      base = 10;        /* <decimal number> */

   x = 0;

   while (IS_DIGIT(tolower(*s))) {      /* work through the string */
      x *= base;
      x += VALUE(tolower(*s));
      s++;
   }

   return neg ? -x : x;
}



void myitoa(int i, char *s, int b)
{
   if (b == 10)
      sprintf(s, "%d", i);
   else if (b == 16)
      sprintf(s, "%X", i);
   else
      strcpy(s, "itoa oops!");
}



int mystricmp(char *s, char *t)
{
   while (tolower(*s) == tolower(*t)) {
      if (!*s)
	 return 0;

      s++;
      t++;
   }

   return tolower(*s) - tolower(*t);
}



int mystrnicmp(char *s, char *t, int n)
{
   if (!n)
      return 0;

   do {
      if (tolower(*s) != tolower(*t++))
	 return tolower(*s) - tolower(*--t);

      if (!*s++)
	 break;

   } while (--n);

   return 0;
}



void mystrlwr(char *s)
{
   while (*s) {
      *s = tolower(*s);
      s++;
   }
}



void mystrupr(char *s)
{
   while (*s) {
      *s = toupper(*s);
      s++;
   }
}



int is_number(char c)
{
   /* check if a character is a digit, for input_number() */

   c = tolower(c);

   return (((c>='0') && (c<='9')) ||
	   ((c>='a') && (c<='f')) ||
	   (c=='x'));
}



int is_all_numbers(char *s)
{
   while (*s) {
      if (!is_number(*s))
	 return FALSE;
      s++;
   }
   return TRUE;
}



int is_filechar(char c)
{
   /* check if a character is valid in a filename, for fn_open() */
   if (_USE_LFN) {
      return ((c>=' ') && (c<127) && (c!='"') && (c!='\'') &&
	      (c!='<') && (c!='>'));
   }
   else {
      c = tolower(c);
      return (((c>='0') && (c<='9')) || ((c>='a') && (c<='z')) ||
	      (c=='_') || (c=='.') || (c==':') || (c=='\\') || (c=='/') ||
	      (c=='?') || (c=='*'));
   }
}



int is_filechar_nospace(char c)
{
   return ((is_filechar(c)) && (c != ' '));
}



int is_asciichar(char c)
{
   return ((c>=' ') && (c<127));
}



int is_anychar(char c)
{
   return (c!=0);
}



int input_cancelled;


int input_number(char *prompt, int i)
{
   /* call input_text() and then convert the string to an integer */

   char buf[21];

   if (i>0)
      itoa(i, buf, 10);
   else
      buf[0]=0;

   if (input_text(prompt, buf, 20, is_number)==ESC) {
      input_cancelled = TRUE;
      return i;
   }

   input_cancelled = FALSE;
   return atoi(buf);
}



int do_input_text(char *buf, int size, int width, int (*valid)(char), int (*proc)(int, char *, int *, int *), int mouse_flag, int (*mouse_proc)(int x, int y, char *buf, int *oldlen))
{
   KEYPRESS k;
   int key;
   int askey;
   int ret;
   int x, y, len, cpos;
   int oldlen, c;
   int gap;
   int sel = TRUE;

   x = x_pos;
   y = y_pos;
   len = oldlen = cpos = strlen(buf);

   n_vid();

   do {
      goto1(x, y);

      if ((width > 0) && (len > width)) {
	 gap = len - width;

	 pch('.');
	 pch('.');
	 pch('.');
	 pch(' ');

	 mywrite(buf+gap+4);
      }
      else {
	 gap = 0;

	 mywrite(buf);
      }

      hi_vid();

      for (c=len-gap; c < ((width > 0) ? width : oldlen); c++)
	 pch(' ');

      oldlen = len;

      if (cpos-gap > 0)
	 goto2(x+cpos-gap, y);
      else
	 goto2(x, y);

      show_c();

      if (mouse_flag) {
	 display_mouse();
	 while (m_b)
	    poll_mouse();
	 while (TRUE) {
	    if (input_waiting()) {
	       k = input_char();
	       key = k.key;
	       if ((ascii(key) == CR) && (k.flags & KF_CTRL))
		  key = LF;
	       if (key)
		  break;
	    }
	    poll_mouse();
	    if ((m_b) && (mouse_proc)) {
	       hide_c(); 
	       key = (*mouse_proc)(x, y, buf, &oldlen);
	       len = cpos = strlen(buf);
	       sel = FALSE;
	       show_c();
	       if (key > 0)
		  break;
	    }
	 }
	 hide_mouse();
      }
      else {
	 while (m_b)
	    poll_mouse();

	 while ((!input_waiting()) && (!m_b))
	    poll_mouse();

	 if (m_b) {
	    if (m_b & 2)
	       key = ESC;
	    else
	       key = CR;
	 }
	 else {
	    k = input_char();
	    key = k.key;
	    if ((ascii(key) == CR) && (k.flags & KF_CTRL))
	       key = LF;
	 }
      }

      if ((!proc) || (!(*proc)(key, buf, &len, &cpos))) {

	 if ((key==CTRL_G) || (key==CTRL_C))
	    askey=ESC;
	 else
	    askey=ascii((char)key);

	 if ((askey==ESC) || (askey==CR) || (askey==LF)) {
	    ret = askey;
	    break;
	 }

	 if (key==LEFT_ARROW) {
	    if (cpos > 0)
	       cpos--;
	 }
	 else if (key==RIGHT_ARROW) {
	    if (cpos < len)
	       cpos++;
	 }
	 else if (key==K_HOME) {
	    cpos = 0;
	 }
	 else if (key==K_END) {
	    cpos = MAX(len, 0);
	 } 
	 else {
	    if ((sel) && ((key==K_DELETE) || (askey==BACKSPACE) || 
		  ((*valid)(askey)))) {
	       len = cpos = 0;
	       buf[0] = 0;
	    }

	    if (key==K_DELETE) {
	       if (cpos < len) {
		  for (c=cpos; c<len; c++)
		     buf[c] = buf[c+1];
		  len--;
	       }
	    }
	    else if (askey==BACKSPACE) {
	       if (cpos > 0) {
		  cpos--;
		  for (c=cpos; c<len; c++)
		     buf[c] = buf[c+1];
		  len--;
	       }
	    }
	    else {
	       if ((len<size) && ((*valid)(askey))) {
		  len++;
		  for (c=len; c>cpos; c--)
		     buf[c] = buf[c-1];
		  buf[cpos++]=askey;
	       }
	    }
	 }

	 if (proc)
	    (*proc)(-1, buf, &len, &cpos);
      }

      sel = FALSE;

   } while (TRUE);

   n_vid();
   return ret;
}



int input_text(char *prompt, char *buf, int size, int (*valid)(char))
{
   /* Input a line of text. buf contains the initial text string, and 
      is where the result will be stored. size is the maximum length of 
      the string, and valid will be called to determine if a key should 
      be inserted or not. input_text() will return the exit key */

   int ret;
   char *b;
   int _x, _y, _w;

   _w = size + 8;
   b = prepare_popup(prompt, &_x, &_y, &_w);
   ret = do_input_text(buf, size, -1, valid, NULL, FALSE, NULL);
   end_popup(b, &_x, &_y, &_w);
   return ret;
}



char *find_extension(char *s)
{
   char *p;

   p=s+strlen(s);

   do {
      p--;
      if (*p=='.') {
	 return p+1;
      }
   } while ((p>s)&&(*p!='\\')&&(*p!='/'));

   return s+strlen(s);
}



void remove_extension(char *s)
{
   char *p;

   p=s+strlen(s);

   do {
      p--;
      if(*p=='.') {
	 *p=0;
	 return;
      }
   } while((p>s) && (*p!='\\')&&(*p!='/'));
}



void append_backslash(char *s)
{
   char *p;

   if (*s!=0) {
      p=s+strlen(s);
      if ((*(p-1)!='\\') && (*(p-1)!='/')) {
   #if (defined TARGET_DJGPP) || (defined TARGET_WIN)
	 *p++='\\';
   #else
	 *p++='/';
   #endif
	 *p=0;
      }
   }
}



KEYPRESS macro[MACRO_LENGTH];   /* keystrokes recorded as a macro */
int macro_size=0;               /* number of keys in the macro */
int macro_pos=0;                /* position when playing a macro */
int macro_mode=0;               /* recording or playing? */

#define MACRO_RECORD    1
#define MACRO_PLAY      2
#define MACRO_FINISHED  3

char macro_already[] = "Already in macro";


#define UNGET_SIZE   32         /* for un_getc() */

int _unget[UNGET_SIZE];
int unget_count = 0;



KEYPRESS input_char()
{
   KEYPRESS c;

   /* all keyboard input should come through this function instead of
      calling gch() directly. This function normally just calls getc(),
      but will insert codes from the macro buffer instead if a macro is
      being executed */

   if (macro_mode == MACRO_FINISHED)
      macro_mode = 0;

   #if (defined TARGET_CURSES) || (defined TARGET_WIN)
      if (macro_mode)
	 nosleep = TRUE;
      else
	 nosleep = FALSE;
   #endif

   if (macro_mode == MACRO_PLAY) {
      c = macro[macro_pos++];
      if (macro_pos >= macro_size)
	 macro_mode = MACRO_FINISHED;
   }
   else {
      if (unget_count > 0) {
	 int l;
	 c.key = _unget[0];
	 c.flags = 0;
	 unget_count--;
	 for (l=0; l<unget_count; l++)
	    _unget[l] = _unget[l+1];
      }
      else {
	 c.key = gch();
	 c.flags = modifiers();
      }
   }

   if (macro_mode == MACRO_RECORD) {
      if (macro_size >= MACRO_LENGTH) {
	 macro_mode = 0;
	 clear_keybuf();
	 alert_box("Macro too long");
      }
      else
	 macro[macro_size++] = c;
   }

   return c;
}



void un_getc(int l)
{
   int c;

   if (unget_count < UNGET_SIZE) {
      for (c=unget_count; c>0; c--)
	 _unget[c] = _unget[c-1];
      unget_count++;
      _unget[0] = l;
   }
}



int input_waiting()
{
   /* check if characters are waiting, so we don't update screen if stuff
      still needs processing */

   if (auto_input_waiting())
      return TRUE;

   if (unget_count > 0)
      return TRUE;

   return keypressed();         /* is keyboard input waiting? */
}



int auto_waiting_flag = FALSE;  /* flag to suppress screen redraws */


int auto_input_waiting()
{
   if (running_macro())         /* are we running a macro? */
      return TRUE;

   if (auto_waiting_flag)
      return TRUE;

   return (repeat_count != 0);  /* are commands being repeated? */
}



int running_macro()
{
   return ((macro_mode == MACRO_PLAY) || (macro_mode == MACRO_FINISHED));
}



int recording_macro()
{
   return (macro_mode == MACRO_RECORD);
}



void fn_macro_s()            /* start recording a macro */
{
   if (macro_mode != 0) {
      fn_macro_e();
   }
   else {
      macro_mode = MACRO_RECORD;
      macro_size = 0;
      disp_dirty();
   }
}



void fn_macro_e()            /* end recording a macro */
{
   if (macro_mode == 0) {
      alert_box("Not recording a macro");
   }
   else {
      if (macro_mode == MACRO_RECORD) {
	 strcpy(message,"Macro recorded");
	 display_message(MESSAGE_KEY);
      }
      macro_mode = 0;
   }
}



void fn_macro_p()            /* play a macro */
{
   if (macro_mode != 0)
      alert_box(macro_already);
   else if (macro_size <= 0)
      alert_box("No macro recorded");
   else {
      macro_mode = MACRO_PLAY;
      macro_pos = 0;
   }
}



void itohex(char *s, int c)             /* output a two digit hex number */
{
   *s++ = hex_digit[(c&0xf0)>>4];
   *s++ = hex_digit[c&0x0f];
   *s=0;
}



char *get_fname(char *s)
{
  /* returns a pointer to a filename, without the path */

  char *pos;

  for (pos=s+strlen(s)-1; pos>=s; pos--)
    if ((*pos=='\\') || (*pos=='/'))
      return pos + 1;

  return s;
}



int ext_in_list(char *fname, char *list, BUFFER *buf)
{
   /* returns true if the extension of the filename is present in the
      list of file extensions, for file type checking */

   char *p = list;
   int buffer_pos;
   char buffer[10];
   char tmp[4096];
   char *ext = find_extension(fname);
   int i, j;

   if ((!ext) || (!ext[0])) {
      if ((buf) && (buf->start) && (buf->start->text)) {
	 memmove(tmp, buf->start->text, buf->start->length);
	 tmp[buf->start->length] = 0;

	 if ((tmp[0] == '#') && (tmp[1] == '!')) {
	    i = 2;

	    while (tmp[i] == ' ')
	       i++;

	    j = 0;

	    while ((isalnum(tmp[i])) || (tmp[i] == '/')) {
	       if (tmp[i] == '/')
		  j = i;
	       i++;
	    }

	    if (j) {
	       tmp[i] = 0;
	       ext = tmp+j+1;
	    }
	 }
      }
   }

   while (*p) {
      while ((*p==' ') || (*p==',') || (*p==';'))
	 p++;

      if (*p) {
	 buffer_pos=0;
	 buffer[0]=0;
	 while ((buffer_pos<10) && (*p) &&
		(*p!=' ') && (*p!=',') && (*p!=';')) {
	    buffer[buffer_pos++]=*(p++);
	    buffer[buffer_pos]=0;
	 }
	 if (((buffer[1]==0) && ((buffer[0]=='?') || (buffer[0]=='*'))) ||
	     (stricmp(buffer,ext)==0))
	    return TRUE;
      }
   }

   return FALSE;
}



#ifndef DJGPP


void my_fixpath(char *in, char *out)
{
   int c1, i;
   char *p;

   /* if the path is relative, make it absolute */
   if (*in == '~') {
      p = getenv("HOME");
      if (p) {
	 strcpy(out, p);
	 in++;
      }
      else
	 out[0] = 0;
   }
   else if ((*in != '/') && (*in != '\\') && (in[1] != ':')) {
      getcwd(out, 256);
      append_backslash(out);
   }
   else
      out[0] = 0;

   /* add our path, and clean it up a bit */
   strcat(out, in);

   for (i=0; out[i]; i++)
      if (out[i] == '\\')
	 out[i] = '/';

   /* remove duplicate slashes */
 #ifdef TARGET_WIN
   if (strlen(out) > 2) {
      while ((p = strstr(out+1, "//")) != NULL)
	 memmove(p, p+1, strlen(p));
   }
 #else
   while ((p = strstr(out, "//")) != NULL)
      memmove(p, p+1, strlen(p));
 #endif

   /* remove /./ patterns */
   while ((p = strstr(out, "/./")) != NULL)
      memmove(p, p+2, strlen(p)-1);

   /* collapse /../ patterns */
   while ((p = strstr(out, "/../")) != NULL) {
      for (i=0; out+i < p; i++)
	 ;

      while (--i > 0) {
	 c1 = out[i];

	 if (c1 == '/')
	    break;
      }

      if (i < 0)
	 i = 0;

      p += 4;
      memmove(out+i+1, p, strlen(p)+1);
   }
}


#define _fixpath(f, b)  my_fixpath(f, b)


#endif



void cleanup_filename(char *f)
{
   char b[256];
   int c;

   if (f[0]) {
      _fixpath(f, b);

      c = f[strlen(f)-1];

      if ((c=='\\') || (c=='/'))
	 append_backslash(b);

      for (c=0; b[c]; c++) {
   #ifdef TARGET_DJGPP
	 if (_preserve_fncase())
	    f[c] = b[c];
	 else
	    f[c] = tolower(b[c]);

	 if (f[c] == '/')
	    f[c] = '\\';
   #elif defined TARGET_WIN
	 f[c] = b[c];

	 if (f[c] == '/')
	    f[c] = '\\';
   #else
	 f[c] = b[c];

	 if (f[c] == '\\')
	    f[c] = '/';
   #endif
      }

      f[c] = 0;

   #ifdef TARGET_WIN
      if (f[1] == ':')
	 f[0] = tolower(f[0]);
   #endif
   }

   errno = 0;
}

