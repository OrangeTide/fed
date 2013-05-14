/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Searching and browsing routines
 */


#include <ctype.h>

#include "fed.h"


#define SEARCH_SIZE  80                /* maximum length of search string */


unsigned char last_search[SEARCH_SIZE] = "";    /* saved search string */


typedef struct search_pos {    /* stack of cursor positions for backspace */
   int c_pos;
   LINE *c_line;
} SEARCH_POS;


SEARCH_POS search_stack[SEARCH_SIZE];
int stack_size = 0;


LINE *search_files = NULL;             /* list of filenames for grep */
LINE *files_pos = NULL;
int loaded_current = FALSE;            /* was this file loaded specially? */



void push_c_pos()
{
   /* push the current cursor position on to the stack */

   if (config.search_mode != SEARCH_KEYWORD) {
      if (stack_size >= SEARCH_SIZE)
	 stack_size = 0;

      search_stack[stack_size].c_pos = buffer[0]->c_pos;
      search_stack[stack_size].c_line = buffer[0]->c_line;
      stack_size++;
   }
   else {
      if (stack_size != 1) {
	 search_stack[0].c_pos = buffer[0]->c_pos;
	 search_stack[0].c_line = buffer[0]->c_line;
	 stack_size = 1;
      }
   }
}



void pop_c_pos()
{
   if (config.search_mode != SEARCH_KEYWORD)
      stack_size--;

   if (stack_size < 1)
      stack_size=1;

   buffer[0]->c_line = search_stack[stack_size-1].c_line;
   buffer[0]->c_pos = search_stack[stack_size-1].c_pos;
   buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
   cursor_move_line();
}



int add_to_file_list(char *name, int flags)
{
   LINE *l;

   l = create_line(strlen(name)+1);
   if (l==NULL)
      return ENOMEM;

   strcpy(l->text, name);

 #ifdef TARGET_DJGPP
   if (!_preserve_fncase())
      strlwr(l->text);
 #endif

   if (files_pos) {
      files_pos->next = files_pos->fold = l;
      l->prev = files_pos;
      files_pos = l;
   }
   else
      search_files = files_pos = l;

   return 0;
}



void destroy_file_list()
{
   LINE *pos, *prev;

   if (search_files) {
      pos=search_files;
      while (pos) {
	 prev=pos;
	 pos=pos->next;
	 if (prev->text)
	    free(prev->text);
	 free(prev);
      }
      search_files = files_pos = NULL;
      loaded_current = FALSE;
   }
}



int open_list_file()
{
   int count;

   if (check_abort()) {
      errno = 1;
      return errno;
   } 

   if (loaded_current) {
      if (buffer[0]->flags & BUF_CHANGED) {
	 write_file(buffer[0]); 
	 if (errno != 0)
	    return errno;
      }
      fn_close();
      loaded_current = FALSE;
   }

   if (check_abort()) {
      errno = 1;
      return errno;
   } 

   count = buffer_count;
   read_file(files_pos->text, 0);
   if (errno==0) {
      loaded_current = (buffer_count==count+1);

      if ((loaded_current) && (buffer[0]->flags & BUF_WRAPPED)) {
	 buffer[0]->flags = 0;
	 fn_close();
	 loaded_current = FALSE;
	 errno = 1;
	 alert_box("File has long lines");
	 return errno;
      }

      if ((loaded_current) && (buffer[0]->flags & BUF_NEW)) {
	 fn_close();
	 loaded_current = FALSE;
	 errno = 1;
	 alert_box("File not found");
	 return errno;
      }
      else {
	 disp_dirty();
	 buffer[0]->undo = new_undo_node(buffer[0]->undo);
	 destroy_undolist(buffer[0]->redo);
	 buffer[0]->redo = NULL;
      }
   }
   else
      loaded_current = FALSE;

   if (errno != 0)
      alert_box(err());

   return errno;
}



void fn_grep(int key)
{
   static char fspec[256] = "";
   int res;

   res=input_text("Search file spec", fspec, 60, is_filechar);

   if (res!=ESC) {
      if (fspec[0]) {
	 search_files = files_pos = NULL;

	 do_for_each_file(fspec,add_to_file_list,0);
	 if (errno!=0) {
	    alert_box(err());
	 }
	 else {
	    files_pos = search_files;
	    open_list_file();
	    if (errno==0) {
	       fn_start();
	       stack_size = 0;
	       push_c_pos();
	       do_search(key);
	    }
	 }
	 destroy_file_list();
      }
   }
}



