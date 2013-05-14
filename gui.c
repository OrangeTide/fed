/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      User interface stuff like menus, listboxes, and the file selector
 */


#include <ctype.h>

#include "fed.h"



void draw_contents(LISTBOX *l)
{
   int x, y;
   int c, c2;
   int len;
   char *p;

   hi_vid();

   if (l->flags & LISTBOX_SCROLL) {
      goto1(l->x+l->xoff+3, l->y+1);
      for (c=0; c<4; c++) {
	 if (l->scroll > 0)
	    pch(UP_CHAR);
	 else
	    pch(' ');
	 pch(' ');
	 pch(' ');
      }

      goto1(l->x+l->xoff+3, l->y+l->h-2);
      for (c=0; c<4; c++) {
	 if (l->scroll + l->width*l->height < l->count)
	    pch(DOWN_CHAR);
	 else
	    pch(' ');
	 pch(' ');
	 pch(' ');
      }
   }

   for (y=0; y<l->height; y++) {
      for (x=0; x<l->width; x++) {
	 goto1(l->x+l->xoff+(l->slen+2)*x, l->y+l->yoff+y);
	 c = l->scroll + y*l->width + x;

	 if (c==l->current)
	    n_vid();

	 pch(' ');

	 if (c < l->count) {
	    if (l->data[c].draw_proc) {
	       (*l->data[c].draw_proc)(l->slen+1, l, l->data+c);
	    }
	    else {
	       p = l->data[c].text;
	       len = l->slen;

	       if (strlen(p) > len) {
		  pch('.');
		  pch('.');
		  pch('.');
		  pch(' ');

		  len -= 4;

		  p += (strlen(p) - len);
	       }

	       for (c2=0; p[c2]; c2++)
		  pch(p[c2]);

	       while (c2++ <= len)
		  pch(' ');
	    }
	 }
	 else
	    for (c2=0; c2<l->slen+1; c2++)
	       pch(' ');

	 if (c==l->current) {
	    if ((config.comments) && (l->data[c].comment)) {
	       strcpy(message, l->data[c].comment);
	       display_message(0);
	       hide_c();
	    }
	    hi_vid();
	 }
      }
   }

   n_vid();
}



void draw_box(char *title, int _x, int _y, int _w, int _h, int shadow)
{
   int x, y;

   hi_vid();

   goto1(_x, _y);
   pch(TL_CHAR);
   pch(TOP_CHAR);
   x=2;
   if (title) {
      mywrite(title);
      x += strlen(title);
   }
   while (x<_w-1) {
      pch(TOP_CHAR);
      x++;
   }
   pch(TR_CHAR);

   for (y=1; y<_h-1; y++) {
      goto1(_x, _y+y);
      pch(SIDE_CHAR);
      for (x=1; x<_w-1; x++)
	 pch(' ');
      pch(SIDE_CHAR);
   }

   goto1(_x, _y+_h-1);
   pch(BL_CHAR);
   for (x=1; x<_w-1; x++)
      pch(BOTTOM_CHAR);
   pch(BR_CHAR);

   if (shadow) {
      tattr(((config.hi_col >> 4) == 8) ? 0 : 128);

      for (y=1; y<_h; y++) {
	 goto1(_x+_w, _y+y);
	 pch(' ');
      }

      goto1(_x+1, _y+_h);
      for (x=0; x<_w; x++)
	 pch(' ');
   }

   n_vid();
}



void draw_listbox(LISTBOX *l)
{
   if (l->draw_proc)
      (*l->draw_proc)(l);
   else
      draw_box(l->title, l->x, l->y, l->w, l->h, TRUE);

   draw_contents(l);
}



int mouse_inside_list(LISTBOX *l)
{
   return ((m_x >= l->x + l->xoff) && 
	   (m_x < l->x + l->xoff + (l->slen+2) * l->width) &&
	   (m_y >= l->y + l->yoff) &&
	   (m_y < l->y + l->yoff + l->height));
}



int mouse_outside_list(LISTBOX *l)
{
   return ((m_x < l->x) || (m_x >= l->x + l->w) ||
	   (m_y < l->y) || (m_y >= l->y + l->h));
}



