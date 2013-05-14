/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Startup code and main event loop
 */


#include <ctype.h>
#include <time.h>
#include <limits.h>

#ifdef DJGPP
#include <crt0.h>
#include <sys/exceptn.h>
#endif

#include "fed.h"

char hex_digit[] = "0123456789ABCDEF";

int buffer_count=0;                 /* the current number of buffers */
BUFFER *buffer[MAX_BUFFERS];        /* buffer[0] is the active one */

int repeat_count=0;                 /* how many times to repeat function */
int repeat_function=0;
int repeat_key=-1;

int last_function=0;                /* previous function */

int exit_flag=0;                    /* exit code to return to caller */

char *exe_path = NULL;              /* stored copy of argv[0] */
char exe_buf[256];

extern int macro_mode;



int valid_word()
{
   return (get_state(buffer[0]->c_line, buffer[0]->c_pos) == STATE_WORD);
}



int valid_line()
{
   return (get_leading_bytes(buffer[0]->c_line) < buffer[0]->c_line->length);
}



int valid_fold()
{
   if (is_selecting(buffer[0]))
      return TRUE;

   return (get_leading_bytes(buffer[0]->c_line) < buffer[0]->c_line->length);
}



int valid_filename()
{
   char b[256];
   scan_for_filename(b);
   return (b[0] != 0);
}



int valid_block()
{
   return (is_selecting(buffer[0]));
}



int valid_paste()
{
   int c;

   for (c=0; c<buffer_count; c++)
      if (buffer[c]->flags & BUF_KILL)
	 if ((buffer[c]->start->next) || (buffer[c]->start->length > 0))
	    return TRUE;

   return FALSE;
}



int valid_lastpos()
{
   return ((remember_filename[0]) && (remember_line > 0));
}



int valid_brace()
{
   int c = char_from_line(buffer[0]->c_line, buffer[0]->c_pos);
   return (brace_direction(c) != 0);
}



int valid_browse()
{
   int c;

   for (c=0; c<buffer_count; c++)
      if (buffer[c]->flags & BUF_BROWSE)
	 return TRUE;

   return FALSE;
}



int valid_undo()
{
   UNDO *undo = buffer[0]->undo;

   while ((undo) && (undo->length==0) && 
	  (undo->cached_operation == UNDO_NOTHING))
      undo = undo->next;

   return (undo != NULL);
}



int valid_redo()
{
   UNDO *redo = buffer[0]->redo;

   while ((redo) && (redo->length==0) && 
	  (redo->cached_operation == UNDO_NOTHING))
      redo = redo->next;

   return (redo != NULL);
}



/* The function code -> function mapping
 */