void fn_search(int key)
{
   loaded_current = FALSE;
   search_files = files_pos = NULL;
   stack_size = 0;
   push_c_pos();

   do_search(key);
}



#define SEARCH_NORM     0          /* current status of the search */
#define SEARCH_FIRST    1
#define SEARCH_ERR      2
#define SEARCH_NF_P     4
#define SEARCH_NF_N     8
#define SEARCH_END      16


void do_search(int fn_key)
{
   /* main search loop, key input etc */

   int key;
   int search_state = SEARCH_FIRST;
   unsigned char str[SEARCH_SIZE]="";
   int c;
   extern int auto_waiting_flag;

   auto_waiting_flag = TRUE;

   display_big_message(TRUE, NULL);
   cursor_move_line();
   goto1(screen_w-(config.show_bar ? 20 : 19), screen_h-1);
   hi_vid();
   mywrite("ctrl+R to replace");

   do {
      if (search_state & (SEARCH_NF_P | SEARCH_NF_N)) {
	 search_state &= (~(SEARCH_NF_P | SEARCH_NF_N));
	 message[0]='"';
	 message[1]=0;
	 strcat(message,str);
	 strcat(message,"\" not found");
	 display_message(MESSAGE_KEY); 
      }
      else
	 if (search_state != SEARCH_ERR) {
	    if ((files_pos) && (search_state != SEARCH_FIRST)) {
	       message[0]='[';
	       message[1]=0;
	       strcat(message,files_pos->text);
	       strcat(message,"] ");
	    }
	    else
	       message[0]=0;

	    if ((search_state == SEARCH_FIRST) && (last_search[0])) {
	       strcat(message,"Search: [");
	       strcat(message,last_search);
	       strcat(message,"] ");
	    }
	    else {
	       strcat(message, "Search: ");
	       strcat(message,str);
	    }
	    display_message(MESSAGE_KEY);
	 }
	 else
	    search_state = SEARCH_NORM;

      redisplay(); 

      while (m_b)
	 poll_mouse();

      auto_waiting_flag = FALSE;
      while ((!input_waiting()) && (!m_b))
	 poll_mouse();
      auto_waiting_flag = TRUE;

      if (m_b & 1)
	 key = CR;
      else if (m_b & 2)
	 key = ESC;
      else
	 key = input_char().key;

      if (search_state == SEARCH_FIRST) {
	 if ((ascii(key)==CR) || (ascii(key)==LF) ||
	     (key==UP_ARROW) || (key==DOWN_ARROW) ||
	     (key==LEFT_ARROW) || (key==RIGHT_ARROW)) {
	    strcpy(str,last_search);
	    if ((ascii(key)==CR) || (ascii(key)==LF))
	       key = RIGHT_ARROW;
	 }
	 search_state = SEARCH_NORM;
      }

      if (key==fn_key) {         /* search for word under cursor */
	 get_word(str);
	 stack_size = 0;
	 push_c_pos();
	 key = RIGHT_ARROW;
      }

      if ((ascii(key)==CR) || (ascii(key)==LF) || (ascii(key)==ESC) ||
	  (key==CTRL_C) || (key==CTRL_G))
	 search_state = SEARCH_END;

      else if (key==CTRL_R) {    /* call replace routine */
	 if (!(buffer[0]->flags & BUF_READONLY))
	    replace(str);
	 if (str[0])
	    strcpy(last_search,str);
	 auto_waiting_flag = FALSE;
	 display_big_message(FALSE, NULL);
	 if (buffer[0]->flags & BUF_READONLY)
	    strcpy(message, "File is read-only");
	 display_message(MESSAGE_KEY);
	 return;
      }

      else if (ascii(key)==BACKSPACE) {   /* backup one char, using stack */
	 c = strlen(str);
	 if (c>0) {
	    str[c-1] = 0;
	    pop_c_pos();
	 }
      }

      else if ((key==UP_ARROW) || (key==LEFT_ARROW)) {   /* prev */
	 search_state = find_prev(str,TRUE);
	 unfold(buffer[0]->c_line);
	 stack_size = 0;
	 push_c_pos();
      }

      else if ((key==DOWN_ARROW) || (key==RIGHT_ARROW)) {   /* next */
	 search_state = find_next(str,TRUE);
	 unfold(buffer[0]->c_line);
	 stack_size = 0;
	 push_c_pos();
      }

      else {
	 c = strlen(str);
	 if ((c < SEARCH_SIZE) && 
	     ((ascii(key)>=' ') || (ascii(key)=='\t'))) {      /* add char */
	    str[c++] = (char)ascii(key);
	    str[c] = 0;
	    if (config.search_mode == SEARCH_KEYWORD)
	       pop_c_pos();
	    search_state = find_next(str,FALSE);
	    unfold(buffer[0]->c_line);
	    push_c_pos();
	 }
      }

   } while (!(search_state & SEARCH_END));

   display_big_message(FALSE, NULL);

   if (str[0])
      strcpy(last_search,str);
   if (!(search_state & SEARCH_ERR)) {
      strcpy(message,"Search completed");
      display_message(MESSAGE_KEY);
   }

   auto_waiting_flag = FALSE;
}



