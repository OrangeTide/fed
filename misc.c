/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Various routines (folding, brace matching, reverse video block
 *      marking, word-based operations) that don't quite fit anywhere else
 */


#include <ctype.h>

#include "fed.h"



int get_leading_bytes(LINE *l)
{
   int c = 0;

   while ((c < l->length) && ((l->text[c] == ' ') || (l->text[c] == '\t')))
      c++;

   return c;
}



int get_leading_chars(LINE *l)
{
   int c = 0;
   int len = 0;

   while (c < l->length) {
      if (l->text[c] == ' ') {
	 len++;
      }
      else if (l->text[c] == '\t') { 
	 if (buffer[0]->flags & BUF_BINARY)
	    len++;
	 else
	    len += TAB_SIZE-(len%TAB_SIZE);
      }
      else
	 break;

      c++;
   }

   return len;
}



int get_wordwrap_bytes(LINE *l, SYNTAX *s)
{
   int c, ch;

   for (c=0; c<l->length; c++) {
      ch = l->text[c];

      if ((ch != ' ') && (ch != '\t') && 
	  (((!s) || (!ch) || (!strchr(s->indents, ch)))))
	 break;
   }

   return c;
}



int get_wordwrap_chars(LINE *l, SYNTAX *s)
{
   int len = 0;
   int c, ch;

   for (c=0; c<l->length; c++) {
      ch = l->text[c];

      if ((ch != ' ') && (ch != '\t') && 
	  (((!s) || (!ch) || (!strchr(s->indents, ch)))))
	 break;

      if (ch == '\t') { 
	 if (buffer[0]->flags & BUF_BINARY)
	    len++;
	 else
	    len += TAB_SIZE-(len%TAB_SIZE);
      }
      else
	 len++;
   }

   return len;
}



int get_line_length(LINE *l, int p)
{
   int c;
   int len = 0;

   for (c=0; (c<p) && (c<l->length); c++) {
      if ((l->text[c] == '\t') && (!(buffer[0]->flags & BUF_BINARY)))
	 len += TAB_SIZE-(len%TAB_SIZE);
      else
	 len++;
   }

   return len;
}



char state_table[256] =
{
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /*   */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /*     */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /*  */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /*   */
   FALSE, FALSE, FALSE, TRUE,  TRUE,  FALSE, FALSE, FALSE,     /*  !"#$%&' */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* ()*+,-./ */
   TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,      /* 01234567 */
   TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* 89:;<=>? */
   FALSE, TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,      /* @ABCDEFG */
   TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,      /* HIJKLMNO */
   TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,      /* PQRSTUVW */
   TRUE,  TRUE,  TRUE,  FALSE, TRUE,  FALSE, FALSE, TRUE,      /* XYZ[\]^_ */
   FALSE, TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,      /* `abcdefg */
   TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,      /* hijklmno */
   TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,      /* pqrstuvw */
   TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,     /* xyz{|}~ */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* ÄÅÇÉÑÖÜá */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* àâäãåçéè */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* êëíìîïñó */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* òôöõúùûü */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* †°¢£§•¶ß */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* ®©™´¨≠ÆØ */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* ∞±≤≥¥µ∂∑ */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* ∏π∫ªºΩæø */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* ¿¡¬√ƒ≈∆« */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* »… ÀÃÕŒœ */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* –—“”‘’÷◊ */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* ÿŸ⁄€‹›ﬁﬂ */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* ‡·‚„‰ÂÊÁ */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* ËÈÍÎÏÌÓÔ */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,     /* ÒÚÛÙıˆ˜ */
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE      /* ¯˘˙˚¸˝˛  */
};



int get_state(LINE *l, int p)
{
   unsigned char c;

   if ((p < 0) || (p >= l->length))
      return STATE_SPACE;

   c = l->text[p];

   if ((c==' ') || (c=='\t'))
      return STATE_SPACE;

   if (state_table[c])
      return STATE_WORD;

   return STATE_CHAR;
}