FN_MAP fn_map[] =
{
   { fn_char,           FN_EDIT | FN_REP, NULL }, 

   /* file menu */
   { fn_open,           FN_MOVE,    valid_filename }, 
   { fn_close,          0,          NULL }, 
   { fn_write,          0,          NULL }, 
   { fn_print,          0,          NULL }, 
   { fn_view,           0,          NULL }, 
   { fn_next_file,      0,          NULL }, 
   { fn_quit,           0,          NULL }, 
   { fn_save_all,       0,          NULL }, 

   /* edit menu */
   { fn_undo,           FN_EDIT | FN_REP | FN_DESEL,           valid_undo },
   { fn_redo,           FN_EDIT | FN_REP | FN_DESEL,           valid_redo },
   { fn_kill_line,      FN_EDIT | FN_REP,                      valid_block },
   { fn_copy,           0,                                     valid_block },
   { fn_yank,           FN_MOVE_NS | FN_EDIT | FN_REP,         valid_paste },
   { fn_lowcase,        FN_MOVE | FN_EDIT | FN_REP | FN_DESEL, valid_word },
   { fn_upcase,         FN_MOVE | FN_EDIT | FN_REP | FN_DESEL, valid_word },
   { fn_ascii,          FN_MOVE | FN_EDIT,                     NULL },
   { fn_transpose,      FN_MOVE | FN_EDIT | FN_REP | FN_DESEL, NULL },
   { fn_indent,         0,                                     NULL },
   { fn_wordwrap,       0,                                     NULL },
   { fn_reformat,       FN_MOVE | FN_EDIT | FN_REP | FN_DESEL, valid_line },

   /* search menu */
   { fn_position,       FN_MOVE,             NULL },
   { fn_goto,           0,                   NULL },
   { fn_remember,       FN_MOVE,             NULL },
   { fn_lastpos,        0,                   valid_lastpos },
   { fn_match,          FN_MOVE,             valid_brace },
   { fn_search,         0,                   NULL }, 
   { fn_grep,           FN_DESEL,            NULL }, 
   { fn_browse,         FN_MOVE | FN_DESEL,  valid_word }, 
   { fn_browse_next,    FN_REP | FN_DESEL,   valid_browse }, 
   { fn_browse_prev,    FN_REP | FN_DESEL,   valid_browse }, 

   /* misc menu */
   { fn_fold,           FN_MOVE_NS, valid_fold },
   { fn_expand,         FN_DESEL,   NULL },
   { fn_macro_s,        0,          NULL },
   { fn_macro_e,        0,          NULL },
   { fn_macro_p,        FN_REP,     NULL },
   { fn_repeat,         0,          NULL },

   /* tool menu */
   { fn_tool1,          0,          NULL },
   { fn_tool2,          0,          NULL },
   { fn_tool3,          0,          NULL },
   { fn_tool4,          0,          NULL },
   { fn_tool5,          0,          NULL },
   { fn_tool6,          0,          NULL },
   { fn_tool7,          0,          NULL },
   { fn_tool8,          0,          NULL },
   { fn_tool9,          0,          NULL },
   { fn_tool10,         0,          NULL },
   { fn_tool11,         0,          NULL },
   { fn_tool12,         0,          NULL },
   { fn_tool13,         0,          NULL },
   { fn_tool14,         0,          NULL },
   { fn_tool15,         0,          NULL },
   { fn_tool16,         0,          NULL },

   /* config menu */
   { fn_screen,         0,          NULL },
   { fn_srch_mode,      0,          NULL },
   { fn_tabsize,        0,          NULL },
   { fn_config,         0,          NULL },
   { fn_colors,         0,          NULL },
   { fn_keymap,         0,          NULL },
   { fn_savecfg,        0,          NULL },

   /* help menu */
   { fn_help,           0,          NULL },
   { fn_tetris,         0,          NULL },
   { fn_about,          0,          NULL },

   /* things that aren't in the menus */
   { fn_block,          FN_MOVE,             NULL },
   { fn_select_word,    FN_MOVE,             valid_word },
   { fn_insert,         0,                   NULL },

   { fn_backspace,      FN_MOVE | FN_EDIT | FN_REP,    NULL },
   { fn_delete,         FN_MOVE | FN_EDIT | FN_REP,    NULL },
   { fn_kill_word,      FN_MOVE | FN_EDIT | FN_REP,    NULL },
   { fn_e_kill,         FN_MOVE | FN_EDIT | FN_REP,    NULL },

   { fn_return,         FN_MOVE | FN_EDIT | FN_REP,    NULL },
   { fn_tab,            FN_MOVE | FN_EDIT | FN_REP,    NULL },

   { fn_left,           FN_MARK | FN_REP,    NULL },
   { fn_right,          FN_MARK | FN_REP,    NULL },
   { fn_word_left,      FN_MARK | FN_REP,    NULL },
   { fn_word_right,     FN_MARK | FN_REP,    NULL },
   { fn_line_start,     FN_MARK | FN_REP,    NULL },
   { fn_line_end,       FN_MARK | FN_REP,    NULL },
   { fn_up,             FN_MARK | FN_REP,    NULL },
   { fn_down,           FN_MARK | FN_REP,    NULL },
   { fn_screen_up,      FN_MARK | FN_REP,    NULL },
   { fn_screen_down,    FN_MARK | FN_REP,    NULL },
   { fn_start,          FN_MARK | FN_REP,    NULL },
   { fn_end,            FN_MARK | FN_REP,    NULL },

   /* recently added functions get tacked on the end */
   { fn_clip_cut,       FN_EDIT,                         valid_block },
   { fn_clip_copy,      0,                               valid_block },
   { fn_clip_yank,      FN_MOVE_NS | FN_EDIT | FN_REP,   got_clipboard_data }
};