int mouse_inside_parent(LISTBOX *l)
{
   LISTBOX *p = l->parent;

   return ((mouse_outside_list(l)) &&
	   (p!= NULL) && 
	   (m_x >= p->x + p->xoff) && 
	   (m_x < p->x + p->xoff + (p->slen+2) * p->width) &&
	   (m_y >= p->y + p->yoff) &&
	   (m_y < p->y + p->yoff + p->height));
}



int get_mouse_item(LISTBOX *l)
{
   int cur = l->scroll;
   cur += (m_y - l->y - l->yoff) * l->width;
   cur += (m_x - l->x - l->xoff) / (l->slen+2);
   return cur;
}



int do_listbox(LISTBOX *l, int *oklist, int *cancellist, int init_key, int instant_mouse)
{
   int key;
   int *pos;
   int done;
   int ret;
   int c;
   char *b = save_screen(l->x, l->y, l->w, l->h);
   int is_key;

   hide_c();
   draw_listbox(l);

   l->key = init_key;
   l->key2 = -1;

   display_mouse();

   while (m_b) {
      poll_mouse();

      if ((instant_mouse) &&
	  ((m_b & 2) ||
	   (mouse_inside_list(l)) ||
	   ((mouse_inside_parent(l)) &&
	    (get_mouse_item(l->parent) != l->parent->current))))
	 break;
   }

   do {
      if (l->key == -1) {
	 while (!input_waiting()) {
	    poll_mouse();
	    if (m_b)
	       break;
	 }
	 if (input_waiting()) {
	    key = input_char().key;
	    is_key = TRUE;
	 }
	 else {
	    int ox, oy, ob; 
	    int sel_item;
	    int first = TRUE;

	    key = 0;
	    is_key = FALSE;

	    while (m_b) {
	       if ((m_b & 2) && (!(l->flags & LISTBOX_USE_RMB))) {
		  key = ESC;
		  is_key = TRUE;
		  break;
	       }

	       sel_item = -1;
	       if (mouse_inside_list(l)) {
		  l->current = get_mouse_item(l);
		  if (l->current >= l->count)
		     l->current = l->count-1;
		  else
		     sel_item = l->current;
	       }

	       hide_mouse();
	       draw_contents(l);
	       display_mouse();

	       if (first) {
		  if (sel_item >= 0) {
		     if (l->flags & LISTBOX_MOUSE_OK) {
			key = CR;
			is_key = TRUE;
			break;
		     }
		     if (!(l->flags & LISTBOX_MOUSE_OK2)) {
			if (!mouse_dclick(FALSE)) {
			   if (mouse_dclick(TRUE)) {
			      key = CR;
			      is_key = TRUE;
			      break;
			   }
			}
		     }
		  }
		  else if (mouse_outside_list(l)) {
		     if (l->flags & LISTBOX_MOUSE_CANCEL) {
			key = ESC;
			is_key = TRUE;
			break;
		     }
		  }

		  first = FALSE;
	       }

	       if (mouse_inside_parent(l)) {
		  if (l->flags & LISTBOX_MOUSE_CANCEL) {
		     key = ESC;
		     is_key = TRUE;
		     break;
		  }
	       }

	       if (l->flags & LISTBOX_SCROLL) {
		  int changed = FALSE;

		  if ((m_y < l->y + l->yoff) && (l->scroll > 0)) {
		     l->scroll--;
		     l->current = l->scroll;
		     m_y = -1;
		     changed = TRUE;
		  }
		  else if ((m_y >= l->y + l->yoff + l->height) &&
			   (l->scroll < l->count - l->height)) {
		     l->scroll++;
		     l->current = l->scroll + l->height - 1;
		     if (l->current >= l->count)
			l->current = l->count-1;
		     m_y = -1;
		     changed = TRUE;
		  }

		  if (changed) {
		     hide_mouse();
		     draw_contents(l);
		     display_mouse();
		     delay(20);
		  }
	       }

	       do {
		  ox = m_x;
		  oy = m_y;
		  ob = m_b;
		  poll_mouse();
	       } while ((m_x == ox) && (m_y == oy) && 
			(m_b == ob) && (m_b) &&
			(!input_waiting()));

	       if ((!m_b) && (sel_item >= 0) &&
		   (l->flags & LISTBOX_MOUSE_OK2)) {
		  key = CR;
		  is_key = TRUE;
		  break;
	       }
	    }
	 }
      }
      else {
	 key = l->key;
	 l->key = l->key2;
	 l->key2 = -1;
	 is_key = TRUE;
      }

      done = FALSE;

      if (is_key) {
	 pos = oklist;
	 while (*pos) {
	    if ((*pos == key) || (ascii(key)==CR) || (ascii(key)==LF)) {
	       if ((l->current >= 0) && (l->current < l->count)) {
		  hide_mouse();
		  if (l->data[l->current].click_proc)
		     ret = (l->data[l->current].click_proc)
			       (l, l->data+l->current);
		  else
		     ret = l->data[l->current].data;
		  if (ret >= 0) {
		     l->key = key;
		     restore_screen(l->x, l->y, l->w, l->h, b);
		     show_c();
		     return ret;
		  }
		  else {
		     hide_c();
		     display_mouse();
		  }
	       }
	       done = TRUE;
	       break;
	    }
	    pos++;
	 }

	 if (!done) {
	    pos = cancellist;
	    while (*pos) {
	       if ((*pos == key) || (ascii(key)==ESC)) {
		  hide_mouse();
		  l->key = key;
		  restore_screen(l->x, l->y, l->w, l->h, b);
		  show_c();
		  return -1;
	       }
	       pos++;
	    }
	 }

	 if (!done) {
	    if (key==LEFT_ARROW)
	       l->current--;
	    else if (key==UP_ARROW)
	       l->current -= l->width;
	    else if (key==RIGHT_ARROW)
	       l->current++;
	    else if (key==DOWN_ARROW)
	       l->current += l->width;
	    else if (key==PAGE_UP)
	       l->current -= l->width * l->height - 1;
	    else if (key==PAGE_DOWN)
	       l->current += l->width * l->height - 1;
	    else if (key==K_HOME)
	       l->current = 0;
	    else if (key==K_END)
	       l->current = l->count-1;
	    else if (l->process_proc)
	       if ((*l->process_proc)(l, key))
		  goto skip;

	    for (c=0; c<l->count; c++)
	       if (tolower(l->data[c].text[0]) == tolower(ascii(key))) {
		  l->current = c;
		  if (l->flags & LISTBOX_FASTKEY)
		     l->key = oklist[0];
		  break;
	       }

	    skip: 
	    if (l->current < 0) {
	       if (l->flags & LISTBOX_WRAP)
		  l->current = MAX(0, l->count-1);
	       else
		  l->current = 0;
	    }
	    else if (l->current >= l->count) {
	       if (l->flags & LISTBOX_WRAP)
		  l->current = 0;
	       else
		  l->current = MAX(0, l->count-1);
	    }

	    while (l->current < l->scroll)
	       l->scroll -= l->width;
	    while (l->current >= l->scroll+l->width*l->height)
	       l->scroll += l->width;
	 } 
      }

      hide_mouse();
      draw_contents(l);
      display_mouse();

   } while (TRUE);
}