void get_word(unsigned char *str)
{
   LINE *l = buffer[0]->c_line;
   int pos = buffer[0]->c_pos;
   int len = 0;

   str[0] = 0;

   while (get_state(l,pos-1) == STATE_WORD)
      pos--;

   while (get_state(l,pos) == STATE_WORD) {
      str[len++] = l->text[pos];
      str[len] = 0;
      pos++;
   }
}



int find_prev(unsigned char *str, int flag)
{
   LINE *l = buffer[0]->c_line;
   int pos = buffer[0]->c_pos;
   int len = strlen(str);
   int c;
   unsigned char *tp;

   if (flag) 
      pos--;
   else
      if (pos > (l->length - len))
	 pos = l->length - len;

   do {
      if (pos < 0) {

	 l = previous_line(l);
	 if (l)
	    pos = l->length - len;
	 else
	    if (files_pos) {
	       if (files_pos->prev) {
		  files_pos = files_pos->prev;
		  open_list_file();
		  if (errno != 0)
		     return (SEARCH_ERR | SEARCH_END);
		  fn_end();
		  stack_size = 0;
		  l = buffer[0]->c_line;
		  pos = l->length - len + 1;
	       }
	       else
		  return SEARCH_NF_P;
	    }
	    else
	       return SEARCH_NF_P;
      }
      else {
	 tp = l->text + pos;
	 c = 0;

	 if (config.search_mode==SEARCH_RELAXED) {
	    while (c < len) {
	       if (tolower(str[c]) != tolower(*tp))
		  goto not_yet;
	       tp++;
	       c++;
	    }
	 }
	 else {
	    if (config.search_mode==SEARCH_KEYWORD) {
	       if (get_state(l, pos-1)==STATE_WORD)
		  goto not_yet;
	       while (c < len) {
		  if (str[c] != *tp)
		     goto not_yet;
		  tp++;
		  c++;
	       }
	       if (get_state(l, pos+c)==STATE_WORD)
		  goto not_yet;
	    }
	    else {
	       while (c < len) {
		  if (str[c] != *tp)
		     goto not_yet;
		  tp++;
		  c++;
	       }
	    }
	 }

	 /* found a match! */

	 buffer[0]->c_line = l;
	 buffer[0]->c_pos = pos;
	 buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
	 cursor_move_line();
	 return SEARCH_NORM;

	 not_yet:
	 pos--;
      }

   } while (TRUE);
}



