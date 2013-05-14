/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Screen redraw and stuff for moving around the document
 */


#include <ctype.h>
#include <time.h>

#include "fed.h"


#ifdef _MSC_VER
   #define static_inline
#else
   #define static_inline   static inline
#endif


/* store a bunch of information about each line on the screen, so we 
   can minimise the amount of screen space that needs updating */

typedef struct DISP_LINE
{
   int start;                /* how much is the line indented? */
   int end;                  /* position of the last non-space character */
   int dirty;                /* does the line need repainting? */
} DISP_LINE;


DISP_LINE *disp=NULL;        /* an array of structures, one for each line */

int disp_cline=0;            /* the screen line that the cursor is on */

char message[160];           /* the text to display on the bottom line */
int message_flags=0;         /* flags the status of the bottom line */

int disp_gap=1;              /* gap at bottom of the screen */

int redraw_bar = FALSE;      /* does the scroll bar need redrawing? */

#define MAX_BANNER_HEIGHT    16

char *banner_line[MAX_BANNER_HEIGHT];
int banner_height=0; 

char current_title[256] = "";

#define BOTTOM    (screen_h - MAX(disp_gap, banner_height+1))



void read_banner_info()      /* load the banner message */
{
   char fn[256];
   char buf[256];
   MYFILE *f;

   banner_height = 0;

   strcpy(fn, exe_path);
   strcpy(get_fname(fn), MESSAGE_FILE);

   f=open_file(fn, FMODE_READ); 

   if (f) {
      while (errno == 0) {
	 read_string(f, buf);
	 banner_line[banner_height] = malloc(strlen(buf)+1);
	 strcpy(banner_line[banner_height], buf);
	 banner_height++;
	 if (banner_height >= MAX_BANNER_HEIGHT)
	    break;
      }

      close_file(f);
   }

   errno = 0;
}



void disp_init()             /* setup the display manager */
{
   if ((disp=malloc(sizeof(DISP_LINE)*screen_h))==NULL) {
      errno=ENOMEM;
      return;
   }

   message[0]=0;
   dirty_everything();
}



void disp_exit()             /* close down the display manager */
{
   if (disp)
      free(disp);
}



void disp_dirty()            /* dirty the screen */
{
   int c;

   for (c=(config.show_menu ? 1 : 0); c<screen_h; c++)
      disp[c].dirty=TRUE;
}



void dirty_everything()       /* dirty, and DISP_LINE structures are wrong */
{
   int c;

   for (c=0;c<screen_h;c++) {
      disp[c].start=0;
      disp[c].end=screen_w;
      disp[c].dirty=TRUE;
   }
}



void dirty_cline()            /* dirty the line that the cursor is on */
{
   disp[disp_cline].dirty = TRUE;
}



void dirty_line(int l)        /* dirty the line cline+l */
{
   l += disp_cline;
   if ((l>=0) && (l<screen_h))
      disp[l].dirty = TRUE;
}



void dirty_message()
{
   disp[screen_h-1].dirty = TRUE;
}



void display_big_message(int flag, char *title)
{
   int c;

   if (flag) {
      disp_gap = 3;
      draw_box(title, 0, screen_h-3, 
	       (config.show_bar ? screen_w-1 : screen_w), 3, FALSE);
   }
   else {
      disp_gap = 1;
      for (c=screen_h-3; c<screen_h; c++) {
	 disp[c].start=0;
	 disp[c].end=screen_w;
	 disp[c].dirty=TRUE;
      }
   }
}



void do_display_message(int flag)
{
   int l = screen_h - (disp_gap==1 ? 1 : 2); 

   goto1(disp_gap-1,l);
   hi_vid();
   mywrite(message);
   if (disp_gap==1)
      del_to_eol();
   else
      while(x_pos < screen_w-2)
	 pch(' ');
   n_vid();

   message_flags=flag;
   disp[l].start=0;
   disp[l].end=screen_w;
   if (flag)
      disp[l].dirty=FALSE;
   else
      disp[l].dirty=TRUE;
}



void display_message(int flag)   /* show a message on the status line */
{
   if (config.show_bar)
      screen_w--;

   do_display_message(flag);

   if (config.show_bar)
      screen_w++;
}



void display_new_buffer()
{
   /* called upon switching to a new buffer. Sets up the message line,
      and invalidates the screen */

   disp[1].end = screen_w;
   disp_dirty();
   cursor_move_line();

   if ((errno==0 && (!config.show_pos))) {
      set_buffer_message(message, 0, TRUE);
      message_flags=MESSAGE_KEY;
      disp[screen_h-1].dirty=TRUE;
   }
   else
      errno=0;      /* get rid of the error next time */
}



void set_buffer_message(char *s, int x, int flag)
{
   /* puts information about the buffer in the message line */

   if (buffer[x]->flags & BUF_CHANGED)
      strcpy(s,"* ");
   else
      strcpy(s,"- ");

   if (flag) {
      strcat(s, VS);
      strcat(s," - ");
   }
   else {
      char tmp[5];

      tmp[0] = (buffer[x]->flags & BUF_INDENT) ? 'a' : '-';
      tmp[1] = (buffer[x]->wrap_col > 0) ? 'w' : '-';
      tmp[2] = (buffer[x]->flags & BUF_UNIX) ? 'u' : 'd';
      tmp[3] = ' ';
      tmp[4] = 0;

      strcat(s,tmp);
   }

   if (buffer[x]->flags & BUF_NEW)
      strcat(s,"new file - ");
   else if (buffer[x]->flags & BUF_BROWSE)
      strcat(s,"browser file - ");
   else if (buffer[x]->flags & BUF_KILL)
      strcat(s,"kill buffer - ");

   strcat(s,buffer[x]->name);

   if (buffer[x]->flags & BUF_BINARY)
      strcat(s," (bin)");
   else if (buffer[x]->flags & BUF_HEX)
      strcat(s," (hex)");

   if (buffer[x]->flags & BUF_READONLY)
      strcat(s," (read only)");

   if (buffer[x]->flags & BUF_TRUNCATED)
      strcat(s," (truncated)");

   if (buffer[x]->flags & BUF_WRAPPED) {
      strcat(s," (long lines were split)");
   }
}



