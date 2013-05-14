/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      The pulldown and popup menus
 */


#include <ctype.h>

#include "fed.h"


char tool_str[16][40];

static char r_ex_tool_str[] = "Run external tool";


LISTITEM keymenu_s[] =
{
   { "Nothing",               0, NULL, NULL, "Don't do anything" },
   { "Open",                  1, NULL, NULL, "Open a file, or switch to it if it is already open" },
   { "Close",                 2, NULL, NULL, "Close the current file" },
   { "Write",                 3, NULL, NULL, "Write the current file to disk" },
   { "Print",                 4, NULL, NULL, "Print the current file" },
   { "View files",            5, NULL, NULL, "View a list of all open files" },
   { "Next file",             6, NULL, NULL, "Switch to the next open file" },
   { "Quit",                  7, NULL, NULL, "Quit from FED" },
   { "Save and quit",         8, NULL, NULL, "Save all modified files and quit from FED" },
   { "Undo",                  9, NULL, NULL, "Undo the last action" },
   { "Redo",                  10, NULL, NULL, "Redo the last action" },
   { "Cut",                   11, NULL, NULL, "Cut the selected text (deletes a line if no selection)" },
   { "Copy",                  12, NULL, NULL, "Copy the selected text" },
   { "Paste",                 13, NULL, NULL, "Paste text back into the file" },
 #ifdef TARGET_DJGPP
   { "Clipboard Cut",         84, NULL, NULL, "Cut text to the Windows clipboard" },
 #endif
   { "Clipboard Copy",        85, NULL, NULL, "Copy text to the Windows clipboard" },
   { "Clipboard Paste",       86, NULL, NULL, "Paste text from the Windows clipboard" },
   { "Lower case",            14, NULL, NULL, "Convert the word under the cursor to lower case" },
   { "Upper case",            15, NULL, NULL, "Convert the word under the cursor to upper case" },
   { "Binary char",           16, NULL, NULL, "Insert an extended ascii or hex character" },
   { "Transpose",             17, NULL, NULL, "Swap the two characters under the cursor" },
   { "Auto indent",           18, NULL, NULL, "Toggle automatic indentation on or off" },
   { "Wordwrap",              19, NULL, NULL, "Set the wordwrap column (0 for no wordwrap)" },
   { "Reformat",              20, NULL, NULL, "Reformat the current line (does nothing if wordwrap is off)" },
   { "Info display",          21, NULL, NULL, "Display information about the file and cursor position" },
   { "Goto line",             22, NULL, NULL, "Go to a specified line number" },
   { "Remember position",     23, NULL, NULL, "Remember the current file and cursor position" },
   { "Last position",         24, NULL, NULL, "Go back to a previously remembered position" },
   { "Match brace",           25, NULL, NULL, "Find the brace matching the one under the cursor" },
   { "Search/replace",        26, NULL, NULL, "Search (once in a search, press ctrl+R to replace)" },
   { "File grep",             27, NULL, NULL, "Search/replace through multiple files" },
   { "Browse",                28, NULL, NULL, "Browse for all occurrences of the word under the cursor" },
   { "Next (browse)",         29, NULL, NULL, "Go to the next browse occurrence or compiler error" },
   { "Previous (browse)",     30, NULL, NULL, "Go to the previous browse occurrence or compiler error" },
   { "Fold",                  31, NULL, NULL, "Make or remove a fold" },
   { "Expand/collapse",       32, NULL, NULL, "Expand or collapse all folds in the file" },
   { "Macro record",          33, NULL, NULL, "Start recording a macro" },
   { "Stop recording",        34, NULL, NULL, "Stop recording a macro" },
   { "Play macro",            35, NULL, NULL, "Play macro" },
   { "Repeat command",        36, NULL, NULL, "Repeat the last command several times" },
   { tool_str[0],             37, NULL, NULL, r_ex_tool_str },
   { tool_str[1],             38, NULL, NULL, r_ex_tool_str },
   { tool_str[2],             39, NULL, NULL, r_ex_tool_str },
   { tool_str[3],             40, NULL, NULL, r_ex_tool_str },
   { tool_str[4],             41, NULL, NULL, r_ex_tool_str },
   { tool_str[5],             42, NULL, NULL, r_ex_tool_str },
   { tool_str[6],             43, NULL, NULL, r_ex_tool_str },
   { tool_str[7],             44, NULL, NULL, r_ex_tool_str },
   { tool_str[8],             45, NULL, NULL, r_ex_tool_str },
   { tool_str[9],             46, NULL, NULL, r_ex_tool_str },
   { tool_str[10],            47, NULL, NULL, r_ex_tool_str },
   { tool_str[11],            48, NULL, NULL, r_ex_tool_str },
   { tool_str[12],            49, NULL, NULL, r_ex_tool_str },
   { tool_str[13],            50, NULL, NULL, r_ex_tool_str },
   { tool_str[14],            51, NULL, NULL, r_ex_tool_str },
   { tool_str[15],            52, NULL, NULL, r_ex_tool_str },
 #if (!defined TARGET_CURSES) && (!defined TARGET_WIN)
   { "Display mode",          53, NULL, NULL, "Alter the screen height" },
 #endif
   { "Search mode",           54, NULL, NULL, "Alter the search mode" },
   { "Tab size",              55, NULL, NULL, "Alter the tab size" },
   { "Options",               56, NULL, NULL, "Configure FED" },
   { "Colors",                57, NULL, NULL, "Alter the display colors" },
   { "Key mapping",           58, NULL, NULL, "Map control keys to functions" },
   { "Save config",           59, NULL, NULL, "Save the current configuration" },
   { "HELP!",                 60, NULL, NULL, "Display the help file" },
   { "Relax...",              61, NULL, NULL, "Play tetris" },
   { "About FED",             62, NULL, NULL, "Display the about box" },
   { "Mark block",            63, NULL, NULL, "Start marking a block" },
   { "Select word",           64, NULL, NULL, "Select the word under the cursor" },
   { "Insert mode",           65, NULL, NULL, "Toggle insert mode on or off" },
   { "Delete previous",       66, NULL, NULL, "Delete the previous character, or block unindent" },
   { "Delete next",           67, NULL, NULL, "Delete the next character" },
   { "Delete word",           68, NULL, NULL, "Delete a word" },
   { "Emacs kill",            69, NULL, NULL, "Emacs-style 'kill' action" },
   { "Insert <cr>",           70, NULL, NULL, "Insert a carriage return" },
   { "Insert <tab>",          71, NULL, NULL, "Insert a tab, or block indent" },
   { "Left character",        72, NULL, NULL, "Move left" },
   { "Right character",       73, NULL, NULL, "Move right" },
   { "Left word",             74, NULL, NULL, "Move a word to the left" },
   { "Right word",            75, NULL, NULL, "Move a word to the right" },
   { "Start of line",         76, NULL, NULL, "Go to the start of the line" },
   { "End of line",           77, NULL, NULL, "Go to the end of the line" },
   { "Up a line",             78, NULL, NULL, "Go up a line" },
   { "Down a line",           79, NULL, NULL, "Go down a line" },
   { "Up a screen",           80, NULL, NULL, "Go up a screen" },
   { "Down a screen",         81, NULL, NULL, "Go down a screen" },
   { "Start of file",         82, NULL, NULL, "Go to the start of the file" },
   { "End of file",           83, NULL, NULL, "Go to the end of the file" },
};