int find_next(unsigned char *str, int flag)
{
   LINE *l = buffer[0]->c_line;
   int len = strlen(str);
   int pos = buffer[0]->c_pos-len;
   int c;
   unsigned char *tp;

   if (flag)
      pos++;

   if (pos < 0)
      pos = 0;

   do {
      if (pos + len > l->length) {

	 if (l->next) {
	    l = l->next;
	    pos = 0;
	 }
	 else {
	    if (files_pos) {
	       if (files_pos->next) {
		  files_pos = files_pos->next;
		  open_list_file();
		  if (errno != 0)
		     return (SEARCH_ERR | SEARCH_END);
		  fn_start();
		  stack_size = 0;
		  l = buffer[0]->start;
		  pos = 0;
	       }
	       else
		  return SEARCH_NF_N;
	    }
	    else
	       return SEARCH_NF_N;
	 }
      }
      else {
	 tp = l->text + pos;
	 c = 0;

	 if (config.search_mode==SEARCH_RELAXED) {
	    while (c < len) {
	       if (tolower(str[c]) != tolower(*tp))
		  goto not_yet;
	       tp++;
	       c++;
	    }
	 }
	 else {
	    if (config.search_mode==SEARCH_KEYWORD) {
	       if (get_state(l, pos-1)==STATE_WORD)
		  goto not_yet;
	       while (c < len) {
		  if (str[c] != *tp)
		     goto not_yet;
		  tp++;
		  c++;
	       }
	       if (get_state(l, pos+c)==STATE_WORD)
		  goto not_yet;
	    }
	    else {
	       while (c < len) {
		  if (str[c] != *tp)
		     goto not_yet;
		  tp++;
		  c++;
	       }
	    }
	 }
	 /* found a match! */

	 buffer[0]->c_line = l;
	 buffer[0]->c_pos = pos + len;
	 buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
	 cursor_move_line();
	 return SEARCH_NORM;

	 not_yet:
	 pos++;
      }

   } while (TRUE);
}



void replace(unsigned char *search_str)
{
   static unsigned char replace_str[SEARCH_SIZE]="";
   char buf[80];
   int res;
   int rep_count = 0;
   extern int auto_waiting_flag;

   if (!search_str[0]) {
      if (last_search[0]) {
	 strcpy(search_str,last_search);
      }
      else {
	 strcpy(message,"No search string");
	 display_message(MESSAGE_KEY);
	 return;
      }
   }

   strcpy(buf, "Replace \"");
   strcat(buf, search_str);
   strcat(buf, "\" with");

   auto_waiting_flag = FALSE;
   res=input_text (buf, replace_str, 60, is_anychar);
   auto_waiting_flag = TRUE;
   n_vid();

   if (res!=ESC) {
      errno=0;

      rep_count = do_replace(search_str,replace_str);

      if (errno==0) {
	 itoa(rep_count, message, 10);
	 strcat(message," replacement");
	 if (rep_count != 1)
	    strcat(message,"s");
	 strcat(message," done");
      }
   }
}



