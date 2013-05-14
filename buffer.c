/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Code for managing buffers: load/save/print etc.
 */


#include <ctype.h>

#ifdef DJGPP
#include <dir.h>
#endif

#ifdef TARGET_CURSES
#include <unistd.h>
#include <sys/stat.h>
#endif

#include "fed.h"


#define LINE_LENGTH     16384   /* point at which the line wraps on read */
#define BINARY_LENGTH   64      /* line length in binary mode */
#define HEX_LENGTH      16      /* number of hex characters on a line */



void already_open(int c, int flags)
{
   int c2;
   BUFFER *buf=buffer[c];

   for(c2=c;c2>0;c2--)
      buffer[c2]=buffer[c2-1];

   buffer[0]=buf;

   if ((flags & BUF_BINARY) || (flags & BUF_HEX) || (flags & BUF_ASCII))
      alert_box("File is already open - switching to the existing copy");
}



char scan_file[256];
int scan_flags;
int scan_found;



int scan_for_file(char *n, int depth)
{
   char name[256];

   if (strstr(n, ".."))
      return 0;

   strcpy(name, n);
   strcpy(get_fname(name), scan_file);
   cleanup_filename(name);

   do_the_read(name, scan_flags);

   if (!errno) {
      scan_found = TRUE;
      errno = -1;
   }
   else {
      errno = 0;

      if (depth > 0) {
      #if (defined TARGET_DJGPP) || (defined TARGET_WIN)
	 strcpy(get_fname(name), "*.*");
      #else
	 strcpy(get_fname(name), "*");
      #endif

	 do_for_each_directory(name, scan_for_file, depth-1);
      }
   }

   return errno;
}



int read_file(char *n, int flags)
{
   /* try to read a file, and add it to the buffer list */
   int c;
   char name[256];
   char path[256];
   strcpy(name, n);

   errno=0;
   cleanup_filename(name);

   for (c=0;c<buffer_count;c++) {        /* see if it is already loaded */
      if (stricmp(name,buffer[c]->name)==0) {
	 already_open(c, flags);
	 return 0;
      }
   }

   if (get_fname(n) == n) {
      for (c=0;c<buffer_count;c++)
	 if (stricmp(n,get_fname(buffer[c]->name))==0) {
	    already_open(c, flags);
	    return 0;
	 }
   }

   do_for_each_file(name, do_the_read, flags);

   if (((errno==ENMFILE) || (errno==ENOENT)) && (!strpbrk(name, "*?["))) {
      if ((config.file_search > 0) && (!strpbrk(n, "/\\:"))) {
	 strcpy(message,"Scanning subdirectories");
	 do_display_message(0);
	 refresh_screen();

	 strcpy(path, name);
      #if (defined TARGET_DJGPP) || (defined TARGET_WIN)
	 strcpy(get_fname(path), "*.*");
      #else
	 strcpy(get_fname(path), "*");
      #endif

	 strcpy(scan_file, n);

	 scan_flags = flags;
	 scan_found = FALSE;

	 do_for_each_directory(path, scan_for_file, config.file_search-1);

	 errno = (scan_found) ? 0 : ENOENT;
      }

      if (errno)
	 do_for_each_file(name, do_the_read, flags | BUF_MAKENEW);
   }

   if (buffer_count > 0)
      if ((buffer[0]->flags & BUF_TRUNCATED) && (errno==ENOMEM))
	 errno = 0;

   if ((errno!=0) && (errno!=-1))
      alert_box(err());

   return errno;
}



