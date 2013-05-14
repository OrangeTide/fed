/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      Assorted dialog boxes and other user interface stuff
 */


#include <ctype.h>

#include "fed.h"



LISTBOX viewbox =
{
   NULL, 
   "View Files", 
   7, 2,                /* x, y */
   0, 0,                /* w, h */
   3, 2,                /* xoff, yoff */
   1, 0,                /* width, height */
   0,                   /* slen */
   0,                   /* count */
   0, 0,                /* scroll, current */
   NULL,                /* data */
   NULL, NULL, 0, 0,
   LISTBOX_SCROLL | LISTBOX_MOUSE_CANCEL
};



void viewdraw_proc(int w, LISTBOX *lb, LISTITEM *li)
{
   char buf[256];
   int c;

   set_buffer_message(buf, li->data, FALSE);

   c = strlen(buf) - w;
   
   if (c > 0)
      memmove(buf+6, buf+7+c, strlen(buf+7+c)+1);

   mywrite(buf);
   for (c=strlen(buf); c<w; c++)
      pch(' ');
}



void fn_view()
{
   int ret;
   int c;
   BUFFER *buf;
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };
   extern int message_flags;

   viewbox.w = MAX(screen_w - 16, 32);
   viewbox.h = screen_h - 6;
   viewbox.height = screen_h - 10;
   viewbox.count = buffer_count;
   viewbox.current = viewbox.scroll = 0;
   viewbox.slen = viewbox.w - 8;

   viewbox.data = malloc(sizeof(LISTITEM)*buffer_count);
   for (c=0; c<buffer_count; c++) {
      viewbox.data[c].text = get_fname(buffer[c]->name);
      viewbox.data[c].data = c;
      viewbox.data[c].click_proc = NULL;
      viewbox.data[c].draw_proc = viewdraw_proc;
      viewbox.data[c].comment = NULL;
   }

   ret = do_listbox(&viewbox, oklist, cancellist, -1, FALSE);

   free(viewbox.data);
   viewbox.data = NULL;

   if (ret >= 0) {
      buf=buffer[ret];
      for(c=ret; c>0; c--)
	 buffer[c]=buffer[c-1];
      buffer[0]=buf;
      display_new_buffer();
      redisplay();
      message_flags = MESSAGE_KEY;
   }
}



char *about_text[] = 
{
   "\\8888888888 8888888888 8888888b.\\    A Folding Text Editor",
   "\\888        888        888  \"Y88b\\",
   "\\888        888        888    888\\   " VS2 " (" TN ")",
   "\\8888888    8888888    888    888\\   " DS,
   "\\888        888        888    888\\",
   "\\888        888        888    888\\   By Shawn Hargreaves",
   "\\888        888        888  .d88P\\   www.talula.demon.co.uk",
   "\\888        8888888888 8888888P\\",
   "",
   "",
   "This program is free, open-source software. It may be",
   "redistributed and/or modified under the terms of the GNU",
   "General Public License as published by the Free Software",
   "Foundation, either version 2 of the License, or (at your",
   "option) any later version. You should have received a copy",
   "of the GPL along with this program in the file COPYING.",
   NULL
};



void fn_about()
{
   int x, y;
   int t = (screen_h - 20) / 2;
   int n = TRUE;

   hide_c();

   draw_box(NULL, 6, t, 65, 20, TRUE);

   n_vid();

   for (y=1; y<19; y++) {
      goto1(7, t+y);
      for (x=0; x<63; x++)
	 pch(' ');
   }

   for (y=0; about_text[y]; y++) {
      goto1(10, t+2+y);
      for (x=0; about_text[y][x]; x++) {
	 if (about_text[y][x]=='\\') {
	    if (n) {
	       keyword_vid();
	       n = FALSE;
	    }
	    else {
	       n_vid();
	       n = TRUE;
	    }
	 }
	 else {
	    pch(about_text[y][x]);
	 }
      }
   }

   while ((!input_waiting()) && (!m_b))
      poll_mouse();

   clear_keybuf();
   n_vid();
   show_c();
   dirty_everything();
}



void colboxdraw_proc(int w, LISTBOX *lb, LISTITEM *li)
{
   int c;
   if (li == lb->data+lb->current)
      pch('*');
   else
      pch(' ');
   pch(' ');
   tattr(li->data<<4);
   mywrite(li->text);
   if (li == lb->data+lb->current)
      n_vid();
   else
      hi_vid();
   for (c=strlen(li->text)+2; c<w; c++)
      pch(' ');
}



