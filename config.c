/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Code for managing the configuration file, and config defaults
 */


#include <ctype.h>
#include <limits.h>

#include "fed.h"


#define DEF_KEYMAPS     70


KEYMAP def_keymaps[DEF_KEYMAPS] = 
{
   { 6159,  1 },           /* ctrl+O = open */
   { 11779, 2 },           /* ctrl+C = close */
   { 4375,  3 },           /* ctrl+W = write */
   { 6416,  4 },           /* ctrl+P = print */
   { 12054, 5 },           /* ctrl+V = view files */
   { 12558, 6 },           /* ctrl+N = next file */
   { 4113,  7 },           /* ctrl+Q = quit */
   { 11290, 8 },           /* ctrl+Z = save and quit */
   { 3711,  9 },           /* ctrl+Backspace = undo */
   { 17408, 10 },          /* F10 = redo */
   { 11544, 11 },          /* ctrl+X = cut */
   { 17152, 12 },          /* F9 = copy */
   { 5401 , 13 },          /* ctrl+Y = paste */
   { 9740,  14 },          /* ctrl+L = lower case */
   { 5653,  15 },          /* ctrl+U = upper case */
   { 5897,  16 },          /* ctrl+I = binary char */
   { 5140,  17 },          /* ctrl+T = transpose */
   { 7681,  18 },          /* ctrl+A = auto indent */
   { 16128, 19 },          /* F5 = wordwrap */
   { 4882,  20 },          /* ctrl+R = reformat */
   { 15360, 21 },          /* F2 = info */
   { 8711,  22 },          /* ctrl+G = goto line */
   { 16896, 23 },          /* F8 = remember position */
   { 25856, 24 },          /* ctrl+F8 = last position */
   { 12813, 25 },          /* ctrl+M = match brace */
   { 7955,  26 },          /* ctrl+S = search/replace */
   { 14122, 27 },          /* keypad '*' = file grep */
   { 12290, 28 },          /* ctrl+B = browse */
   { 20011, 29 },          /* keypad '+' = browse next */
   { 18989, 30 },          /* keypad '-' = browse previous */
   { 8454,  31 },          /* ctrl+F = fold */
   { 4613,  32 },          /* ctrl+E = expand/collapse */
   { 6683,  33 },          /* ctrl+[ = macro record */
   { 6941,  34 },          /* ctrl+] = stop recording */
   { 7178,  35 },          /* ctrl+Enter = play macro */
   { 24832, 36 },          /* ctrl+F4 = repeat command */
   { 15616, 37 },          /* F3 = run external command */
   { 24576, 38 },          /* ctrl+F3 = DOS shell */
   { 16640, 39 },          /* F7 = make */
   { 25600, 40 },          /* ctrl+F7 = compile */
   { 8968,  41 },          /* ctrl+H = libc info */
   { 24064, 41 },          /* ctrl+F1 = libc info */
   { 15872, 42 },          /* F4 = run filter */
   { 16384, 54 },          /* F6 = search mode */
   { 15104, 60 },          /* F1 = HELP! */
   { 9226,  61 },          /* ctrl+J = tetris */
   { 20992, 65 },          /* Insert = insert mode */
   { 3592,  66 },          /* Backspace = delete previous */
   { 21248, 67 },          /* Delete = delete next */
   { 8196,  68 },          /* ctrl+D = delete word */
   { 9483,  69 },          /* ctrl+K = emacs kill */
   { 7181,  70 },          /* Enter = insert <cr> */
   { 3849,  71 },          /* Tab = insert <tab> */
   { 19200, 72 },          /* Left = left char */
   { 19712, 73 },          /* Right = right char */
   { 29440, 74 },          /* ctrl+Left = left word */
   { 29696, 75 },          /* ctrl+Right = right word */
   { 18176, 76 },          /* Home = start of line */
   { 20224, 77 },          /* End = end of line */
   { 18432, 78 },          /* Up = up a line */
   { 20480, 79 },          /* Down = down a line */
   { 18688, 80 },          /* Page up = up a screen */
   { 20736, 81 },          /* Page down = down a screen */
   { 30464, 82 },          /* ctrl+Home = start of file */
   { 29952, 83 },          /* ctrl+End = end of file */
   { 11520, 84 },          /* alt+X = clipboard cut */
   { 11776, 85 },          /* alt+C = clipboard copy */
   { 12032, 86 },          /* alt+V = clipboard yank */
   { 26112, 85 },          /* ctrl+F9 = clipboard copy */
   { 26368, 86 },          /* ctrl+F10 = clipboard yank */
};



