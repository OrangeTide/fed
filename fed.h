/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Various constants and data structures, plus function prototypes
 */


#ifndef FED_H
#define FED_H

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "io.h"

#define VS              "FED 2.23"
#define VS2             "Version 2.23"
#define VS3             "2.23"
#define DS              "November 2006"
#define DS2             "1994/2006"

#define CFG_FILE        "fed.cfg"
#define SYN_FILE        "fed.syn"
#define RECORD_FILE     "fed.rec"
#define MESSAGE_FILE    "fed.msg"
#define CLIP_FILE       ".clip"
#define ERROR_FILE      "_errors_"
#define FILT_IN_FILE    "_in_"
#define FILT_OUT_FILE   "_out_"

#ifndef TRUE
#define TRUE            -1
#define FALSE           0
#endif

#ifndef MIN
#define MIN(x,y)        ((x)<(y) ? (x) : (y))
#define MAX(x,y)        ((x)<(y) ? (y) : (x))
#define MID(x,y,z)      MIN(MAX(x, y), z)
#endif

#ifndef ENMFILE
#define ENMFILE         ENOENT
#endif

#define VALUE(c)     (((c >= 'a') && (c <= 'f')) ? \
		      (c - 'a' + 10) : (c - '0'))

#define atoi(s)            myatoi(s)
#define itoa(i, s, b)      myitoa(i, s, b)
#define stricmp(s, t)      mystricmp(s, t)
#define strnicmp(s, t, n)  mystrnicmp(s, t, n)
#define strlwr(s)          mystrlwr(s)
#define strupr(s)          mystrupr(s)

extern char hex_digit[];

#define MAX_BUFFERS     256     /* maximum number of open files */
#define MACRO_LENGTH    4096    /* maximum number of keys in a macro */


typedef struct KEYPRESS         /* as returned by input_char() */
{
   int key;                     /* bios keycode */
   int flags;                   /* shift, alt and ctrl flags */
} KEYPRESS;


#define KF_SHIFT        3
#define KF_CTRL         4
#define KF_ALT          8


typedef struct KEYMAP           /* a key code -> function code map */
{
   int key;                     /* bios keycode */
   int fn_code;                 /* index into the FNMAP table */
} KEYMAP;


typedef struct FN_MAP           /* a function code -> function map */
{                               /* fn_code is implied by the array index */
   void (*fn)();                /* pointer to the actual code */
   int fn_flags;                /* flags about the function */
   int (*valid)();              /* is this function ok for a popup menu? */
} FN_MAP;


#define FN_EDIT         1       /* function changes the text */
#define FN_REP          2       /* function can be repeated */
#define FN_MARK         4       /* can combine with shift to mark text */
#define FN_DESEL        8       /* clear selection before executing */
#define FN_MOVE         16      /* move cursor from popup menu */
#define FN_MOVE_NS      32      /* move cursor if no selection */


#define KEYWORD_HASH(a,b)       (((a) ^ (b)) & 127)


#define COM_SIZE        8       /* max size of comments in syntax struct */


typedef struct KEYWORD          /* an entry in the keyword hash table */
{
   int count;                   /* how many keywords in this entry? */
   char *data;                  /* the actual keywords */
} KEYWORD;


typedef struct SYNTAX           /* syntax highlighting data */
{
   struct SYNTAX *next;         /* linked list */
   char files[80];              /* which files this data applies to */
   char open_com1[COM_SIZE];    /* multi-line comment type 1 */
   char close_com1[COM_SIZE];
   char open_com2[COM_SIZE];    /* multi-line comment type 2 */
   char close_com2[COM_SIZE];
   char eol_com1[COM_SIZE];     /* single-line comments */
   char eol_com2[COM_SIZE];
   char hex_marker[COM_SIZE];   /* marker for hex numbers */
   int string[2];               /* string marker characters */
   int escape;                  /* string escape character */
   int case_sense;              /* are keywords case sensitive? */
   int color_numbers;           /* should numbers be hilighted? */
   int indent_comments;         /* can comments be indented? */
   int tab_size;                /* size of tabs for files of this type */
   int real_tabs;               /* are tabs real or emulated with spaces? */
   char indents[32];            /* characters to use in wordwrap indent regions */
   char wrappers[128];          /* characters to accept at the end of a wordwrap */
   char symbols[256];           /* flags whether chars are symbols */
   KEYWORD keyword[128];
} SYNTAX;