int get_buffer_line(BUFFER *b)
{
   LINE *l;
   int c;

   if ((buffer[0]->cached_line > 0) && 
       (buffer[0]->_cached_line == buffer[0]->c_line))
      return buffer[0]->cached_line;

   if ((buffer[0]->cached_line > 0) &&
       (buffer[0]->_cached_line)) {       /* try plus or minus a line... */

      if (buffer[0]->_cached_line == buffer[0]->c_line->next) {
	 buffer[0]->cached_line--;
	 buffer[0]->_cached_line = buffer[0]->c_line;
	 return buffer[0]->cached_line;
      }

      if (buffer[0]->_cached_line->next == buffer[0]->c_line) {
	 buffer[0]->cached_line++;
	 buffer[0]->_cached_line = buffer[0]->c_line;
	 return buffer[0]->cached_line;
      }
   }

   l = buffer[0]->start;
   c = 1;

   while ((l) && (l != buffer[0]->c_line)) {
      c++;
      l = l->next;
   }

   buffer[0]->cached_line = c;
   buffer[0]->_cached_line = buffer[0]->c_line;
   return c;
}



void get_bar_data(BUFFER *b, int *pos, int *lines, int *top, int *h)
{
   LINE *l;
   int s;

   if ((buffer[0]->cached_top >= 0) && (buffer[0]->cached_size >= 0) &&
       (buffer[0]->_cached_top == buffer[0]->top)) {
      *pos = buffer[0]->cached_top;
      *lines = buffer[0]->cached_size;
   }
   else if ((buffer[0]->cached_top >= 0) && (buffer[0]->cached_size >= 0) &&
	    (buffer[0]->_cached_top) && 
	    (buffer[0]->_cached_top == buffer[0]->top->fold)) {
      buffer[0]->cached_top--;
      buffer[0]->_cached_top = buffer[0]->top;
      *pos = buffer[0]->cached_top;
      *lines = buffer[0]->cached_size;
   }
   else if ((buffer[0]->cached_top >= 0) && (buffer[0]->cached_size >= 0) &&
	    (buffer[0]->_cached_top) && 
	    (buffer[0]->_cached_top == buffer[0]->top->prev)) {
      buffer[0]->cached_top++;
      buffer[0]->_cached_top = buffer[0]->top;
      *pos = buffer[0]->cached_top;
      *lines = buffer[0]->cached_size;
   }
   else {
      l = buffer[0]->start;
      s = 0;

      while (l) {
	 if (l == buffer[0]->top) {
	    buffer[0]->cached_top = *pos = s;
	    buffer[0]->_cached_top = l;
	    if (buffer[0]->cached_size >= 0) {
	       s = buffer[0]->cached_size;
	       break;
	    }
	 }
	 s++;
	 l = l->fold;
      }

      buffer[0]->cached_size = *lines = s;
   }

   if (*lines > 0) {
      *top = (*pos * (screen_h-(config.show_menu ? 2:1)) + *lines/2) / *lines;
      if ((*top==0) && (*pos > 0))
	 *top=1;

      *h = ((screen_h-(config.show_menu ? 2:1)) * screen_h + *lines/2) / *lines;
      if (*h<=0)
	 *h=1;
   }
   else {
      *top = 0;
      *h = screen_h;
   }
}



void go_to_line(int line)
{
   LINE *l;
   int c;

   l = buffer[0]->start;
   c = 1;

   while ((c < line) && (l->next)) {
      l = l->next;
      c++;
   }

   unfold(l);
   buffer[0]->c_line = buffer[0]->_cached_line = l;
   buffer[0]->cached_line = c;
   buffer[0]->c_pos = buffer[0]->old_c_pos = 0;
   cursor_move_line();
}



void go_to_browse_line(int line)
{
   LINE *l;

   l=buffer[0]->start;
   while (l) {
      if (l->line_no == line)
	 goto found_it;
      l=l->next;
   }

   l=buffer[0]->start;
   while ((l->line_no < line) && (l->next))
      l = l->next;

   found_it:
   unfold(l);
   buffer[0]->c_line=l;
   buffer[0]->c_pos = buffer[0]->old_c_pos = 0;
   cursor_move_line();
}



void fn_goto()               /* goto a specified line */
{
   int num;

   num=input_number("Goto line", 0);
   n_vid();

   if (num>0)
      go_to_line(num);
}