CONFIG config =
{
 #ifdef TARGET_ALLEG
   60,                                 /* use 60x80 screen mode */
 #else
   25,                                 /* use 25x80 screen mode */
 #endif
   "exe,com",                                            /* binary mode */
   "",                                                   /* hex mode */
   "c,cpp,cc,h,pas",                                     /* fold these */
   "c,cpp,cc,h,pas,s,asm,txt,doc,_tx,sh,bash,mak,perl",  /* auto indent */
   "txt,doc,htm,html,_tx",                               /* wordwrap */
   77,                                 /* column for word wrap */
   "c,cpp,cc,h,pas,s,asm",             /* files to load from makefile */

   SEARCH_RELAXED,                     /* relaxed search match mode */
   FALSE,                              /* use normal cursor */
   FALSE,                              /* don't make backups */
   TRUE,                               /* check for changes to files */
   4,                                  /* four char tabs */
   8,                                  /* eight char tabs on load */
   TRUE,                               /* strip spaces on save */
   FALSE,                              /* print any characters */

   5,                                  /* five minutes before screen saver */
   16,                                 /* sixteen undo levels */
   2,                                  /* recurse two levels when looking for files */
   TRUE,                               /* display comments */
   TRUE,                               /* show cursor position */
   FALSE,                              /* don't show menus */
   FALSE,                              /* don't show scroll bar */

   112,                                /* normal text color */
   7,                                  /* UI color */
   7,                                  /* selection color */
   64,                                 /* folded text color */
   116,                                /* keywords color */
   113,                                /* comment color */
   116,                                /* strings color */
   116,                                /* numbers color */
   112,                                /* symbols color */

   {                                   /* external tools */
 #ifdef DJGPP
      "Run command|%p",
      "DOS shell|%h%eCOMSPEC%",
      "Make|redir -e " ERROR_FILE " make.exe",
      "Compile|redir -e " ERROR_FILE " gcc -c %f",
      "Info (libc)|%h%sinfo libc alphabetical %w",
      "Filter|%h%n%s%p",
      "",
 #elif defined TARGET_WIN
      "Run command|%p",
      "Command prompt|%h%eCOMSPEC%",
      "Make|bash -c \"make | tee " ERROR_FILE "\"",
      "Compile|bash -c \"cl -c %f | tee " ERROR_FILE "\"",
      "Man|%h%sman %w",
      "Filter|%h%N%sbash -c \"%p < " FILT_IN_FILE " > " FILT_OUT_FILE "\"",
      "",
 #else
      "Run command|%p",
      "Shell|%h%eSHELL%",
      "Make|make 2> " ERROR_FILE,
      "Compile|gcc -c %f 2> " ERROR_FILE,
      "Man|%h%sman %w",
      "Filter|%h%n%s%p",
      "Spell|%h%N%sispell -x _in_; mv _in_ _out_",
 #endif
      "", "", "", "", "", "", "", "", ""
   },

   64,                                 /* double-click selects word */
   {                                   /* right mouse menu... */
      11,                              /* cut */
      12,                              /* copy */
      31,                              /* fold */
      28,                              /* browse */
      41,                              /* keyword help */
      25,                              /* match brace */
      1,                               /* open */
      0, 0, 0, 0, 0, 0, 0, 0, 0
   },

   0, NULL,                            /* keymaps */

   NULL                                /* syntax highlighting */
};



