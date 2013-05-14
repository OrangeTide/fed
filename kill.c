/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Kill, copy and yank
 */


#include "fed.h"



BUFFER *find_kill_buffer()
{
   int c;
   BUFFER *buf;

   for (c=0; c<buffer_count; c++)
      if (buffer[c]->flags & BUF_KILL)
	 return buffer[c];

   if (buffer_count >= MAX_BUFFERS) {
      alert_box("Too many open files");
      return NULL;
   }

   buf = malloc(sizeof(buffer));
   if (!buf) {
      out_of_mem();
      return NULL;
   }

   buf->start = buf->top = buf->c_line = create_line(0);
   if (!buf->start) {
      out_of_mem();
      free(buf);
      return NULL;
   }

   buffer[buffer_count++] = buf;

   strcpy(buf->name, "kill");
 #ifdef DJGPP
   buf->flags = BUF_KILL;
 #else
   buf->flags = BUF_KILL | BUF_UNIX;
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

   return buf;
}



void destroy_kill_buffer(int completely)
{
   int c, d;
   LINE *pos, *prev;

   /* start at 1, so as not to destroy current file... */
   for (c=1; c<buffer_count; c++) {
      if (buffer[c]->flags & BUF_KILL) {
	 if (completely) {
	    destroy_buffer(buffer[c]);

	    buffer_count--;
	    for (d=c; d<buffer_count; d++)
	       buffer[d] = buffer[d+1];

	    c--;
	 }
	 else {
	    pos=buffer[c]->start->next;      /* delete existing data */
	    while (pos) {
	       prev=pos;
	       pos=pos->next;
	       if (prev->text)
		  free(prev->text);
	       free(prev);
	    }

	    buffer[c]->top = buffer[c]->c_line = buffer[c]->start;
	    buffer[c]->start->next = buffer[c]->start->fold = NULL;
	    buffer[c]->start->length = 0;
	    buffer[c]->c_pos = buffer[c]->old_c_pos = 0;

	    destroy_undolist(buffer[c]->undo);
	    destroy_undolist(buffer[c]->redo);

	    buffer[c]->cached_line = -1;
	    buffer[c]->cached_top = -1;
	    buffer[c]->cached_size = -1;
	    buffer[c]->undo = NULL;
	    buffer[c]->redo = NULL;
	    buffer[c]->last_function = -1;
	    buffer[c]->syntax = NULL;
	 }
      }
   }
}



void check_kill(void (*proc)())
{
   if ((last_function < 0) || (fn_map[last_function].fn != proc))
      destroy_kill_buffer(FALSE);
}



int sel_right()
{
   if (buffer[0]->sel_pos < buffer[0]->sel_line->length) {
      buffer[0]->sel_pos++;
      return TRUE;
   }

   if (!buffer[0]->sel_line->fold)
      return FALSE;

   buffer[0]->sel_line = buffer[0]->sel_line->fold;
   buffer[0]->sel_pos = 0;
   buffer[0]->sel_offset--; 

   return TRUE;
}



void fn_kill_word()                   /* kills the next word */
{
   int state;
   int old_state;

   check_kill(fn_kill_word);

   if (!is_selecting(buffer[0])) {
      buffer[0]->sel_line = buffer[0]->c_line;
      buffer[0]->sel_pos = buffer[0]->c_pos;
      buffer[0]->sel_offset = 0;

      state = get_state(buffer[0]->sel_line, buffer[0]->sel_pos);

      if (state==STATE_SPACE) {
	 while (state==STATE_SPACE) {
	    if (!sel_right())
	       goto get_out;
	    state = get_state(buffer[0]->sel_line, buffer[0]->sel_pos);
	 }
      }
      else {
	 old_state = state;
	 while (state >= old_state) {
	    if (!sel_right())
	       goto get_out;
	    old_state = state;
	    state = get_state(buffer[0]->sel_line, buffer[0]->sel_pos);
	 }
	 while ((state==STATE_SPACE) && 
		(buffer[0]->sel_pos < buffer[0]->sel_line->length)) {
	    if (!sel_right())
	       goto get_out;
	    state = get_state(buffer[0]->sel_line, buffer[0]->sel_pos);
	 }
      }
   }
   else
      last_function = -2;

   get_out:
   kill_block(TRUE);
}