LISTBOX keymenu =
{
   NULL, 
   NULL,
   23, 2, 32, 0,
   3, 2, 1, 0, 24,
 #ifdef TARGET_DJGPP
   87,
 #elif (defined TARGET_CURSES) || (defined TARGET_WIN)
   85,
 #else
   86,
 #endif
   0, 0,
   keymenu_s,
   NULL, NULL, 
   0, 0,
   LISTBOX_SCROLL | LISTBOX_MOUSE_CANCEL
};



int select_function(char *buf, int pos)
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };

   keymenu.h = screen_h - 5;
   keymenu.height = screen_h - 9;
   keymenu.title = buf;

   for (keymenu.current=0; keymenu.current<keymenu.count; keymenu.current++)
      if (keymenu_s[keymenu.current].data == pos)
	 break;

   if (keymenu.current >= keymenu.count)
      keymenu.current = 0;

   keymenu.scroll = keymenu.current - (keymenu.height / 2);
   if (keymenu.scroll < 0)
      keymenu.scroll = 0;

   if (do_listbox(&keymenu, oklist, cancellist, -1, FALSE) != -1)
      return keymenu_s[keymenu.current].data;
   else
      return -1;
}



void fn_keymap()
{
   int x = (screen_w - 40) / 2 - 1;
   int y = (screen_h - 6) / 2;
   int key;
   char buf[80];
   int fn;

   draw_box("Map Keys", x, y, 40, 6, TRUE);
   hi_vid();
   hide_c();
   goto1(x+6, y+2);
   mywrite("Press the key to map");
   goto1(x+6, y+3);
   mywrite("(Esc to finish)");

   while ((!input_waiting()) && (!m_b))
      poll_mouse();

   if (m_b)
      key = ESC_SCANCODE;
   else
      key = input_char().key;

   while ((key) && (key != ESC_SCANCODE)) {
      strcpy(buf, "Map key ");
      itoa(key, buf+strlen(buf), 10);
      strcat(buf, " (ascii ");
      itoa(key&0xff, buf+strlen(buf), 10);
      strcat(buf, ")");

      fn = select_function(buf, search_keymap(key));
      hide_c();
      if (fn >= 0)
	 add_to_keymap(key, fn);

      while (m_b)
	 poll_mouse();

      while ((!input_waiting()) && (!m_b))
	 poll_mouse();

      if (m_b)
	 key = ESC_SCANCODE;
      else
	 key = input_char().key;
   }

   n_vid();
   show_c();
   dirty_everything();
}