int command_valid(int cmd)
{
   if (cmd <= 0)
      return FALSE;

   if (fn_map[cmd].valid)
      return (*fn_map[cmd].valid)();
   else
      return TRUE;
}



int command_move(int cmd)
{
   if (cmd <= 0)
      return FALSE;

   if (fn_map[cmd].fn_flags & FN_MOVE)
      return TRUE;

   if (fn_map[cmd].fn_flags & FN_MOVE_NS)
      return (!is_selecting(buffer[0]));

   return FALSE;
}



int main(int argc, char *argv[])
{
   int c;
   int open_flags = 0;
   int tmp;
   int goto_line = -1;
   char find[80] = "";

 #ifdef TARGET_DJGPP
   dos_seg = _go32_conventional_mem_selector();

   __djgpp_set_ctrl_c(0);
   setcbrk(0);

   if (_USE_LFN)
      _crt0_startup_flags |= _CRT0_FLAG_PRESERVE_FILENAME_CASE;
 #endif

   srand(time(NULL));

   windows_init();

   exe_path = argv[0];

   if (get_fname(exe_path) == exe_path)
      if (search_path(exe_buf, exe_path, "", "PATH"))
	 exe_path = exe_buf;
 #if (defined TARGET_DJGPP) || (defined TARGET_WIN)
      else if (search_path(exe_buf, exe_path, "exe", "PATH"))
	 exe_path = exe_buf;
 #endif

   if (!read_config(NULL))
      return 1;

   sort_out_tools();

 #ifdef DJGPP
   #define IS_OPTION(c)    ((c=='-') || (c=='/'))
   #define IS_FLAG(s, f)   ((stricmp(s, "-" f)==0) || (stricmp(s, "/" f)==0))
 #else
   #define IS_OPTION(c)    (c=='-')
   #define IS_FLAG(s, f)   (stricmp(s, "-" f)==0)
 #endif

   for (c=1;c<argc;c++) {               /* process the command line flags */
      if ((IS_OPTION(argv[c][0])) && (argv[c][1]>='0') && (argv[c][1]<='9')) {
	 config.screen_height = atoi(argv[c]+1);
      }
      else if ((IS_OPTION(argv[c][0])) && (tolower(argv[c][1])=='g')) {
	 goto_line = atoi(argv[c]+2);
      }
      else if ((IS_OPTION(argv[c][0])) && (tolower(argv[c][1])=='s')) {
	 strcpy(default_syntax, "default.");
	 strcat(default_syntax, argv[c]+2);
      }
      else if (argv[c][0]=='?') {
	 strcpy(find, argv[c]+1);
      }
      else if ((IS_OPTION(argv[c][0])) && (tolower(argv[c][1])=='t')) {
	 set_tab_size(atoi(argv[c]+2));
	 if ((TAB_SIZE <= 0) || (TAB_SIZE > 79)) {
	    fatal_error("Illegal tab size");
	    terminate();
   #ifdef TARGET_DJGPP
	    __djgpp_set_ctrl_c(1);
   #endif
	    return -1;
	 }
      }
      else if ((IS_OPTION(argv[c][0])) && (argv[c][1]=='?')) {
       #ifdef TARGET_WIN
	 #define T "\t\t"
       #else
	 #define T "\t"
       #endif

	 static char msg[] =
	    "\n" VS ", by Shawn Hargreaves, " DS "\n"
	    "\nCommand line options:\n"
	    "\n"
	    "<file>  " T "Open <file> (the name may contain wildcards)\n"
	    "-a      " T "Open next file in normal ascii mode\n"
	    "-b      " T "Open next file in binary mode\n"
	    "-e      " T "Use next file as an error list\n"
	    "-f      " T "Automatically fold the next file\n"
	    "-g<line>" T "Go to the specified line number\n"
	    "-h      " T "Open next file in hex edit mode\n"
	    "-i      " T "Open next file in autoindent mode\n"
	    "-r      " T "Open next file in read-only mode\n"
	    "-s<ext> " T "Sets the file extension for syntax highlighting\n"
	    "-t<size>" T "Set the tab size\n"
	    "-w[col] " T "Open next file in wordwrap mode\n"
	    "?<text> " T "Browse for the string <text>\n"
	 #ifdef TARGET_WIN
	    "-<w>x<h>\tSet the window size\n"
	    "-<w>+<h>\tSet the window location\n"
	 #elif !defined TARGET_CURSES
	    "-<lines>" T "Set the number of screen lines\n"
	 #endif
	    "-?      " T "Display this message\n"
	    "\n"
	    "For help on using FED, press F1 from within the program\n\n";

	 printf(msg);

	 terminate();
   #ifdef TARGET_DJGPP
	 __djgpp_set_ctrl_c(1);
   #endif
	 return 0;
      }
   }

   errno = 0;
   read_banner_info();
   term_init(config.screen_height);
   disp_init();
   if (errno!=0) {
      fatal_error(NULL);
      exit_flag=1;
      goto get_out;
   }

   for (c=1;c<argc;c++) {               /* now to load some files... */
      if (IS_FLAG(argv[c], "a"))
	 open_flags |= BUF_ASCII;
      else if (IS_FLAG(argv[c], "b"))
	 open_flags |= BUF_BINARY;
      else if (IS_FLAG(argv[c], "e"))
	 open_flags |= BUF_BROWSE;
      else if (IS_FLAG(argv[c], "f"))
	 open_flags |= BUF_FOLD;
      else if (IS_FLAG(argv[c], "h"))
	 open_flags |= BUF_HEX;
      else if (IS_FLAG(argv[c], "i"))
	 open_flags |= BUF_INDENT;
      else if (IS_FLAG(argv[c], "r"))
	 open_flags |= BUF_READONLY;
      else if (IS_FLAG(argv[c], "w"))
	 open_flags |= BUF_WORDWRAP;
      else if ((IS_OPTION(argv[c][0])) && (tolower(argv[c][1]) == 'w') && (atoi(argv[c]+2) != 0)) {
	 open_flags |= BUF_WORDWRAP;
	 config.wrap_col = atoi(argv[c]+2);
      }
      else {
				/* load up the file(s) */
	 if ((!IS_OPTION(argv[c][0])) && (argv[c][0] != '?')) {

	    read_file(argv[c], open_flags);
	    open_flags=0;

	    if (errno==ENOMEM)            /* abort if out of memory */
	       break;

	    if (buffer_count>0)
	       if (buffer[0]->flags & BUF_TRUNCATED)
		  break;
	 }
      }
   }

   if (buffer_count==0) {
      tmp=errno;
      errno = 0;
      read_file("untitled", 0);
      errno=tmp;
   }

   find_kill_buffer();

   if (buffer_count>0)
      do_input_loop(goto_line, find);

   get_out:
   terminate();
   disp_exit();
   term_exit();
   return exit_flag;
}