extern char default_syntax[];


typedef struct CONFIG           /* a complete FED configuration */
{
   int screen_height;           /* height of the screen, in lines */
   char bin[80];                /* files to load in binary mode */
   char hex[80];                /* files to load in hex mode */
   char auto_fold[80];          /* the files to fold on load */
   char auto_indent[80];        /* the files to auto-indent */
   char wrap[80];               /* the files to word wrap */
   int wrap_col;                /* column at which to word wrap */
   char makefiles[80];          /* files to auto-load */
   int search_mode;             /* relaxed, case-sensitive, keyword */
   int big_cursor;              /* type of cursor to display */
   int make_backups;            /* make .bak files on save? */
   int check_files;             /* check for external changes to files? */
   int default_tab_size;        /* size of tabs */
   int load_tabs;               /* size of tabs when reading/writing files */
   int save_strip;              /* convert spaces to tabs on write? */
   int print_ascii;             /* true if only ascii chars can be printed */
   int screen_save;             /* minutes before screensaver kicks in */
   int undo_levels;             /* number of undo operations (per file) */
   int file_search;             /* depth of directories to scan for files */
   int comments;                /* display explanations of menu items, etc */
   int show_pos;                /* display cursor position */
   int show_menu;               /* display menus at top of screen */
   int show_bar;                /* display scroll bar */
   int norm_col;                /* color of normal text */
   int hi_col;                  /* color of highlighted text */
   int sel_col;                 /* color of selected text */
   int fold_col;                /* color of folded text */
   int keyword_col;             /* keyword syntax highlighting */
   int comment_col;             /* comment syntax highlighting */
   int string_col;              /* string constant syntax highlighting */
   int number_col;              /* numeric constant syntax highlighting */
   int symbol_col;              /* symbol syntax highlighting */
   char tool[16][80];           /* external tools */
   int dclick_command;          /* command to execute on double-click */
   int right_menu[16];          /* right-click menu */
   int keymap_size;             /* number of keys that have been mapped */
   KEYMAP *keymap;              /* the keymap array, sorted by scan code */
   SYNTAX *syntax;              /* head pointer to list of syntax data */
} CONFIG;


extern CONFIG config;


#define REAL_TABS    (((buffer_count > 0) && \
		       (buffer[0]->syntax) && \
		       (buffer[0]->syntax->real_tabs >= 0)) ? \
		      buffer[0]->syntax->real_tabs : (config.load_tabs == 0))


#define TAB_SIZE     (((buffer_count > 0) && \
		       (buffer[0]->syntax) && \
		       (buffer[0]->syntax->tab_size > 0)) ? \
		      buffer[0]->syntax->tab_size : config.default_tab_size)


#define set_tab_size(t) {                                               \
			   if ((buffer_count > 0) &&                    \
			       (buffer[0]->syntax) &&                   \
			       (buffer[0]->syntax->tab_size > 0))       \
			      buffer[0]->syntax->tab_size = t;          \
			   else                                         \
			      config.default_tab_size = t;              \
			}


extern char tool_str[16][40];

extern FN_MAP fn_map[];
extern int last_function;


#define SEARCH_RELAXED  0       /* flags for search mode */
#define SEARCH_CASE     1
#define SEARCH_KEYWORD  2

#define MESSAGE_KEY     1       /* redraw on next keypress */
#define MESSAGE_LINE    2       /* redraw when cursor changes line */

#define WRAP_INSERT     1       /* flags for do_wordwrap */
#define WRAP_DELETE     2

extern char state_table[];      /* flags whether chars belong in words */