LISTITEM colorbox_s[16] =
{
   { NULL, 0, NULL, colboxdraw_proc, "Black" },
   { NULL, 1, NULL, colboxdraw_proc, "Blue" },
   { NULL, 2, NULL, colboxdraw_proc, "Green" },
   { NULL, 3, NULL, colboxdraw_proc, "Cyan" },
   { NULL, 4, NULL, colboxdraw_proc, "Red" },
   { NULL, 5, NULL, colboxdraw_proc, "Magenta" },
   { NULL, 6, NULL, colboxdraw_proc, "Brown" },
   { NULL, 7, NULL, colboxdraw_proc, "Light Grey" },
   { NULL, 8, NULL, colboxdraw_proc, "Dark Grey" },
   { NULL, 9, NULL, colboxdraw_proc, "Light Blue" },
   { NULL, 10, NULL, colboxdraw_proc, "Light Green" },
   { NULL, 11, NULL, colboxdraw_proc, "Light Cyan" },
   { NULL, 12, NULL, colboxdraw_proc, "Light Red" },
   { NULL, 13, NULL, colboxdraw_proc, "Light Magenta" },
   { NULL, 14, NULL, colboxdraw_proc, "Yellow" },
   { NULL, 15, NULL, colboxdraw_proc, "White" }
};



LISTBOX colorbox =
{
   NULL,
   NULL,
   10, 10, 32, 20,
   2, 2, 1, 16, 26, 16,
   0, 0,
   colorbox_s,
   NULL, NULL, 
   0, 0, 
   LISTBOX_MOUSE_CANCEL
};



int colclick_proc(LISTBOX *lb, LISTITEM *li)
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };
   int c;
   char buf[80];
   int ret;

   colorbox.title = li->text;
   colorbox.x = (screen_w - colorbox.w) / 2 - 1;
   colorbox.y = (screen_h - colorbox.h) / 2;

   if (lb->current & 1)
      colorbox.current = (*((int *)li->data) & 0xf0) >> 4;
   else
      colorbox.current = *((int *)li->data) & 0xf;

   for (c=0; c<22; c++)
      buf[c] = ' ';
   buf[c] = 0;
   for (c=0; c<colorbox.count; c++)
      colorbox_s[c].text = buf;

   ret = do_listbox(&colorbox, oklist, cancellist, -1, FALSE);
   hide_c();

   if (ret != -1) {
      int *i = (int *)li->data;
      if (lb->current & 1) {
	 *i &= 0xf;
	 if (*i == colorbox.current)
	    (*i)++;
	 *i |= colorbox.current << 4;
      }
      else {
	 *i >>= 4;
	 if (*i == colorbox.current) {
	    (*i)++;
	    if (*i > 7)
	       *i = 0;
	 }
	 *i <<= 4;
	 *i |= colorbox.current;
      }
      dirty_everything();
      redisplay();
      draw_listbox(lb);
   }

   while (m_b)
      poll_mouse();

   return -1;
}



void coldraw_proc(int w, LISTBOX *lb, LISTITEM *li)
{
   int c;
   if (li == lb->data+lb->current)
      pch('*');
   else
      pch(' ');
   pch(' ');
   tattr(*((int *)li->data));
   mywrite(li->text);
   for (c=strlen(li->text)+5; c<w; c++)
      pch(' ');
   if (li == lb->data+lb->current)
      n_vid();
   else
      hi_vid();
   pch(' ');
   pch(' ');
   pch(' ');
}



LISTITEM colormenu_s[18] =
{
   { "Text Color",           (int)&config.norm_col,    colclick_proc, coldraw_proc, NULL },
   { "Text Background",      (int)&config.norm_col,    colclick_proc, coldraw_proc, NULL },
   { "Selected Text Color",  (int)&config.sel_col,     colclick_proc, coldraw_proc, NULL },
   { "Selected Background",  (int)&config.sel_col,     colclick_proc, coldraw_proc, NULL },
   { "Folded Text Color",    (int)&config.fold_col,    colclick_proc, coldraw_proc, NULL },
   { "Folded Background ",   (int)&config.fold_col,    colclick_proc, coldraw_proc, NULL },
   { "User Interface Color", (int)&config.hi_col,      colclick_proc, coldraw_proc, NULL },
   { "Interface Background", (int)&config.hi_col,      colclick_proc, coldraw_proc, NULL },
   { "Language Keywords",    (int)&config.keyword_col, colclick_proc, coldraw_proc, NULL },
   { "Keywords Background",  (int)&config.keyword_col, colclick_proc, coldraw_proc, NULL },
   { "Comments",             (int)&config.comment_col, colclick_proc, coldraw_proc, NULL },
   { "Comment Background",   (int)&config.comment_col, colclick_proc, coldraw_proc, NULL },
   { "String Constants",     (int)&config.string_col,  colclick_proc, coldraw_proc, NULL },
   { "String Background",    (int)&config.string_col,  colclick_proc, coldraw_proc, NULL },
   { "Numeric Constants",    (int)&config.number_col,  colclick_proc, coldraw_proc, NULL },
   { "Numeric Background",   (int)&config.number_col,  colclick_proc, coldraw_proc, NULL },
   { "Symbol Characters",    (int)&config.symbol_col,  colclick_proc, coldraw_proc, NULL },
   { "Symbol Background",    (int)&config.symbol_col,  colclick_proc, coldraw_proc, NULL },
};



LISTBOX colormenu =
{
   NULL, 
   "Colors",
   0, 0, 58, 13,
   3, 2, 2, 9, 24, 18,
   0, 0,
   colormenu_s,
   NULL, NULL, 
   0, 0,
   LISTBOX_MOUSE_CANCEL
};