char default_syntax[80] = "";



int search_keymap(int key)
{
   int c;

   for (c=0; c<config.keymap_size; c++)
      if (config.keymap[c].key == key)
	 return config.keymap[c].fn_code;

   return 0;
}



void add_to_keymap(int key, int value)
{
   int c, c2;

   for (c=0; c<config.keymap_size; c++) {
      if (config.keymap[c].key == key) {
	 if (value != 0)
	    config.keymap[c].fn_code = value;
	 else {
	    config.keymap_size--;
	    for (c2=c; c2<config.keymap_size; c2++)
	       config.keymap[c2] = config.keymap[c2+1];
	    config.keymap = realloc(config.keymap, 
				    config.keymap_size * sizeof(KEYMAP));
	 }
	 return; 
      }
   }

   if (value != 0) {
      config.keymap_size++;
      config.keymap = realloc(config.keymap, 
			      config.keymap_size * sizeof(KEYMAP));
      for (c=0; c<config.keymap_size-1; c++)
	 if (config.keymap[c].key > key)
	    break;
      for (c2=config.keymap_size-1; c2>c; c2--)
	 config.keymap[c2] = config.keymap[c2-1];

      config.keymap[c].key = key;
      config.keymap[c].fn_code = value;
   }
}



void save_string(MYFILE *f, char *s)
{
   while (*s) {
      put_char(f, *s);
      s++;
   }
}



void save_int(MYFILE *f, int i)
{
   char b[40];
   itoa(i, b, 10);
   save_string(f, b);
}



void write_int(MYFILE *f, char *s, int i)
{
   save_string(f, s);
   save_int(f, i);
   save_string(f, "\r\n");
}



void write_string(MYFILE *f, char *s, char *d)
{
   save_string(f, s);
   save_string(f, d);
   save_string(f, "\r\n");
}



void save_cfg(MYFILE *f)
{
   int c;

   save_string(f, "# configuration file for ");
   save_string(f, VS);
   save_string(f, "\r\n");

   save_string(f, "\r\n# options\r\n");
   write_int(f, "ScreenHeight=", config.screen_height);
   write_string(f, "BinaryFiles=", config.bin);
   write_string(f, "HexFiles=", config.hex);
   write_string(f, "AutoFoldFiles=", config.auto_fold);
   write_string(f, "AutoIndentFiles=", config.auto_indent);
   write_string(f, "WrapFiles=", config.wrap);
   write_int(f, "WrapColumn=", config.wrap_col);
   write_string(f, "MakeFiles=", config.makefiles);
   write_int(f, "SearchMode=", config.search_mode);
   write_int(f, "BigCursor=", config.big_cursor);
   write_int(f, "MakeBackups=", config.make_backups);
   write_int(f, "CheckFiles=", config.check_files);
   write_int(f, "TabSize=", config.default_tab_size);
   write_int(f, "LoadTabSize=", config.load_tabs);
   write_int(f, "StripOnSave=", config.save_strip);
   write_int(f, "PrintAscii=", config.print_ascii);
   write_int(f, "ScreenSave=", config.screen_save);
   write_int(f, "UndoLevels=", config.undo_levels);
   write_int(f, "FileSearch=", config.file_search);
   write_int(f, "UseComments=", config.comments);
   write_int(f, "ShowPos=", config.show_pos);
   write_int(f, "ShowMenu=", config.show_menu);
   write_int(f, "ShowBar=", config.show_bar);

   save_string(f, "\r\n# colors\r\n");
   write_int(f, "NormCol=", config.norm_col);
   write_int(f, "UICol=", config.hi_col);
   write_int(f, "SelCol=", config.sel_col);
   write_int(f, "FoldCol=", config.fold_col);
   write_int(f, "KeywordCol=", config.keyword_col);
   write_int(f, "CommentCol=", config.comment_col);
   write_int(f, "StringCol=", config.string_col);
   write_int(f, "NumberCol=", config.number_col);
   write_int(f, "SymbolCol=", config.symbol_col);

   save_string(f, "\r\n# tools\r\n");
   for (c=0; c<16; c++) {
      save_string(f, "Tool");
      put_char(f, hex_digit[c]);
      write_string(f, "=", config.tool[c]);
   }

   save_string(f, "\r\n# menu commands\r\n");
   write_int(f, "dClickCommand=", config.dclick_command);

   for (c=0; c<16; c++) {
      save_string(f, "RightMenu");
      put_char(f, hex_digit[c]);
      put_char(f, '=');
      save_int(f, config.right_menu[c]);
      save_string(f, "\r\n");
   }

   save_string(f, "\r\n# keymap table\r\n");

   for (c=0; c<config.keymap_size; c++) {
      save_string(f, "Keymap=");
      save_int(f, config.keymap[c].key);
      put_char(f, ':');
      save_int(f, config.keymap[c].fn_code);
      save_string(f, "\r\n");
   }

   save_string(f, "\r\nEndOfConfig\r\n");
}



