/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Code for storing and editing lines of text, basic insert/delete
 *      operations and undo routines
 */


#include <limits.h>

#include "fed.h"


#ifdef _MSC_VER
   #define static_inline
#else
   #define static_inline   static inline
#endif


#define MAX_TAB   80

static char spc[MAX_TAB] = "                                                                                ";



LINE *create_line(int size)   /* create a line, set up to hold size chars */
{
   LINE *tmp;

   if (size > SHRT_MAX) {
      line_too_long();
      return NULL;
   }

   if ((tmp=malloc(sizeof(LINE)))==NULL) {
      out_of_mem();
      return NULL;
   }

   if (size>0) {
      if ((tmp->text=malloc(size))==NULL) {
	 free(tmp);
	 out_of_mem();
	 return NULL;
      }
   }
   else
      tmp->text = NULL;

   tmp->prev = tmp->next = tmp->fold = NULL;
   tmp->length = tmp->size = size;
   tmp->line_no = 0;
   tmp->comment_state = tmp->comment_effect = COMMENT_UNKNOWN;

   return tmp;
}



#define GROW_CHUNK      4       /* amount of bytes to grow or shrink by */


int resize_line(LINE *line, int size)     /* change the size of a line */
{
   char *old_text = line->text;
   int old_size = line->size;

   line->comment_state = line->comment_effect = COMMENT_UNKNOWN;

   if (size <= (line->size-GROW_CHUNK)) {       /* shrink line */
      line->text = realloc(line->text, size);
      line->size = size;
   }
   else {
      if (size > line->size) {                  /* grow line */
	 if (size+GROW_CHUNK > SHRT_MAX) {
	    line_too_long();
	    return FALSE;
	 }
	 line->text = realloc(line->text, size+GROW_CHUNK);
	 line->size = size+GROW_CHUNK;
      }
   }

   if ((line->text==NULL) && (size>0)) {        /* out of memory */
      out_of_mem();
      line->size = old_size;
      line->text = old_text;
      return FALSE;
   }

   return TRUE;
}



void out_of_mem()
{
   extern int macro_mode;
   repeat_count = 0;
   macro_mode = 0;
   alert_box("Out of memory");
}



void line_too_long()
{
   extern int macro_mode;
   repeat_count = 0;
   macro_mode = 0;
   alert_box("Line too long");
}



LINE *previous_line(LINE *l)
{
   /* find the actual previous line, ignoring folding */

   LINE *p = l->prev;

   if (p)
      while (p->next != l)
	 p=p->next;

   return p;
}



void left_ignoring_folds(int count)
{
   LINE *l = buffer[0]->c_line;
   int c;

   while (count > 0) {
      if (buffer[0]->c_pos <= 0) {
	 if (!l->prev)
	    return;
	 while (l->prev->next != l->prev->fold)
	    remove_fold(l->prev);
	 fn_up();
	 fn_line_end();
	 l = buffer[0]->c_line;
	 count--;
      }
      else {
	 c = MIN(buffer[0]->c_pos, count);
	 buffer[0]->c_pos -= c;
	 buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
	 cursor_move();
	 count -= c;
      }
   }
}



void fn_transpose()
{
   unsigned char tmp;
   int f = FALSE;

   if ((buffer[0]->c_line->length >= 2) && (buffer[0]->c_pos > 0)) {
      if (buffer[0]->c_pos >= buffer[0]->c_line->length) {
	 fn_left();
	 f = TRUE;
      }
      tmp = delete_chars(buffer[0], 1, &buffer[0]->undo);
      fn_left();
      insert_string(buffer[0], &tmp, 1, &buffer[0]->undo);
      if (f)
	 fn_right();
   }
}



void fn_char(int key)
{
   unsigned char tmp;

   if (is_selecting(buffer[0])) {
      destroy_kill_buffer(FALSE);
      kill_block(TRUE);
   }
   else 
      if ((buffer[0]->flags & BUF_OVERWRITE) && (key >= 0) &&
	  (buffer[0]->c_pos < buffer[0]->c_line->length))
	 delete_chars(buffer[0], 1, &buffer[0]->undo);

   tmp=(unsigned char)key;
   insert_string(buffer[0], &tmp, 1, &buffer[0]->undo);
   do_wordwrap(WRAP_INSERT);
}



#define WORDWRAP_LINE_COUNT   256