void display_position()
{
   set_buffer_message(message,0,FALSE);
   strcat(message," - line ");
   itoa(get_buffer_line(buffer[0]), message+strlen(message), 10);
   strcat(message," - col ");
   itoa(get_line_length(buffer[0]->c_line, buffer[0]->c_pos)+1, message+strlen(message), 10);

   if (buffer[0]->c_pos+1 < 10)
      strcat(message, " ");
   strcat(message," - 0x");

   if (buffer[0]->c_pos < buffer[0]->c_line->length) {
      itohex(message+strlen(message),
	     char_from_line(buffer[0]->c_line, buffer[0]->c_pos));
      strcat(message," (");
      itoa(char_from_line(buffer[0]->c_line,buffer[0]->c_pos),
	   message+strlen(message), 10);

      strcat(message,")");
   }
   else
      strcat(message,"-- (--)");

   do_display_message(0);
}



void fn_position()           /* display the current position in the file */
{
   #define INFO_W    60
   #define INFO_H    11

   int x = (screen_w - INFO_W) / 2 - 1;
   int y = (screen_h - INFO_H) / 2;
   int c;
   char b[80];
   LINE *l;
   int lines = 0;
   int line = 0;
   int chars = 0;
   int words = 0;
   int bytes = 0;
   int in_word;

   draw_box("Information", x, y, INFO_W, INFO_H, TRUE);
   hi_vid();
   hide_c();

   l = buffer[0]->start;
   while (l) {
      lines++;

      if (l == buffer[0]->c_line)
	 line = lines;

      bytes += l->length;
      if ((l->next) && (!(buffer[0]->flags & BUF_BINARY)))
	 bytes += (buffer[0]->flags & BUF_UNIX) ? 1 : 2;    /* cr/lf */

      in_word = FALSE;
      for (c=0; c<l->length; c++) {
	 if ((l->text[c] != ' ') && (l->text[c] != '\t')) {
	    chars++;
	    if (!in_word) {
	       words++;
	       in_word = TRUE;
	    }
	 }
	 else
	    in_word = FALSE;
      }

      l = l->next; 
   }

   goto1(x+6, y+2);
   set_buffer_message(b, 0, FALSE);
   mywrite(b+6);

   goto1(x+6, y+3);
   mywrite("Lines: ");
   itoa(lines, b, 10);
   mywrite(b);

   goto1(x+6, y+4);
   mywrite("Words: ");
   itoa(words, b, 10);
   mywrite(b);

   goto1(x+6, y+5);
   mywrite("Chars: ");
   itoa(chars, b, 10);
   mywrite(b);

   goto1(x+6, y+6);
   mywrite("Bytes: ");
   itoa(bytes, b, 10);
   mywrite(b);

   goto1(x+6, y+7);
   mywrite("Position: line ");
   itoa(line, b, 10);
   mywrite(b);
   mywrite(", column ");
   itoa(buffer[0]->c_pos+1, b, 10);
   mywrite(b);

   goto1(x+6, y+8);
   mywrite("Character: 0x");
   if (buffer[0]->c_pos < buffer[0]->c_line->length) {
      itohex(b, char_from_line(buffer[0]->c_line, buffer[0]->c_pos));
      mywrite(b);
      mywrite(" (");
      itoa(char_from_line(buffer[0]->c_line,buffer[0]->c_pos), b, 10);
      mywrite(b);
      mywrite(")");
   }
   else
      mywrite("-- (--)");

   while (m_b)
      poll_mouse();
   clear_keybuf();

   while ((!input_waiting()) && (!m_b))
      poll_mouse();

   dirty_everything();
   clear_keybuf(); 
   n_vid();
   show_c();
}



void fn_left()               /* move the cursor left one character */
{
   if (buffer[0]->c_pos==0) {           /* wrap on to new line */
      if (buffer[0]->c_line->prev) {
	 fn_up();
	 fn_line_end();
      }
   }
   else {
      buffer[0]->c_pos--;
      buffer[0]->old_c_pos=get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
      cursor_move();
   }
}



void fn_right()              /* move the cursor right one character */
{
   if (buffer[0]->c_pos==buffer[0]->c_line->length) {
      if (buffer[0]->c_line->fold) {            /* wrap on to new line */
	 fn_down();
	 fn_line_start();
      }
   }
   else {
      buffer[0]->c_pos++;
      buffer[0]->old_c_pos=get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
      cursor_move();
   }
}



void fn_line_start()         /* go to the start of the line */
{
   buffer[0]->c_pos = buffer[0]->old_c_pos = 0;
   cursor_move();
}



void fn_line_end()           /* go to the end of the line */
{
   buffer[0]->c_pos = buffer[0]->c_line->length;
   buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
   cursor_move();
}



void fn_start()              /* go to start of file */
{
   buffer[0]->c_line = buffer[0]->start;
   buffer[0]->c_pos = buffer[0]->old_c_pos = 0;
   cursor_move_line();
}



void fn_end()                /* go to end of file */
{
   LINE *l;

   l=buffer[0]->start;

   while (l->fold) 
      l=l->fold;

   buffer[0]->c_line = l;
   buffer[0]->c_pos = buffer[0]->c_line->length;
   buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
   cursor_move_line();
}



void fn_up()                 /* move up one line */
{
   if (buffer[0]->c_line->prev) {
      buffer[0]->c_line = buffer[0]->c_line->prev;
      buffer[0]->c_pos = 0;
      while ((buffer[0]->c_pos < buffer[0]->c_line->length) &&
	     (get_line_length(buffer[0]->c_line, buffer[0]->c_pos) < buffer[0]->old_c_pos))
	 buffer[0]->c_pos++;
      cursor_move_line();
   }
}