int do_the_read(char *n, int flags)
{
   /* try to read a file, and add it to the buffer list */

   BUFFER *buf=NULL;
   MYFILE *f=NULL;
   LINE *l=NULL;
   int c, c2;
   char *workspace=NULL;
   char name[256];

   if (buffer_count>=MAX_BUFFERS) {
      alert_box("Too many open files");
      errno = -1;
      return -1;
   }

   strcpy(name, n);
   cleanup_filename(name);

   for (c=0;c<buffer_count;c++) {         /* see if it is already loaded */
      if (stricmp(name,buffer[c]->name)==0) {
	 buf=buffer[c];
	 for(c2=c;c2>0;c2--)
	    buffer[c2]=buffer[c2-1];
	 buffer[0]=buf;
	 return 0;
      }
   }

   if ((buf=malloc(sizeof(BUFFER)))==NULL) {
      errno=ENOMEM;
      goto get_out;
   }

   if ((workspace=malloc(LINE_LENGTH))==NULL) {
      errno=ENOMEM;
      goto get_out;
   }

   strcpy(buf->name, (flags & BUF_BROWSE) ? "errors" : name);

   buf->flags=0;

   if (flags & BUF_BINARY)
      buf->flags |= BUF_BINARY;
   else if (flags & BUF_HEX)
      buf->flags |= BUF_HEX;
   else if (!(flags & BUF_ASCII)) {
      if (ext_in_list(name, config.bin, NULL)) {
	 buf->flags |= BUF_BINARY;
	 flags |= BUF_BINARY;
      }
      else if (ext_in_list(name, config.hex, NULL)) {
	 buf->flags |= BUF_HEX;
	 flags |= BUF_HEX;
      }
   }

   if (buf->flags & BUF_BINARY)
      buf->flags |= BUF_OVERWRITE;

   if (flags & BUF_BROWSE)
      buf->flags |= BUF_BROWSE;

   if (flags & (BUF_READONLY | BUF_FORCEREADONLY))
      buf->flags |= (BUF_READONLY | BUF_FORCEREADONLY);

   buf->wrap_col = 0;

   if (access(name, W_OK) != 0)
      buf->flags |= BUF_READONLY;

   buf->filetime = file_time(name);
   buf->hscroll=0;
   buf->c_pos=0;
   buf->old_c_pos=0;
   buf->sel_line=NULL;
   buf->sel_pos=0;
   buf->sel_offset=0;
   buf->start = buf->top = buf->c_line = NULL;
   buf->cached_line = -1;
   buf->_cached_line = NULL;
   buf->cached_top = -1;
   buf->_cached_top = NULL;
   buf->cached_size = -1;
   buf->undo = buf->redo = NULL;
   buf->last_function = -1;
   buf->syntax = NULL;

   f = open_file(name, FMODE_READ);

   if (errno!=0) {
      if (((errno==ENMFILE) || (errno==ENOENT)) && (flags & BUF_MAKENEW)) {
	 errno=0;
	 buf->flags |= BUF_NEW;
	 buf->flags &= ~BUF_READONLY;
    #ifndef DJGPP
	 buf->flags |= BUF_UNIX;
    #endif
	 buf->start = buf->top = buf->c_line = create_line(0);
	 if (buf->start==NULL)
	    errno=ENOMEM;
      }
      goto get_out;
   }

   strcpy(message,"Reading ");
   if (flags & BUF_BINARY)
      strcat(message,"(bin) ");
   else if (flags & BUF_HEX)
      strcat(message,"(hex) ");
   strcat(message,name);
   do_display_message(0);
   refresh_screen();

   if (flags & BUF_BROWSE) {
      l = create_line(10);
      memmove(l->text, "<messages>", 10);
   }
   else
      l=read_line(f, flags, &buf->flags, workspace);  /* read first line */

   if ((errno==0) || (errno==EOF)) {
      buf->start = buf->top = buf->c_line = l;
      l->line_no = 1;
   }

   c = 1;
   while (errno==0) {                       /* read the other lines */
      l->next = l->fold = read_line(f,flags,&buf->flags,workspace);
      if (l->next) {        /* set up the links */
	 l->next->prev = l;
	 l->next->line_no = ++c;
	 l = l->next;
      }
   }

   close_file(f);

   if (errno == EOF)
      errno = 0;

   if (errno==ENOMEM) {             /* ask whether to truncate or abort */
      if (ask("Error", "Out of memory: truncate", NULL)) {
	 buf->flags |= BUF_TRUNCATED;
	 errno=0;
      }
   }

   get_out:

   if (workspace)
      free(workspace);

   if (errno!=0) {                  /* in case of errors */
      if (buf)
	 destroy_buffer(buf);
   }
   else {             /* store the file information */
      if ((ext_in_list(name, config.auto_indent, buf)) || (flags & BUF_INDENT))
	 buf->flags |= BUF_INDENT;

      if ((ext_in_list(name, config.wrap, buf)) || (flags & BUF_WORDWRAP))
	 buf->wrap_col = config.wrap_col;

      buf->syntax = get_syntax(buf->name, buf);

      for (c=buffer_count; c>0; c--)
	 buffer[c]= buffer[c-1];

      buffer_count++;

      buffer[0] = buf;
      buffer[0]->undo = new_undo_node(NULL);

      if (buf->flags & BUF_TRUNCATED)
	 errno = ENOMEM;

      if ((!(flags & BUF_BINARY)) && (!(flags & BUF_HEX)))
	 if ((flags & BUF_FOLD) || (ext_in_list(name,config.auto_fold, buf)))
	    fold_all();
   }

   return errno;
}