#ifdef DJGPP

/* prevent djgpp from doing too much screwy stuff at startup */

int _crt0_startup_flags = _CRT0_FLAG_USE_DOS_SLASHES | 
			  _CRT0_FLAG_DISALLOW_RESPONSE_FILES;


char **__crt0_glob_function(char *_arg)
{
   return NULL;
}

#endif



void terminate()             /* gracefully terminate the program */
{
   int c;
   SYNTAX *s, *next;

   for(c=0; c<buffer_count; c++)
      destroy_buffer(buffer[c]);

   buffer_count = 0;

   if (config.keymap)
      free(config.keymap);

   s = config.syntax;
   while (s) {
      for (c=0; c<128; c++)
	 if (s->keyword[c].data)
	    free(s->keyword[c].data);
      next = s->next;
      free(s);
      s = next;
   }

   repeat_count=0;
   macro_mode=0;
}



void fatal_error(char *s)
{
   cls();
   linefeed();
   if ((errno > 0) && (errno < sys_nerr))
      mywrite((char *)sys_errlist[errno]);
   else
      mywrite("Error");
   if(s) {
      newline();
      mywrite(s);
   }
   newline();
   newline();
   mywrite("  Press a key");
   newline();
   clear_keybuf();
   gch();
}



void do_input_loop(int goto_line, char *find)
{
   /* main key input and dispatching loop, including keymap lookups */

   KEYPRESS key;       /* the key */
   int a,b,c;          /* for binary search */
   int d;             /* difference value for search */
   int in_macro;
   int changed = TRUE;

   display_new_buffer();
   if (goto_line >= 0)
      go_to_line(goto_line);
   else
      if (find[0])
	 browse(find);
      else
	 browse_goto(0, FALSE);

   while (buffer_count>0) {     /* while there is something to edit */

      in_macro = running_macro();

      if ((repeat_count>0) || (in_macro))
	 if (check_abort()) {
	    repeat_count=0;
	    macro_mode=0;
	    in_macro = FALSE;
	 }

      if (!input_waiting())      /* only redisplay when we have time */
      #if (defined TARGET_CURSES) || (defined TARGET_WIN)
	 if ((changed) || (winched)) {
	    redisplay();
	    changed = FALSE;

	    if (winched == 1)
	       winched = 0;
	 }
      #else
	 if (changed) {
	    redisplay();
	    changed = FALSE;
	 }
      #endif

      while (m_b)
	 poll_mouse();

      if ((repeat_count==0) || (in_macro)) {

	 wait_for_input(FALSE);

	 if (input_waiting()) {
	    changed = TRUE;
	    key = input_char();     /* this is where it all happens... */

	    #ifndef TARGET_CURSES
	       poll_mouse();
	    #endif

	    a = 0;                  /* binary search on the keymap array */
	    b = config.keymap_size-1;

	    do {
	       c = (a + b) / 2;
	       d = key.key - config.keymap[c].key;
	       if (d < 0)
		  b = c - 1;
	       else
		  if (d > 0)
		     a = c + 1;

	    } while ((a <= b) && (d!=0));

	    if (d!=0) {            /* fake binding to 0, insert character */
	       key.key = ascii(key.key);
	       if (key.key >= ' ') {
		  d=0;
		  c=0;
	       }
	       else {
		  if (((key.key==ESC) || (key.key==ascii(CTRL_G))) &&
		      (buffer[0]->sel_line))
			block_finished();
	       }
	    }
	    else
	       c=config.keymap[c].fn_code;
	 }
	 else {
	    if (alt_pressed()) {
	       if (recording_macro()) {
		  strcpy(message, "Can't record menu actions in a macro");
		  display_message(0);
		  c = 0;
		  d = -1;
	       }
	       else {
		  changed = TRUE;
		  d = 0;
		  c = show_menu();
		  if (c==-1) {
		     c = 0;
		     d = -1;
		  }
		  poll_mouse();
	       }
	    }
	    else {
	       c = 0;
	       d = -1;
	       if ((poll_mouse()) || 
		   ((m_y == 0) && (!config.show_menu)) ||
		   ((m_x >= screen_w-1) && (!config.show_bar))) {
		  changed = TRUE;
		  c = do_mouse_input();
		  if (c != -1)
		     last_function = buffer[0]->last_function = -1;
		  if (c >= 0)
		     d = 0;
	       }
	    }
	    key.key = 0;
	    key.flags = 0;
	 }
      }

      else {            /* just repeat the last command */
	 changed = TRUE;
	 repeat_count--;
	 c=repeat_function;
	 key.key = repeat_key;
	 key.flags = 0;
	 d=0;
      }

      if (d==0) {       /* execute the command */

	 if ((buffer[0]->flags & BUF_READONLY) && 
	     (fn_map[c].fn_flags & FN_EDIT)) {
	    strcpy(message,"File is read-only");
	    display_message(MESSAGE_KEY);
	 }

	 else {
	    if (fn_map[c].fn_flags & FN_MARK) {
	       if (key.flags & KF_SHIFT) {
		  if (!buffer[0]->sel_line)
		     fn_block();
	       }
	       else
		  block_finished();
	    }
	    else if (fn_map[c].fn_flags & FN_DESEL)
	       block_finished();

	    if ((!in_macro) && (fn_map[c].fn != fn_repeat)) {
	       repeat_function=c;
	       repeat_key=key.key;
	    }

	    if ((!in_macro) && ((c != buffer[0]->last_function) || 
				(buffer[0]->last_function < 0)))
	       buffer[0]->undo = new_undo_node(buffer[0]->undo);

	    ((*fn_map[c].fn)(key.key));           /* DO IT! */

	    if ((c != 9 /* undo */) && (c != 10 /* redo */) && (buffer_count > 0) &&
		((!buffer[0]->undo) || (buffer[0]->undo->length!=0) || 
		 (buffer[0]->undo->cached_operation != UNDO_NOTHING))) {
	       destroy_undolist(buffer[0]->redo);
	       buffer[0]->redo = NULL;
	    }

	    if (buffer_count > 0) {
	       last_function = (last_function == -2) ? -1 : c;
	       buffer[0]->last_function = last_function;
	    }
	 }
      }
   }
}