void indent_block(int c)
{
   LINE *start, *end;
   int saved_c;
   int saved_sel;
   LINE *saved_l;
   int mv_flag;
   LINE *wordwrap_line[WORDWRAP_LINE_COUNT];
   int wordwrap_count = 0;
   int in_wordwrap_block = FALSE;
   int c2;

   saved_c = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
   saved_sel = get_line_length(buffer[0]->sel_line, buffer[0]->sel_pos);
   saved_l = buffer[0]->c_line;

   get_selection_info(&start, &end, NULL, NULL, TRUE);
   end = end->fold;
   mv_flag = (buffer[0]->c_line == end);
   buffer[0]->c_line = start;

   while (buffer[0]->c_line != end) {
      if (buffer[0]->wrap_col > 0) {
	 if (!in_wordwrap_block)
	    if (wordwrap_count < WORDWRAP_LINE_COUNT)
	       wordwrap_line[wordwrap_count++] = buffer[0]->c_line;
	 in_wordwrap_block = (next_wordwrap_line(buffer[0]->c_line, buffer[0]->syntax) != NULL);
      }

      buffer[0]->c_pos = 0;
      if (c >= 0) {
	 if (buffer[0]->c_line->length > 0) {
	    if (REAL_TABS)
	       insert_string(buffer[0], "\t", 1, &buffer[0]->undo);
	    else
	       insert_string(buffer[0], spc, c, &buffer[0]->undo);
	 }
      }
      else {
	 if (REAL_TABS) {
	    c2 = get_leading_chars(buffer[0]->c_line);
	    delete_chars(buffer[0], get_leading_bytes(buffer[0]->c_line), &buffer[0]->undo);
	    c2 += c;
	    while (c2 >= TAB_SIZE) {
	       insert_string(buffer[0], "\t", 1, &buffer[0]->undo);
	       c2 -= TAB_SIZE;
	    }
	    if (c2 > 0)
	       insert_string(buffer[0], spc, c2, &buffer[0]->undo);
	 }
	 else {
	    delete_chars(buffer[0], 
			 MIN(-c, get_leading_bytes(buffer[0]->c_line)), 
			 &buffer[0]->undo);
	 }
      }

      buffer[0]->c_line = buffer[0]->c_line->next;
   }

   if (buffer[0]->wrap_col > 0) {            /* sort out wordwrap */
      for (c2=0; c2<wordwrap_count; c2++) {
	 buffer[0]->c_line = wordwrap_line[c2];
	 buffer[0]->c_pos = 0;
	 cursor_move_line();
	 do_wordwrap(WRAP_INSERT | WRAP_DELETE);
      }
      if ((saved_l == start) || (saved_l == start->prev)) {
	 buffer[0]->sel_line = buffer[0]->c_line;
	 while (next_wordwrap_line(buffer[0]->sel_line, buffer[0]->syntax))
	    buffer[0]->sel_line = buffer[0]->sel_line->next;
	 buffer[0]->sel_pos = buffer[0]->sel_line->length;
	 saved_sel = get_line_length(buffer[0]->sel_line, buffer[0]->sel_pos);
      }
      else {
	 buffer[0]->sel_line = start;
	 buffer[0]->sel_pos = 0;
	 saved_sel = 0;
	 saved_l = buffer[0]->c_line;
	 while (next_wordwrap_line(saved_l, buffer[0]->syntax))
	    saved_l = saved_l->next;
	 if ((mv_flag) && (saved_l->fold)) {
	    saved_l = saved_l->fold;
	    saved_c = 0;
	 }
	 else
	    saved_c = get_line_length(saved_l, saved_l->length);
      }
      c = 0;
   }

   end = end->prev;

   if ((saved_c > 0) && ((saved_l == start) || (saved_l == end)))
      saved_c += c;
   buffer[0]->c_line = saved_l;
   buffer[0]->c_pos = 0;
   while ((buffer[0]->c_pos < buffer[0]->c_line->length) &&
	  (get_line_length(buffer[0]->c_line, buffer[0]->c_pos) < saved_c))
      buffer[0]->c_pos++;

   if ((saved_sel > 0) && 
       ((buffer[0]->sel_line == start) || (buffer[0]->sel_line == end)))
      saved_sel += c;
   buffer[0]->sel_pos = 0;
   while ((buffer[0]->sel_pos < buffer[0]->sel_line->length) &&
	  (get_line_length(buffer[0]->sel_line, buffer[0]->sel_pos) < saved_sel))
      buffer[0]->sel_pos++;

   cursor_move_line();
   disp_dirty();
}