void fn_savecfg()
{
   char fn[256];
   MYFILE *f;

   strcpy(fn, exe_path);
   strcpy(get_fname(fn), CFG_FILE);

   strcpy(message,"Writing ");
   strcat(message, fn);
   display_message(MESSAGE_KEY);

   f=open_file(fn, FMODE_WRITE);

   if (f) {
      save_cfg(f);
      close_file(f);
   }

   if (errno != 0) {
      alert_box(err());
      delete_file(fn);
   }
   else {
      strcpy(message, "Saved ");
      strcat(message, fn);
      display_message(MESSAGE_KEY);
   }
}



void read_string(MYFILE *f, char *buf)
{
   int c = 0;
   int ch = get_char(f);

   buf[0] = 0;

   while ((errno == 0) && (ch != '\r') && (ch != '\n') && (c < 80)) {
      buf[c++] = ch;
      buf[c] = 0;
      ch = get_char(f);
   }

   ch = peek_char(f);
   if (ch == '\r') {
      get_char(f);
      ch = peek_char(f);
   }
   if (ch == '\n') {
      get_char(f);
      ch = peek_char(f);
   }
}



int check_int(char *buf, char *prompt, int *ret, int min, int max)
{
   char b[40];
   int len = 0;
   int v;

   if (strstr(buf, prompt) != buf)
      return TRUE;

   b[0] = 0;
   buf += strlen(prompt);

   while ((*buf == '-') || ((*buf >= '0') && (*buf <= '9'))) {
      b[len++] = *(buf++);
      b[len] = 0;
   }

   v = atoi(b);
   if (( v >= min) && (v <= max)) {
      *ret = v;
      return TRUE;
   }

   printf("\nError in config file:\nValue of %s must be between %d and %d\n", prompt, min, max);
   return FALSE;
}



void check_string(char *buf, char *prompt, char *ret, int max)
{
   if (strstr(buf, prompt) == buf) {
      strncpy(ret, buf+strlen(prompt), max-1);
      ret[max-1] = 0;
   }
}



void check_string2(char *buf, char *buf2, char *prompt, char *ret, int max)
{
   if (strstr(buf, prompt) == buf) {
      strncpy(ret, buf2+strlen(prompt), max-1);
      ret[max-1] = 0;
   }
}