#define STATE_SPACE  1          /* return values for get_state() */
#define STATE_CHAR   2
#define STATE_WORD   3

#define COMMENT_UNKNOWN    -1   /* flags for comment data in lines */
#define COMMENT_NONE       0
#define COMMENT_1          1
#define COMMENT_2          2
#define COMMENT_CLOSE_1    3
#define COMMENT_CLOSE_2    4


typedef struct LINE
{
   struct LINE *prev;           /* previous line in the linked list */
   struct LINE *next;           /* 'real' next line, may be inside a fold */
   struct LINE *fold;           /* next line, allowing for folds */
   unsigned short length;       /* length of the line */
   unsigned short size;         /* amount of memory allocated */
   unsigned short line_no;      /* line number in the original file */
   signed char comment_state;   /* comment status at the end of the line */
   signed char comment_effect;  /* effect the line has on comment status */
   unsigned char *text;         /* the characters */
} LINE;


#define char_from_line(l, p)     (((p >= l->length) || (p < 0)) ?    \
				  ' ' :                              \
				  l->text[p])


#define UNDO_NOTHING    0
#define UNDO_CHAR       1
#define UNDO_CR         2
#define UNDO_DEL        3
#define UNDO_GOTO       4


typedef struct UNDO
{
   struct UNDO *next;           /* next item in the list */
   int cached_line;             /* cursor line where last thing happened */
   int cached_pos;              /* cursor pos where last thing happened */
   int cached_operation;        /* most recent operation */
   int cached_count;            /* how many of this operation? */
   int length;                  /* length of the undo data */
   int size;                    /* amount of memory allocated */
   unsigned char data[0];       /* the undo data */
} UNDO;


typedef struct BUFFER
{
   char name[256];              /* name of the file */
   int flags;                   /* flags about the buffer state */
   long filetime;               /* time of the disk file */
   LINE *start;                 /* start of the text */
   LINE *top;                   /* top line on the screen */
   LINE *c_line;                /* line that the cursor is on */
   int c_pos;                   /* cursor position within the line */
   int old_c_pos;               /* store of c_pos for vertical movement */
   int hscroll;                 /* offset when screen is scrolled sideways */
   int wrap_col;                /* word wrap column (0 for off) */
   LINE *sel_line;              /* line that the selection began at */
   int sel_pos;                 /* selection offset within the line */
   int sel_offset;              /* offset in lines from sel_line to cursor */
   int cached_line;             /* cached cursor line number (real) */
   LINE *_cached_line;          /* the line it was on */
   int cached_top;              /* cached top position (using folds) */
   LINE *_cached_top;           /* the line it was on */
   int cached_size;             /* cached size (using folds) */
   UNDO *undo;                  /* data to be undone */
   UNDO *redo;                  /* data to be redone */
   int last_function;           /* new op, or append to undo list? */
   SYNTAX *syntax;              /* syntax highlighting data */
} BUFFER;


#define BUF_BINARY         1       /* the buffer is in binary mode */
#define BUF_HEX            2       /* the buffer is in hex mode */
#define BUF_ASCII          4       /* for loading, force ascii mode */
#define BUF_KILL           8       /* this is the kill buffer */
#define BUF_BROWSE         16      /* this is the browser buffer */
#define BUF_CHANGED        32      /* the buffer has been altered */
#define BUF_READONLY       64      /* file is read-only */
#define BUF_FORCEREADONLY  128     /* keep the file read-only */
#define BUF_INDENT         256     /* auto-indentation is on */
#define BUF_OVERWRITE      512     /* buffer is in overwrite mode */
#define BUF_NEW            1024    /* file doesn't exist on disk */
#define BUF_TRUNCATED      2048    /* the buffer was truncated on load */
#define BUF_WRAPPED        4096    /* long lines have been split */
#define BUF_FOLD           8192    /* auto-fold file when loading */
#define BUF_UNIX           16384   /* save in unix text format */
#define BUF_WORDWRAP       32768   /* open in wordwrap mode */
#define BUF_MAKENEW        65536   /* allow creation of new files */