LINE *read_line(MYFILE *f, int flags, int *flag_ret, char *workspace)
{
   /* reads a line from the file. Errors are reported in errno */

   int c;              /* temporary character store */
   int len=0;          /* line length */
   char *p;            /* current text pos, for speed */
   LINE *l;            /* the line */

   p=workspace;

   if (flags & BUF_BINARY) {           /* easy case... */
      while (len<BINARY_LENGTH) {
	 *p = get_char(f);
	 if (errno != 0)
	    goto get_out;
	 len++;
	 p++;
      }
   }
   else if (flags & BUF_HEX) {         /* this is pretty easy as well... */
      while (len<HEX_LENGTH) {
	 c = get_char(f);
	 if (errno != 0)
	    goto get_out;
	 *(p++) = hex_digit[c >> 4];
	 *(p++) = hex_digit[c & 15];
	 *(p++) = ' ';
	 len++;
      }
   }
   else {            /* have to do some more work to deal with text files */

      while (len<LINE_LENGTH) {

	 c = get_char(f);

	 if (errno != 0)
	    goto get_out;

	 if (c==CR) {                   /* EOL, nice and flexible */
	    *flag_ret &= ~BUF_UNIX;
	    if (peek_char(f)==LF)
	       get_char(f);
	    goto get_out;
	 }

	 if (c==LF) {
	    *flag_ret |= BUF_UNIX;
	    goto get_out;
	 }

	 if ((config.load_tabs != 0) && (c==TAB)) {  /* fake tabs */
	    do {
	       *(p++)=' ';
	       len++;
	    } while (((len/config.load_tabs)*config.load_tabs != len) && (len<LINE_LENGTH));
	 }
	 else {  /* actually insert the character */
	    *(p++)=c;
	    len++;
	 }
      }

      *flag_ret |= BUF_WRAPPED;
   }

   get_out:             /* store the results */

   if ((errno!=0) && (errno!=EOF))
      return NULL;

   c=(int)(p-workspace);                /* length of the text */
   if ((l=create_line(c)) != NULL) {    /* put everything into a line */
      l->length = l->size = c;
      memmove(l->text, workspace, c);
   }
   else
      errno=ENOMEM;

   return l; 
}



void destroy_buffer(BUFFER *b)       /* free up a buffer */
{
   LINE *pos, *prev;

   if (b) {
      pos=b->start;
      while(pos) {
	 prev=pos;
	 pos=pos->next;
	 if (prev->text)
	    free(prev->text);
	 free(prev);
      }
      destroy_undolist(b->undo);
      destroy_undolist(b->redo);
      free(b);
   }
}



void fn_close()              /* close the current buffer */
{
   int c;

   if (buffer[0]->flags & BUF_CHANGED)
      if (!ask("Close", "Abandon changes to ", buffer[0]->name))
	 return;

   destroy_buffer(buffer[0]);

   buffer_count--;
   for (c=0;c<buffer_count;c++)
      buffer[c]=buffer[c+1];

   if (buffer_count > 0) {
      for (c=0; c<buffer_count; c++) {
	 if (((!(buffer[c]->flags & BUF_BROWSE)) && 
	      (!(buffer[c]->flags & BUF_KILL))) ||
	     (buffer[c]->flags & BUF_CHANGED)) {
	    display_new_buffer();
	    return;
	 }
      }
   }

   fn_quit();
}



void fn_quit()           /* quit, prompting if there are unsaved buffers */
{
   int c;

   for (c=0;c<buffer_count;c++) {
      if (buffer[c]->flags & BUF_CHANGED) {
	 if (!ask("Quit", "Abandon Changes", NULL))
	    return;
	 else
	    break;
      }
   }

   for (c=0;c<buffer_count;c++)
      destroy_buffer(buffer[c]);

   buffer_count = 0;
   exit_flag = 1;
}