void menu_draw_proc(LISTBOX *l)
{
   int c;

   hi_vid();
   goto1(l->x, l->y);
   for (c=l->x; c<=l->x+l->w; c++)
      pch(' ');
   n_vid();
}



int altkeys[] =
{
   8448, 4608, 7936, 12800, 5120, 11776, 8960, 0
};



int altkey_menu_proc(LISTBOX *l, int key)
{
   int *p = altkeys;

   while (*p) {
      if (*p == key) {
	 l->key = 1;
	 l->parent->key = key;
	 return TRUE;
      }
      p++;
   }

   return FALSE;
}



int recursive_menu_proc(LISTBOX *lb, LISTITEM *li)
{
   int oklist[] = { 2, 0 };
   int cancellist[] = { 1, CTRL_C, CTRL_G, LEFT_ARROW, RIGHT_ARROW, 0 };
   LISTBOX *l = (LISTBOX *)li->data;

   int ret = do_listbox(l, oklist, cancellist, -1, TRUE);

   if (ret==-1) {
      if ((!m_b) && (l->key != 1)) {
	 lb->key = l->key;
	 lb->key2 = DOWN_ARROW;
      }
   }

   return ret; 
}



int altkey_mainmenu_proc(LISTBOX *l, int key)
{
   int c;

   for (c=0; altkeys[c]; c++) {
      if (altkeys[c] == key) {
	 l->current = c;
	 l->key = DOWN_ARROW;
	 return TRUE;
      }
   }

   return FALSE;
}



extern LISTBOX menu;


LISTBOX filemenu =
{
   &menu, NULL,
   2, 1, 19, 10,           /* x, y, w, h */
   2, 1,                   /* xoff, yoff */
   1, 8,                   /* width, height */
   13,                     /* slen */
   8,                      /* count */
   0, 0,
   keymenu_s+1,
   NULL, 
   altkey_menu_proc, 
   0, 0,
   LISTBOX_WRAP | LISTBOX_FASTKEY | LISTBOX_MOUSE_CANCEL | LISTBOX_MOUSE_OK2
};