void fn_tab()
{
   int c;

   if (is_selecting(buffer[0])) { 
      c = TAB_SIZE - (get_leading_chars(buffer[0]->c_line) % TAB_SIZE);
      if (c > MAX_TAB)
	 c = MAX_TAB;

      indent_block(c);
   }
   else {
      if (REAL_TABS) {
	 insert_string(buffer[0], "\t", 1, &buffer[0]->undo);
      }
      else {
	 c = TAB_SIZE - (buffer[0]->c_pos % TAB_SIZE);
	 if (c > MAX_TAB)
	    c = MAX_TAB;
	 insert_string(buffer[0], spc, c, &buffer[0]->undo);
      }
      do_wordwrap(WRAP_INSERT);
   }
}



void fn_backspace()
{
   LINE *l = buffer[0]->c_line;
   int c;

   if (is_selecting(buffer[0])) {                  /* backtab block */
      c = get_leading_chars(buffer[0]->c_line) % TAB_SIZE;
      if (c==0)
	 c = TAB_SIZE;

      indent_block(-c);
   }
   else if ((buffer[0]->c_pos > 0) || (l->prev != NULL)) {

      if ((buffer[0]->c_pos > 0) &&
	  (buffer[0]->c_pos <= get_leading_bytes(l))) {     /* backtab */
	 if (l->text[buffer[0]->c_pos-1] == '\t') {
	    c = 1;
	 }
	 else {
	    c = get_line_length(l, buffer[0]->c_pos) % TAB_SIZE;
	    if (c==0)
	       c = TAB_SIZE;
	 }

	 buffer[0]->c_pos = MAX(buffer[0]->c_pos-c, 0);
	 buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
	 cursor_move();

	 if (c > get_leading_bytes(l))
	    c = get_leading_bytes(l);

	 delete_chars(buffer[0], c, &buffer[0]->undo);
	 do_wordwrap(WRAP_DELETE);
      }

      else {               /* just backspace */
	 left_ignoring_folds(1);
	 delete_chars(buffer[0], 1, &buffer[0]->undo);
	 do_wordwrap(WRAP_DELETE);
      }
   }
}



void fn_delete()
{
   if (is_selecting(buffer[0])) {
      destroy_kill_buffer(FALSE);
      kill_block(TRUE);
      return;
   }

   delete_chars(buffer[0], 1, &buffer[0]->undo);
   do_wordwrap(WRAP_DELETE);
}



void fn_return()
{
   int leading = 0;

   if (is_selecting(buffer[0])) {
      destroy_kill_buffer(FALSE);
      kill_block(TRUE);
   }

   if (buffer[0]->flags & BUF_INDENT) {
      leading = get_leading_chars(buffer[0]->c_line);
      if (leading > get_line_length(buffer[0]->c_line, buffer[0]->c_pos))
	 leading = 0;
   }

   insert_cr(buffer[0], &buffer[0]->undo);

   if (REAL_TABS) {
      while (leading >= TAB_SIZE) {
	 insert_string(buffer[0], "\t", 1, &buffer[0]->undo);
	 leading -= TAB_SIZE;
      }
   }

   if (leading > 0)
      insert_string(buffer[0], spc, MIN(leading, MAX_TAB), &buffer[0]->undo);
}



LINE *next_wordwrap_line(LINE *l, SYNTAX *s)
{
   LINE *next = l->next;
   int leading, next_leading;
   int leadingc, next_leadingc;

   if (!next)
      return FALSE;

   leading = get_wordwrap_bytes(l, s);
   leadingc = get_wordwrap_chars(l, s);
   next_leading = get_wordwrap_bytes(next, s);
   next_leadingc = get_wordwrap_chars(next, s);

   if ((l->length <= leading) || (next->length <= next_leading))
      return NULL;

   if ((next_leading > 0) && (next_leadingc != leadingc))
      return NULL;

   if ((s) && (s->wrappers[0]) && ((!s->indents[0]) || (strchr(s->indents, l->text[0])))) {
      if (!strchr(s->wrappers, l->text[l->length-1]))
	 return NULL;
   }
   else {
      if ((l->text[l->length-1] != ' ') && (l->text[l->length-1] != '\t'))
	 return NULL;
   }

   if (next != l->fold)
      remove_fold(l);

   return next;
}