void fn_colors()
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };

   colormenu.x = (screen_w - colormenu.w) / 2 - 1;
   colormenu.y = (screen_h - colormenu.h) / 2;

   do_listbox(&colormenu, oklist, cancellist, -1, FALSE);
   dirty_everything();
}



int boolclick_proc(LISTBOX *lb, LISTITEM *li)
{
   int *i = (int *)li->data;
   *i = *i ? FALSE : TRUE;
   return -1;
}



void booldraw_proc(int w, LISTBOX *lb, LISTITEM *li)
{
   int c;

   mywrite(li->text);
   mywrite(": ");

   if (*((int *)li->data))
      mywrite("YES");
   else
      mywrite("NO ");

   for (c = strlen(li->text)+5; c<w; c++)
      pch(' ');
}



int strclick_proc(LISTBOX *lb, LISTITEM *li)
{
   char buf[80];
   strcpy(buf, (char *)li->data);
   if (input_text (li->text, buf, 64, is_asciichar) != ESC)
      strcpy((char *)li->data, buf);
   hide_c();
   draw_contents(lb);
   while (m_b)
      poll_mouse();
   return -1;
}



void strdraw_proc(int w, LISTBOX *lb, LISTITEM *li)
{
   int c;
   char *p = (char *)li->data;

   mywrite(li->text);
   mywrite(": ");

   for (c=strlen(li->text)+2; (*p) && (c<w); c++)
      pch(*(p++));

   while (c++ < w)
      pch(' ');
}



int numclick_proc(LISTBOX *lb, LISTITEM *li)
{
   int i = input_number(li->text, *((int *)li->data));
   if (i < 0)
      i = 0;
   else if (i > 256)
      i = 256;
   *((int *)li->data) = i;
   draw_contents(lb);
   hide_c();
   while (m_b)
      poll_mouse();
   return -1;
}



void numdraw_proc(int w, LISTBOX *lb, LISTITEM *li)
{
   int c;
   char buf[40];

   mywrite(li->text);
   mywrite(": ");
   itoa(*((int *)li->data), buf, 10);
   mywrite(buf);

   for (c = strlen(li->text)+2+strlen(buf); c<w; c++)
      pch(' ');
}



LISTITEM displaybox_s[5] =
{
   { "Display comments",         (int)&config.comments,  boolclick_proc, booldraw_proc, "If set, the menus display comments (like this message)" },
   { "Display cursor position",  (int)&config.show_pos,  boolclick_proc, booldraw_proc, "If set, the cursor position is displayed at the bottom of the screen" },
   { "Display menu bar",         (int)&config.show_menu, boolclick_proc, booldraw_proc, "If set, the menus are always visible at the top of the screen" },
   { "Display scroll bar",       (int)&config.show_bar,  boolclick_proc, booldraw_proc, "If set, the scroll bar is always visible at the right of the screen" },
};



LISTBOX displaybox =
{
   NULL, 
   "Screen display options",
   20, 2,               /* x, y */
   38, 8,               /* w, h */
   3, 2,                /* xoff, yoff */
   1, 4,                /* width, height */
   30,                  /* slen */
   4,                   /* count */
   0, 0,                /* scroll, current */
   displaybox_s,
   NULL, NULL, 0, 0,
   LISTBOX_MOUSE_CANCEL
};



int display_proc(LISTBOX *lb, LISTITEM *li)
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };

   displaybox.y = (screen_h - displaybox.h) / 2;

   do_listbox(&displaybox, oklist, cancellist, -1, FALSE);

   dirty_everything();
   cursor_move_line();
   draw_contents(lb);

   while (m_b)
      poll_mouse();

   return -1;
}



LISTITEM toolsmenubox_s[16] =
{
   { "Tool 1",  (int)&config.tool[0],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 2",  (int)&config.tool[1],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 3",  (int)&config.tool[2],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 4",  (int)&config.tool[3],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 5",  (int)&config.tool[4],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 6",  (int)&config.tool[5],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 7",  (int)&config.tool[6],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 8",  (int)&config.tool[7],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 9",  (int)&config.tool[8],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 10",  (int)&config.tool[9],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 11",  (int)&config.tool[10],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 12",  (int)&config.tool[11],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 13",  (int)&config.tool[12],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 14",  (int)&config.tool[13],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 15",  (int)&config.tool[14],  strclick_proc,  strdraw_proc, NULL  },
   { "Tool 16",  (int)&config.tool[15],  strclick_proc,  strdraw_proc, NULL  }
};



LISTBOX toolsmenubox =
{
   NULL, 
   "Edit tools menu",
   5, 2,                /* x, y */
   68, 20,              /* w, h */
   3, 2,                /* xoff, yoff */
   1, 16,               /* width, height */
   60,                  /* slen */
   16,                  /* count */
   0, 0,                /* scroll, current */
   toolsmenubox_s,
   NULL, NULL, 0, 0,
   LISTBOX_MOUSE_CANCEL
};