void do_bar()
{
   int lines = 0;
   int pos = 0;
   int top;
   int h;
   LINE *l;
   int c;
   int b_pressed = FALSE;
   int turn_off = FALSE;
   int g = 0;
   int start_time;
   int timeout = 0;
   int fast_scroll = FALSE;
   int virgin = TRUE;

   if (!config.show_bar) {
      turn_off = TRUE;
      config.show_bar = TRUE;
      dirty_everything();
   }

   do {
      if (!b_pressed) {
	 hide_mouse();
	 redisplay();
	 display_mouse();
	 start_time = clock();
	 fast_scroll = FALSE;
	 while (m_b) {
	    if (timeout >= 0) {
	       if ((clock() - start_time) > CLOCKS_PER_SEC / 16 * timeout) {
		  fast_scroll = (timeout > 0);
		  timeout = -1;
		  break;
	       }
	    }
	    poll_mouse();
	 }
      }
      else {
	 if (virgin)
	    virgin = FALSE;
	 else
	    redisplay();
	 fast_scroll = FALSE;
      }

      get_bar_data(buffer[0], &pos, &lines, &top, &h);

      do {
	 delay(10);
	 if (!b_pressed)
	    wait_for_input(TRUE);
	 poll_mouse();
	 if ((!b_pressed) && (m_b)) {
	    if (m_y < top) {
	       fn_screen_up();
	       timeout = fast_scroll ? 1 : 8;
	       break;
	    }
	    else if (m_y >= top+h) {
	       fn_screen_down();
	       timeout = fast_scroll ? 1 : 8;
	       break;
	    }
	    else {
	       b_pressed = TRUE;
	       g = m_y - top;
	       hide_mouse();
	       set_mouse_height(32);
	       set_mouse_pos(m_x, (pos*32*(screen_h-1) + lines/2) / lines);
	    }
	 }
	 else if ((b_pressed) && (!m_b)) {
	    b_pressed = FALSE;
	    set_mouse_height(1);
	    set_mouse_pos(m_x, MIN(m_y+g, screen_h-1));
	    display_mouse();
	 }
      } while ((m_x >= screen_w-1) && 
	       (!b_pressed) && (!input_waiting()));

      if (b_pressed) {
	 pos = ((m_y * lines + screen_h/2) / (screen_h-1)) / 32;
	 top = (pos * (screen_h-1) + lines/2) / lines;
	 if ((top==0) && (pos > 0))
	    top=1;
	 l = buffer[0]->start;
	 for (c=0; c<pos; c++) {
	    if (l->fold)
	       l = l->fold;
	 }
	 buffer[0]->top = buffer[0]->c_line = l;
	 for (c=0; c<screen_h/2; c++)
	    if (buffer[0]->c_line->fold)
	       buffer[0]->c_line = buffer[0]->c_line->fold;
	 buffer[0]->c_pos = buffer[0]->old_c_pos = 0;
	 cursor_move_line();
	 disp_dirty();
      }
      else
	 if ((m_x < screen_w-1) || (input_waiting()))
	    break;

   } while (TRUE);

   if (turn_off) {
      config.show_bar = FALSE;
      dirty_everything();
   }

   display_mouse();
}