int join_wordwrap_lines(LINE *l, SYNTAX *s)
{
   LINE *next;
   int leading;
   int c, c2;

   next = next_wordwrap_line(l, s);
   if (!next)
      return FALSE;

   leading = get_wordwrap_bytes(next, s);
   for (c=leading; c<next->length; c++) {
      if ((next->text[c] == ' ') || (next->text[c] == '\t')) {
	 c++;
	 break;
      }
   }
   c -= leading;

   if ((c <= 0) || (get_line_length(l, l->length)+c > buffer[0]->wrap_col))
      return FALSE;

   buffer[0]->c_line = l;
   buffer[0]->c_pos = l->length;
   cursor_move_line();

   if (char_from_line(l, l->length-1) != ' ')
      insert_string(buffer[0], " ", 1, &buffer[0]->undo);

   insert_string(buffer[0], next->text+leading, c, &buffer[0]->undo);

   buffer[0]->c_line = next;
   cursor_move_line();
   buffer[0]->c_pos = leading;
   for (c2=leading+c; c2<next->length; c2++) {
      if ((next->text[c2] != ' ') && (next->text[c2] != '\t')) {
	 delete_chars(buffer[0], c, &buffer[0]->undo);
	 return TRUE;
      }
   }
   delete_line(buffer[0], &buffer[0]->undo);
   return TRUE;
}



LINE *split_wordwrap_line(LINE *l, LINE **old_c_line, int *old_c_pos, SYNTAX *s)
{
   int c, c2, c3;
   char *p;

   if (get_line_length(l, l->length-1) < buffer[0]->wrap_col)
      return next_wordwrap_line(l, s);

   c2 = get_wordwrap_bytes(l, s);
   c = 0;
   while (get_line_length(l, c) < buffer[0]->wrap_col)
      c++;

   while ((c > c2) && 
	  (((l->text[c] == ' ') || (l->text[c] == '\t')) || 
	   ((l->text[c-1] != ' ') && (l->text[c-1] != '\t'))))
      c--;

   if (c <= c2) {
      c = 0;
      while (get_line_length(l, c) < buffer[0]->wrap_col)
	 c++;

      while (((l->text[c] == ' ') || (l->text[c] == '\t')) || 
	     ((l->text[c-1] != ' ') && (l->text[c-1] != '\t'))) {
	 c++;
	 if (c >= l->length) {
	    c = -1;
	    break;
	 } 
      }
   }

   if (c <= c2)
      return next_wordwrap_line(l, s);

   buffer[0]->c_line = l;
   buffer[0]->c_pos = c;
   cursor_move_line();
   insert_cr(buffer[0], &buffer[0]->undo);

   if (buffer[0]->flags & BUF_INDENT) {
      LINE *next = next_wordwrap_line(buffer[0]->c_line, s);
      if (next) {
	 p = next->text;
	 c2 = get_wordwrap_chars(next, s);
	 c3 = get_wordwrap_bytes(next, s);
      }
      else {
	 p = l->text;
	 c2 = get_wordwrap_chars(l, s); 
	 c3 = get_wordwrap_bytes(l, s); 
      }
      if (c2 > 0)
	 insert_string(buffer[0], p, c3, &buffer[0]->undo);
   }
   else
      c2 = 0;

   if ((*old_c_line == l) && (*old_c_pos >= get_line_length(l, c))) {
      *old_c_pos -= (get_line_length(l, c) - c2);
      *old_c_line = buffer[0]->c_line;
   }

   return buffer[0]->c_line;
}



void do_wordwrap(int mode) 
{
   extern int auto_waiting_flag;

   LINE *old_c_line, *old_top;
   int old_c_pos;
   LINE *l;

   if (buffer[0]->wrap_col <= 0)
      return;

   old_top = buffer[0]->top;
   old_c_line = l = buffer[0]->c_line;
   old_c_pos = get_line_length(old_c_line, buffer[0]->c_pos);

   auto_waiting_flag = TRUE;

   while (l) {
      if (mode & WRAP_DELETE) {
	 do {
	 } while (join_wordwrap_lines(l, buffer[0]->syntax));
      }
      else
	 mode |= WRAP_DELETE;

      if (mode & WRAP_INSERT)
	 l = split_wordwrap_line(l, &old_c_line, &old_c_pos, buffer[0]->syntax);
      else {
	 l = next_wordwrap_line(l, buffer[0]->syntax);
	 mode |= WRAP_INSERT;
      }
   }

   buffer[0]->top = old_top;
   buffer[0]->c_line = old_c_line;
   buffer[0]->c_pos = 0;
   while ((buffer[0]->c_pos < buffer[0]->c_line->length) &&
	  (get_line_length(buffer[0]->c_line, buffer[0]->c_pos) < old_c_pos))
      buffer[0]->c_pos++;
   buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
   cursor_move_line();
   auto_waiting_flag = FALSE;
}