int tools_proc(LISTBOX *lb, LISTITEM *li)
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };

   toolsmenubox.y = (screen_h - toolsmenubox.h) / 2;

   do_listbox(&toolsmenubox, oklist, cancellist, -1, FALSE);

   draw_contents(lb);
   while (m_b)
      poll_mouse();

   sort_out_tools();

   return -1;
}



LISTITEM configbox_s[20] =
{
   { "Screen display options",   0,                            display_proc,     NULL,          "Toggle display of cursor position, menus and the scroll bar" },
   { "Edit tools menu",          0,                            tools_proc,       NULL,          "Customise the tools menu" },
   { "Edit right click menu",    0,                            right_menu_proc,  NULL,          "Customise the right mouse button popup menu" },
   { "Double click function",    (int)&config.dclick_command,  funcclick_proc,   funcdraw_proc, "The function to execute when the left mouse button is double-clicked" },
   { "Auto fold files",          (int)&config.auto_fold,       strclick_proc,    strdraw_proc,  "Files with these extensions will automatically be folded" },
   { "Auto indent files",        (int)&config.auto_indent,     strclick_proc,    strdraw_proc,  "Files with these extensions will have automatic indentation turned on" },
   { "Binary files",             (int)&config.bin,             strclick_proc,    strdraw_proc,  "Files with these extensions will be loaded in binary mode" },
   { "Hex mode files",           (int)&config.hex,             strclick_proc,    strdraw_proc,  "Files with these extensions will be loaded in hex editing mode" },
   { "Browse files",             (int)&config.makefiles,       strclick_proc,    strdraw_proc,  "Files with these extensions will be loaded by the browse command" },
   { "Wordwrap files",           (int)&config.wrap,            strclick_proc,    strdraw_proc,  "Files with these extensions will have wordwrap turned on" },
   { "Wrap at column",           (int)&config.wrap_col,        numclick_proc,    numdraw_proc,  "The default column at which to wordwrap" },
   { "Use big cursor",           (int)&config.big_cursor,      boolclick_proc,   booldraw_proc, "If set, a tall editing cursor is displayed" },
   { "Make backups",             (int)&config.make_backups,    boolclick_proc,   booldraw_proc, "If set, when saving files the old versions are renamed to *.bak" },
   { "Check for changed files",  (int)&config.check_files,     boolclick_proc,   booldraw_proc, "If set, checks for files being changed by external programs" },
   { "Strip spaces on save",     (int)&config.save_strip,      boolclick_proc,   booldraw_proc, "If set, trailing spaces are removed and tabs are inserted when saving files" },
   { "Print only ascii chars",   (int)&config.print_ascii,     boolclick_proc,   booldraw_proc, "If set, extended ascii characters are replaced with spaces during printing" },
   { "External tab size",        (int)&config.load_tabs,       numclick_proc,    numdraw_proc,  "The tab size used by external files, for the read/write routines" },
   { "Screensaver delay (mins)", (int)&config.screen_save,     numclick_proc,    numdraw_proc,  "The delay before the screensaver kicks in. Zero for no screensaver" },
   { "Undo levels",              (int)&config.undo_levels,     numclick_proc,    numdraw_proc,  "The number of stored undo operations (per file)" },
   { "File search",              (int)&config.file_search,     numclick_proc,    numdraw_proc,  "How many directories to recurse when looking for a file" }
};



LISTBOX configbox =
{
   NULL, 
   "Configuration Options",
   7, 2,                /* x, y */
   64, 24,              /* w, h */
   3, 2,                /* xoff, yoff */
   1, 20,               /* width, height */
   56,                  /* slen */
   20,                  /* count */
   0, 0,                /* scroll, current */
   configbox_s,
   NULL, NULL, 0, 0,
   LISTBOX_SCROLL | LISTBOX_MOUSE_CANCEL
};



void fn_config()
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };

   configbox.y = (screen_h - configbox.h) / 2;

   do_listbox(&configbox, oklist, cancellist, -1, FALSE);
}