extern int buffer_count;                /* the number of buffers */
extern BUFFER *buffer[MAX_BUFFERS];     /* buffer[0] is the active one */
extern int repeat_count;        /* how many times to repeat command */
extern int exit_flag;           /* exit code to return to caller */
extern char message[];          /* the text to display on the bottom line */
extern int disp_cline;          /* the line that the cursor is on */
extern char *exe_path;          /* saved copy of argv[0] */
extern char remember_filename[];
extern int remember_line;
extern int remember_col;


struct LISTBOX;

typedef struct LISTITEM
{
   char *text;
   intptr_t data;
   int (*click_proc)(struct LISTBOX *lb, struct LISTITEM *li);
   void (*draw_proc)(int w, struct LISTBOX *lb, struct LISTITEM *li);
   char *comment;
} LISTITEM;


typedef struct LISTBOX
{
   struct LISTBOX *parent;      /* so we can draw recursively */
   char *title;                 /* to display on the top */
   int x, y, w, h;              /* in characters */
   int xoff, yoff;              /* gap between border and contents */
   int width, height;           /* in strings */
   int slen;                    /* length of each string */
   int count;                   /* how many items? */
   int scroll;                  /* index of top item */
   int current;                 /* index of selected item */
   LISTITEM *data;              /* the strings */
   void (*draw_proc)(struct LISTBOX *l);
   int (*process_proc)(struct LISTBOX *l, int key);
   int key, key2;               /* feed keyboard input in from outside */
   int flags;
} LISTBOX;


#define LISTBOX_SCROLL        1
#define LISTBOX_WRAP          2
#define LISTBOX_FASTKEY       4
#define LISTBOX_MOUSE_CANCEL  8
#define LISTBOX_MOUSE_OK      16
#define LISTBOX_MOUSE_OK2     32
#define LISTBOX_USE_RMB       64


/* functions from buffer.c */

void already_open(int c, int flags);
int read_file(char *n, int flags);
int do_the_read(char *n, int flags);
LINE *read_line(MYFILE *f, int flags, int *flag_ret, char *workspace);
void destroy_buffer(BUFFER *b);
void fn_close();
void fn_quit();
void fn_help();
int update_file_status();
void run_tool(char *cmd, char *desc);
void fn_tool1();
void fn_tool2();
void fn_tool3();
void fn_tool4();
void fn_tool5();
void fn_tool6();
void fn_tool7();
void fn_tool8();
void fn_tool9();
void fn_tool10();
void fn_tool11();
void fn_tool12();
void fn_tool13();
void fn_tool14();
void fn_tool15();
void fn_tool16();
void scan_for_filename(char *str);
void fn_open();
void fn_next_file(int key);
void fn_write();
void fn_save_all();
void write_file(BUFFER *buf);
void write_line(MYFILE *f, LINE *l, int flags);
void fn_print();
void read_browse_files();


/* functions from config.c */

int search_keymap(int key);
void add_to_keymap(int key, int value);
void save_string(MYFILE *f, char *s);
void save_int(MYFILE *f, int i);
void write_int(MYFILE *f, char *s, int i);
void write_string(MYFILE *f, char *s, char *d);
void save_cfg(MYFILE *f);
void fn_savecfg();
void read_string(MYFILE *f, char *buf);
int check_int(char *buf, char *prompt, int *ret, int min, int max);
void check_string(char *buf, char *prompt, char *ret, int max);
void check_string2(char *buf, char *buf2, char *prompt, char *ret, int max);
int read_config(char *filename);
int read_syntax();
SYNTAX *get_syntax(char *name, BUFFER *buf);
void sort_out_tools();


/* functions from dialog.c */

void fn_view();
void fn_about();
void fn_colors();
int tools_proc(LISTBOX *lb, LISTITEM *li);
void fn_config();
int get_binary_char();
int input_screen_height();
void fn_srch_mode();


/* functions from disp.c */