int read_config(char *filename)
{
   char fn[256];
   MYFILE *f;
   int c;
   char buf[256];
   char buf2[256];
   char b[256];

   if (filename)
      strcpy(fn, filename);
   else {
      for (c=0; c<DEF_KEYMAPS; c++)
	 add_to_keymap(def_keymaps[c].key, def_keymaps[c].fn_code);

      strcpy(fn, exe_path);
      strcpy(get_fname(fn), CFG_FILE);
   }

   f=open_file(fn, FMODE_READ); 

   if ((errno==ENMFILE) || (errno==ENOENT)) {
      errno = 0;
      if (!read_syntax())
	 goto oops;
      return TRUE;
   }

   if (errno == 0) {
      while (errno == 0) {
	 read_string(f, buf);
	 strcpy(buf2, buf);
	 strlwr(buf);

	 if (!check_int(buf, "screenheight=", &config.screen_height, 0, 150))
	    goto get_out;

	 check_string(buf, "binaryfiles=", config.bin, 80);
	 check_string(buf, "hexfiles=", config.hex, 80);
	 check_string(buf, "autofoldfiles=", config.auto_fold, 80);
	 check_string(buf, "autoindentfiles=", config.auto_indent, 80);
	 check_string(buf, "wrapfiles=", config.wrap, 80);

	 if (!check_int(buf, "wrapcolumn=", &config.wrap_col, 0, 256))
	    goto get_out;

	 check_string(buf, "makefiles=", config.makefiles, 80);

	 if (!check_int(buf, "searchmode=", &config.search_mode, 0, 2))
	    goto get_out;

	 if (!check_int(buf, "bigcursor=", &config.big_cursor, -1, 1))
	    goto get_out;

	 if (!check_int(buf, "makebackups=", &config.make_backups, -1, 1))
	    goto get_out;

	 if (!check_int(buf, "checkfiles=", &config.check_files, -1, 1))
	    goto get_out;

	 if (!check_int(buf, "tabsize=", &config.default_tab_size, 1, 30))
	    goto get_out;

	 if (!check_int(buf, "loadtabsize=", &config.load_tabs, 0, 256))
	    goto get_out;

	 if (!check_int(buf, "striponsave=", &config.save_strip, -1, 1))
	    goto get_out;

	 if (!check_int(buf, "printascii=", &config.print_ascii, -1, 1))
	    goto get_out;

	 if (!check_int(buf, "screensave=", &config.screen_save, 0, 256))
	    goto get_out;

	 if (!check_int(buf, "undolevels=", &config.undo_levels, 0, 256))
	    goto get_out;

	 if (!check_int(buf, "filesearch=", &config.file_search, 0, 256))
	    goto get_out;

	 if (!check_int(buf, "usecomments=", &config.comments, -1, 1))
	    goto get_out;

	 if (!check_int(buf, "showpos=", &config.show_pos, -1, 1))
	    goto get_out;

	 if (!check_int(buf, "showmenu=", &config.show_menu, -1, 1))
	    goto get_out;

	 if (!check_int(buf, "showbar=", &config.show_bar, -1, 1))
	    goto get_out;

	 if (!check_int(buf, "normcol=", &config.norm_col, 0, 999))
	    goto get_out;

	 if (!check_int(buf, "uicol=", &config.hi_col, 0, 999))
	    goto get_out;

	 if (!check_int(buf, "selcol=", &config.sel_col, 0, 999))
	    goto get_out;

	 if (!check_int(buf, "foldcol=", &config.fold_col, 0, 999))
	    goto get_out;

	 if (!check_int(buf, "keywordcol=", &config.keyword_col, 0, 999))
	    goto get_out;

	 if (!check_int(buf, "commentcol=", &config.comment_col, 0, 999))
	    goto get_out;

	 if (!check_int(buf, "stringcol=", &config.string_col, 0, 999))
	    goto get_out;

	 if (!check_int(buf, "numbercol=", &config.number_col, 0, 999))
	    goto get_out;

	 if (!check_int(buf, "symbolcol=", &config.symbol_col, 0, 999))
	    goto get_out;

	 for (c=0; c<16; c++) {
	    strcpy(b, "tool");
	    b[4] = tolower(hex_digit[c]);
	    b[5] = '=';
	    b[6] = 0;
	    check_string2(buf, buf2, b, config.tool[c], 80);
	 }

	 if (!check_int(buf, "dclickcommand=", &config.dclick_command, 0, 86))
	    goto get_out;

	 if (strstr(buf, "rightmenu") == buf) {
	    c = VALUE(buf[strlen("rightmenu")]);
	    if (c < 0)
	       c = 0;
	    else if (c >= 16)
	       c = 15;
	    strcpy(b, buf);
	    b[strlen("rightmenu")+2] = 0;
	    if (!check_int(buf, b, &config.right_menu[c], 0, 86))
	       goto get_out;
	 }

	 if (strstr(buf, "keymap=") == buf) {
	    int key, fn;
	    if (!check_int(buf, "keymap=", &key, INT_MIN, INT_MAX))
	       goto get_out;
	    strcpy(b, buf);
	    c=0;
	    while ((b[c]) && (b[c] != ':'))
	       c++;
	    if (b[c]==':')
	       b[c+1] = 0;
	    if (!check_int(buf, b, &fn, 0, 86))
	       goto get_out;
	    add_to_keymap(key, fn);
	 }

	 if (strstr(buf, "EndOfConfig") == buf)
	    break;
      }

      close_file(f);
   }

   if (errno == EOF)
      errno = 0;

   if (errno != 0) {
      printf("\nError reading %s:\n%s\n", fn, err());
      goto oops;
   }

   if (!read_syntax())
      goto oops;

   return TRUE;

   get_out:
   close_file(f);

   oops:
   printf("\n");
   return FALSE;
}