LISTITEM binary_box_s[256] =
{
   { "\x20", 0x00, NULL, NULL, NULL },    { "\x01", 0x01, NULL, NULL, NULL },
   { "\x02", 0x02, NULL, NULL, NULL },    { "\x03", 0x03, NULL, NULL, NULL },
   { "\x04", 0x04, NULL, NULL, NULL },    { "\x05", 0x05, NULL, NULL, NULL },
   { "\x06", 0x06, NULL, NULL, NULL },    { "\x07", 0x07, NULL, NULL, NULL },
   { "\x08", 0x08, NULL, NULL, NULL },    { "\x09", 0x09, NULL, NULL, NULL },
   { "\x0A", 0x0A, NULL, NULL, NULL },    { "\x0B", 0x0B, NULL, NULL, NULL },
   { "\x0C", 0x0C, NULL, NULL, NULL },    { "\x0D", 0x0D, NULL, NULL, NULL },
   { "\x0E", 0x0E, NULL, NULL, NULL },    { "\x0F", 0x0F, NULL, NULL, NULL },

   { "\x10", 0x10, NULL, NULL, NULL },    { "\x11", 0x11, NULL, NULL, NULL },
   { "\x12", 0x12, NULL, NULL, NULL },    { "\x13", 0x13, NULL, NULL, NULL },
   { "\x14", 0x14, NULL, NULL, NULL },    { "\x15", 0x15, NULL, NULL, NULL },
   { "\x16", 0x16, NULL, NULL, NULL },    { "\x17", 0x17, NULL, NULL, NULL },
   { "\x18", 0x18, NULL, NULL, NULL },    { "\x19", 0x19, NULL, NULL, NULL },
   { "\x1A", 0x1A, NULL, NULL, NULL },    { "\x1B", 0x1B, NULL, NULL, NULL },
   { "\x1C", 0x1C, NULL, NULL, NULL },    { "\x1D", 0x1D, NULL, NULL, NULL },
   { "\x1E", 0x1E, NULL, NULL, NULL },    { "\x1F", 0x1F, NULL, NULL, NULL },

   { "\x20", 0x20, NULL, NULL, NULL },    { "\x21", 0x21, NULL, NULL, NULL },
   { "\x22", 0x22, NULL, NULL, NULL },    { "\x23", 0x23, NULL, NULL, NULL },
   { "\x24", 0x24, NULL, NULL, NULL },    { "\x25", 0x25, NULL, NULL, NULL },
   { "\x26", 0x26, NULL, NULL, NULL },    { "\x27", 0x27, NULL, NULL, NULL },
   { "\x28", 0x28, NULL, NULL, NULL },    { "\x29", 0x29, NULL, NULL, NULL },
   { "\x2A", 0x2A, NULL, NULL, NULL },    { "\x2B", 0x2B, NULL, NULL, NULL },
   { "\x2C", 0x2C, NULL, NULL, NULL },    { "\x2D", 0x2D, NULL, NULL, NULL },
   { "\x2E", 0x2E, NULL, NULL, NULL },    { "\x2F", 0x2F, NULL, NULL, NULL },

   { "\x30", 0x30, NULL, NULL, NULL },    { "\x31", 0x31, NULL, NULL, NULL },
   { "\x32", 0x32, NULL, NULL, NULL },    { "\x33", 0x33, NULL, NULL, NULL },
   { "\x34", 0x34, NULL, NULL, NULL },    { "\x35", 0x35, NULL, NULL, NULL },
   { "\x36", 0x36, NULL, NULL, NULL },    { "\x37", 0x37, NULL, NULL, NULL },
   { "\x38", 0x38, NULL, NULL, NULL },    { "\x39", 0x39, NULL, NULL, NULL },
   { "\x3A", 0x3A, NULL, NULL, NULL },    { "\x3B", 0x3B, NULL, NULL, NULL },
   { "\x3C", 0x3C, NULL, NULL, NULL },    { "\x3D", 0x3D, NULL, NULL, NULL },
   { "\x3E", 0x3E, NULL, NULL, NULL },    { "\x3F", 0x3F, NULL, NULL, NULL },

   { "\x40", 0x40, NULL, NULL, NULL },    { "\x41", 0x41, NULL, NULL, NULL },
   { "\x42", 0x42, NULL, NULL, NULL },    { "\x43", 0x43, NULL, NULL, NULL },
   { "\x44", 0x44, NULL, NULL, NULL },    { "\x45", 0x45, NULL, NULL, NULL },
   { "\x46", 0x46, NULL, NULL, NULL },    { "\x47", 0x47, NULL, NULL, NULL },
   { "\x48", 0x48, NULL, NULL, NULL },    { "\x49", 0x49, NULL, NULL, NULL },
   { "\x4A", 0x4A, NULL, NULL, NULL },    { "\x4B", 0x4B, NULL, NULL, NULL },
   { "\x4C", 0x4C, NULL, NULL, NULL },    { "\x4D", 0x4D, NULL, NULL, NULL },
   { "\x4E", 0x4E, NULL, NULL, NULL },    { "\x4F", 0x4F, NULL, NULL, NULL },

   { "\x50", 0x50, NULL, NULL, NULL },    { "\x51", 0x51, NULL, NULL, NULL },
   { "\x52", 0x52, NULL, NULL, NULL },    { "\x53", 0x53, NULL, NULL, NULL },
   { "\x54", 0x54, NULL, NULL, NULL },    { "\x55", 0x55, NULL, NULL, NULL },
   { "\x56", 0x56, NULL, NULL, NULL },    { "\x57", 0x57, NULL, NULL, NULL },
   { "\x58", 0x58, NULL, NULL, NULL },    { "\x59", 0x59, NULL, NULL, NULL },
   { "\x5A", 0x5A, NULL, NULL, NULL },    { "\x5B", 0x5B, NULL, NULL, NULL },
   { "\x5C", 0x5C, NULL, NULL, NULL },    { "\x5D", 0x5D, NULL, NULL, NULL },
   { "\x5E", 0x5E, NULL, NULL, NULL },    { "\x5F", 0x5F, NULL, NULL, NULL },

   { "\x60", 0x60, NULL, NULL, NULL },    { "\x61", 0x61, NULL, NULL, NULL },
   { "\x62", 0x62, NULL, NULL, NULL },    { "\x63", 0x63, NULL, NULL, NULL },
   { "\x64", 0x64, NULL, NULL, NULL },    { "\x65", 0x65, NULL, NULL, NULL },
   { "\x66", 0x66, NULL, NULL, NULL },    { "\x67", 0x67, NULL, NULL, NULL },
   { "\x68", 0x68, NULL, NULL, NULL },    { "\x69", 0x69, NULL, NULL, NULL },
   { "\x6A", 0x6A, NULL, NULL, NULL },    { "\x6B", 0x6B, NULL, NULL, NULL },
   { "\x6C", 0x6C, NULL, NULL, NULL },    { "\x6D", 0x6D, NULL, NULL, NULL },
   { "\x6E", 0x6E, NULL, NULL, NULL },    { "\x6F", 0x6F, NULL, NULL, NULL },

   { "\x70", 0x70, NULL, NULL, NULL },    { "\x71", 0x71, NULL, NULL, NULL },
   { "\x72", 0x72, NULL, NULL, NULL },    { "\x73", 0x73, NULL, NULL, NULL },
   { "\x74", 0x74, NULL, NULL, NULL },    { "\x75", 0x75, NULL, NULL, NULL },
   { "\x76", 0x76, NULL, NULL, NULL },    { "\x77", 0x77, NULL, NULL, NULL },
   { "\x78", 0x78, NULL, NULL, NULL },    { "\x79", 0x79, NULL, NULL, NULL },
   { "\x7A", 0x7A, NULL, NULL, NULL },    { "\x7B", 0x7B, NULL, NULL, NULL },
   { "\x7C", 0x7C, NULL, NULL, NULL },    { "\x7D", 0x7D, NULL, NULL, NULL },
   { "\x7E", 0x7E, NULL, NULL, NULL },    { "\x7F", 0x7F, NULL, NULL, NULL },

   { "\x80", 0x80, NULL, NULL, NULL },    { "\x81", 0x81, NULL, NULL, NULL },
   { "\x82", 0x82, NULL, NULL, NULL },    { "\x83", 0x83, NULL, NULL, NULL },
   { "\x84", 0x84, NULL, NULL, NULL },    { "\x85", 0x85, NULL, NULL, NULL },
   { "\x86", 0x86, NULL, NULL, NULL },    { "\x87", 0x87, NULL, NULL, NULL },
   { "\x88", 0x88, NULL, NULL, NULL },    { "\x89", 0x89, NULL, NULL, NULL },
   { "\x8A", 0x8A, NULL, NULL, NULL },    { "\x8B", 0x8B, NULL, NULL, NULL },
   { "\x8C", 0x8C, NULL, NULL, NULL },    { "\x8D", 0x8D, NULL, NULL, NULL },
   { "\x8E", 0x8E, NULL, NULL, NULL },    { "\x8F", 0x8F, NULL, NULL, NULL },

   { "\x90", 0x90, NULL, NULL, NULL },    { "\x91", 0x91, NULL, NULL, NULL },
   { "\x92", 0x92, NULL, NULL, NULL },    { "\x93", 0x93, NULL, NULL, NULL },
   { "\x94", 0x94, NULL, NULL, NULL },    { "\x95", 0x95, NULL, NULL, NULL },
   { "\x96", 0x96, NULL, NULL, NULL },    { "\x97", 0x97, NULL, NULL, NULL },
   { "\x98", 0x98, NULL, NULL, NULL },    { "\x99", 0x99, NULL, NULL, NULL },
   { "\x9A", 0x9A, NULL, NULL, NULL },    { "\x9B", 0x9B, NULL, NULL, NULL },
   { "\x9C", 0x9C, NULL, NULL, NULL },    { "\x9D", 0x9D, NULL, NULL, NULL },
   { "\x9E", 0x9E, NULL, NULL, NULL },    { "\x9F", 0x9F, NULL, NULL, NULL },

   { "\xA0", 0xA0, NULL, NULL, NULL },    { "\xA1", 0xA1, NULL, NULL, NULL },
   { "\xA2", 0xA2, NULL, NULL, NULL },    { "\xA3", 0xA3, NULL, NULL, NULL },
   { "\xA4", 0xA4, NULL, NULL, NULL },    { "\xA5", 0xA5, NULL, NULL, NULL },
   { "\xA6", 0xA6, NULL, NULL, NULL },    { "\xA7", 0xA7, NULL, NULL, NULL },
   { "\xA8", 0xA8, NULL, NULL, NULL },    { "\xA9", 0xA9, NULL, NULL, NULL },
   { "\xAA", 0xAA, NULL, NULL, NULL },    { "\xAB", 0xAB, NULL, NULL, NULL },
   { "\xAC", 0xAC, NULL, NULL, NULL },    { "\xAD", 0xAD, NULL, NULL, NULL },
   { "\xAE", 0xAE, NULL, NULL, NULL },    { "\xAF", 0xAF, NULL, NULL, NULL },

   { "\xB0", 0xB0, NULL, NULL, NULL },    { "\xB1", 0xB1, NULL, NULL, NULL },
   { "\xB2", 0xB2, NULL, NULL, NULL },    { "\xB3", 0xB3, NULL, NULL, NULL },
   { "\xB4", 0xB4, NULL, NULL, NULL },    { "\xB5", 0xB5, NULL, NULL, NULL },
   { "\xB6", 0xB6, NULL, NULL, NULL },    { "\xB7", 0xB7, NULL, NULL, NULL },
   { "\xB8", 0xB8, NULL, NULL, NULL },    { "\xB9", 0xB9, NULL, NULL, NULL },
   { "\xBA", 0xBA, NULL, NULL, NULL },    { "\xBB", 0xBB, NULL, NULL, NULL },
   { "\xBC", 0xBC, NULL, NULL, NULL },    { "\xBD", 0xBD, NULL, NULL, NULL },
   { "\xBE", 0xBE, NULL, NULL, NULL },    { "\xBF", 0xBF, NULL, NULL, NULL },

   { "\xC0", 0xC0, NULL, NULL, NULL },    { "\xC1", 0xC1, NULL, NULL, NULL },
   { "\xC2", 0xC2, NULL, NULL, NULL },    { "\xC3", 0xC3, NULL, NULL, NULL },
   { "\xC4", 0xC4, NULL, NULL, NULL },    { "\xC5", 0xC5, NULL, NULL, NULL },
   { "\xC6", 0xC6, NULL, NULL, NULL },    { "\xC7", 0xC7, NULL, NULL, NULL },
   { "\xC8", 0xC8, NULL, NULL, NULL },    { "\xC9", 0xC9, NULL, NULL, NULL },
   { "\xCA", 0xCA, NULL, NULL, NULL },    { "\xCB", 0xCB, NULL, NULL, NULL },
   { "\xCC", 0xCC, NULL, NULL, NULL },    { "\xCD", 0xCD, NULL, NULL, NULL },
   { "\xCE", 0xCE, NULL, NULL, NULL },    { "\xCF", 0xCF, NULL, NULL, NULL },

   { "\xD0", 0xD0, NULL, NULL, NULL },    { "\xD1", 0xD1, NULL, NULL, NULL },
   { "\xD2", 0xD2, NULL, NULL, NULL },    { "\xD3", 0xD3, NULL, NULL, NULL },
   { "\xD4", 0xD4, NULL, NULL, NULL },    { "\xD5", 0xD5, NULL, NULL, NULL },
   { "\xD6", 0xD6, NULL, NULL, NULL },    { "\xD7", 0xD7, NULL, NULL, NULL },
   { "\xD8", 0xD8, NULL, NULL, NULL },    { "\xD9", 0xD9, NULL, NULL, NULL },
   { "\xDA", 0xDA, NULL, NULL, NULL },    { "\xDB", 0xDB, NULL, NULL, NULL },
   { "\xDC", 0xDC, NULL, NULL, NULL },    { "\xDD", 0xDD, NULL, NULL, NULL },
   { "\xDE", 0xDE, NULL, NULL, NULL },    { "\xDF", 0xDF, NULL, NULL, NULL },

   { "\xE0", 0xE0, NULL, NULL, NULL },    { "\xE1", 0xE1, NULL, NULL, NULL },
   { "\xE2", 0xE2, NULL, NULL, NULL },    { "\xE3", 0xE3, NULL, NULL, NULL },
   { "\xE4", 0xE4, NULL, NULL, NULL },    { "\xE5", 0xE5, NULL, NULL, NULL },
   { "\xE6", 0xE6, NULL, NULL, NULL },    { "\xE7", 0xE7, NULL, NULL, NULL },
   { "\xE8", 0xE8, NULL, NULL, NULL },    { "\xE9", 0xE9, NULL, NULL, NULL },
   { "\xEA", 0xEA, NULL, NULL, NULL },    { "\xEB", 0xEB, NULL, NULL, NULL },
   { "\xEC", 0xEC, NULL, NULL, NULL },    { "\xED", 0xED, NULL, NULL, NULL },
   { "\xEE", 0xEE, NULL, NULL, NULL },    { "\xEF", 0xEF, NULL, NULL, NULL },

   { "\xF0", 0xF0, NULL, NULL, NULL },    { "\xF1", 0xF1, NULL, NULL, NULL },
   { "\xF2", 0xF2, NULL, NULL, NULL },    { "\xF3", 0xF3, NULL, NULL, NULL },
   { "\xF4", 0xF4, NULL, NULL, NULL },    { "\xF5", 0xF5, NULL, NULL, NULL },
   { "\xF6", 0xF6, NULL, NULL, NULL },    { "\xF7", 0xF7, NULL, NULL, NULL },
   { "\xF8", 0xF8, NULL, NULL, NULL },    { "\xF9", 0xF9, NULL, NULL, NULL },
   { "\xFA", 0xFA, NULL, NULL, NULL },    { "\xFB", 0xFB, NULL, NULL, NULL },
   { "\xFC", 0xFC, NULL, NULL, NULL },    { "\xFD", 0xFD, NULL, NULL, NULL },
   { "\xFE", 0xFE, NULL, NULL, NULL },    { "\xFF", 0xFF, NULL, NULL, NULL },
};