LISTBOX filebox =
{
   NULL, NULL,
   7, 6,                /* x, y */
   64, 0,               /* w, h */
   3, 2,                /* xoff, yoff */
   1, 0,                /* width, height */
   56,                  /* slen */
   0,                   /* count */
   0, -1,               /* scroll, current */
   NULL,                /* data */
   NULL, NULL, 0, 0,
   LISTBOX_SCROLL | LISTBOX_MOUSE_CANCEL
};



typedef struct FILENODE
{
   struct FILENODE *next;
   char name[0];
} FILENODE;


FILENODE *filenode = NULL;

char filepath[256] = "";



int add_to_filelist(char *f, int d)
{
   FILENODE *pos, *prev, *node;
   char fn[256];

   strcpy(fn, f);
   cleanup_filename(fn);

   node = malloc(sizeof(FILENODE) + strlen(fn) + 1);
   if (!node)
      return (errno = ENOMEM);

   strcpy(node->name, fn);

   prev = NULL;
   pos = filenode;

   while (pos) {
      if (((node->name[strlen(node->name)-1] == '\\') && (pos->name[strlen(pos->name)-1] != '\\')) ||
	  ((node->name[strlen(node->name)-1] == '/') && (pos->name[strlen(pos->name)-1] != '/')) ||
	  (stricmp(pos->name, node->name) > 0))
	 break;
      prev = pos;
      pos = pos->next;
   }

   node->next = pos;
   if (prev)
      prev->next = node;
   else
      filenode = node;

   filebox.count++;
   return 0;
}