LISTBOX editmenu =
{
   &menu, NULL,
 #ifdef TARGET_DJGPP
   12, 1, 21, 14,          /* x, y, w, h */
   2, 1,                   /* xoff, yoff */
   1, 12,                  /* width, height */
   15,                     /* slen */
   12,                     /* count */
 #else
   12, 1, 21, 13,          /* x, y, w, h */
   2, 1,                   /* xoff, yoff */
   1, 11,                  /* width, height */
   15,                     /* slen */
   11,                     /* count */
 #endif
   0, 0,
   keymenu_s+9,
   NULL, 
   altkey_menu_proc, 
   0, 0,
   LISTBOX_WRAP | LISTBOX_FASTKEY | LISTBOX_MOUSE_CANCEL | LISTBOX_MOUSE_OK2
};



LISTBOX searchmenu =
{
   &menu, NULL,
   22, 1, 23, 12,          /* x, y, w, h */
   2, 1,                   /* xoff, yoff */
   1, 10,                  /* width, height */
   17,                     /* slen */
   10,                     /* count */
   0, 0,
 #ifdef TARGET_DJGPP
   keymenu_s+24,
 #else
   keymenu_s+23,
 #endif
   NULL, 
   altkey_menu_proc, 
   0, 0,
   LISTBOX_WRAP | LISTBOX_FASTKEY | LISTBOX_MOUSE_CANCEL | LISTBOX_MOUSE_OK2
};



LISTBOX miscmenu =
{
   &menu, NULL,
   32, 1, 21, 8,           /* x, y, w, h */
   2, 1,                   /* xoff, yoff */
   1, 6,                   /* width, height */
   15,                     /* slen */
   6,                      /* count */
   0, 0,
 #ifdef TARGET_DJGPP
   keymenu_s+34,
 #else
   keymenu_s+33,
 #endif
   NULL, 
   altkey_menu_proc, 
   0, 0,
   LISTBOX_WRAP | LISTBOX_FASTKEY | LISTBOX_MOUSE_CANCEL | LISTBOX_MOUSE_OK2
};



LISTBOX toolmenu =
{
   &menu, NULL,
   42, 1, 21, 18,          /* x, y, w, h */
   2, 1,                   /* xoff, yoff */
   1, 16,                  /* width, height */
   15,                     /* slen */
   16,                     /* count */
   0, 0,
 #ifdef TARGET_DJGPP
   keymenu_s+40,
 #else
   keymenu_s+39,
 #endif
   NULL, 
   altkey_menu_proc, 
   0, 0,
   LISTBOX_WRAP | LISTBOX_FASTKEY | LISTBOX_MOUSE_CANCEL | LISTBOX_MOUSE_OK2
};



LISTBOX configmenu =
{
   &menu, NULL,
 #if (!defined TARGET_CURSES) && (!defined TARGET_WIN)
   52, 1, 18, 9,           /* x, y, w, h */
   2, 1,                   /* xoff, yoff */
   1, 7,                   /* width, height */
   12,                     /* slen */
   7,                      /* count */
   0, 0,
 #else
   52, 1, 17, 8,           /* x, y, w, h */
   2, 1,                   /* xoff, yoff */
   1, 6,                   /* width, height */
   11,                     /* slen */
   6,                      /* count */
   0, 0,
 #endif
 #ifdef TARGET_DJGPP
   keymenu_s+56,
 #else
   keymenu_s+55,
 #endif
   NULL, 
   altkey_menu_proc, 
   0, 0,
   LISTBOX_WRAP | LISTBOX_FASTKEY | LISTBOX_MOUSE_CANCEL | LISTBOX_MOUSE_OK2
};



LISTBOX helpmenu =
{
   &menu, NULL,
   62, 1, 15, 5,           /* x, y, w, h */
   2, 1,                   /* xoff, yoff */
   1, 3,                   /* width, height */
   9,                      /* slen */
   3,                      /* count */
   0, 0,
 #ifdef TARGET_DJGPP
   keymenu_s+63,
 #elif (defined TARGET_CURSES) || (defined TARGET_WIN)
   keymenu_s+61,
 #else
   keymenu_s+62,
 #endif
   NULL, 
   altkey_menu_proc, 
   0, 0,
   LISTBOX_WRAP | LISTBOX_FASTKEY | LISTBOX_MOUSE_CANCEL | LISTBOX_MOUSE_OK2
};