void parse_keywords(SYNTAX *syn, unsigned char *k)
{
   char buf[256];
   int len, len2, c;
   KEYWORD *keyword;

   while (*k) {
      while ((*k) && (!state_table[*k]))
	 k++;

      len = 0;
      while ((state_table[*k]) && (len < 80-1))
	 buf[len++] = *(k++);

      if (len > 0) {
	 buf[len] = 0;
	 keyword = syn->keyword + KEYWORD_HASH(buf[0], buf[1]);
	 len2 = 0;
	 for (c=0; c<keyword->count; c++)
	    len2 += strlen(keyword->data+len2) + 1;
	 keyword->data = realloc(keyword->data, len2+len+1);
	 strcpy(keyword->data+len2, buf);
	 keyword->count++;
      }
   }
}



int read_syntax()
{
   char fn[256];
   MYFILE *f;
   int c;
   char buf[256], buf2[256];
   SYNTAX *s = NULL;

   strcpy(fn, exe_path);
   strcpy(get_fname(fn), SYN_FILE);

   f=open_file(fn, FMODE_READ);

   if ((errno==ENMFILE) || (errno==ENOENT))
      return TRUE;

   if (errno == 0) {
      while (errno == 0) {
	 read_string(f, buf);
	 strcpy(buf2, buf);
	 strlwr(buf);

	 if (!s) {                        /* make a new syntax type */
	    s = malloc(sizeof(SYNTAX));
	    s->next = config.syntax;
	    s->files[0] = 0;
	    s->open_com1[0] = s->close_com1[0] = 0;
	    s->open_com2[0] = s->close_com2[0] = 0;
	    s->eol_com1[0] = s->eol_com2[0] = 0;
	    s->hex_marker[0] = 0;
	    s->string[0] = s->string[1] = -1;
	    s->escape = -1;
	    s->case_sense = FALSE;
	    s->color_numbers = TRUE;
	    s->indent_comments = TRUE;
	    s->tab_size = -1;
	    s->real_tabs = -1;
	    s->indents[0] = 0;
	    s->wrappers[0] = 0;
	    for (c=0; c<256; c++)
	       s->symbols[c] = FALSE;
	    for (c=0; c<128; c++) {
	       s->keyword[c].count = 0;
	       s->keyword[c].data = NULL;
	    }
	    parse_keywords(s, "shit,fuck");
	       /* have you ever got so frustrated you started typing stuff
		  like that into your editor? Well, FED can handle it... */
	    config.syntax = s;
	 }

	 if (strstr(buf, "files=") == buf)
	    strcpy(s->files, buf+6);

	 check_string(buf, "opencomment1=", s->open_com1, COM_SIZE);
	 check_string(buf, "closecomment1=", s->close_com1, COM_SIZE);
	 check_string(buf, "opencomment2=", s->open_com2, COM_SIZE);
	 check_string(buf, "closecomment2=", s->close_com2, COM_SIZE);
	 check_string(buf, "eolcomment1=", s->eol_com1, COM_SIZE);
	 check_string(buf, "eolcomment2=", s->eol_com2, COM_SIZE);
	 check_string(buf, "hexmarker=", s->hex_marker, COM_SIZE);
	 check_string(buf, "indents=", s->indents, 32);

	 check_string2(buf, buf2, "wrappers=", s->wrappers, 128);

	 if (strstr(buf, "symbols=") == buf)
	    for (c=8; buf[c]; c++)
	       s->symbols[(unsigned char)buf[c]] = TRUE;

	 if (strstr(buf, "string=") == buf) {
	    s->string[0] = buf[7];
	    s->string[1] = buf[8] ? buf[8] : -1;
	 }

	 if (strstr(buf, "escape=") == buf)
	    s->escape = (unsigned char)buf[7];

	 if (strstr(buf, "case=") == buf)
	    s->case_sense = (buf[5]=='0') ? FALSE : TRUE;

	 if (strstr(buf, "numbers=") == buf)
	    s->color_numbers = (buf[8]=='0') ? FALSE : TRUE;

	 if (strstr(buf, "indentc=") == buf)
	    s->indent_comments = (buf[8]=='0') ? FALSE : TRUE;

	 if (strstr(buf, "tabsize=") == buf)
	    check_int(buf, "tabsize=", &s->tab_size, 1, 30);

	 if (strstr(buf, "realtabs=") == buf)
	    check_int(buf, "realtabs=", &s->real_tabs, 0, 1);

	 if (strstr(buf, "keywords=") == buf)
	    parse_keywords(s, (s->case_sense ? buf2 : buf)+9);

	 if (strcmp(buf, "end") == 0)
	    s = NULL;
      }

      close_file(f);
   }

   if (errno == EOF)
      errno = 0;

   if (errno != 0)
      printf("\nError reading %s:\n%s\n", fn, err());

   return (errno == 0);
}