void draw_binary_box(LISTBOX *l)
{
   int c;

   draw_box(l->title, l->x, l->y, l->w, l->h, TRUE);

   hi_vid();

   for (c=0; c<16; c++) {
      goto1(l->x+l->xoff+1+c*3, l->y+2);
      pch(hex_digit[c]);

      goto1(l->x+2, l->y+l->yoff+c);
      pch(hex_digit[c]);
   }

   goto1(l->x+l->xoff-2, l->y+3);
   pch(TL_CHAR);
   for (c=l->x+l->xoff; c<l->x+l->w; c++)
      pch(TOP_CHAR);
   pch(RJOIN_CHAR);

   for (c=l->y+l->yoff; c<l->y+l->h-1; c++) {
      goto1(l->x+4, c);
      pch(SIDE_CHAR);
   }
   goto1(l->x+4, c);
   pch(BJOIN_CHAR);

   n_vid();
}



int binary_box_proc(LISTBOX *l, int key)
{
   int c;
   int digit = -1;

   c = tolower(ascii(key));
   if ((c >= '0') && (c <= '9'))
      digit = c - '0';
   else if ((c >= 'a') && (c <= 'f'))
      digit = 10 + c - 'a';

   if (digit >= 0) {
      l->current = ((l->current & 0xf) << 4) + digit;
      return TRUE;
   }
   else 
      return FALSE;
}