LISTITEM menu_s[7] =
{
   { "File",    (int)&filemenu,   recursive_menu_proc, NULL, NULL },
   { "Edit",    (int)&editmenu,   recursive_menu_proc, NULL, NULL },
   { "Search",  (int)&searchmenu, recursive_menu_proc, NULL, NULL },
   { "Misc",    (int)&miscmenu,   recursive_menu_proc, NULL, NULL },
   { "Tools",   (int)&toolmenu,   recursive_menu_proc, NULL, NULL },
   { "Config",  (int)&configmenu, recursive_menu_proc, NULL, NULL },
   { "Help",    (int)&helpmenu,   recursive_menu_proc, NULL, NULL },
};



LISTBOX menu =
{
   NULL, NULL, 
   0, 0, 79, 1,
   2, 0, 7, 1, 8, 7,
   0, 0,
   menu_s,
   menu_draw_proc, 
   altkey_mainmenu_proc,
   0, 0,
   LISTBOX_WRAP | LISTBOX_FASTKEY | LISTBOX_MOUSE_CANCEL | LISTBOX_MOUSE_OK
};



void draw_menu()
{
   menu.w = (config.show_bar ? 78 : 79);
   menu.current = -1;
   draw_listbox(&menu);
}



int show_menu()
{
   int oklist[] = { DOWN_ARROW, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };
   int ret;
   char *b;

   hide_c();

   menu.w = (config.show_bar ? 78 : 79);
   b = save_screen(menu.x, menu.y, menu.w, menu.h);
   draw_menu();
   display_mouse();

   while ((!input_waiting()) && (!m_b)) {
      poll_mouse();
      if (((m_y != 0) || (m_x >= screen_w-1) || (config.show_menu)) 
	  && (!alt_pressed()))
	 break;
   }

   hide_mouse();

   if ((input_waiting()) || (m_b))
      ret = do_listbox(&menu, oklist, cancellist, -1, TRUE);
   else
      ret = -1;

   restore_screen(menu.x, menu.y, menu.w, menu.h, b);
   show_c();
   return ret;
}



int funcclick_proc(LISTBOX *lb, LISTITEM *li)
{
   int fn;

   fn = select_function(li->text, *((int *)li->data));
   hide_c();

   if (fn >= 0)
      *((int *)li->data) = fn;

   draw_contents(lb);

   while (m_b)
      poll_mouse();

   return -1;
}



void funcdraw_proc(int w, LISTBOX *lb, LISTITEM *li)
{
   int c;
   char *s = "";

   for (c=0; c<keymenu.count; c++) {
      if (keymenu_s[c].data == *((int *)li->data)) {
	 s = keymenu_s[c].text;
	 break;
      }
   }

   mywrite(li->text);
   mywrite(": ");
   mywrite(s);

   for (c = strlen(li->text)+2+strlen(s); c<w; c++)
      pch(' ');
}



LISTITEM rightmenubox_s[16] =
{
   { "Function 1", (int)&config.right_menu[0],  funcclick_proc,   funcdraw_proc, NULL },
   { "Function 2", (int)&config.right_menu[1],  funcclick_proc,   funcdraw_proc, NULL },
   { "Function 3", (int)&config.right_menu[2],  funcclick_proc,   funcdraw_proc, NULL },
   { "Function 4", (int)&config.right_menu[3],  funcclick_proc,   funcdraw_proc, NULL },
   { "Function 5", (int)&config.right_menu[4],  funcclick_proc,   funcdraw_proc, NULL },
   { "Function 6", (int)&config.right_menu[5],  funcclick_proc,   funcdraw_proc, NULL },
   { "Function 7", (int)&config.right_menu[6],  funcclick_proc,   funcdraw_proc, NULL },
   { "Function 8", (int)&config.right_menu[7],  funcclick_proc,   funcdraw_proc, NULL },
   { "Function 9", (int)&config.right_menu[8],  funcclick_proc,   funcdraw_proc, NULL },
   { "Function 10", (int)&config.right_menu[9],  funcclick_proc,   funcdraw_proc, NULL },
   { "Function 11", (int)&config.right_menu[10], funcclick_proc,   funcdraw_proc, NULL },
   { "Function 12", (int)&config.right_menu[11], funcclick_proc,   funcdraw_proc, NULL },
   { "Function 13", (int)&config.right_menu[12], funcclick_proc,   funcdraw_proc, NULL },
   { "Function 14", (int)&config.right_menu[13], funcclick_proc,   funcdraw_proc, NULL },
   { "Function 15", (int)&config.right_menu[14], funcclick_proc,   funcdraw_proc, NULL },
   { "Function 16", (int)&config.right_menu[15], funcclick_proc,   funcdraw_proc, NULL },
};