void fn_help()               /* display the help file */
{
   int c, c2;
   BUFFER *buf;
   LINE *l, *last;
   extern char *help_text[];

   for (c=0;c<buffer_count;c++) {                  /* already loaded? */
      if (stricmp("help", buffer[c]->name)==0) {
	 buf=buffer[c];
	 for(c2=c;c2>0;c2--)
	    buffer[c2]=buffer[c2-1];
	 buffer[0]=buf;
	 display_new_buffer();
	 return;
      }
   }

   errno = 0;

   buf=malloc(sizeof(BUFFER));

   if (!buf) {
      errno = ENOMEM;
   }
   else {
      strcpy(buf->name, "help");
      buf->flags = BUF_READONLY | BUF_FORCEREADONLY;
      buf->filetime = -1;
      buf->wrap_col = 0;
      buf->hscroll=0;
      buf->c_pos=0;
      buf->old_c_pos=0;
      buf->sel_line=NULL;
      buf->sel_pos=0;
      buf->sel_offset=0;
      buf->start = buf->top = buf->c_line = NULL;
      buf->cached_line = -1;
      buf->cached_top = -1;
      buf->cached_size = -1;
      buf->undo = NULL;
      buf->redo = NULL;
      buf->last_function = -1;
      buf->syntax = NULL;
      last = NULL;

      for (c=0; help_text[c]; c++) {
	 l = create_line(strlen(help_text[c]));
	 if (!l) {
	    destroy_buffer(buf);
	    return;
	 }
	 memmove(l->text, help_text[c], strlen(help_text[c]));

	 if (last) {
	    l->prev = last;
	    last->next = last->fold = l;
	 }
	 else
	    buf->start = buf->top = buf->c_line = l;

	 last = l;
      }
   }

   if (errno) {
      alert_box(err());
      errno=0;
   }
   else {
      for (c=buffer_count; c>0; c--)
	 buffer[c]= buffer[c-1];
      buffer_count++;
      buffer[0]=buf;
      fold_all();
   }

   display_new_buffer();
}



int update_file_status()
{
   int c, i;
   long t;
   int flags;
   char name[256];
   int changed = FALSE;
   int changed_this_time;

   if (!config.check_files)
      return FALSE;

   do {
      changed_this_time = FALSE;

      for (i=0; i<buffer_count; i++) {
	 if (file_exists(buffer[i]->name)) {
	    if (!(buffer[i]->flags & BUF_FORCEREADONLY)) {
	       if (access(buffer[i]->name, W_OK) != 0)
		  buffer[i]->flags |= BUF_READONLY;
	       else
		  buffer[i]->flags &= ~BUF_READONLY;
	    }

	    t = file_time(buffer[i]->name);
	    if ((t >= 0) && (buffer[i]->filetime >= 0) && 
		(t > buffer[i]->filetime)) {
	       if (ask("File Changed", "Reload ", buffer[i]->name)) {
		  strcpy(name, buffer[i]->name);
		  flags = buffer[i]->flags & (BUF_BINARY | BUF_HEX | BUF_ASCII |
					      BUF_FORCEREADONLY);

		  destroy_buffer(buffer[i]);
		  buffer_count--;
		  for (c=i;c<buffer_count;c++)
		     buffer[c]=buffer[c+1];

		  read_file(name, flags);
		  display_new_buffer();
		  changed = changed_this_time = TRUE;
	       }
	       else
		  buffer[i]->filetime = t;
	    }
	 }
      }
   } while (changed_this_time);

   return changed;
}