void fn_reformat() 
{
   do_wordwrap(WRAP_INSERT | WRAP_DELETE);
}



void fn_ascii()
{
   int code;

   code=get_binary_char();

   if (code >= 0)
      fn_char(code&0xff);
}



void fn_insert()
{
   buffer[0]->flags ^= BUF_OVERWRITE;
}



void fn_wordwrap()
{
   buffer[0]->wrap_col = input_number("Wrap column (0=off)", 
				       buffer[0]->wrap_col);
   strcpy(message,"Wordwrap ");
   if (buffer[0]->wrap_col <= 0)
      strcat(message,"is off");
   else {
      strcat(message,"at column ");
      itoa(buffer[0]->wrap_col, message+strlen(message), 10);
   }
   display_message(MESSAGE_KEY);
}



void fn_indent()
{
   buffer[0]->flags ^= BUF_INDENT;
   strcpy(message, "Auto indent is ");
   strcat(message, (buffer[0]->flags & BUF_INDENT) ? "on" : "off");
   display_message(MESSAGE_KEY);
}



void fn_tabsize()
{
   int t = input_number("Tab size", TAB_SIZE);
   if (t < 1)
      t = 1;
   else if (t > 79)
      t = 79;
   set_tab_size(t);
   strcpy(message,"Tab size set to ");
   itoa(TAB_SIZE, message+strlen(message), 10);
   display_message(MESSAGE_KEY);
   disp_dirty();
}



void destroy_undolist(UNDO *undo)
{
   UNDO *next;

   while (undo) {
      next = undo->next;
      free(undo);
      undo = next;
   }
}



#define UNDO_GROW_CHUNK    16
#define UNDO_LIMIT         16384    /* don't undo anything bigger than 16K */



static_inline void undo_write_byte(UNDO *u, unsigned char b)
{
   u->data[u->length++] = b;
}



static_inline void undo_write_word(UNDO *u, unsigned short w)
{
   u->data[u->length++] = (w&0x00ff);
   u->data[u->length++] = (w&0xff00)>>8;
}



static_inline void undo_write_int(UNDO *u, unsigned long i)
{
   u->data[u->length++] = (i&0x000000ff);
   u->data[u->length++] = (i&0x0000ff00)>>8;
   u->data[u->length++] = (i&0x00ff0000)>>16;
   u->data[u->length++] = (i&0xff000000)>>24;
}



static_inline unsigned char undo_read_byte(UNDO *u)
{
   return u->data[--u->length];
}



static_inline unsigned short undo_read_word(UNDO *u)
{
   int w1 = u->data[--u->length];
   int w2 = u->data[--u->length];

   return ((w1 << 8) | (w2));
}



static_inline unsigned long undo_read_int(UNDO *u)
{
   int w1 = u->data[--u->length];
   int w2 = u->data[--u->length];
   int w3 = u->data[--u->length];
   int w4 = u->data[--u->length];

   return ((w1 << 24) | (w2 << 16) | (w3 << 8) | (w4));
}



UNDO *new_undo_node(UNDO *undo)
{
   UNDO *node;
   UNDO *n;
   int c, line, pos;

   if (config.undo_levels <= 0) {
      destroy_undolist(undo);
      return NULL;
   }

   line = get_buffer_line(buffer[0]);
   pos = buffer[0]->c_pos;

   if ((undo) && (undo->length==0) && 
       (undo->cached_operation == UNDO_NOTHING)) {
      undo->cached_line = line;
      undo->cached_pos = pos;
      return undo;
   }

   node = malloc(sizeof(UNDO)+UNDO_GROW_CHUNK);
   if (!node)
      return undo;

   node->next = undo;
   node->cached_line = line;
   node->cached_pos = pos;
   node->cached_operation = UNDO_NOTHING;
   node->cached_count = 0;
   node->length = 0;
   node->size = UNDO_GROW_CHUNK;

   for (c=0, n=node; n; n=n->next, c++) {
      if (c >= config.undo_levels) {
	 destroy_undolist(n->next);
	 n->next = NULL;
	 break;
      }
   }

   return node;
}