int do_replace(unsigned char *s, unsigned char *r)
{
   KEYPRESS k;
   int key;
   int state = SEARCH_NORM;
   int rep_all = FALSE;
   int sl = strlen(s);
   int rl = strlen(r);
   int rep_count = 0;
   extern int auto_waiting_flag;

   hi_vid();

   #ifdef TARGET_CURSES
      goto1(screen_w-(config.show_bar ? 27 : 26), screen_h-1);
      mywrite("alt+Enter to replace all");
   #else
      goto1(screen_w-(config.show_bar ? 28 : 27), screen_h-1);
      mywrite("ctrl+Enter to replace all");
   #endif

   if (find_next(s,FALSE)==SEARCH_NORM) {

      buffer[0]->c_pos -= sl;
      buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);

      do {
	 if (!rep_all) {
	    if (state & SEARCH_ERR) {
	       #ifdef TARGET_CURSES
		  strcpy(message,"SPACE, ENTER, y = replace  |  n = skip  |  alt+ENTER = replace all");
	       #else
		  strcpy(message,"SPACE, ENTER, y = replace  ³  n = skip  ³  ctrl+ENTER = replace all");
	       #endif
	    }
	    else {
	       if (state & (SEARCH_NF_P | (SEARCH_NF_N))) {
		  strcpy(message,"Not found");
	       }
	       else {
		  if (files_pos) {
		     message[0]='[';
		     message[1]=0;
		     strcat(message,files_pos->text);
		     strcat(message,"] ");
		  }
		  else
		     message[0]=0;
		  strcat(message,"Replace \"");
		  strcat(message,s);
		  strcat(message,"\" with \"");
		  strcat(message,r);
		  strcat(message,"\" ?");
	       }
	    }
	    display_message(MESSAGE_KEY);
	    unfold(buffer[0]->c_line);
	    redisplay();

	    while (m_b)
	       poll_mouse();

	    auto_waiting_flag = FALSE;
	    while ((!input_waiting()) && (!m_b))
	       poll_mouse();
	    auto_waiting_flag = TRUE;

	    if (m_b & 1)
	       key = CR;
	    else if (m_b & 2)
	       key = ESC;
	    else {
	       k = input_char();
	       key = k.key;
	       if ((ascii(key)==CR) && (k.flags & KF_CTRL))
		  key = LF;
	    }
	 }
	 else {
	    if (check_abort())
	       key = ESC;
	    else
	       key = ' ';
	 }

	 if ((ascii(key)==' ') || (ascii(key)==CR) ||
	     (tolower((char)ascii(key))=='y')) {          /* replace */

	    if (buffer[0]->flags & BUF_READONLY) {
	       strcpy(message,"File is read-only");
	       display_message(MESSAGE_KEY);
	       errno = 1;
	       state |= SEARCH_END;
	       break;
	    }

	    delete_chars(buffer[0], sl, &buffer[0]->undo);

	    if (!insert_string(buffer[0],r,rl,&buffer[0]->undo)) {
	       state = SEARCH_END;
	       errno = ENOMEM;
	    }
	    else {
	       rep_count++;
	       buffer[0]->c_pos += sl;
	       state = find_next(s,FALSE);
	       if (!(state & (SEARCH_ERR | SEARCH_END))) {
		  buffer[0]->c_pos -= sl;
		  buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
	       }
	       if (state & SEARCH_NF_N)
		  state |= SEARCH_END;
	    }
	 }

	 else if (ascii(key)==LF) {  /* replace all */
	    #if (defined TARGET_CURSES) || (defined TARGET_WIN)
	       nosleep = TRUE;
	    #endif
	    strcpy(message,"Replacing...");
	    display_message(MESSAGE_KEY);
	    refresh_screen();
	    rep_all=TRUE;
	 }

	 else if ((ascii(key)==ESC) || (key==CTRL_C) || (key==CTRL_G)) {
	    state = SEARCH_END;
	 }

	 else if ((key==LEFT_ARROW) || (key==UP_ARROW)) {   /* prev */
	    state = find_prev(s,TRUE); 
	 }

	 else if ((key==RIGHT_ARROW) || (key==DOWN_ARROW) ||
		  (tolower((char)ascii(key))=='n')) {              /* next */
	    buffer[0]->c_pos += sl;
	    state = find_next(s,TRUE);
	    if (!(state & (SEARCH_ERR | SEARCH_END))) {
	       buffer[0]->c_pos -= sl;
	       buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
	    }
	 }

	 else
	    state = SEARCH_ERR;                    /* prompt */

      } while (!(state & SEARCH_END));
   }

   unfold(buffer[0]->c_line);
   return rep_count;
}



int contains_browse_string(LINE *l, char *s, int *offset)
{
   int c, c2;
   int pos = 0;

   for (c=0; c<l->length; c++) {
      if (l->text[c] == (unsigned char)s[pos]) {      /* a match */
	 if (pos==0) {
	    if (get_state(l, c-1) == STATE_WORD)      /* oops, a substring */
	       pos = -1; 
	    else
	       *offset = c;
	 }
	 pos++;
	 if (s[pos]==0) {
	    if (get_state(l, c+1) != STATE_WORD) {    /* cool, it matches! */
	       for (c2=*offset; c2>=0; c2--)
		  if (l->text[c2] == '(')    /* hack to put function names */
		     return *offset;         /* before their parameters */
	       return get_leading_chars(l);
	    }
	    else
	       pos = 0;
	 }
      }
      else
	 pos = 0;
   }

   return -1;
}



int is_header(LINE *l)
{
   int c;

   for (c=0; c<l->length; c++) {
      if (l->text[c] == '.') {
	 if (tolower(char_from_line(l, c+1)) == 'h')
	    return TRUE;
	 else
	    break;
      }
   }

   return FALSE;
}



int compare_browse_strings(LINE *l1, LINE *l2)
{
   if (l1->fold == l2->fold) {
      if ((is_header(l1)) && (!is_header(l2)))
	 return FALSE;
      else
	 return TRUE;
   }
   else
      return (l1->fold < l2->fold);
}