void goto_point(int x, int y)
{
   int c;
   LINE *l = l = buffer[0]->top;
   extern int banner_height;

   for (c=(config.show_menu ? 1 : 0); c<MIN(y, screen_h-banner_height-1); c++)
      if (l->fold)
	 l = l->fold;
      else
	 break;

   if (c==screen_h-banner_height-1) {
      buffer[0]->top = buffer[0]->top->fold;
      disp_dirty();
   }

   buffer[0]->c_line = l;
   buffer[0]->c_pos = 0;
   while ((buffer[0]->c_pos < buffer[0]->c_line->length) &&
	  (get_line_length(l, buffer[0]->c_pos) < buffer[0]->hscroll+x))
      buffer[0]->c_pos++;
   buffer[0]->old_c_pos = get_line_length(buffer[0]->c_line, buffer[0]->c_pos);
   cursor_move_line();
}



int do_mouse_input()
{
   int changed = FALSE;
   int ret;
   extern int banner_height;

   display_mouse();

   while ((!input_waiting()) && (!alt_pressed())) {

      wait_for_input(TRUE);

      if (mouse_changed(NULL, NULL))
	 poll_mouse();

      #if (defined TARGET_CURSES) || (defined TARGET_WIN)
	 if (winched)
	    break;
      #endif

      if ((m_b == 1) && (m_y > 0) && (m_x < screen_w-1)) {
	 changed = TRUE;

	 if ((m_b == 1) && (buffer[0]->sel_line))
	    block_finished();

	 goto_point(m_x, m_y);

	 hide_mouse();
	 redisplay();
	 display_mouse();

	 if (mouse_dclick(FALSE)) {

	    while (m_b == 1) {

	       int x = m_x;
	       int y = m_y;

	       poll_mouse();

	       if ((m_x != x) || (m_y != y)) {

		  fn_block();

		  while (m_b == 1) {

		     if ((m_y <= 0) && (buffer[0]->top->prev)) {
			buffer[0]->top = buffer[0]->top->prev;
			disp_dirty();
			delay(10);
		     }
		     else if ((m_y >= screen_h-banner_height-1) && (buffer[0]->top->fold)) {
			buffer[0]->top = buffer[0]->top->fold;
			disp_dirty();
			delay(10);
			m_y = screen_h-banner_height-2;
		     }

		     goto_point(m_x, m_y);

		     hide_mouse();
		     redisplay();
		     display_mouse();

		     poll_mouse();
		     clear_keybuf();
		  }
	       }
	    }
	 }
	 else if (mouse_dclick(TRUE)) {
	    if (config.dclick_command > 0) {
	       while (m_b)
		  poll_mouse();
	       hide_mouse();
	       return config.dclick_command;
	    }
	 }
      }
      else if ((m_b & 2) && (m_y > 0) && (m_x < screen_w-1)) {
	 int x = m_x;
	 int y = m_y;

	 LINE *t = buffer[0]->top;
	 LINE *c_l = buffer[0]->c_line;
	 int c_pos = buffer[0]->c_pos;
	 goto_point(x, y);

	 hide_mouse();

	 ret = do_popup();

	 if ((ret > 0) && (command_move(ret))) {
	    changed = TRUE;
	 }
	 else {
	    buffer[0]->top = t;
	    buffer[0]->c_line = c_l;
	    buffer[0]->c_pos = c_pos;
	    cursor_move_line();
	    disp_dirty();
	 }

	 if (ret >= 0)
	    return ret;
	 else
	    return changed ? -2 : -1;
      }
      else {
	 if ((m_x >= screen_w-1) && ((!config.show_bar) || (m_b))) {
	    do_bar();
	    hide_mouse();
	    dirty_everything();
	    redisplay();
	    display_mouse();
	    while (m_b)
	       poll_mouse();
	    changed = TRUE;
	 } 
	 else if ((m_y == 0) && ((!config.show_menu) || (m_b))) {
	    hide_mouse();
	    ret = show_menu();
	    if (ret >= 0)
	       return ret;
	    else
	       return changed ? -2 : -1;
	 }
      }
   }

   hide_mouse(); 
   return changed ? -2 : -1;
}



void fn_repeat()
{
   if (fn_map[repeat_function].fn_flags & FN_REP) {
      repeat_count=input_number("Repeat Command", 0);
   }
   else
      alert_box("Can't repeat the last command");
}



void fn_screen()
{
   int x = input_screen_height();

   if ((x > 0) && (x != config.screen_height)) {
      disp_exit();
      config.screen_height = x;
      term_init(x);
      errno = 0;
      disp_init();
      cursor_move_line();
   }
}



int check_abort()
{
   int key;
   extern int unget_count;

   while (keypressed()) {
      key = gch();
      if ((ascii(key)==ESC) || (key==CTRL_C) || (key==CTRL_G)) {
	 strcpy(message,"Abort");
	 display_message(MESSAGE_KEY);
	 unget_count = 0;
	 return TRUE;
      }
      un_getc(key);
   }

   return FALSE;
}