LISTBOX rightmenubox =
{
   NULL, 
   "Edit right click menu",
   15, 2,               /* x, y */
   48, 20,              /* w, h */
   3, 2,                /* xoff, yoff */
   1, 16,               /* width, height */
   40,                  /* slen */
   16,                  /* count */
   0, 0,                /* scroll, current */
   rightmenubox_s,
   NULL, NULL, 0, 0,
   LISTBOX_MOUSE_CANCEL
};



int right_menu_proc(LISTBOX *lb, LISTITEM *li)
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };

   rightmenubox.y = (screen_h - rightmenubox.h) / 2;

   do_listbox(&rightmenubox, oklist, cancellist, -1, FALSE);

   draw_contents(lb);
   while (m_b)
      poll_mouse();

   return -1;
}



LISTITEM popup_s[16] =
{
   { NULL, 0,  NULL, NULL },
   { NULL, 0,  NULL, NULL },
   { NULL, 0,  NULL, NULL },
   { NULL, 0,  NULL, NULL },
   { NULL, 0,  NULL, NULL },
   { NULL, 0,  NULL, NULL },
   { NULL, 0,  NULL, NULL },
   { NULL, 0,  NULL, NULL }
};



LISTBOX popup =
{
   NULL, NULL,
   0, 0, 0, 0,             /* x, y, w, h */
   2, 1,                   /* xoff, yoff */
   1, 0,                   /* width, height */
   0,                      /* slen */
   0,                      /* count */
   0, 0,
   popup_s,
   NULL, NULL, 
   0, 0,
   LISTBOX_WRAP | LISTBOX_MOUSE_CANCEL | LISTBOX_MOUSE_OK2 | LISTBOX_USE_RMB
};



int do_popup()
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };
   int ret;
   int c, i;
   int cmd;

   popup.count = 0;
   popup.slen = 0;

   for (c=0; c<16; c++) {
      cmd = config.right_menu[c];
      if ((cmd > 0) && (command_valid(cmd))) {
	 for (i=0; i<keymenu.count; i++) {
	    if (keymenu_s[i].data == cmd) {
	       popup_s[popup.count].text = keymenu_s[i].text;
	       popup_s[popup.count].comment = keymenu_s[i].comment;
	       break;
	    }
	 }

	 if (i >= keymenu.count) {
	    popup_s[popup.count].text = "";
	    popup_s[popup.count].comment = "";
	 }

	 popup_s[popup.count].data = cmd;
	 if (strlen(popup_s[popup.count].text) > popup.slen)
	    popup.slen = strlen(popup_s[popup.count].text);
	 popup.count++;
      } 
   }

   if (popup.count <= 0)
      return -1;

   popup.height = popup.count;
   popup.h = popup.count + 2;
   popup.w = popup.slen + 6;
   popup.scroll = popup.current = 0;

   popup.x = m_x;
   if (popup.x + popup.w >= screen_w)
      popup.x = screen_w - popup.w - 1;
   else if (popup.x < 0)
      popup.x = 0;

   popup.y = m_y;
   if (popup.y + popup.h >= screen_h)
      popup.y = screen_h - popup.h - 1;
   else if (popup.y < 0)
      popup.y = 0;

   hide_c();

   ret = do_listbox(&popup, oklist, cancellist, -1, TRUE);

   show_c();
   return ret;
}