void fn_down()               /* move down one line */
{
   if (buffer[0]->c_line->fold) {
      buffer[0]->c_line = buffer[0]->c_line->fold;
      buffer[0]->c_pos = 0;
      while ((buffer[0]->c_pos < buffer[0]->c_line->length) &&
	     (get_line_length(buffer[0]->c_line, buffer[0]->c_pos) < buffer[0]->old_c_pos))
	 buffer[0]->c_pos++;
      cursor_move_line();
   }
}



void fn_screen_up()          /* move up one screen */
{
   int c=0;

   while ((c<BOTTOM-1) && (buffer[0]->c_line->prev)) {
      buffer[0]->c_line = buffer[0]->c_line->prev;
      if (buffer[0]->top->prev)
	 buffer[0]->top = buffer[0]->top->prev;
      c++;
   }

   buffer[0]->c_pos = 0;
   while ((buffer[0]->c_pos < buffer[0]->c_line->length) &&
	  (get_line_length(buffer[0]->c_line, buffer[0]->c_pos) < buffer[0]->old_c_pos))
      buffer[0]->c_pos++;
   cursor_move_line();
   disp_dirty();
}



void fn_screen_down()        /* move down one screen */
{
   int c=0;

   while ((c<BOTTOM-1) && (buffer[0]->c_line->fold)) {
      buffer[0]->c_line = buffer[0]->c_line->fold;
      if (buffer[0]->top->fold)
	 buffer[0]->top = buffer[0]->top->fold;
      c++;
   }

   buffer[0]->c_pos = 0;
   while ((buffer[0]->c_pos < buffer[0]->c_line->length) &&
	  (get_line_length(buffer[0]->c_line, buffer[0]->c_pos) < buffer[0]->old_c_pos))
      buffer[0]->c_pos++;
   cursor_move_line();
   disp_dirty();
}



void check_cline()
{
   /* re-calculates the disp_cline varaible, moving the buffer.top 
      position if required */

   LINE *l;
   int c;

   l=buffer[0]->top;
   for (c=(config.show_menu ? 1 : 0); c<BOTTOM; c++) { 
      if (l==buffer[0]->c_line) {
	 disp_cline=c;
	 return;
      }
      if (l)
	 l=l->fold;
   }

   if (l==buffer[0]->c_line) {            /* check for scroll down */
      disp_dirty();
      buffer[0]->top = buffer[0]->top->fold;
      disp_cline=BOTTOM-1;
      return;
   }

   if (buffer[0]->top->prev==buffer[0]->c_line) {       /* scroll up */
      disp_dirty();
      buffer[0]->top = buffer[0]->top->prev;
      disp_cline = config.show_menu ? 1 : 0;
      return;
   }

   l = buffer[0]->c_line;      /* oh well, have to completely recalculate */
   c = config.show_menu ? 1 : 0;
   while (l->prev) {
      l=l->prev;
      c++;
      if (c==((BOTTOM)/2))
	 break;
   }

   buffer[0]->top=l;
   disp_cline=c;
   disp_dirty();
}



void disp_move_on_screen(LINE *start, LINE *end)
{
   LINE *l = buffer[0]->top;
   int c = 0;

   while (l != end) {
      c++;
      if (c > BOTTOM) {
	 if ((buffer[0]->top != buffer[0]->c_line) && (buffer[0]->top->fold)) {
	    buffer[0]->top = buffer[0]->top->fold;
	    c--;
	 }
      }
      l = l->fold;
      if (l == NULL)
	 break;
   }

   cursor_move_line();
   disp_dirty();
}



void cursor_move_line()   /* sort out life, when changing the cursor line */
{
   if (message_flags==MESSAGE_LINE) {
      message_flags=0;                  /* get rid of the message */
      disp[screen_h-1].dirty = TRUE;
   }

   if (buffer[0]->sel_line)
      selection_change();

   check_cline();                       /* check the new position */
   cursor_move();                       /* scroll the new line? */
}



void selection_change()
{
   /* Look for a new selection offset. Start looking at the cursor line,
      since selection line is likely to be close to cursor */

   int c;
   LINE *l;
   LINE *x = buffer[0]->sel_line;

   c = 0;
   l = buffer[0]->c_line;

   if (buffer[0]->sel_offset < 0) {
      while (l) {                       /* start by looking forwards */
	 if (l==x)
	    goto found_it;
	 l=l->fold;
	 c--;
      }

      c = 0;
      l = buffer[0]->c_line;

      while (l) {                       /* no luck? Try looking backwards */
	 if (l==x)
	    goto found_it;
	 l=l->prev;
	 c++;
      }
   }
   else {
      while (l) {                       /* start by looking backwards */
	 if (l==x)
	    goto found_it;
	 l=l->prev;
	 c++;
      }

      c = 0;
      l = buffer[0]->c_line;

      while (l) {                       /* no luck? Try looking forwards */
	 if (l==x)
	    goto found_it;
	 l=l->fold;
	 c--;
      }
   }

   found_it:

   if (c==buffer[0]->sel_offset-1) {
      disp[disp_cline].dirty = TRUE;
      if (disp_cline>0)
	 disp[disp_cline-1].dirty = TRUE;
   }
   else
      if (c==buffer[0]->sel_offset+1)
	 disp[disp_cline].dirty = disp[disp_cline+1].dirty = TRUE;
      else
	 if (c != buffer[0]->sel_offset)
	    disp_dirty();

   buffer[0]->sel_offset = c;
}