void read_banner_info();
void disp_init();
void disp_exit();
void disp_dirty();
void dirty_everything();
void dirty_cline();
void dirty_line(int l);
void dirty_message();
void display_big_message(int flag, char *title);
void do_display_message(int flag);
void display_message(int flag);
void display_new_buffer();
void set_buffer_message(char *s, int x, int flag);
int get_buffer_line(BUFFER *b);
void get_bar_data(BUFFER *b, int *pos, int *lines, int *top, int *h);
void go_to_line(int line);
void go_to_browse_line(int line);
void fn_goto();
void display_position();
void fn_position();
void fn_left();
void fn_right();
void fn_line_start();
void fn_line_end();
void fn_start();
void fn_end();
void fn_up();
void fn_down();
void fn_screen_up();
void fn_screen_down();
void check_cline();
void disp_move_on_screen(LINE *start, LINE *end);
void cursor_move_line();
void selection_change();
void cursor_move();
void dirty_bar();
void draw_bar();
void redisplay();
int display_line(int y, LINE *l, int *comment_state);
void wait_for_input(int mouse);


/* functions from fed.c */

int valid_word();
int valid_line();
int valid_fold();
int valid_filename();
int valid_block();
int valid_paste();
int valid_lastpos();
int valid_brace();
int valid_browse();
int command_valid(int cmd);
int command_move(int cmd);
void terminate();
void fatal_error(char *s);
void do_input_loop(int goto_line, char *find);
void do_bar();
void goto_point(int x, int y);
int do_mouse_input();
void fn_repeat();
void fn_screen();
int check_abort();


/* functions from gui.c */

void draw_contents(LISTBOX *l);
void draw_box(char *title, int _x, int _y, int _w, int _h, int shadow);
void draw_listbox(LISTBOX *l);
int mouse_inside_list(LISTBOX *l);
int mouse_outside_list(LISTBOX *l);
int mouse_inside_parent(LISTBOX *l);
int get_mouse_item(LISTBOX *l);
int do_listbox(LISTBOX *l, int *oklist, int *cancellist, int init_key, int instant_mouse);
int select_file(char *prompt, char *fname);
int open_file_type();
int ask(char *title, char *s1, char *s2);


/* functions from kill.c */

BUFFER *find_kill_buffer();
void destroy_kill_buffer(int completely);
void check_kill(void (*proc)());
int sel_right();
void fn_kill_word();
void fn_kill_line();
void fn_e_kill();
void kill_block(int insert_flag);
void fn_copy();
void fn_yank();
void fn_clip_cut();
void fn_clip_copy();
void fn_clip_yank();


/* functions from line.c */

LINE *create_line(int size);
int resize_line(LINE *line, int size);
void out_of_mem();
void line_too_long();
LINE *previous_line(LINE *l);
void left_ignoring_folds(int count);
void fn_transpose();
void fn_char(int key);
void indent_block(int c);
void fn_tab();
void fn_backspace();
void fn_delete();
void fn_return();
LINE *next_wordwrap_line(LINE *l, SYNTAX *s);
int join_wordwrap_lines(LINE *l, SYNTAX *s);
LINE *split_wordwrap_line(LINE *l, LINE **old_c_line, int *old_c_pos, SYNTAX *s);
void do_wordwrap(int mode);
void fn_reformat();
void fn_ascii();
void fn_insert();
void fn_wordwrap();
void fn_indent();
void fn_tabsize();
void destroy_undolist(UNDO *undo);
UNDO *new_undo_node(UNDO *undo);
UNDO *undo_grow(UNDO *undo, int x);
UNDO *undo_flush_data(UNDO *undo);
UNDO *undo_write_pos(UNDO *undo, int line, int pos);
UNDO *undo_add(UNDO *undo, int op, int c, unsigned char *p);
UNDO *do_the_undo(UNDO *undo, UNDO *redo);
void fn_undo();
void fn_redo();
int insert_string(BUFFER *buf, char *s, int len, UNDO **undo);
int insert_line(BUFFER *buf, LINE *l, UNDO **undo);
int insert_cr(BUFFER *buf, UNDO **undo);
int delete_chars(BUFFER *buf, int count, UNDO **undo);
void delete_line(BUFFER *buf, UNDO **undo);