void fn_fold()               /* make (or remove) a fold */
{
   LINE *start;         /* start of fold */
   LINE *end;           /* end of fold */

   if (is_selecting(buffer[0]) ) {          /* fold the selected block */

      get_selection_info(&start, &end, NULL, NULL, TRUE);
      end = end->fold;

      if (start->next != start->fold)
	 remove_fold(start);

      if (end) {
	 start->fold = end;
	 end->prev = start;
      }
      else
	 start->fold = NULL;

      block_finished();

      if (start!=buffer[0]->c_line) {
	 buffer[0]->c_line=start;
	 buffer[0]->c_pos = 0;
	 while ((buffer[0]->c_pos < buffer[0]->c_line->length) &&
		(get_line_length(buffer[0]->c_line, buffer[0]->c_pos) < buffer[0]->old_c_pos))
	    buffer[0]->c_pos++;
	 cursor_move_line();
      }
   }

   else {
      if (buffer[0]->c_line->next != buffer[0]->c_line->fold) {
	 start = buffer[0]->c_line;          /* get rid of a fold */
	 end = buffer[0]->c_line->fold;
	 remove_fold(start); 
	 disp_move_on_screen(start, end);
      }
      else {
	 fold_from_line(buffer[0]->c_line,0);   /* fold by indentation */
	 disp_dirty();
      }
   }

   buffer[0]->cached_size = -1;
}



void fold_all()
{
   /* folds all the functions in a file, when it is loaded */

   LINE *l = buffer[0]->start;

   while (l) {
      if ((get_leading_bytes(l)==0) && (l->length>0))
	 fold_from_line(l, 3);

      l=l->fold;
   }

   buffer[0]->cached_size = -1;
}



void fold_from_line(LINE *start, int size)
{
   LINE *end;          /* end of the fold */
   LINE *pos;          /* position being looked at */
   int indent;         /* indentation of the current line */
   int prev_indent;    /* indentation of the previous line */
   int count = 1;     /* current number of folded lines */
   int x = 0;

   if (start->length==0)
      return;

   pos = end = start->fold;
   prev_indent = get_leading_chars(start);

   while (pos) {

      if (end->length > 0) {
	 end = pos;
	 count += x;
	 x = 0;
      }

      if (pos->length==get_leading_chars(pos)) {
	 if (prev_indent==0)
	    break;
	 else
	    indent = prev_indent;
      }
      else
	 indent = get_leading_chars(pos);

      if ((indent < get_leading_chars(start)) ||
	  ((indent == get_leading_chars(start)) && (prev_indent > indent)))
	 break;

      if (pos->length > get_leading_chars(pos)) {
	 end = pos;
	 count += x;
	 x = 0;
      }

      prev_indent = indent;
      pos = pos->fold;
      x++;
   }

   if (count > size) {          /* make the fold */
      if (start->next != start->fold)
	 remove_fold(start);

      if (end) {
	 start->fold = end;
	 end->prev = start;
      }
      else
	 start->fold = NULL;
   }
}



void remove_fold(LINE *l) 
{
   LINE *pos = l;
   LINE *end = l->fold;

   if (end != l->next) {

      l->fold = l->next;                /* sort out the start pointer */

      if (end) {
	 while (pos->fold != end)
	    pos = pos->next;

	 end->prev = pos;               /* sort out the prev pointer */
      }

      disp_dirty();
      buffer[0]->cached_size = -1;
   }
}



void unfold(LINE *l)     /* make sure that a line is visible, for a goto */
{
   LINE *pos;
   LINE *fold = (LINE *)-1L;     /* to force at least one pass */

   while (fold) {

      pos = buffer[0]->start;
      fold = NULL;

      while (TRUE) {

	 if (fold)
	    if (pos == fold->fold)
	       fold = NULL;

	 if (!fold)
	    if (pos->next != pos->fold)
	       fold = pos;

	 if (pos==l) {
	    if (fold)
	       remove_fold(fold);
	    break;
	 }

	 pos = pos->next;
      }
   }
}



LINE *outside_fold(LINE *l)       /* return the fold containing l */
{
   LINE *pos = buffer[0]->start;
   LINE *fold = NULL;

   while (TRUE) {

      if (fold)
	 if (pos == fold->fold)
	    fold = NULL;

      if (!fold)
	 fold = pos;

      if (pos==l)
	 return fold;

      pos = pos->next;
   }
}



void fn_expand()             /* expand all folds in the doc */
{
   int done = FALSE;
   LINE *prev = NULL;
   LINE *l = buffer[0]->start;

   while (l) {
      if (l->fold != l->next)
	 done = TRUE;
      l->fold = l->next;
      l->prev = prev;
      prev = l;
      l = l->next;
   }

   if (!done) {
      fold_all();
      buffer[0]->c_line = outside_fold(buffer[0]->c_line);
      buffer[0]->c_pos = 0;
      while ((buffer[0]->c_pos < buffer[0]->c_line->length) &&
	     (get_line_length(buffer[0]->c_line, buffer[0]->c_pos) < buffer[0]->old_c_pos))
	 buffer[0]->c_pos++;
      buffer[0]->top = outside_fold(buffer[0]->top);
   }

   buffer[0]->cached_size = -1;
   disp_dirty();
   cursor_move_line();
}