void cursor_move()               /* check for horizontal scrolling */
{
   while ((get_line_length(buffer[0]->c_line, buffer[0]->c_pos) <= buffer[0]->hscroll) &&
	  (buffer[0]->hscroll > 0)) {
      buffer[0]->hscroll -= screen_w/2;
      disp_dirty();
   }

   while (get_line_length(buffer[0]->c_line, buffer[0]->c_pos) - buffer[0]->hscroll >= 
	  screen_w - (config.show_bar ? 2 : 1)) {
      buffer[0]->hscroll += screen_w/2;
      disp_dirty();
   }

   if (buffer[0]->sel_line)
      dirty_cline();
}



void dirty_bar()
{
   redraw_bar = TRUE;
}



void draw_bar()
{
   int lines, pos, top, h, c;

   get_bar_data(buffer[0], &pos, &lines, &top, &h);

   hi_vid();

   for (c=0; c<screen_h; c++) {
      goto1(screen_w-1, c);
      pch(((c<top) || (c>=top+h)) ? ' ' : BAR_CHAR);
   }

   n_vid();
   redraw_bar = FALSE;
}



void redisplay()
{
   /* main screen redraw routine */

   LINE *pos, *l;
   int c, c2;
   int comment_state = COMMENT_NONE;
   char *new_title = get_fname(buffer[0]->name);

   if (stricmp(new_title, current_title) != 0) {
      strcpy(current_title, new_title);
      set_window_title(current_title);
   }

   goto2(get_line_length(buffer[0]->c_line, buffer[0]->c_pos) - buffer[0]->hscroll, disp_cline);
   show_c2(buffer[0]->flags & BUF_OVERWRITE);

   n_vid();

   if (config.show_bar)
      screen_w--;

   if (config.show_menu) {
      if (disp[0].end > screen_w)
	 redraw_bar = TRUE;
      if (disp[0].dirty) {
	 draw_menu();
	 disp[0].start = 0;
	 disp[0].end = screen_w;
	 disp[0].dirty = FALSE;
      }
      c = 1;
   }
   else
      c = 0;

   pos=buffer[0]->top;
   l = previous_line(pos);
   if ((l) && (l->comment_state != COMMENT_UNKNOWN))
      comment_state = l->comment_state;

   while (c<BOTTOM) {   /* look at each line in turn */

      if (disp[c].dirty) {
	 if (disp[c].end > screen_w)
	    redraw_bar = TRUE;

	 if (display_line(c, pos, &comment_state))
	    for (c2 = c+1; c2<BOTTOM; c2++)
	       disp[c2].dirty = TRUE;
      }
      else if (pos)
	 comment_state = pos->comment_state;

      if (pos) {
	 if ((pos->next != pos->fold) && (pos->fold) && (buffer[0]->syntax)) {
	    l = previous_line(pos->fold);
	    if (l->comment_state != COMMENT_UNKNOWN)
	       comment_state = l->comment_state;
	    else
	       comment_state = COMMENT_NONE;
	 }

	 pos=pos->fold;
      }

      c++;
   }

   for (c2=0; (c2<banner_height) && (c<screen_h-disp_gap); c2++) {
      if (disp[c].end > screen_w)
	 redraw_bar = TRUE;

      if (disp[c].dirty) {
	 goto1(0,c);
	 hi_vid();
	 mywrite(banner_line[c2]);
	 del_to_eol();
	 n_vid();

	 disp[c].start=0;
	 disp[c].end=screen_w;
	 disp[c].dirty=FALSE;
      } 
      c++;
   }

   if (disp_gap==1) {
      if (disp[c].end > screen_w)
	 redraw_bar = TRUE;

      if (message_flags) {
	 if (disp[c].dirty)
	    do_display_message(message_flags);

	 if (message_flags==MESSAGE_KEY) {
	    message_flags=0;
	    disp[c].dirty=TRUE;
	 }
      }
      else if (recording_macro()) {
	 strcpy(message, "- recording macro - press ctrl+] to stop -");
	 do_display_message(0);
      }
      else if (config.show_pos) {
	 display_position();
      }
      else if (disp[c].dirty) {           /* draw a regular line */
	 display_line(c, pos, &comment_state);
      }
   }
   else {                                 /* draw tall input area */
      if (disp[screen_h-2].dirty)
	 do_display_message(message_flags);
   }

   if (config.show_bar) {
      screen_w++;
      if ((redraw_bar) || (buffer[0]->cached_size < 0) || 
	  (buffer[0]->_cached_top != buffer[0]->top) ||
	  (buffer[0]->cached_top < 0))
	 draw_bar();
   }
}



static_inline int check_keyword(SYNTAX *syn, unsigned char *s, int len)
{
   int c, i;
   KEYWORD *keyword;
   char *p1, *p2;

   unsigned char s0 = s[0];
   unsigned char s1 = (len <= 1) ? 0 : s[1];

   if (syn->case_sense) {           /* case sensitive keyword lookup */
      keyword = syn->keyword + KEYWORD_HASH(s0, s1);
      p1 = keyword->data;

      for (c=keyword->count; c>0; c--) {
	 p2 = s;

	 for (i=len; i>0; i--) {
	    if (*p1 != *p2)
	       break;
	    p1++;
	    p2++;
	 }

	 if ((i==0) && (*p1==0))
	    return TRUE;

	 while (*p1 != 0)
	    p1++;
	 p1++;
      }
   }
   else {                           /* case insensitive lookup */
      keyword = syn->keyword + KEYWORD_HASH((unsigned)tolower(s0), 
					    (unsigned)tolower(s1));
      p1 = keyword->data;

      for (c=keyword->count; c>0; c--) {
	 p2 = s;

	 for (i=len; i>0; i--) {
	    if (*p1 != (unsigned)tolower(*p2))
	       break;
	    p1++;
	    p2++;
	 }

	 if ((i==0) && (*p1==0))
	    return TRUE;

	 while (*p1 != 0)
	    p1++;

	 p1++;
      }
   }

   return FALSE;
}