UNDO *undo_grow(UNDO *undo, int x)
{
   if (undo->length+x >= undo->size) {
      undo = realloc(undo, sizeof(UNDO)+undo->length+x+UNDO_GROW_CHUNK);
      if (!undo)
	 return NULL;

      undo->size = undo->length+x+UNDO_GROW_CHUNK;
   }

   return undo;
}



UNDO *undo_flush_data(UNDO *undo)
{
   UNDO *t;

   if (undo->cached_operation == UNDO_NOTHING)
      return undo;

   t = undo_grow(undo, 3);
   if (!t)
      return undo;

   undo_write_word(t, t->cached_count);
   undo_write_byte(t, t->cached_operation);

   t->cached_count = 0;
   t->cached_operation = UNDO_NOTHING;

   return t;
}



UNDO *undo_write_pos(UNDO *undo, int line, int pos)
{
   UNDO *t;

   if ((undo->cached_line <= 0) || (undo->cached_pos < 0))
      return undo;

   undo = undo_flush_data(undo);

   t = undo_grow(undo, 7);
   if (!t)
      return undo;

   undo_write_int(t, t->cached_line);
   undo_write_word(t, t->cached_pos);
   undo_write_byte(t, UNDO_GOTO);

   t->cached_line = line;
   t->cached_pos = pos;
   return t;
}



UNDO *undo_add(UNDO *undo, int op, int c, unsigned char *p)
{
   int line = get_buffer_line(buffer[0]);
   int pos = buffer[0]->c_pos;
   int xc = (op == UNDO_DEL) ? c : 0;

   if (!undo)
      return NULL;

   if ((undo == buffer[0]->undo) && 
       ((undo->cached_count + c > UNDO_LIMIT) ||
	(undo->length + c > UNDO_LIMIT))) {
      strcpy(message, "Warning: operation is too big to be undone");
      display_message(MESSAGE_KEY);
      destroy_undolist(buffer[0]->undo);
      destroy_undolist(buffer[0]->redo);
      buffer[0]->undo = NULL;
      buffer[0]->redo = NULL;
      return NULL;
   }

   if ((line != undo->cached_line) || (pos != undo->cached_pos+xc))
      undo = undo_write_pos(undo, line, pos);
   else
      undo->cached_pos += xc;

   if (op != undo->cached_operation) {
      undo = undo_flush_data(undo);
      undo->cached_operation = op;
   }

   if (op == UNDO_DEL) {
      undo->cached_count += c; 
   }
   else if (op == UNDO_CR) {
      undo->cached_count++; 
   } 
   else if (op == UNDO_CHAR) {
      UNDO *t = undo_grow(undo, c);
      if (t) {
	 undo = t;
	 undo->cached_count += c;
	 while (c-- > 0)
	    undo_write_byte(undo, *(p++));
      }
   }

   return undo;
}



UNDO *do_the_undo(UNDO *undo, UNDO *redo)
{
   if ((undo->cached_line > 0) && (undo->cached_pos >= 0)) {
      go_to_line(undo->cached_line);
      buffer[0]->c_pos = MIN(undo->cached_pos, buffer[0]->c_line->length);
      buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
      cursor_move();
   }

   redo = new_undo_node(redo);

   while ((undo->cached_count > 0) || (undo->length > 0)) {
      if (undo->cached_count > 0) {
	 if (undo->cached_operation == UNDO_DEL) {
	    left_ignoring_folds(undo->cached_count);
	    delete_chars(buffer[0], undo->cached_count, &redo);
	 }
	 else if (undo->cached_operation == UNDO_CR) {
	    while (undo->cached_count-- > 0) {
	       if (!insert_cr(buffer[0], &redo))
		  goto bottle_out;
	       left_ignoring_folds(1);
	    }
	 }
	 else if (undo->cached_operation == UNDO_CHAR) {
	    undo->length -= undo->cached_count;
	    if (!insert_string(buffer[0], undo->data+undo->length, 
			       undo->cached_count, &redo))
	       goto bottle_out;
	    left_ignoring_folds(undo->cached_count);
	 }
      }

      if (undo->length > 0) {
	 undo->cached_operation = undo_read_byte(undo);

	 while (undo->cached_operation == UNDO_GOTO) {
	    undo->cached_pos = undo_read_word(undo);
	    undo->cached_line = undo_read_int(undo);
	    go_to_line(undo->cached_line);
	    buffer[0]->c_pos = MIN(undo->cached_pos, buffer[0]->c_line->length);
	    buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
	    cursor_move();

	    if (undo->length > 0)
	       undo->cached_operation = undo_read_byte(undo);
	    else
	       undo->cached_operation = UNDO_NOTHING;
	 }

	 if (undo->length > 0)
	    undo->cached_count = undo_read_word(undo);
	 else
	    undo->cached_count = 0;
      }
      else
	 undo->cached_count = 0;
   }

   bottle_out:
   free(undo);
   return redo;
}