LISTBOX binarybox =
{
   NULL, 
   "Insert binary character",
   11, 1,               /* x, y */
   56, 21,              /* w, h */
   6, 4,                /* xoff, yoff */
   16, 16,              /* width, height */
   1,                   /* slen */
   256,                 /* count */
   0, 0,                /* scroll, current */
   binary_box_s,        /* data */
   draw_binary_box,
   binary_box_proc, 
   0, 0,
   LISTBOX_WRAP | LISTBOX_MOUSE_CANCEL
};



int get_binary_char()
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };
   binarybox.y = (screen_h-binarybox.h)/2;
   return do_listbox(&binarybox, oklist, cancellist, -1, FALSE);
}



LISTITEM inputscreenheight_s[6] =
{
 #ifdef TARGET_ALLEG
   { "25 lines",   25,  NULL, NULL, NULL },
   { "50 lines",   50,  NULL, NULL, NULL },
   { "60 lines",   60,  NULL, NULL, NULL },
   { "75 lines",   75,  NULL, NULL, NULL },
   { "96 lines",   96,  NULL, NULL, NULL },
   { "128 lines",  128, NULL, NULL, NULL },
 #else
   { "25 lines",   25,  NULL, NULL, NULL },
   { "28 lines",   28,  NULL, NULL, NULL },
   { "35 lines",   35,  NULL, NULL, NULL },
   { "40 lines",   40,  NULL, NULL, NULL },
   { "43 lines",   43,  NULL, NULL, NULL },
   { "50 lines",   50,  NULL, NULL, NULL },
 #endif
};