void empty_filelist()
{
   FILENODE *node, *prev;

   filebox.count = filebox.scroll = 0;
   filebox.current = -1;

   free(filebox.data);
   filebox.data = NULL;

   node = filenode;
   while (node) {
      prev = node;
      node = node->next;
      free(prev);
   }
   filenode = NULL;
   filepath[0] = 0;
}



void fill_filelist(char *path)
{
   char b[256];
   int c;
   FILENODE *node;
   int redraw = FALSE;
   int old;

   strcpy(b, path);
 #if (defined TARGET_DJGPP) || (defined TARGET_WIN)
   strcpy(get_fname(b), "*.*");
 #else
   strcpy(get_fname(b), "*");
 #endif
   cleanup_filename(b);

   if (stricmp(b, filepath) != 0) {
      empty_filelist();
      do_for_each_file(b, add_to_filelist, 0);
      do_for_each_directory(b, add_to_filelist, 0);

      if (filebox.count > 0) {
	 filebox.data = malloc(sizeof(LISTITEM)*filebox.count);
	 if (filebox.data) {
	    node = filenode;
	    for (c=0; c<filebox.count; c++) {
	       filebox.data[c].text = node->name;
	       filebox.data[c].data = -1;
	       filebox.data[c].click_proc = NULL;
	       filebox.data[c].draw_proc = NULL;
	       filebox.data[c].comment = NULL;
	       node = node->next;
	    }
	 }
	 else
	    errno = ENOMEM;
      }

      redraw = TRUE;
      strcpy(filepath, b);
   }

   old = filebox.current;
   filebox.current = -1;

   if (path[0]) {
      char *p = get_fname(path);
      int len = strlen(p);
      if (p[0]) {
	 for (c=0; c<filebox.count; c++) {
	    if (strnicmp(p, get_fname(filebox.data[c].text), len)==0) {
	       filebox.current = c;
	       break;
	    } 
	 }
      }
   }

   if (filebox.current != old) {
      if (filebox.current != -1) {
	 while (filebox.current < filebox.scroll)
	    filebox.scroll--;
	 while (filebox.current >= filebox.scroll+filebox.height)
	    filebox.scroll++;
      }
      redraw = TRUE;
   }

   if (redraw) {
      if (errno==0)
	 draw_contents(&filebox);
      else {
	 filebox.current = -1;
	 filebox.count = -1;
	 draw_contents(&filebox);
	 hi_vid();
	 goto1(filebox.x+filebox.xoff+1, filebox.y+filebox.yoff);
	 mywrite("Error: ");
	 mywrite(err());
      }
   }
}