static_inline int check_number(SYNTAX *syn, unsigned char *s, int len)
{
   int c;
   int hex;

   if (!syn->color_numbers)
      return FALSE;

   if (syn->hex_marker[0]) {
      c = 0;
      while ((syn->hex_marker[c]) && (c<len) && 
	     (syn->hex_marker[c] == (char)s[c]))
	 c++;
      if (syn->hex_marker[c])
	 hex = FALSE;
      else {
	 hex = TRUE;
	 s += c;
	 len -= c;
      }
   }
   else
      hex = FALSE;

   for (c=0; c<len; c++) {
      if (((*s < '0') || (*s > '9')) &&
	  ((!hex) || (((*s < 'a') || (*s > 'f')) && 
		      ((*s < 'A') || (*s > 'F')))) &&
	  (((*s != 'l') && (*s != 'L') && (*s != 'u') && (*s != 'U') && (*s != 'f')) || 
	   (c != len-1) || (c <= 0)))
	 return FALSE;

      s++;
   }

   return TRUE;
}



static_inline int comment_match(char *s1, char *s2, int len, int case_sense)
{
   int c;

   if ((!s1[0]) || (len <= 0))
      return FALSE;

   if (case_sense) {
      for (c=0; s1[c]; c++) {
	 if ((s1[c] != s2[c]) || (c >= len))
	    return FALSE;
      }
   }
   else {
      for (c=0; s1[c]; c++) {
	 if ((s1[c] != (unsigned)tolower(s2[c])) || (c >= len))
	    return FALSE;
      }
   }

   return TRUE;
}



#define FOLD_VID     1
#define SEL_VID      2
#define KEYWORD_VID  4
#define COMMENT_VID  8
#define STRING_VID   16
#define NUMBER_VID   32
#define SYMBOL_VID   64