/* functions from menu.c */

int select_function(char *buf, int pos);
void fn_keymap();
void draw_menu();
int show_menu();
int funcclick_proc(LISTBOX *lb, LISTITEM *li);
void funcdraw_proc(int w, LISTBOX *lb, LISTITEM *li);
int right_menu_proc(LISTBOX *lb, LISTITEM *li);
int do_popup();


/* functions from misc.c */

int get_leading_bytes(LINE *l);
int get_leading_chars(LINE *l);
int get_wordwrap_bytes(LINE *l, SYNTAX *s);
int get_wordwrap_chars(LINE *l, SYNTAX *s);
int get_line_length(LINE *l, int p);
int get_state(LINE *l, int p);
void fn_fold();
void fold_all();
void fold_from_line(LINE *start, int size);
void remove_fold(LINE *l);
void unfold(LINE *l);
LINE *outside_fold(LINE *l);
void fn_expand();
void fn_block();
void fn_select_word();
void block_finished();
void get_selection_info(LINE **start_l, LINE **end_l, int *start_i, int *end_i, int fixup);
int is_selecting(BUFFER *b);
int brace_direction(char c);
int opposite_brace(char c);
void fn_match();
void fn_word_left();
void fn_word_right();
void fn_lowcase();
void fn_upcase();
void change_case(int flag);
void fn_remember();
void fn_lastpos();


/* functions from search.c */

void push_c_pos();
void pop_c_pos();
int add_to_file_list(char *name, int flags);
void destroy_file_list();
int open_list_file();
void fn_grep(int key);
void fn_search(int key);
void do_search(int fn_key);
void get_word(unsigned char *str);
int find_prev(unsigned char *str, int flag);
int find_next(unsigned char *str, int flag);
void replace(unsigned char *search_str);
int do_replace(unsigned char *s, unsigned char *r);
int contains_browse_string(LINE *l, char *s, int *offset);
int is_header(LINE *l);
int compare_browse_strings(LINE *l1, LINE *l2);
void add_to_browse_list(int i, BUFFER *buf, LINE *l, char *name, int line, int offset);
void browse(char *s);
void fn_browse();
void fn_browse_next();
void fn_browse_prev();
void browse_goto(int direction, int e_flag);


/* functions from tetris.c */

void fn_tetris();
void screen_saver();


/* functions from util.c */

char *prepare_popup(char *t, int *_x, int *_y, int *_w);
void end_popup(char *b, int *_x, int *_y, int *_w);
void alert_box(char *s);
char *err();
int myatoi(char *s);
void myitoa(int i, char *s, int b);
int mystricmp(char *s, char *t);
int mystrnicmp(char *s, char *t, int n);
void mystrlwr(char *s);
void mystrupr(char *s);
int is_number(char c);
int is_all_numbers(char *s);
int is_filechar(char c);
int is_filechar_nospace(char c);
int is_asciichar(char c);
int is_anychar(char c);
int input_number(char *prompt, int i);
int do_input_text(char *buf, int size, int width, int (*valid)(char), int (*proc)(int, char *, int *, int *), int mouse_flag, int (*mouse_proc)(int x, int y, char *buf, int *oldlen));
int input_text(char *prompt, char *buf, int size, int (*valid)(char));
char *find_extension(char *s);
void remove_extension(char *s);
void append_backslash(char *s);
KEYPRESS input_char();
void un_getc(int l);
int input_waiting();
int auto_input_waiting();
int running_macro();
int recording_macro();
void fn_macro_s();
void fn_macro_e();
void fn_macro_p();
void itohex(char *s, int c);
char *get_fname(char *s);
int ext_in_list(char *fname, char *list, BUFFER *buf);
void cleanup_filename(char *f);


#endif          /* FED_H */