void add_to_browse_list(int i, BUFFER *buf, LINE *l, char *name, int line, int offset)
{
   char b[70];
   int c;
   LINE *new;
   LINE *pos, *prev;

   strcpy(b, name);
   strcat(b, " ");
   itoa(line, b+strlen(b), 10);
   strcat(b, ":");
   itoa(offset+1, b+strlen(b), 10);

   c = strlen(b);
   new = create_line(c);
   memmove(new->text, b, c);
   new->length = c;
   new->fold = (LINE *)i;   /* ugly, but it works... the pointer isn't 
			       needed for anything else here */

   pos = buf->start;
   prev = NULL;
   while ((pos) && (compare_browse_strings(pos, new))) {
      prev = pos;
      pos = pos->next;
   }

   new->next = pos;
   new->prev = prev;
   if (pos)
      pos->prev = new;
   if (prev)
      prev->next = new;
   else
      buf->start = new;
}



void browse(char *s)
{
   int c, l, i, offset;
   BUFFER *buf;
   LINE *pos, *prev;

   if (!s[0])
      return;

   hide_c();

   read_browse_files();

   for (c=0; c<buffer_count; c++)
      if (buffer[c]->flags & BUF_BROWSE) {
	 buf=buffer[c];
	 goto found_it;
      }

   if (buffer_count >= MAX_BUFFERS) {
      alert_box("Too many open files");
      return;
   }

   buf = malloc(sizeof(buffer));
   if (!buf) {
      out_of_mem();
      return;
   }

   buf->start = NULL; 
   buf->undo = buf->redo = NULL;
   buffer[buffer_count++] = buf;

   found_it:

   pos=buf->start;      /* delete existing data */
   while (pos) {
      prev=pos;
      pos=pos->next;
      if (prev->text)
	 free(prev->text);
      free(prev);
   }
   buf->start = NULL;
   destroy_undolist(buf->undo);
   destroy_undolist(buf->redo);

   strcpy(buf->name, "browse");
 #ifdef DJGPP
   buf->flags = BUF_BROWSE;
 #else
   buf->flags = BUF_BROWSE | BUF_UNIX;
 #endif
   buf->filetime = -1;
   buf->wrap_col = 0;
   buf->hscroll = 0;
   buf->c_pos = 0;
   buf->old_c_pos = 0;
   buf->sel_line = NULL;
   buf->sel_pos = 0;
   buf->sel_offset = 0;
   buf->cached_line = -1;
   buf->cached_top = -1;
   buf->cached_size = -1;
   buf->undo = NULL;
   buf->redo = NULL;
   buf->last_function = -1;
   buf->syntax = NULL;

   strcpy(message,"Searching for '");
   strcat(message,s);
   strcat(message,"'");
   display_message(MESSAGE_KEY);

   for (c=0; c<buffer_count; c++) {
      if ((!(buffer[c]->flags & BUF_BINARY)) && 
	  (!(buffer[c]->flags & BUF_HEX)) &&
	  (!(buffer[c]->flags & BUF_KILL)) &&
	  (!(buffer[c]->flags & BUF_BROWSE))) {
	 l = 0;
	 pos = buffer[c]->start;
	 while (pos) {
	    pos->line_no = ++l;
	    i = contains_browse_string(pos, s, &offset);
	    if (i >= 0)
	       add_to_browse_list(i, buf, pos, buffer[c]->name, l, offset);
	    pos = pos->next;
	 }
      }
   }

   if (buf->start == NULL) {
      buf->start = create_line(strlen(s) + 13);
      buf->start->text[0] = '\"';
      strcpy(buf->start->text+1, s);
      strcat(buf->start->text, "\" not found");
      buf->start->length = strlen(s) + 12;
   }
   else {
      pos = buf->start;
      while (pos) {
	 pos->fold = pos->next;
	 pos = pos->next;
      } 
   }

   buf->top = buf->c_line = buf->start;
   browse_goto(0, TRUE);
}



void fn_browse()
{
   char s[80];

   get_word(s);

   if (!s[0]) {
      if (input_text("Browse", s, 60, is_anychar)==ESC)
	 return;
      if (!s[0])
	 return;
   }

   browse(s);
}



void fn_browse_next()
{
   browse_goto(1, TRUE);
}



void fn_browse_prev()
{
   browse_goto(-1, TRUE);
}