int display_line(int y, LINE *l, int *comment_state) 
{
   int pos;                            /* position in the line text */
   int tpos;                           /* position including tabs */
   int vid = 0;                        /* color flags */
   int old_vid = 0;                    /* previous color flags */
   int start_sel = 0xffff;             /* start column for sel vid */
   int end_sel = -1;                   /* finish column for sel vid */
   SYNTAX *syn = buffer[0]->syntax;    /* save a ptr lookup */
   unsigned char c = 0;
   unsigned char last_char = 0;
   unsigned char string_char = 0;
   int keyword_count = 0;
   int x;
   int i;

   if (!l) {                                          /* empty line? */
      goto1(disp[y].start, y);
      del_to_eol();
      disp[y].dirty = FALSE;
      disp[y].start = disp[y].end = 0;
      return FALSE;
   }

   if ((buffer[0]->sel_line) &&                       /* check selection */
       (((disp_cline-y >= 0) && (disp_cline-y <= buffer[0]->sel_offset)) ||
	((disp_cline-y >= buffer[0]->sel_offset) && (disp_cline-y <= 0)))) {
      LINE *ss, *se;
      get_selection_info(&ss, &se, &start_sel, &end_sel, FALSE);
      if (l != ss)
	 start_sel = 0;
      if (l != se)
	 end_sel = 0xffff;
   }

   if (*comment_state != COMMENT_NONE)                /* in a comment? */
      vid |= COMMENT_VID;

   if (l->next != l->fold)                            /* folded line? */
      vid |= FOLD_VID;

   if ((start_sel <= end_sel) || (vid)) {
      pos = 0;
      tpos = 0;
      x_pos = 0 - buffer[0]->hscroll;
      disp[y].start = 0;
   }
   else {
      pos = get_leading_bytes(l);
      tpos = get_leading_chars(l);
      x_pos = x = tpos - buffer[0]->hscroll;
      if (disp[y].start < x_pos) {
	 pos = 0;
	 tpos = 0;
	 x_pos = 0 - buffer[0]->hscroll;
	 disp[y].start = 0;
      }
      else
	 disp[y].start = MAX(x, 0);
   }
   y_pos = y;

   l->comment_effect = COMMENT_NONE;

   n_vid();

   while (pos < l->length) {
      if ((pos >= start_sel) && (!(vid & SEL_VID)))   /* start selection? */
	 vid |= SEL_VID;

      if ((pos >= end_sel) && (vid & SEL_VID))        /* end selection? */
	 vid &= ~SEL_VID;

      if ((vid & STRING_VID) && (last_char == syn->escape))
	 last_char = 0;
      else
	 last_char = c;

      c = l->text[pos];

      if (syn) {
	 if (keyword_count > 0) {                     /* in a keyword */
	    keyword_count--;
	    if (keyword_count==0)
	       vid &= ~(KEYWORD_VID | COMMENT_VID | STRING_VID | NUMBER_VID);
	 }

	 if (keyword_count == 0) {                    /* not in a keyword */

	    if (vid & COMMENT_VID) {                  /* in a comment */
	       if (*comment_state == COMMENT_1) {
		  if (comment_match(syn->close_com1, l->text+pos, 
				    l->length-pos, syn->case_sense)) {
		     keyword_count = strlen(syn->close_com1);
		     *comment_state = COMMENT_NONE;
		     l->comment_effect = COMMENT_CLOSE_1;
		  }
	       }
	       else if (*comment_state == COMMENT_2) {
		  if (comment_match(syn->close_com2, l->text+pos, 
				    l->length-pos, syn->case_sense)) {
		     keyword_count = strlen(syn->close_com1);
		     *comment_state = COMMENT_NONE;
		     l->comment_effect = COMMENT_CLOSE_2;
		  }
	       }
	    }
	    else if (vid & STRING_VID) {              /* in a string */
	       if ((c == string_char) && (last_char != syn->escape))
		  keyword_count = 1;
	    }
	    else if ((c == syn->string[0]) ||         /* start a string */
		     (c == syn->string[1])) {
	       vid |= STRING_VID;
	       string_char = c;
	    }                                         /* start comments */
	    else if (comment_match(syn->open_com1, l->text+pos, 
				   l->length-pos, syn->case_sense)) {
	       vid |= COMMENT_VID;
	       *comment_state = l->comment_effect = COMMENT_1;
	    }
	    else if (comment_match(syn->open_com2, l->text+pos, 
				   l->length-pos, syn->case_sense)) {
	       vid |= COMMENT_VID;
	       *comment_state = l->comment_effect = COMMENT_2;
	    }
	    else if (((syn->indent_comments) || (pos == 0)) &&
		     (comment_match(syn->eol_com1, l->text+pos,
				    l->length-pos, syn->case_sense))) {
	       vid |= COMMENT_VID;
	    }
	    else if (((syn->indent_comments) || (pos == 0)) &&
		     (comment_match(syn->eol_com2, l->text+pos, 
				    l->length-pos, syn->case_sense))) {
	       vid |= COMMENT_VID;
	    }
	    else {
	       if (syn->symbols[c])                   /* symbol character */
		  vid |= SYMBOL_VID;
	       else {
		  vid &= ~SYMBOL_VID;

		  if (state_table[c]) {               /* keyword / number */
		     while ((pos+keyword_count < l->length) &&
			    (state_table[l->text[pos+keyword_count]]))
			keyword_count++;

		     if (check_keyword(syn, l->text+pos, keyword_count))
			vid |= KEYWORD_VID;
		     else 
			if (check_number(syn, l->text+pos, keyword_count))
			   vid |= NUMBER_VID;
		  }
	       }
	    }
	 }
      }

      if (vid != old_vid) {
	 if ((vid & SEL_VID) && (vid & FOLD_VID))
	    sf_vid();
	 else if (vid & SEL_VID)
	    sel_vid();
	 else if (vid & FOLD_VID)
	    fold_vid();
	 else if (vid & KEYWORD_VID)
	    keyword_vid();
	 else if (vid & COMMENT_VID)
	    comment_vid();
	 else if (vid & STRING_VID)
	    string_vid();
	 else if (vid & NUMBER_VID)
	    number_vid();
	 else if (vid & SYMBOL_VID)
	    symbol_vid();
	 else
	    n_vid();
	 old_vid = vid;
      }

      if ((c == '\t') && (!(buffer[0]->flags & BUF_BINARY))) {
	 for (i=0; i<TAB_SIZE-(tpos%TAB_SIZE); i++) {
   #ifdef TARGET_DJGPP
	    if ((x_pos >= 0) && (x_pos < screen_w-1))
	       _farpokew(dos_seg, SCRN_ADDR(x_pos, y_pos), (attrib<<8)|' ');
	    else if (x_pos == screen_w-1)
	       _farpokew(dos_seg, SCRN_ADDR(x_pos, y_pos), (config.sel_col<<8)|'>');
	    x_pos++;
   #else
	    if (x_pos == screen_w-1) {
	       int a = attrib;
	       sel_vid();
	       pch('>');
	       tattr(a);
	    }
	    else
	       pch(' ');
   #endif
	 }
	 tpos += TAB_SIZE-(tpos%TAB_SIZE);
      }
      else {
   #ifdef TARGET_DJGPP
	 if ((x_pos >= 0) && (x_pos < screen_w-1))
	    _farpokew(dos_seg, SCRN_ADDR(x_pos, y_pos), (attrib<<8)|c);
	 else if (x_pos == screen_w-1)
	    _farpokew(dos_seg, SCRN_ADDR(x_pos, y_pos), (config.sel_col<<8)|'>');
	 x_pos++;
   #else
	 if (x_pos == screen_w-1) {
	    int a = attrib;
	    sel_vid();
	    pch('>');
	    tattr(a);
	 }
	 else
	    pch(c);
   #endif
	 tpos++;
      }
      pos++;
   }

   if ((vid & FOLD_VID) && (l->length == 0)) {
      fold_vid();
      pch(' ');
   }

   n_vid();

   if (disp[y].end > x_pos)
      del_to_eol();

   disp[y].end = MIN(x_pos, screen_w);
   disp[y].dirty = FALSE;

   if (l->comment_state != *comment_state) {
      l->comment_state = *comment_state;
      return TRUE;
   }
   else
      return FALSE;
}