void run_tool(char *cmd, char *desc)
{
   char path[256];
   static char user_input[80] = "";
   char buf[80];
   char command[256];
   int saved = FALSE;
   int c, c2;
   int ret;
   char *p;
   int save_altered = TRUE;
   int hold_screen = TRUE;
   int read_errors = FALSE;
   int filter = FALSE;
   int filter_redir = FALSE;
   BUFFER *kill;
   LINE *l;

 #if (defined DJGPP) || (defined TARGET_WIN)
   int disk;
 #endif

   while ((*cmd) && (*cmd != '|'))        /* skip command menu description */
      cmd++;
   if (*cmd)
      cmd++;
   if (*cmd == 0) {
      alert_box("No command in tool definition");
      return;
   }

   c = 0;
   while ((*cmd) && (c < 160)) {          /* parse command tokens */
      if (*cmd == '%') {
	 cmd++;
	 switch (*(cmd++)) {

	    case 'e':                     /* environment variable */
	    case 'E':
	       c2 = 0;
	       while ((*cmd) && (*cmd != '%'))
		  buf[c2++] = *(cmd++);
	       buf[c2] = 0;
	       if (*cmd)
		  cmd++;
	       strupr(buf);
	       p = getenv(buf);
	       if (p) {
		  strcpy(command+c, p);
		  while (command[c])
		     c++;
	       }
	       break;

	    case 'f':                     /* name of the current file */
	    case 'F':
	       strcpy(command+c, buffer[0]->name);
	       while (command[c])
		  c++;
	       break;

	    case 'h':                     /* don't hold screen */
	    case 'H':
	       hold_screen = FALSE;
	       break;

	    case 'n':                     /* use the tool as a text filter */
	       filter_redir = TRUE;
	       /* fall through */

	    case 'N':
	       if (buffer[0]->flags & BUF_READONLY) {
		  strcpy(message,"File is read-only");
		  display_message(MESSAGE_KEY);
		  return;
	       }
	       filter = TRUE;
	       break;

	    case 'p':                     /* prompt user for text */
	    case 'P':
	       if (input_text(desc, user_input, 60, is_anychar)==ESC)
		  return;
	       strcpy(command+c, user_input);
	       while (command[c])
		  c++;
	       break;

	    case 's':                     /* don't save altered files */
	    case 'S':
	       save_altered = FALSE;
	       break;

	    case 'w':                     /* word under the cursor */
	    case 'W':
	       get_word(command+c);
	       if (!command[c]) {
		  if (input_text(desc, user_input, 60, is_anychar)==ESC)
		     return;
		  strcpy(command+c, user_input);
	       }
	       while (command[c])
		  c++;
	       break;
	 } 
      }
      else
	 command[c++] = *(cmd++);
   }

   if (filter) {
      if (filter_redir)
	 strcpy(command+c, " < " FILT_IN_FILE " > " FILT_OUT_FILE);
      else
	 command[c] = 0;

      if (is_selecting(buffer[0]))
	 fn_copy();
      else
	 destroy_kill_buffer(FALSE);

      kill = find_kill_buffer();

      strcpy(buf, kill->name);
      strcpy(kill->name, FILT_IN_FILE);

      write_file(kill);

      strcpy(kill->name, buf);

      if (errno != 0) {
	 n_vid();
	 dirty_everything();
	 return;
      }

      if (hold_screen) {
	 goto2(0,screen_h-1);
	 cr_scroll();
	 saved = TRUE;
      }
   }
   else
      command[c] = 0;

   if (save_altered) {
      for (c=0; c<buffer_count; c++) {       /* save altered files */
	 if (buffer[c]->flags & BUF_CHANGED) {
	    write_file(buffer[c]);
	    if (errno != 0) {
	       n_vid();
	       dirty_everything();
	       return;
	    }
	    if ((!filter) || (hold_screen)) {
	       goto2(0,screen_h-1);
	       cr_scroll();
	       saved = TRUE;
	    }
	 }
      }
   }

   goto2(0,screen_h-1);

   if ((!saved) && ((!filter) || (hold_screen)))
      cr_scroll();

   tattr(norm_attrib);
   mywrite(command);
   del_to_eol();

   if ((!filter) || (hold_screen)) {
      cr_scroll();
      del_to_eol();
   }

 #if (defined DJGPP) || (defined TARGET_WIN)
   disk = getdisk();
 #endif

   getcwd(path, 256);
   show_c();
   term_suspend((!filter) || (hold_screen));

   delete_file(ERROR_FILE);

   errno = 0;
   ret = system(command);                 /* run it! */
   c = (errno == EACCES) ? 0 : errno;

   chdir(path);

 #if (defined DJGPP) || (defined TARGET_WIN)
   setdisk(disk);
 #endif

   if (file_size(ERROR_FILE) > 0) {
      read_errors = TRUE;
      term_reinit(FALSE);
   }
   else if ((ret) || (c) || (hold_screen)) {
      if (ret) {
	 itoa(ret, buf, 10);
	 printf("\nExit code %s", buf);
      }
      else if ((c) && (c != ENOENT)) {
	 errno = c;
	 printf("\n<%s>", err());
      }

   #ifndef TARGET_WIN
      printf("\n<press a key>");
      fflush(stdout);
   #endif

      term_reinit(TRUE);
   }
   else
      term_reinit(FALSE);

   n_vid();
   tattr(attrib);
   dirty_everything();

   if (read_errors) {
      for (c=0; c<buffer_count; c++) {
	 if (buffer[c]->flags & BUF_BROWSE) {
	    destroy_buffer(buffer[c]);
	    buffer_count--;
	    for (c2=c; c2<buffer_count; c2++)
	       buffer[c2] = buffer[c2+1];
	 }
	 else {
	    l = buffer[c]->start;
	    c2 = 1;
	    while (l) {
	       l->line_no = c2++;
	       l = l->next;
	    }
	 }
      }

      read_file(ERROR_FILE, BUF_BROWSE);     /* read error list */

      strcpy(buf,"Exit code ");
      itoa(ret, buf+strlen(buf), 10);
      l=buffer[0]->start;
      while (l->next)
	 l = l->next;
      l->next = l->fold = create_line(strlen(buf));
      memmove(l->next->text, buf, strlen(buf));
      l->next->prev = l;

      display_new_buffer();
      browse_goto(0, FALSE);
   }
   else if ((filter) && ((!ret) && (!c)) && (file_size(FILT_OUT_FILE) > 0)) {
      destroy_kill_buffer(TRUE);
      read_file(FILT_OUT_FILE, 0);

      if (errno == 0) {
	 kill = buffer[0];
	 buffer[0] = buffer[1];
	 buffer[1] = kill;

	 kill->flags |= BUF_KILL;
	 fn_yank();

	 destroy_kill_buffer(TRUE);
      }

      update_file_status();
   }
   else
      update_file_status();

   if (filter) {
      delete_file(FILT_IN_FILE);
      delete_file(FILT_OUT_FILE);
   }

   delete_file(ERROR_FILE);
   errno = 0;
}