void browse_goto(int direction, int e_flag)
{
   BUFFER *buf;
   int c, c2, c3;
   LINE *l;
   int line = 0;
   int col = 0;
   char tmp[4][256];
   char *fname = NULL;
   static char last_fname[256] = "";
   static int last_line = 0;

   if (direction == 0) {
      last_fname[0] = 0;
      last_line = 0;
   }

   for (c=0; c<buffer_count; c++)
      if (buffer[c]->flags & BUF_BROWSE) {
	 buf = buffer[c];
	 goto found_it;
      }

   if (e_flag)
      alert_box("No browser file");
   return;

   found_it:

   if (direction<0) {
      if (buf->c_line->prev) {
	 buf->c_line = buf->c_line->prev;
	 buf->c_pos = buf->old_c_pos = 0;
      }
      else {
	 last_fname[0] = 0;
	 last_line = 0;
      }
   }
   else {
      if (direction>0) {
	 if ((buf->c_line->fold) && (buf->c_line->fold->length > 0)) {
	    buf->c_line = buf->c_line->fold;
	    buf->c_pos = buf->old_c_pos = 0;
	 }
	 else {
	    last_fname[0] = 0;
	    last_line = 0;
	 }
      }
   }

   for (c=0; c<4; c++)
      tmp[c][0] = 0;

   c = c2 = c3 = 0;

   while ((c<buf->c_line->length) && (c<255) && (c2<4)) {
      if ((buf->c_line->text[c] == '\t') ||
	  (buf->c_line->text[c] == ' ') ||
	  (buf->c_line->text[c] == ',') ||
	  (buf->c_line->text[c] == ';') ||
	  ((buf->c_line->text[c] == ':') &&
	   (buf->c_line->text[c+1] != '\\') &&
	   (buf->c_line->text[c+1] != '/') && 
	   (c<buf->c_line->length-1))) {
	 if (c3 > 0) {
	    tmp[c2][c3] = 0;
	    c2++;
	    if (c2 >= 4)
	       break;
	    c3=0;
	 }
      }
      else {
	 tmp[c2][c3++] = buf->c_line->text[c];
	 tmp[c2][c3] = 0;
      }

      c++;
   }

   for (c=0; c<4; c++) {
      if (strchr(tmp[c], '.')) {
	 fname = tmp[c];
	 break;
      }
   }

   if (!fname) {
      fname = tmp[0];
      for (c=0; fname[c]; c++) {
	 if (!is_filechar_nospace(fname[c])) {
	    fname = NULL;
	    break;
	 }
      }
   }

   for (c=0; c<4; c++) {
      if (is_all_numbers(tmp[c]) && (tmp[c] != fname)) {
	 line = atoi(tmp[c]);
	 break;
      }
   }

   for (c++; c<4; c++) {
      if (is_all_numbers(tmp[c])) {
	 col = atoi(tmp[c]);
	 break;
      }
   }

   if ((fname) && (line > 0) && 
       ((stricmp(fname, last_fname) != 0) || (line != last_line))) {
      read_file(fname, 0);
      disp_dirty();
      if (errno!=0) {
	 cursor_move_line();
	 return;
      }
      go_to_browse_line(line);
      buffer[0]->c_pos = MIN(MAX(col-1, 0), buffer[0]->c_line->length);
      buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
      cursor_move();
      last_line = line;
      strcpy(last_fname, fname);
   }
   else {
      l = buf->c_line;
      if (((direction < 0) && (l->prev)) ||
	  ((direction > 0) && (l->fold) && (l->fold->length > 0))) {
	 line = col = 0;
	 fname = NULL;
	 goto found_it;
      }
      else {
	 if ((!fname) || (line <= 0)) {
	    for (c=0; c<buffer_count; c++)
	       if (buffer[c]->flags & BUF_BROWSE)
		  break;
	    buf = buffer[c];
	    buffer[c] = buffer[0];
	    buffer[0] = buf;
	    display_new_buffer();
	    last_fname[0] = 0;
	    last_line = 0;
	 }
      }
   }

   c = c2 = 0;
   l = buf->start;
   while (l) {
      c++;
      if (l == buf->c_line)
	 c2 = c;
      l = l->next;
   }

   message[0]='[';
   itoa(c2, message+1, 10);
   c2 = strlen(message);
   message[c2] = '/';
   itoa(c, message+c2+1, 10);
   c2 = strlen(message);
   message[c2++]=']';
   message[c2++]=' ';
   if (buf->c_line->length > 0) {
      c = buf->c_line->length;
      if (c > 78-c2)
	 c = 78-c2;
      memmove(message+c2,buf->c_line->text,c);
      message[c+c2]=0;
   }
   else
      message[c2] = 0;

   display_message(MESSAGE_LINE);
}