static_inline int check_comment_state(LINE *l, int *comment_state, SYNTAX *syn)
{
   int old_state = l->comment_state;
   int pos = 0;
   unsigned char c = 0;
   unsigned char last_char = 0;
   unsigned char string_char = 0;
   int x;

   if (!syn)
      return FALSE;

   switch (l->comment_effect) {
      case COMMENT_1:
	 if (*comment_state == COMMENT_NONE)
	    *comment_state = COMMENT_1;
	 break;

      case COMMENT_2:
	 if (*comment_state == COMMENT_NONE)
	    *comment_state = COMMENT_2;
	 break;

      case COMMENT_CLOSE_1:
	 if (*comment_state == COMMENT_1)
	    *comment_state = COMMENT_NONE;
	 break;

      case COMMENT_CLOSE_2:
	 if (*comment_state == COMMENT_2)
	    *comment_state = COMMENT_NONE;
	 break;

      case COMMENT_UNKNOWN:
	 pos = 0;
	 l->comment_effect = COMMENT_NONE;

	 while (pos < l->length) {
	    if ((string_char) && (last_char == syn->escape))
	       last_char = 0;
	    else
	       last_char = c;

	    c = l->text[pos];

	    if (*comment_state == COMMENT_1) {        /* close comment1? */
	       if (comment_match(syn->close_com1, l->text+pos, 
				 l->length-pos, syn->case_sense)) {
		  *comment_state = COMMENT_NONE;
		  l->comment_effect = COMMENT_CLOSE_1;
		  for (x=1; syn->close_com1[x]; x++)
		     pos++;
	       }
	    }
	    else if (*comment_state == COMMENT_2) {   /* close comment2? */
	       if (comment_match(syn->close_com2, l->text+pos, 
				 l->length-pos, syn->case_sense)) {
		  *comment_state = COMMENT_NONE;
		  l->comment_effect = COMMENT_CLOSE_2;
		  for (x=1; syn->close_com2[x]; x++)
		     pos++;
	       }
	    }
	    else if (string_char) {                   /* in a string */
	       if ((c == string_char) && (last_char != syn->escape))
		  string_char = 0;
	    }
	    else if ((c == syn->string[0]) ||         /* start a string */
		     (c == syn->string[1])) {
	       string_char = c;
	    }                                         /* start comments */
	    else if (comment_match(syn->open_com1, l->text+pos, 
				   l->length-pos, syn->case_sense)) {
	       *comment_state = l->comment_effect = COMMENT_1;
	       for (x=1; syn->open_com1[x]; x++)
		  pos++;
	    }
	    else if (comment_match(syn->open_com2, l->text+pos, 
				   l->length-pos, syn->case_sense)) {
	       *comment_state = l->comment_effect = COMMENT_2;
	       for (x=1; syn->open_com2[x]; x++)
		  pos++;
	    }
	    else if (((syn->indent_comments) || (pos == 0)) &&
		     ((comment_match(syn->eol_com1, l->text+pos, 
				     l->length-pos, syn->case_sense)) ||
		      (comment_match(syn->eol_com2, l->text+pos,
				     l->length-pos, syn->case_sense)))) {
	       break;
	    }

	    pos++;
	 }
	 break;
   }

   l->comment_state = *comment_state;
   return (l->comment_state != old_state);
}



static_inline LINE *get_bottom_screen_line()
{
   int c;
   LINE *l = buffer[0]->top;

   for (c=0; c<=BOTTOM; c++)
      if (l)
	 l = l->fold;

   return l;
}



void wait_for_input(int mouse) 
{
   /* screensaver checks, and background updates of the cached syntax
      highlighting comment info */

   static int last_file_update = 0;
   int start = clock();
   int x, y;
   LINE *screen_bottom_line = get_bottom_screen_line();
   int dirty = FALSE;
   int buf = 0;
   LINE *l = buffer[0]->top;
   LINE *tmp;
   int comment_state = COMMENT_NONE;
   int first_pass = TRUE;

   tmp = previous_line(l);
   if ((tmp) && (tmp->comment_state != COMMENT_UNKNOWN))
      comment_state = tmp->comment_state;

   do {
      if ((input_waiting()) || (mouse_changed(&x, &y)) || (alt_pressed()))
	 return;

      #if (defined TARGET_CURSES) || (defined TARGET_WIN)
	 if (winched)
	    return;

	 usleep(100);
      #endif

      if (check_comment_state(l, &comment_state, buffer[buf]->syntax))
	 dirty = TRUE;

      if (((clock() - last_file_update) / CLOCKS_PER_SEC) > 60) {
	 last_file_update = clock();
	 if (mouse)
	    hide_mouse();
	 if (update_file_status()) {
	    redisplay();
	    if (mouse)
	       display_mouse();
	    return;
	 }
	 if (mouse)
	    display_mouse();
      }

      l = l->next;

      if ((buf==0) && (dirty) && (l == screen_bottom_line)) {
	 disp_dirty();
	 if (mouse)
	    hide_mouse();
	 redisplay();
	 if (mouse)
	    display_mouse();
	 dirty = FALSE;
      }

      if (!l) {
	 if (first_pass)
	    first_pass = FALSE;
	 else {
	    buf++;
	    if (buf >= buffer_count)
	       buf = 0;
	 }
	 l = buffer[buf]->start;
	 comment_state = COMMENT_NONE;
      }

   } while ((((clock() - start) / CLOCKS_PER_SEC) < config.screen_save*60) ||
	    (config.screen_save <= 0));

   if (mouse)
      hide_mouse();

   screen_saver();
   dirty_everything();
   redisplay();

   if (mouse)
      display_mouse();
}