#define TOOL_FUNC(x)    \
	    void fn_tool ## x() { run_tool(config.tool[x-1], tool_str[x-1]); }

TOOL_FUNC(1);
TOOL_FUNC(2);
TOOL_FUNC(3);
TOOL_FUNC(4);
TOOL_FUNC(5);
TOOL_FUNC(6);
TOOL_FUNC(7);
TOOL_FUNC(8);
TOOL_FUNC(9);
TOOL_FUNC(10);
TOOL_FUNC(11);
TOOL_FUNC(12);
TOOL_FUNC(13);
TOOL_FUNC(14);
TOOL_FUNC(15);
TOOL_FUNC(16);



void scan_for_filename(char *str)
{
   LINE *l = buffer[0]->c_line;
   int pos = buffer[0]->c_pos;
   int len = 0;
   int c = char_from_line(l,pos);

   if ((c == '"') || (c == '<') || (c == '\'')) {
      pos++;
      c = char_from_line(l,pos);
   }

   if (is_filechar_nospace(c)) {

      while (is_filechar_nospace(c)) {
	 pos--;
	 c = char_from_line(l, pos);
      }

      if ((c != '"') && (c != '<') && (c != '\'')) {
	 str[0] = 0;
	 return;
      }

      pos++;
      c = char_from_line(l, pos);

      while (is_filechar_nospace(c)) {
	 str[len++] = l->text[pos];
	 str[len] = 0;
	 pos++;
	 c = char_from_line(l, pos);
      }

      if ((c != '"') && (c != '>') && (c != '\'')) {
	 str[0] = 0;
	 return;
      }

      if (str[0]) {
	 char fname[256];
	 if (search_path(fname, str, "", "INCLUDE"))
	    strcpy(str, fname);
	 else if (search_path(fname, str, "", "FED_INCLUDE"))
	    strcpy(str, fname);
	 cleanup_filename(str);
	 errno = 0;
      }
   }
   else
      str[0] = 0;
}



void fn_open()               /* open a file */
{
   char fname[256];
   int res;
   int flags = 0;

   scan_for_filename(fname);

   #ifdef TARGET_CURSES
      res=select_file("Open (alt+enter for binary mode)", fname);
   #else
      res=select_file("Open (ctrl+enter for binary mode)", fname);
   #endif

   if (res==ESC)
      return;

   if (res==LF) {
      flags = open_file_type();
      if (flags < 0)
	 return;
   }

   read_file(fname, flags);
   display_new_buffer();
}