void fn_kill_line()              /* kills the line that the cursor is on */
{
   check_kill(fn_kill_line);

   if (!is_selecting(buffer[0])) {
      buffer[0]->c_pos = buffer[0]->old_c_pos = 0;
      cursor_move();
      if (buffer[0]->c_line->fold) {
	 buffer[0]->sel_line = buffer[0]->c_line->fold;
	 buffer[0]->sel_pos = 0;
	 buffer[0]->sel_offset = -1;
      }
      else {
	 buffer[0]->sel_line = buffer[0]->c_line;
	 buffer[0]->sel_pos = buffer[0]->c_line->length;
	 buffer[0]->sel_offset = 0;
      }
   }
   else
      last_function = -2; 

   kill_block(TRUE);
}



void fn_e_kill()                      /* emacs style kill */
{
   check_kill(fn_e_kill);

   if (!is_selecting(buffer[0])) {
      if ((buffer[0]->c_pos < buffer[0]->c_line->length) ||
	  (!buffer[0]->c_line->fold)) {
	 buffer[0]->sel_line = buffer[0]->c_line;
	 buffer[0]->sel_pos = buffer[0]->c_line->length;
	 buffer[0]->sel_offset = 0;
      }
      else {
	 buffer[0]->sel_line = buffer[0]->c_line->fold;
	 buffer[0]->sel_pos = 0;
	 buffer[0]->sel_offset = -1;
      }
   }
   else
      last_function = -2;

   kill_block(TRUE);
}



void kill_block(int insert_flag)          /* kill the marked block */
{
   LINE *s, *e;
   int si, ei;
   int c, count;
   BUFFER *buf;

   get_selection_info(&s, &e, &si, &ei, FALSE);

   buffer[0]->c_line = s;
   buffer[0]->c_pos = si;
   buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);

   block_finished();

   count = 0;
   while (s != e) {
      count += (s->length - si + 1);
      s = s->next;
      si = 0;
   }
   count += (ei - si);

   buf = find_kill_buffer();
   if (!buf)
      return;

   if ((insert_flag) && (buf == buffer[0])) {
      strcpy(message, "Can't cut from the kill buffer into itself");
      display_message(MESSAGE_KEY);
      insert_flag = FALSE;
   }

   while (count > 0) {
      if ((buffer[0]->c_pos == 0) && (buf->c_pos == 0) &&
	  (count >= buffer[0]->c_line->length+1)) {   /* fast method */
	 count -= buffer[0]->c_line->length + 1;
	 if (insert_flag)
	    if (!insert_line(buf, buffer[0]->c_line, NULL))
	       break;
	 delete_line(buffer[0], &buffer[0]->undo);
      }
      else {                          /* oh well, have to use the slow way */
	 if (buffer[0]->c_pos >= buffer[0]->c_line->length) {
	    c = 1;
	    if (insert_flag)
	       if (!insert_cr(buf, NULL))
		  break;
	 }
	 else {
	    c = buffer[0]->c_line->length - buffer[0]->c_pos;
	    if (c > count)
	       c = count;
	    if (insert_flag)
	       if (!insert_string(buf, buffer[0]->c_line->text + 
					  buffer[0]->c_pos, c, NULL))
		  break;
	 }

	 delete_chars(buffer[0], c, &buffer[0]->undo);
	 count -= c;
      }
   }

   buf->flags &= (~BUF_CHANGED);

   buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
   cursor_move_line();
   do_wordwrap(WRAP_DELETE);
}



void fn_copy()
{
   LINE *s, *e;
   int si, ei;
   int c;
   BUFFER *buf;

   if (!is_selecting(buffer[0])) {
      strcpy(message, "Nothing selected");
      display_message(MESSAGE_KEY);
      return;
   }

   destroy_kill_buffer(FALSE);

   buf = find_kill_buffer();
   if (!buf)
      return;

   if (buf == buffer[0]) {
      strcpy(message, "Can't copy from the kill buffer into itself");
      display_message(MESSAGE_KEY);
      return;
   }

   get_selection_info(&s, &e, &si, &ei, FALSE);

   while ((s != e) || (si != ei)) {
      if (si >= s->length) {
	 if (!insert_cr(buf, NULL))
	    break;
	 s = s->next;
	 si = 0;
      }
      else {
	 c = s->length - si;
	 if ((s == e) && (si + c > ei))
	    c = ei - si;
	 if (!insert_string(buf, s->text + si, c, NULL))
	    break;
	 si += c;
      }
   }

   buf->flags &= (~BUF_CHANGED);

   strcpy(message, "Block copied");
   display_message(MESSAGE_KEY);
}