SYNTAX *get_syntax(char *name, BUFFER *buf)
{
   SYNTAX *s = config.syntax;

   while (s) {
      if (ext_in_list(name, s->files, buf))
	 return s;

      if ((default_syntax[0]) && (ext_in_list(default_syntax, s->files, NULL)))
	 return s;

      s = s->next;
   }

   return NULL;
}



extern LISTBOX toolmenu;


void sort_out_tools()
{
   int c, c2;
   int first = TRUE;

   toolmenu.w = 11;
   toolmenu.slen = 5;

   for (c=0; c<16; c++) {
      for (c2=0;
	   (config.tool[c][c2]) && (config.tool[c][c2] != '|') && (c2<40);
	   c2++)
	 tool_str[c][c2] = config.tool[c][c2];

      if (c2 > 0)  {
	 tool_str[c][c2] = 0;
	 if ((first) && (c2 > toolmenu.slen)) {
	    toolmenu.slen = c2;
	    toolmenu.w = c2+6;
	 }
      }
      else {
	 strcpy(tool_str[c], "Tool");
	 itoa(c+1, tool_str[c]+4, 10);
	 if (first) {
	    toolmenu.height = toolmenu.count = MAX(c,1);
	    toolmenu.h = MAX(c,1)+2;
	    first = FALSE;
	 }
      }

      if (strstr(config.tool[c]+c2, "%w")) {
	 fn_map[37+c].fn_flags = FN_MOVE;
	 fn_map[37+c].valid = valid_word;
      }
      else {
	 fn_map[37+c].fn_flags = 0;
	 fn_map[37+c].valid = NULL;
      }
   }
}