void fn_block()              /* begin a block marking operation */
{
   if (buffer[0]->sel_line)
      block_finished();
   else {
      buffer[0]->sel_line=buffer[0]->c_line;
      buffer[0]->sel_pos=buffer[0]->c_pos;
      buffer[0]->sel_offset=0;
      dirty_cline(0);
   }
}



void fn_select_word() 
{
   LINE *l = buffer[0]->c_line;

   if (buffer[0]->sel_line)
      block_finished();

   if (get_state(l, buffer[0]->c_pos) == STATE_WORD) {
      buffer[0]->sel_line=l;
      buffer[0]->sel_pos=buffer[0]->c_pos;
      buffer[0]->sel_offset=0;

      while ((buffer[0]->sel_pos > 0) &&
	     (get_state(l, buffer[0]->sel_pos-1) == STATE_WORD))
	 buffer[0]->sel_pos--;

      while (get_state(l, buffer[0]->c_pos) == STATE_WORD)
	 buffer[0]->c_pos++;

      buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
      dirty_cline(0);
   }
}



void block_finished()        /* end a block marking operation */
{
   if (buffer[0]->sel_line) {
      buffer[0]->sel_line=NULL;
      buffer[0]->sel_pos=0;
      buffer[0]->sel_offset=0;
      disp_dirty();
   }
}



void get_selection_info(LINE **start_l, LINE **end_l, int *start_i, int *end_i, int fixup)
{
   LINE *sl, *el;
   int si, ei;

   if (buffer[0]->sel_offset == 0) {
      if (buffer[0]->c_pos > buffer[0]->sel_pos) {
	 sl = buffer[0]->sel_line;
	 si = buffer[0]->sel_pos;
      }
      else {
	 sl = buffer[0]->c_line;
	 si = buffer[0]->c_pos;
      }
   }
   else {
      if (buffer[0]->sel_offset > 0) {
	 sl = buffer[0]->sel_line;
	 si = buffer[0]->sel_pos;
      }
      else {
	 sl = buffer[0]->c_line;
	 si = buffer[0]->c_pos;
      }
   }

   if (buffer[0]->sel_offset == 0) {
      if (buffer[0]->c_pos > buffer[0]->sel_pos) {
	 el = buffer[0]->c_line;
	 ei = buffer[0]->c_pos;
      }
      else {
	 el = buffer[0]->sel_line;
	 ei = buffer[0]->sel_pos;
      }
   }
   else {
      if (buffer[0]->sel_offset > 0) {
	 el = buffer[0]->c_line;
	 ei = buffer[0]->c_pos;
      }
      else {
	 el = buffer[0]->sel_line;
	 ei = buffer[0]->sel_pos;
      }
   }

   if (fixup) {
      if ((si >= sl->length) && (sl != el) && (sl->fold)) {
	 sl = sl->fold;
	 si = 0;
      }
      if ((ei <= 0) && (el != sl) && (el->prev)) {
	 el = el->prev;
	 ei = el->length;
      }
   }

   if (start_l)
      *start_l = sl;

   if (end_l)
      *end_l = el;

   if (start_i)
      *start_i = si;

   if (end_i)
      *end_i = ei;
}



int is_selecting(BUFFER *b)
{
   if ((b->sel_line) && 
       ((b->sel_line != b->c_line) || (b->sel_pos != b->c_pos)))
      return TRUE;

   block_finished();
   return FALSE;
}



int brace_direction(char c)
{
   if ((c=='{') || (c=='(') || (c=='[') || (c=='<'))
      return 1;

   if ((c=='}') || (c==')') || (c==']') || (c=='>'))
      return -1;

   return 0;
}



int opposite_brace(char c)
{
   switch (c) {
      case '{': return '}';
      case '}': return '{';
      case '(': return ')';
      case ')': return '(';
      case '[': return ']';
      case ']': return '[';
      case '<': return '>';
      case '>': return '<';
   }
   return 0;
}