int fsel_mouse(int x, int y, char *s, int *len)
{
   int c, gap;
   int selitem;
   int first = TRUE;
   int changed;
   int d;

   #ifdef TARGET_CURSES
      int count = 0;
   #endif

   if ((m_x < filebox.x) || (m_x >= filebox.x + filebox.w) ||
       (m_y < filebox.y) || (m_y >= filebox.y + filebox.h) || 
       (m_b & 2))
      return ESC;

   if (filebox.count <= 0)
      return 0;

   do {
      delay(10);

      selitem = -1;
      changed = d = FALSE;

      if (mouse_inside_list(&filebox)) {
	 filebox.current = get_mouse_item(&filebox); 
	 if (filebox.current >= filebox.count)
	    filebox.current = filebox.count - 1;
	 else
	    selitem = filebox.current;
	 changed = TRUE;
      }
      else {
	 if ((m_y < filebox.y + filebox.yoff) && (filebox.scroll > 0)) {
	    filebox.scroll--;
	    filebox.current = filebox.scroll;
	    changed = d = TRUE;
	 }
	 else if ((m_y >= filebox.y + filebox.yoff + filebox.height) &&
		  (filebox.scroll < filebox.count - filebox.height)) {
	    filebox.scroll++;
	    filebox.current = filebox.scroll + filebox.height - 1;
	    if (filebox.current >= filebox.count)
	       filebox.current = filebox.count-1;
	    changed = d = TRUE;
	 }
      }

      if (changed) {
	 hide_mouse(); 
	 draw_contents(&filebox);
	 strcpy(s, filebox.data[filebox.current].text);

	 goto1(x, y);
	 hi_vid();

	 if (strlen(s) > 54) {
	    gap = strlen(s) - 54;

	    pch('.');
	    pch('.');
	    pch('.');
	    pch(' ');

	    mywrite(s+gap+4);
	 }
	 else {
	    gap = 0;

	    mywrite(s);
	 }

	 for (c=strlen(s)-gap; c<54; c++)
	    pch(' ');

	 goto2(x+strlen(s+gap), y);

	 *len = strlen(s);
	 display_mouse();
      }

      if (d)
	 delay(20);

      if ((first) && (selitem >= 0))
	 if (!mouse_dclick(FALSE))
	    if (mouse_dclick(TRUE))
	       return CR;

      first = FALSE;

      refresh_screen();

   #ifdef TARGET_CURSES
      if (d) {
	 if (count++ >= 10)
	    m_b = 0;
      }
      else
	 while (m_b)
   #endif
	    poll_mouse(); 

   } while(m_b);

   clear_keybuf();
   return 0;
}



int fsel_proc(int key, char *s, int *len, int *cpos)
{
   if ((ascii(key)==CR) || (ascii(key)==LF)) {
      if ((*len > 0) && ((s[*len-1] == '\\') || (s[*len-1] == '/'))) {
	 fill_filelist(s);
	 hi_vid();
	 return TRUE; 
      }
   }
   if (key==UP_ARROW) {
      if (filebox.count > 0) {
	 if (filebox.current >= 0)
	    filebox.current--;
	 else
	    filebox.current = filebox.scroll;
	 if (filebox.current < 0)
	    filebox.current = 0;
	 if (filebox.current < filebox.scroll)
	    filebox.scroll = filebox.current;
	 goto changed;
      }
   }
   else if (key==DOWN_ARROW) {
      if (filebox.count > 0) {
	 if (filebox.current >= 0)
	    filebox.current++;
	 else
	    filebox.current = filebox.scroll;
	 if (filebox.current >= filebox.count)
	    filebox.current = filebox.count-1;
	 if (filebox.current >= filebox.scroll + filebox.height)
	    filebox.scroll = filebox.current - filebox.height + 1;
	 goto changed;
      }
   }
   else if (key==PAGE_UP) {
      if (filebox.count > 0) {
	 if (filebox.current >= 0)
	    filebox.current -= filebox.height - 1;
	 else
	    filebox.current = filebox.scroll;
	 if (filebox.current < 0)
	    filebox.current = 0;
	 if (filebox.current < filebox.scroll)
	    filebox.scroll = filebox.current;
	 goto changed;
      }
   }
   else if (key==PAGE_DOWN) {
      if (filebox.count > 0) {
	 if (filebox.current >= 0)
	    filebox.current += filebox.height - 1;
	 else
	    filebox.current = filebox.scroll;
	 if (filebox.current >= filebox.count)
	    filebox.current = filebox.count-1;
	 if (filebox.current >= filebox.scroll + filebox.height)
	    filebox.scroll = filebox.current - filebox.height + 1;
	 goto changed;
      }
   }
   else if (ascii(key)==TAB) {
      if (filebox.current >= 0)
	 goto changed;
   }
   else if (key==-1)
      fill_filelist(s);

   hi_vid();
   return FALSE;

   changed:
   strcpy(s, filebox.data[filebox.current].text);
   *len = *cpos = strlen(s);
   draw_contents(&filebox);
   hi_vid();
   return TRUE; 
}