void fn_undo()
{
   UNDO *undo = buffer[0]->undo;
   UNDO *redo = buffer[0]->redo;

   while ((undo) && (undo->length==0) && 
	  (undo->cached_operation == UNDO_NOTHING)) {
      undo = undo->next;
      free(buffer[0]->undo);
      buffer[0]->undo = undo;
   }

   if (undo) {
      buffer[0]->undo = undo->next;
      buffer[0]->redo = NULL;    /* prevent accidental deletion */
      buffer[0]->redo = do_the_undo(undo, redo);
   }
   else {
      strcpy(message, "The undo list is empty");
      display_message(MESSAGE_KEY);
   }
}



void fn_redo()
{
   UNDO *redo = buffer[0]->redo;
   UNDO *undo = buffer[0]->undo;

   while ((redo) && (redo->length==0) && 
	  (redo->cached_operation == UNDO_NOTHING)) {
      redo = redo->next;
      free(buffer[0]->redo);
      buffer[0]->redo = redo;
   }

   if (redo) {
      buffer[0]->redo = redo->next;
      buffer[0]->undo = NULL;    /* prevent accidental deletion */
      buffer[0]->undo = do_the_undo(redo, undo);
   }
   else {
      strcpy(message, "The redo list is empty");
      display_message(MESSAGE_KEY);
   }
}



int insert_string(BUFFER *buf, char *s, int len, UNDO **undo)
{
   LINE *l = buf->c_line;
   int pos = buf->c_pos;
   int old_len;

   if (len <= 0)
      return TRUE;

   if (!resize_line(l, l->length+len))
      return FALSE;

   old_len = l->length;
   l->length += len;
   memmove(l->text+pos+len, l->text+pos, old_len-pos);
   memmove(l->text+pos, s, len);

   dirty_cline();
   buf->c_pos += len;
   buf->old_c_pos = get_line_length(buf->c_line, buf->c_pos);
   buf->flags |= BUF_CHANGED;
   cursor_move();

   if (undo)
      *undo = undo_add(*undo, UNDO_DEL, len, NULL);

   return TRUE;
}



int insert_line(BUFFER *buf, LINE *l, UNDO **undo)
{
   /* assumes buf->c_pos = 0, ie. can't insert into the middle of a line */

   LINE *new;

   new = create_line(l->length);
   if (!new)
      return FALSE;

   new->line_no = l->line_no;
   memmove(new->text, l->text, l->length);

   new->prev = buf->c_line->prev;
   new->next = new->fold = buf->c_line;
   buf->c_line->prev = new;
   if (new->prev) {
      LINE *t = new->prev;
      while (t != new) {
	 if (t->next == buf->c_line)
	    t->next = new;
	 if (t->fold == buf->c_line)
	    t->fold = new;
	 t = t->next;
      }
   }
   else
      buf->start = new;
   if (buf->top == buf->c_line)
      buf->top = new;

   if ((buf->cached_line > 0) && (buf->_cached_line == buf->c_line))
      buf->cached_line++;
   else
      buf->cached_line = -1;

   if (buf->cached_size > 0) {
      buf->cached_size++;
      dirty_bar();
   }
   else
      buf->cached_size = -1;

   buf->flags |= BUF_CHANGED;
   cursor_move_line();
   disp_dirty();

   if (undo)
      *undo = undo_add(*undo, UNDO_DEL, new->length+1, NULL);

   return TRUE;
}