void fn_match()      /* find the brace matching the one under the cursor */
{
   LINE *l;
   int pos;
   int direction;
   int braces;
   char ob, cb;
   char tmp;

   l = buffer[0]->c_line;
   pos = buffer[0]->c_pos;
   direction = brace_direction(char_from_line(l,pos));

   if ((direction==0) && (pos<get_leading_bytes(l))) {
      pos = get_leading_bytes(l);
      direction = brace_direction(char_from_line(l,pos));
   }
   else
      while ((direction==0) && (pos>0)) {
	 pos--;
	 direction = brace_direction(char_from_line(l,pos));
      }

   if (direction==0) {
      strcpy(message,"Not on a brace");
      display_message(MESSAGE_KEY);
      return;
   }
   else {
      braces = 1;
      ob = char_from_line(l,pos);
      cb = opposite_brace(ob);

      while (braces > 0) {

	 if (direction < 0) {
	    pos--;
	    while (pos < 0) {
	       l=previous_line(l);
	       if (l==NULL)
		  goto no_match;
	       pos = l->length-1;
	    }
	 }
	 else {
	    pos++;
	    while (pos >= l->length) {
	       l=l->next;
	       if (l==NULL)
		  goto no_match;
	       pos = 0;
	    }
	 }

	 tmp = l->text[pos];
	 if (tmp==cb)
	    braces--;
	 else
	    if (tmp==ob)
	       braces++;
      }

      unfold(l);
      buffer[0]->c_line=l;
      buffer[0]->c_pos = pos;
      buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
      cursor_move_line();
      return;
   }

   no_match:

   strcpy(message,"No match");
   display_message(MESSAGE_KEY);
}



void fn_word_left()          /* move one word to the left */
{
   int state = STATE_SPACE;
   int old_state;

   do {
      if ((buffer[0]->c_pos <= 0) && (!buffer[0]->c_line->prev))
	 return;

      fn_left();
      old_state = state;
      state = get_state(buffer[0]->c_line, buffer[0]->c_pos);

   } while (state >= old_state);

   fn_right();
}



void fn_word_right()         /* move one word to the right */
{
   int state = get_state(buffer[0]->c_line, buffer[0]->c_pos);
   int old_state;
   LINE *l = NULL;
   int p = -1;

   do {
      if ((buffer[0]->c_pos >= buffer[0]->c_line->length) &&
	  (!buffer[0]->c_line->next))
	 return;

      fn_right();
      old_state = state;
      state = get_state(buffer[0]->c_line, buffer[0]->c_pos);

      if (buffer[0]->sel_line)
	 get_selection_info(NULL, &l, NULL, &p, FALSE);

   } while (((l == buffer[0]->c_line) && (p == buffer[0]->c_pos)) ?
	    (state >= old_state) : (state <= old_state));
}



void fn_lowcase()
{
   change_case(-1);
}



void fn_upcase()
{
   change_case(1);
}



void change_case(int flag)
{
   int state = get_state(buffer[0]->c_line, buffer[0]->c_pos);
   unsigned char c;

   while (state != STATE_WORD) {
      if ((buffer[0]->c_pos >= buffer[0]->c_line->length) &&
	  (!buffer[0]->c_line->fold))
	 return;

      fn_right();
      state = get_state(buffer[0]->c_line, buffer[0]->c_pos);
   }

   while (state == STATE_WORD) {
      c = delete_chars(buffer[0], 1, &buffer[0]->undo);
      if (flag > 0)
	 c = toupper(c);
      else
	 c = tolower(c);

      insert_string(buffer[0], &c, 1, &buffer[0]->undo);
      state = get_state(buffer[0]->c_line, buffer[0]->c_pos);
   }
}



char remember_filename[256] = "";
int remember_line = 0;
int remember_col = 0;


void fn_remember()
{
   LINE *l;
   int x = 1;

   strcpy(remember_filename, buffer[0]->name);
   remember_col = buffer[0]->c_pos;

   l = buffer[0]->start;
   while (l) {
      if (l == buffer[0]->c_line)
	 remember_line = x;
      l->line_no = x++;
      l = l->next;
   }

   strcpy(message, "Cursor position stored");
   display_message(MESSAGE_KEY); 
}



void fn_lastpos()
{
   if ((!remember_filename[0]) || (remember_line <= 0)) {
      alert_box("No position stored");
      return;
   }

   read_file(remember_filename, 0);
   if (errno==0) {
      go_to_browse_line(remember_line);
      buffer[0]->c_pos = MIN(buffer[0]->c_line->length, remember_col);
      buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
      display_new_buffer();
   }
}