void fn_yank()                        /* yank back from kill buffer */
{
   LINE *l;
   BUFFER *buf;

   if (is_selecting(buffer[0]))
      kill_block(FALSE);

   buf = find_kill_buffer();
   if (!buf)
      return;

   if (buf == buffer[0]) {
      strcpy(message, "Can't paste from the kill buffer into itself");
      display_message(MESSAGE_KEY);
      return;
   }

   l = buf->start;

   while (l) {
      if ((l->next) && (buffer[0]->c_pos == 0)) {
	 if (!insert_line(buffer[0], l, &buffer[0]->undo))
	    return;
      }
      else {
	 if (l->length > 0)
	    if (!insert_string(buffer[0], l->text, l->length, &buffer[0]->undo))
	       return;
	 if (l->next)
	    if (!insert_cr(buffer[0], &buffer[0]->undo))
	       return;
      }
      do_wordwrap(WRAP_INSERT);
      l = l->next;
   }
}



void update_clipboard()
{
   BUFFER *buf;
   LINE *line;
   char *tmp = NULL;
   int size = 0;
   int pos = 0;

   buf = find_kill_buffer();
   if (!buf)
      return;

   line = buf->start;

   if (!got_clipboard()) {
      alert_box("Clipboard not available");
      return;
   }

   while (line) {
      if (pos+line->length+2 >= size) {
	 size = (pos+line->length+2+0xFFFF)&0xFFFF0000;
	 tmp = realloc(tmp, size);
      }

      if (line->length > 0) {
	 memcpy(tmp+pos, line->text, line->length);
	 pos += line->length;
      }

      if (line->next) {
	 tmp[pos++] = '\r';
	 tmp[pos++] = '\n';
      }

      line = line->next;
   }

   if (pos > 0) {
      if (!set_clipboard_data(tmp, pos))
	 alert_box("Error sending data to clipboard");
   }

   if (tmp)
      free(tmp);
}



void fn_clip_cut()                  /* clipboard support */
{
   if (!is_selecting(buffer[0])) {
      strcpy(message, "Nothing selected");
      display_message(MESSAGE_KEY);
      return;
   }

   destroy_kill_buffer(FALSE);

   kill_block(TRUE);

   update_clipboard();
}



void fn_clip_copy()                 /* clipboard support */
{
   if (!is_selecting(buffer[0])) {
      strcpy(message, "Nothing selected");
      display_message(MESSAGE_KEY);
      return;
   }

   fn_copy();

   update_clipboard();
}



void fn_clip_yank()                 /* clipboard support */
{
   char *data = NULL;
   BUFFER *buf;
   int size, pos, c;

   if (got_clipboard())
      data = get_clipboard_data(&size);

   if (!data) {
      strcpy(message, "Clipboard data not available");
      display_message(MESSAGE_KEY);
      return;
   }

   destroy_kill_buffer(FALSE);

   buf = find_kill_buffer();

   if (buf) {
      pos = 0;

      while ((pos < size) && (data[pos])) {
	 c = 0; 

	 while ((pos+c < size) && (data[pos+c]) && (data[pos+c] != '\r') && (data[pos+c] != '\n'))
	    c++;

	 if (c > 0) {
	    if (!insert_string(buf, data+pos, c, NULL))
	       break;

	    pos += c;
	 }

	 if ((pos < size) && (data[pos])) {
	    if (!insert_cr(buf, NULL))
	       break;

	    if (data[pos] == '\r')
	       pos++;

	    if ((pos < size) && (data[pos] == '\n'))
	       pos++;
	 }
      }
   }

   buf->flags &= (~BUF_CHANGED);

   free(data);

   fn_yank();
}