int select_file(char *prompt, char *fname)
{
   int ret;
   static char p[256] = "";

   filebox.h = screen_h - 10;
   filebox.height = screen_h - 14;
   filebox.w = MAX(screen_w - 16, 32);
   filebox.slen = filebox.w - 8;

   draw_box(prompt, 7, 2, filebox.w, 5, TRUE);

   draw_listbox(&filebox);
   hi_vid();
   goto1(7,6);
   pch(LJOIN_CHAR);
   goto1(filebox.w+6,6);
   pch(RJOIN_CHAR);

   if (!fname[0]) {
      if (p[0])
	 strcpy(fname, p);
      else
	 getcwd(fname, 256);

      cleanup_filename(fname);
      append_backslash(fname);
   }

   fill_filelist(fname);

   goto1(12, 4);
   ret = do_input_text(fname, 255, 54, is_filechar, fsel_proc, TRUE, fsel_mouse);

   if (ret != ESC) {
      strcpy(p, fname);
      *get_fname(p) = 0;
   }

   n_vid();
   empty_filelist();
   dirty_everything();
   return ret;
}



LISTITEM filetype_s[3] =
{
   { "Text",   BUF_ASCII,  NULL, NULL, "Open as a normal ascii text file" },
   { "Binary", BUF_BINARY, NULL, NULL, "Ignore formatting characters (tabs and carriage returns)" },
   { "Hex",    BUF_HEX,    NULL, NULL, "Open in hex editing mode" }
};



LISTBOX filetype =
{
   NULL, "Open mode",
   0, 0, 19, 7,            /* x, y, w, h */
   2, 2,                   /* xoff, yoff */
   1, 3,                   /* width, height */
   13,                     /* slen */
   3,                      /* count */
   0, 1,
   filetype_s,
   NULL, NULL, 
   0, 0, 
   LISTBOX_MOUSE_CANCEL
};



int open_file_type()
{
   int oklist[] = { -1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };

   filetype.x = (screen_w-filetype.w)/2;
   filetype.y = (screen_h-filetype.h)/2;

   return do_listbox(&filetype, oklist, cancellist, -1, FALSE);
}



LISTITEM askbox_s[2] =
{
   { "Yes", 1, NULL, NULL, NULL },
   { "No",  0, NULL, NULL, NULL },
};



void draw_askbox(LISTBOX *l)
{
   draw_box(l->title, l->x, l->y, l->w, l->h, TRUE);
   draw_contents(l);
   hi_vid();
   goto1(l->x+4, l->y+2);
   mywrite((char *)l->parent);
   pch('?');
   n_vid();
}



LISTBOX askbox =
{
   NULL, NULL,
   0, 0, 0, 6,             /* x, y, w, h */
   0, 4,                   /* xoff, yoff */
   2, 1,                   /* width, height */
   4,                      /* slen */
   2,                      /* count */
   0, 1,
   askbox_s,
   draw_askbox,
   NULL, 
   0, 0, 
   LISTBOX_WRAP | LISTBOX_FASTKEY | LISTBOX_MOUSE_CANCEL | LISTBOX_MOUSE_OK2
};



int ask(char *title, char *s1, char *s2)        /* ask a y/n question */
{
   int oklist[] = { 1, 0 };
   int cancellist[] = { CTRL_C, CTRL_G, 0 };
   int ret;
   char msg[256];

   if (s1)
      strcpy(msg, s1);
   else
      msg[0] = 0;
   if (s2)
      strcat(msg, s2);

   askbox.parent = (LISTBOX *)msg;
   askbox.title = title;
   askbox.w = strlen(msg) + 14;
   if (askbox.w < 32)
      askbox.w = 32;
   askbox.xoff = (askbox.w - 8) / 2 - 1;
   askbox.x = (screen_w - askbox.w) / 2 - 1;
   askbox.y = (screen_h - askbox.h) / 2;
   askbox.current = 1;

   ret = do_listbox(&askbox, oklist, cancellist, -1, FALSE);
   if (ret == -1)
      return FALSE;
   else
      return (ret==0) ? FALSE: TRUE;
}