int insert_cr(BUFFER *buf, UNDO **undo)
{
   LINE *l = buf->c_line;
   int pos = buf->c_pos;
   LINE *new;

   new = create_line(l->length-pos);
   if (!new)
      return FALSE;

   memmove(new->text, l->text+pos, l->length-pos);
   new->line_no = l->line_no;
   l->length = pos;
   resize_line(l, l->length); 

   new->next = l->next;
   new->fold = l->fold;
   new->prev = l;
   l->next = l->fold = new;
   if (new->next)
      if (new->next->prev==l)
	 new->next->prev = new;
   if (new->fold)
      if (new->fold->prev==l)
	 new->fold->prev = new;

   if ((buf->cached_line > 0) && (buf->_cached_line == buf->c_line)) {
      buf->cached_line++;
      buf->_cached_line = new;
   }
   else
      buf->cached_line = -1;

   if (buf->cached_size > 0) {
      buf->cached_size++;
      dirty_bar();
   }
   else
      buf->cached_size = -1;

   buf->c_line = new;
   buf->c_pos = buf->old_c_pos = 0;
   buf->flags |= BUF_CHANGED;
   cursor_move_line();
   disp_dirty();

   if (undo)
      *undo = undo_add(*undo, UNDO_DEL, 1, NULL);

   return TRUE;
}



int delete_chars(BUFFER *buf, int count, UNDO **undo)
{
   LINE *l;
   LINE *next;
   int ret = 0;       /* only meaningful when count==1 */
   int x;

   while (count > 0) {
      l = buf->c_line;
      buf->old_c_pos = get_line_length(buf->c_line, buf->c_pos);

      if (buf->c_pos < l->length) {    /* delete from within the line */

	 x = MIN(count, l->length-buf->c_pos);

	 if (undo)
	    *undo = undo_add(*undo, UNDO_CHAR, x, l->text+buf->c_pos);

	 ret = l->text[buf->c_pos];

	 memmove(l->text+buf->c_pos, l->text+buf->c_pos+x,
		 l->length-buf->c_pos-x);

	 if (resize_line(l, l->length-x))
	    l->length -= x;

	 dirty_cline();
	 count -= x;
      }
      else {                           /* merge two lines */
	 next = l->next;
	 if (!next)
	    return -1;

	 if (next != l->fold)
	    remove_fold(l);

	 if ((next->line_no > 0) &&
	     ((l->line_no <= 0) || (l->length < next->length)))
	    l->line_no = next->line_no;

	 if (!resize_line(l, l->length+next->length))
	    return -1;

	 memmove(l->text+l->length, next->text, next->length);
	 l->length += next->length;

	 l->next = next->next;            /* remove old line from list */
	 l->fold = next->fold;
	 if (l->next)
	    if (l->next->prev == next)
	       l->next->prev = l;
	 if (l->fold)
	    if (l->fold->prev == next)
	       l->fold->prev = l;

	 if (next->text)
	    free(next->text);
	 free(next);

	 if (buf->cached_size > 1) {
	    buf->cached_size--;
	    dirty_bar();
	 }
	 else
	    buf->cached_size = -1;

	 cursor_move_line();
	 disp_dirty();

	 if (undo)
	    *undo = undo_add(*undo, UNDO_CR, 0, NULL);

	 ret = -1;
	 count--;
      }
   }

   buf->flags |= BUF_CHANGED;
   return ret;
}



void delete_line(BUFFER *buf, UNDO **undo)
{
   LINE *l = buf->c_line;
   LINE *l2;

   buf->c_pos = buf->old_c_pos = 0;

   if (!l->next) {
      delete_chars(buf, buf->c_line->length+1, undo);
      return;
   }

   if (undo) {
      if (l->length > 0)
	 *undo = undo_add(*undo, UNDO_CHAR, l->length, l->text);
      *undo = undo_add(*undo, UNDO_CR, 0, NULL);
   }

   if (l->next != l->fold)
      remove_fold(l);

   l->next->prev = l->prev;

   l2 = l->prev;
   while ((l2) && (l2 != l->next)) {
      if (l2->next == l)
	 l2->next = l->next;
      if (l2->fold == l)
	 l2->fold = l->next;
      l2 = l2->next;
   }

   if (buf->cached_line > 0) {
      if (buf->_cached_line == l)
	 buf->_cached_line = l->next;
      else {
	 buf->_cached_line = NULL;
	 buf->cached_line = -1;
      }
   }
   buf->c_line = l->next;

   if (buf->top == l)
      buf->top = l->next;
   if (buf->start == l)
      buf->start = l->next;

   if (buf->cached_size > 1) {
      buf->cached_size--;
      dirty_bar();
   }
   else
      buf->cached_size = -1;

   if (l->text)
      free(l->text);
   free(l);

   disp_dirty();
   buf->flags |= BUF_CHANGED;
}