LISTBOX inputscreenheight =
{
   NULL, "Screen height",
   0, 0, 19, 10,           /* x, y, w, h */
   2, 2,                   /* xoff, yoff */
   1, 6,                   /* width, height */
   13,                     /* slen */
   6,                      /* count */
   0, 1,
   inputscreenheight_s,
   NULL, NULL, 
   0, 0, 
   LISTBOX_MOUSE_CANCEL
};



int input_screen_height()
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };
   int c;

   inputscreenheight.x = (screen_w-inputscreenheight.w)/2;
   inputscreenheight.y = (screen_h-inputscreenheight.h)/2;

   for (c=0; c<6; c++)
      if (inputscreenheight_s[c].data == config.screen_height)
	 inputscreenheight.current = c;

   return do_listbox(&inputscreenheight, oklist, cancellist, -1, FALSE);
}



LISTITEM searchmode_s[3] =
{
   { "Relaxed",         SEARCH_RELAXED,  NULL, NULL, "'ABC' matches 'abc' and 'subABCstrings'" },
   { "Case sensitive",  SEARCH_CASE,     NULL, NULL, "'ABC' matches 'subABCstrings' but not 'abc'" },
   { "Keyword",         SEARCH_KEYWORD,  NULL, NULL, "'ABC' does not match 'abc' or 'subABCstrings'" },
};



LISTBOX searchmode =
{
   NULL, "Search mode",
   0, 0, 21, 7,            /* x, y, w, h */
   2, 2,                   /* xoff, yoff */
   1, 3,                   /* width, height */
   15,                     /* slen */
   3,                      /* count */
   0, 1,
   searchmode_s,
   NULL, NULL, 
   0, 0, 
   LISTBOX_MOUSE_CANCEL
};



void fn_srch_mode()
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };
   int c;

   searchmode.x = (screen_w-searchmode.w)/2;
   searchmode.y = (screen_h-searchmode.h)/2;

   for (c=0; c<3; c++)
      if (searchmode_s[c].data == config.search_mode)
	 searchmode.current = c;

   c = do_listbox(&searchmode, oklist, cancellist, -1, FALSE);

   if (c >= 0)
      config.search_mode = c;
}