void fn_next_file(int key)       /* switch to the next file */
{
   BUFFER *tmp;
   int c;
   int next_pos = 0;
   int cont_flag;
   int new_key;
   extern int message_flags;

   if (buffer_count>1) {

      do {
	 cont_flag = FALSE;

	 tmp=buffer[0];
	 for(c=0; c<next_pos; c++)
	    buffer[c]=buffer[c+1];
	 buffer[next_pos] = tmp;

	 next_pos++;
	 if (next_pos >= buffer_count)
	    next_pos=0;

	 tmp=buffer[next_pos];
	 for(c=next_pos; c>0; c--)
	    buffer[c]=buffer[c-1];
	 buffer[0]=tmp;

	 display_new_buffer();
	 redisplay();

	 if (!auto_input_waiting()) {
      #ifdef TARGET_CURSES
	    key = ' ';
	    while (1) {
      #else
	    while (ctrl_pressed()) {
      #endif
	       if (keypressed()) {
		  new_key = gch();
		  if (new_key == key)
		     cont_flag = TRUE;
		  else
		     un_getc(new_key);
		  break;
	       }
	    }
	 }

      } while(cont_flag);

      if (!config.show_pos)
	 message_flags = MESSAGE_KEY;
   }
   else
      display_new_buffer();
}



void fn_write()
{
   char fname[256];
   int res;
   LINE *l;

   strcpy(fname,buffer[0]->name);
   res=select_file("Write", fname);

   if (res!=ESC) {
      cleanup_filename(fname);

      if (stricmp(buffer[0]->name, fname) !=0 ) {
	 if (file_exists(fname)) {
	    if (!ask("Save", fname, " already exists - overwrite"))
	       return;
	 }
	 strcpy(buffer[0]->name,fname);
	 buffer[0]->syntax = get_syntax(fname, buffer[0]);
	 for (l=buffer[0]->start; l; l=l->next)
	    l->comment_state = l->comment_effect = COMMENT_UNKNOWN;
	 disp_dirty();
      }

      buffer[0]->flags |= BUF_CHANGED;
      write_file(buffer[0]);
      display_new_buffer();
   }
}



void fn_save_all()
{
   int c;
   int saved_one = FALSE;

   tattr(norm_attrib);

   while (buffer_count > 0) {

      if (buffer[0]->flags & BUF_CHANGED) {
	 if (saved_one) {
	    goto2(0,screen_h-1);
	    cr_scroll();
	 }

	 write_file(buffer[0]);
	 saved_one = TRUE;
	 if (errno != 0) {
	    dirty_everything();
	    return;
	 }
      }

      destroy_buffer(buffer[0]);

      buffer_count--;
      for (c=0;c<buffer_count;c++)
	 buffer[c]=buffer[c+1];
   }
}



void write_file(BUFFER *buf)
{
   char backname[256];
   int madeback = FALSE;
   MYFILE *f, *f2;
   LINE *l = buf->start;
   int flags = buf->flags;
   char *e;
   int c;

   if (buf->flags & BUF_TRUNCATED) {
      if (ask("Save", buf->name, " was truncated - write anyway"))
	 buf->flags ^= BUF_TRUNCATED;
      else {
	 errno = EOF;
	 return;
      }
   }

   strcpy(backname, buf->name);
   e = find_extension(backname);
   if ((e<=backname) || (e[-1]!='.'))
      *(e++) = '.';
   strcpy(e, "bak");

   if (stricmp(buf->name, backname) != 0) {
      f = open_file(buf->name, FMODE_READ);
      f2 = open_file(backname, FMODE_WRITE);

      if ((f) && (f2)) {
	 c = get_char(f);

	 while (!errno) {
	    put_char(f2, c); 
	    c = get_char(f);
	 }
      }

      if (f)
	 close_file(f);

      if (f2)
	 close_file(f2);

      madeback = TRUE;
      errno = 0;
   }

   strcpy(message,"Writing ");
   if (buf->flags & BUF_BINARY)
      strcat(message,"(bin) ");
   else if (buf->flags & BUF_HEX)
      strcat(message,"(hex) ");
   strcat(message,buf->name);
   do_display_message(0);
   refresh_screen();

   f=open_file(buf->name, FMODE_WRITE);

   if (errno == 0) {
      while ((errno==0) && (l)) {
	 write_line(f,l,flags);
	 l = l->next;
      }

      close_file(f);

      if (errno != 0) {
	 delete_file(buf->name);
	 buf->filetime = -1;
	 buf->flags |= BUF_CHANGED;
      }
      else {
	 buf->filetime = file_time(buf->name);
	 buf->flags &= ~BUF_CHANGED;
      }
   }

   if (errno != 0)
      alert_box(err());
   else {
      if ((!config.make_backups) && (madeback)) {
	 delete_file(backname);
	 errno = 0;
      }
   }
}



void write_line(MYFILE *f, LINE *l, int flags)
{
   int c, c2, c3;
   char *pos;

   if (flags & BUF_HEX) {
      c = 0;
      while (c < l->length-1) {
	 int d1 = tolower(l->text[c]);
	 int d2 = tolower(l->text[c+1]);
	 if (l->text[c] != ' ') {
	    if ((((d1 > '9') || (d1 < '0')) && ((d1 > 'f') || (d1 < 'a'))) ||
		(((d2 > '9') || (d2 < '0')) && ((d2 > 'f') || (d2 < 'a')))) {
	       buffer[0]->c_line = l;
	       buffer[0]->c_pos = c;
	       buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
	       unfold(l);
	       cursor_move_line();
	       errno = ENOEXEC;
	       return;
	    }
	    c2 = VALUE(d1);
	    c3 = VALUE(d2);
	    put_char(f, (c2 << 4) + c3);
	    c++;
	 } 
	 c++;
      } 
   }
   else {
      c = get_leading_chars(l);
      c2 = get_leading_bytes(l);

      if ((c2<l->length) || (!config.save_strip) || (flags & BUF_BINARY)) {

	 if ((flags & BUF_BINARY) || ((!config.save_strip) && (config.load_tabs == 0))) {
	    c2 = 0;
	 }
	 else {
	    while (c > 0) {              /* write leading spaces */
	       if ((!(flags & BUF_BINARY)) && (config.save_strip) &&
		   (((config.load_tabs > 0) && (c >= config.load_tabs)) ||
		    ((config.load_tabs == 0) && (c >= TAB_SIZE)))) {
		  if (config.load_tabs)
		     c -= config.load_tabs;
		  else
		     c -= TAB_SIZE;
		  put_char(f, TAB);
	       }
	       else {
		  c--;
		  put_char(f,' ');
	       }
	       if (errno != 0)
		  return;
	    }
	 }

	 pos = l->text + c2;
	 c2 = l->length - c2;

	 if ((!(flags & BUF_BINARY)) && (config.save_strip))
	    while ((c2 > 1) && (pos[c2-1]==' ') && (pos[c2-2]==' '))
	       c2--;

	 for (c=0; c<c2; c++) {
	    put_char(f,*pos);
	    pos++;
	    if (errno != 0)
	       return;
	 }
      }

      if ((!(flags & BUF_BINARY)) && (l->next)) {
	 if (!(flags & BUF_UNIX))
	    put_char(f,CR);
	 if (errno == 0)
	    put_char(f,LF);
      }
   }
}



void fn_print()
{
   LINE *l;
   static int indent=4;
   int x;
   extern int input_cancelled;

   if (!(buffer[0]->flags & BUF_BINARY)) {
      indent = input_number("Print indent", indent);
      if (input_cancelled)
	 return;
   }

   if (!printer_ready()) {
      alert_box("Printer not ready");
      return;
   }

   strcpy(message,"Printing...");
   display_message(0);
   clear_keybuf();
   hide_c();

   l = buffer[0]->start;

   while (l) {

      for (x=0; x<l->length; x++) {
	 if (check_abort())
	    return;
	 if ((is_asciichar(l->text[x])) || (buffer[0]->flags & BUF_BINARY) ||
	     (!config.print_ascii))
	    print(l->text[x]);
	 else
	    print(' ');
      }

      l = l->next;

      if ((l) && (!(buffer[0]->flags & BUF_BINARY))) {
	 print(CR);
	 print(LF);
      }
   }

   print(FF);

   strcpy(message,"Printing completed");
   display_message(MESSAGE_KEY);
}



void read_browse_files()
{
   char buf[256];
   int c, pos;

   c = 0;

   while (config.makefiles[c]) {
      while ((config.makefiles[c]==' ') || 
	     (config.makefiles[c]==',') || 
	     (config.makefiles[c]==';'))
	 c++;

      if (config.makefiles[c]) {
	 buf[0] = '*';
	 buf[1] = '.';
	 pos = 2;
	 while ((pos < 10) && (config.makefiles[c]) &&
		(config.makefiles[c] != ' ') && 
		(config.makefiles[c] != ',') && 
		(config.makefiles[c] != ';')) {
	    buf[pos++] = config.makefiles[c++];
	 }
	 buf[pos] = 0;

	 if (file_exists(buf))
	    read_file(buf, 0);
      }
   }

   errno = 0;
}

