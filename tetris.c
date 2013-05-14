/*
 *      FED - Folding Editor
 *
 *      By Shawn Hargreaves, 1994
 *
 *      See README.TXT for copyright conditions
 *
 *      The tetris game and screensaver
 */


#include <time.h>

#include "fed.h"


#define TW      8
#define TH      16

#define START_TIME      64


typedef struct {
   int x,y;
} SHAPE;


typedef struct {
   int time;
   int box[TW][TH];
   int old_box[TW][TH];
   SHAPE shape[4];
   int rot_flag;
   int score;
   int cont;
   int x,y,w,h;
} TETRIS;


static int record = -1;
static char record_name[80] = "";


void redraw_tetris(TETRIS *data);
void draw_square(TETRIS *data, int x, int y, int col);
void create_tetris_shape(TETRIS *data);
void show_shape(TETRIS *data, int col);
int ok_to_move(TETRIS *data, int x, int y);
void move_shape(TETRIS *data, int d);
void rotate_shape(TETRIS *data);
void drop_shape(TETRIS *data);
int down_shape(TETRIS *data);
void check_shape_position(TETRIS *data);
int check_row(TETRIS *data, int row);
void tetris_message(int score);



void fn_tetris()
{
   TETRIS data;
   int x,y;
   int key;
   int count;
   int dirty;
   float c;

   if (record == -1) {
      char fn[256];
      MYFILE *f;
      strcpy(fn, exe_path);
      strcpy(get_fname(fn), RECORD_FILE);
      f = open_file(fn, FMODE_READ);
      if (f) {
	 count = 0;
	 while (errno == 0) {
	    record_name[count] = ~get_char(f);
	    if ((record_name[count] > '9') || (record_name[count] < 0))
	       break;
	    count++;
	 }
	 record_name[count] = 0;
	 record = atoi(record_name);
	 count = 0;
	 while (errno == 0) {
	    record_name[count] = ~get_char(f);
	    if (errno == EOF) {
	       record_name[count] = 0;
	       break;
	    }
	    count++;
	 }
	 record_name[count] = 0;
	 close_file(f); 
      }
      errno = 0;
   }

   data.time = count = START_TIME;
   data.cont = TRUE;
   data.score = 0L;

   for(y=0;y<TH;y++)
      for(x=0;x<TW;x++) {
	 data.box[x][y] = FALSE;
	 data.old_box[x][y] = 10;       /* force redraw inside box */
      }

   data.h=(screen_h-4)/TH;
   data.y=((screen_h-2)-(data.h*TH)) / 2;
   data.w=(screen_w-2)/TW/4;
   data.x=((screen_w-2)-(data.w*TW)) / 3;

   hide_c();
   hi_vid();

   goto1(data.x-1,data.y-1);
   pch(TL_CHAR);
   for (x=1;x<data.w*TW+1;x++)
      pch(TOP_CHAR);
   pch(TR_CHAR);

   for (y=0;y<data.h*TH;y++) {
      goto1(data.x-1,data.y+y);
	 pch(SIDE_CHAR);
      goto1(data.x+data.w*TW,data.y+y);
	 pch(SIDE_CHAR);
   }

   goto1(data.x-1,data.y+data.h*TH);
   pch(BL_CHAR);
   for (x=1;x<data.w*TW+1;x++)
      pch(BOTTOM_CHAR);
   pch(BR_CHAR);

   create_tetris_shape(&data);
   clear_keybuf();

   tetris_message(0L);
   dirty_everything();

   dirty = TRUE;

   c = (float)clock()*1000;

   do {
      if (dirty) {
	 redraw_tetris(&data);
	 dirty = FALSE;
      }

      #if (defined TARGET_CURSES) || (defined TARGET_WIN)
	 nosleep = TRUE;
      #endif

      if (keypressed()) {
	 key = gch();

	 #if (defined TARGET_CURSES) || (defined TARGET_WIN)
	    nosleep = FALSE;
	 #endif

	 if (key==UP_ARROW) {
	    rotate_shape(&data);
	    dirty = TRUE;
	 }
	 else if (key==LEFT_ARROW) {
	    move_shape(&data,-1);
	    dirty = TRUE;
	 }
	 else if (key==RIGHT_ARROW) {
	    move_shape(&data,1);
	    dirty = TRUE;
	 }
	 else if ((key==DOWN_ARROW) || (ascii(key)==' ')) {
	    drop_shape(&data);
	    dirty = TRUE;
	    c = (float)clock()*1000;
	 }
	 else if ((ascii(key)==ESC) || (key==CTRL_G)) {
	    n_vid();
	    show_c();
	    return;
	 }
	 key = 0L;
      }
      #if (defined TARGET_CURSES) || (defined TARGET_WIN)
	 else
	    nosleep = FALSE;
      #endif

      if ((count--) < 0) {
	 count = data.time;
	 down_shape(&data);
	 dirty = TRUE;
      }

      c += CLOCKS_PER_SEC*3;

      do {
      } while ((float)clock() < c/1000);

      refresh_screen();

   } while (data.cont);

   n_vid();

   if (data.score > record) {
      char fn[256];
      MYFILE *f;

      record = data.score;
      clear_keybuf();
      input_text("A new record! Enter your name", record_name, 30, is_asciichar);

      strcpy(fn, exe_path);
      strcpy(get_fname(fn), RECORD_FILE);
      f = open_file(fn, FMODE_WRITE);
      if (f) {
	 itoa(record, fn, 10);
	 for (count=0; fn[count]; count++)
	    put_char(f, ~fn[count]);
	 put_char(f, ~':');
	 for (count=0; record_name[count]; count++)
	    put_char(f, ~record_name[count]);
	 close_file(f); 
      }
      errno = 0;
   }

   strcpy(message,"Tetris score: ");
   itoa(data.score, message+strlen(message), 10);
   display_message(MESSAGE_KEY);
   show_c();
   clear_keybuf();
}



void redraw_tetris(TETRIS *data)
{
   int x,y;

   for (y=0;y<TH;y++)
      for (x=0;x<TW;x++)
	 if (data->box[x][y]!=data->old_box[x][y]) {
	    draw_square(data,x,y,data->box[x][y]);
	    data->old_box[x][y] = data->box[x][y];
	 }
}



void draw_square(TETRIS *data, int x, int y, int col)
{
   int xc, yc;

   if (col)
      tattr(config.fold_col & 0xf0);
   else
      tattr(config.fold_col << 4);

   for (yc=0;yc<data->h;yc++) {
      goto1(data->x+x*data->w,data->y+yc+y*data->h);
      for (xc=0;xc<data->w;xc++)
	 pch(' ');
   }
}



void create_tetris_shape(TETRIS *data)
{
   switch (rand()%5) {

      case 0:
	 data->rot_flag=TRUE;
	 if (rand()%2) {
	    data->shape[0].x=3;
	    data->shape[0].y=0;
	    data->shape[1].x=4;
	    data->shape[1].y=0;
	    data->shape[2].x=3;
	    data->shape[2].y=1;
	    data->shape[3].x=2;
	    data->shape[3].y=1;
	 }
	 else {
	    data->shape[0].x=3;
	    data->shape[0].y=0;
	    data->shape[1].x=2;
	    data->shape[1].y=0;
	    data->shape[2].x=3;
	    data->shape[2].y=1;
	    data->shape[3].x=4;
	    data->shape[3].y=1;
	 }
	 break;

      case 1:
	 data->rot_flag=TRUE;
	 data->shape[0].x=3;
	 data->shape[0].y=0;
	 data->shape[1].x=4;
	 data->shape[1].y=0;
	 data->shape[2].x=2;
	 data->shape[2].y=0;
	 data->shape[3].x=3;
	 data->shape[3].y=1;
	 break;

      case 2:
	 data->rot_flag=TRUE;
	 if (rand()%2) {
	    data->shape[0].x=3;
	    data->shape[0].y=0;
	    data->shape[1].x=4;
	    data->shape[1].y=0;
	    data->shape[2].x=4;
	    data->shape[2].y=1;
	    data->shape[3].x=2;
	    data->shape[3].y=0;
	 }
	 else {
	    data->shape[0].x=3;
	    data->shape[0].y=0;
	    data->shape[1].x=4;
	    data->shape[1].y=0;
	    data->shape[2].x=2;
	    data->shape[2].y=1;
	    data->shape[3].x=2;
	    data->shape[3].y=0;
	 }
	 break;

      case 3:
	 data->rot_flag=FALSE;
	 data->shape[0].x=3;
	 data->shape[0].y=0;
	 data->shape[1].x=4;
	 data->shape[1].y=0;
	 data->shape[2].x=3;
	 data->shape[2].y=1;
	 data->shape[3].x=4;
	 data->shape[3].y=1;
	 break;

      case 4:
	 data->rot_flag=TRUE;
	 data->shape[0].x=3;
	 data->shape[0].y=0;
	 data->shape[1].x=4;
	 data->shape[1].y=0;
	 data->shape[2].x=2;
	 data->shape[2].y=0;
	 data->shape[3].x=5;
	 data->shape[3].y=0;
	 break;
   }

   show_shape(data,TRUE);
}



void show_shape(TETRIS *data, int col)
{
   int x;

   for(x=0;x<4;x++)
      data->box[data->shape[x].x][data->shape[x].y]=col;
}



int ok_to_move(TETRIS *data, int x, int y)
{
   int c;

   show_shape(data,FALSE);

   for (c=0;c<4;c++)
      if ((data->shape[c].x+x < 0) || (data->shape[c].x+x >= TW) ||
	  (data->shape[c].y+y >= TH) ||
	  (data->box[data->shape[c].x+x][data->shape[c].y+y]!=0)) {

	 show_shape(data,TRUE);
	 return FALSE;
      }

   show_shape(data,TRUE);
   return TRUE;
}



void move_shape(TETRIS *data, int d)
{
   if (ok_to_move(data,d,0)) {
      show_shape(data,FALSE);
      data->shape[0].x+=d;
      data->shape[1].x+=d;
      data->shape[2].x+=d;
      data->shape[3].x+=d;
      show_shape(data,TRUE);
   }
}



void rotate_shape(TETRIS *data)
{
   int c;
   int x,y;
   SHAPE old_shape[4];

   if (data->rot_flag) {
      show_shape(data,FALSE);
      for(c=0;c<4;c++)
	 old_shape[c]=data->shape[c];
      for(c=1;c<4;c++) {
	 x=data->shape[0].x+data->shape[0].y-data->shape[c].y;
	 y=data->shape[0].y-data->shape[0].x+data->shape[c].x;
	 data->shape[c].x=x;
	 data->shape[c].y=y;
      }

      for(c=1;c<4;c++) {
	 if (data->shape[c].x < 0 || data->shape[c].x >= TW ||
	     data->shape[c].y < 0 || data->shape[c].y >= TH ||
	     data->box[data->shape[c].x][data->shape[c].y]!=0) {

	    for(c=0;c<4;c++)
		data->shape[c]=old_shape[c];

	    goto getout;
	 }
      }

      getout:
      show_shape(data,TRUE);
   }
}



void drop_shape(TETRIS *data)
{
   int l;

   while (down_shape(data)) {
      redraw_tetris(data);
      for (l=0; l<800; l++)
	 ;
      delay(10);
   }
}



int down_shape(TETRIS *data)
{
   if (ok_to_move(data,0,1)) {
      show_shape(data,FALSE);

      data->shape[0].y++;
      data->shape[1].y++;
      data->shape[2].y++;
      data->shape[3].y++;

      show_shape(data,TRUE);
      return TRUE;
   }
   else {
      check_shape_position(data);
      create_tetris_shape(data);
      return FALSE;
   }
}



void check_shape_position(TETRIS *data)
{
   int x,y;

   for(x=0;x<TW;x++)
      if(data->box[x][0]!=0) {
	 data->cont=FALSE;
	 return;
      }

   for(y=TH-1;y>=0;y--)
      if(check_row(data,y))
	 y++;
}



int check_row(TETRIS *data, int row)
{
   int x,y;

   for(x=0;x<TW;x++)
      if(data->box[x][row]==0)
	 return(FALSE);

   for(y=row;y>0;y--)
      for(x=0;x<TW;x++)
	 data->box[x][y]=data->box[x][y-1];

   tetris_message(++data->score);
   if(data->time>1)
     data->time-=(data->time/14);

   return(TRUE);
}



void tetris_message(int score)
{
   char tmp[80];

   goto1(0,screen_h-1);
   hi_vid();
   mywrite("Tetris score: ");
   itoa(score, tmp, 10);
   mywrite(tmp);

   if (record >= 0) {
      strcpy(tmp, "The record is ");
      itoa(record, tmp+strlen(tmp), 10);
      strcat(tmp, ", by ");
      strcat(tmp, record_name);
      del_to_eol();
      goto1(screen_w-strlen(tmp)-1, screen_h-1);
      mywrite(tmp);
   }
   else
      del_to_eol();
}



static char *life_shape1[] =
{
   "...**",
   "...**",
   "...*.**",
   "...*.*",
   "...***",
   "....*",
   ".***",
   "*.*",
   ".**",
   "....**",
   ".**",
   "*.*",
   ".***",
   "....*",
   "...***",
   "...*.*",
   "...*.**",
   "...**",
   "...**",
   NULL
};



static char *life_shape2[] =
{
   "..........*..........*",
   "......*.**.**.*.....**",
   ".....**.......**...***",
   "......**.*.*.*.*..**",
   "....***.......*.*.*..*",
   "......*.........*.*.*",
   ".***............***.*",
   ".*................*",
   ".",
   "**................**",
   NULL
};



static char *life_shape3[] =
{
   "..........*",
   "*.....*.**.**.*",
   "**...**.......**",
   ".**..*.*.*.*.**",
   "..*.*.*.......***",
   "*.*.*.........*",
   "*.***............***",
   "..*................*",
   ".",
   ".**................**",
   NULL
};



static char *life_shape4[] =
{
   ".**",
   "*.*",
   "..*",
   NULL
};



static char *life_shape5[] =
{
   "**.",
   "*.*",
   "*..",
   NULL
};



static char *life_shape6[] =
{
   "..*",
   "*.*",
   ".**",
   NULL
};



static char *life_shape7[] =
{
   "***",
   "..*",
   ".*",
   NULL
};



static char *life_shape8[] =
{
   "***",
   "*..*",
   "*...*",
   ".*..*",
   "..***",
   NULL
};



static char *life_shape9[] =
{
   ".***",
   "*...*",
   "*....*",
   "*..*..*",
   ".*....*",
   "..*...*",
   "...***",
   NULL
};



static char *life_shape10[] =
{
   "...*",
   "..***",
   ".**.*",
   "**",
   ".**",
   NULL
};



static char *life_shape11[] =
{
   "***",
   "*..",
   ".*",
   NULL
};



static char *life_shape12[] =
{
   "*",
   "*.*",
   "**",
   NULL
};



static char *life_shape13[] =
{
   "....*",
   "....**",
   "..**.**",
   "..*....*",
   "**......*",
   ".**.....**",
   "..*......**",
   "...*....*",
   "....**.**",
   ".....**",
   "......*",
   NULL
};



static char *life_shape14[] =
{
   "....*",
   "....**",
   "..**.**",
   "..*....*",
   "**......*",
   ".**.....**",
   "..*......**",
   "...*....*",
   "....**.**",
   ".....**",
   "......*",
   ".",
   "....***",
   "...*..*",
   "..*...*",
   "..*..*",
   "..***",
   NULL
};



static char *life_shape15[] =
{
   "......***",
   ".....*..*",
   "....*...*",
   "....*..*",
   "....***",
   ".",
   "....*",
   "....**",
   "..**.**",
   "..*....*",
   "**......*",
   ".**.....**",
   "..*......**",
   "...*....*",
   "....**.**",
   ".....**",
   "......*",
   NULL
};



static char *life_shape16[] =
{
   ".*",
   "..*",
   "***",
   NULL
};



static char *life_shape17[] =
{
   "*",
   "***",
   "...*",
   "..*",
   "..*...*",
   ".",
   ".....*",
   ".",
   "....*.*",
   ".",
   ".....*",
   ".....*",
   "...**.**",
   ".....*",
   NULL
};



static char *life_shape18[] =
{
   "..***",
   "..*.*",
   "..*.*",
   "..***",
   ".",
   ".*...*",
   ".....*",
   "....*",
   "........**",
   "*........*",
   ".**......*.*",
   "..........**",
   NULL
};



static char *life_shape19[] =
{
   "......*",
   "....***",
   "...*",
   "...**",
   ".",
   ".",
   "***",
   "*.*",
   "***",
   ".",
   "***",
   "***",
   ".*",
   NULL
};



static char *life_shape20[] =
{
   ".*..*",
   "*...*",
   "*..*.*",
   "*...*",
   ".*..*",
   ".",
   ".....*.*",
   ".....**",
   "......*",
   NULL
};



static char *life_shape21[] =
{
   ".*",
   "***",
   "***",
   ".",
   "***",
   "*.*",
   "***",
   NULL
};



static char *life_shape22[] =
{
   ".*",
   "*",
   "***",
   NULL
};



#define LIFE_SHAPE_COUNT   22


static char **life_shape[LIFE_SHAPE_COUNT] =
{
   life_shape1,
   life_shape2,
   life_shape3,
   life_shape4,
   life_shape5,
   life_shape6,
   life_shape7,
   life_shape8,
   life_shape9,
   life_shape10,
   life_shape11,
   life_shape12,
   life_shape13,
   life_shape14,
   life_shape15,
   life_shape16,
   life_shape17,
   life_shape18,
   life_shape19,
   life_shape20,
   life_shape21,
   life_shape22,
};



void screen_saver()
{
   char *p1, *p2, *p3, *p4;
   int x, y, xo, yo, c, c2;
   int count = 0;
   int clear = 4;

   p1 = malloc(screen_w * screen_h);
   p2 = malloc(screen_w * screen_h);
   p3 = malloc(screen_w * screen_h);
   memset(p1, 0, screen_w * screen_h);
   memset(p2, 0, screen_w * screen_h);
   memset(p3, 0, screen_w * screen_h);

   hide_c();
   tattr(7);
   cls();

   do {
      if (count == 0) {
	 if (clear <= 0) {
	    memset(p1, 0, screen_w * screen_h);
	    clear = 4;
	 }
	 else
	    clear--;

	 c = rand() % LIFE_SHAPE_COUNT;
	 x=0;
	 for (y=0; life_shape[c][y]; y++)
	    if (strlen(life_shape[c][y]) > x)
	       x = strlen(life_shape[c][y]);

	 xo = 2+rand()%(screen_w-x-4);
	 yo = 2+rand()%(screen_h-y-4);

	 for (y=0; life_shape[c][y]; y++) {
	    for (x=0; life_shape[c][y][x]; x++) {
	       if (life_shape[c][y][x]=='*')
		  p1[x+xo + (y+yo)*screen_w] = 1;
	    }
	 }

	 count = 4 + (rand() & 127);
      }
      else
	 if (count > 0)
	    count--;
	 else
	    count++;

      for (y=1; y<screen_h-1; y++) {
	 for (x=1; x<screen_w-1; x++) {
	    c = p1[x-1 + (y-1)*screen_w] +
		p1[x + (y-1)*screen_w] +
		p1[x+1 + (y-1)*screen_w] +
		p1[x-1 + y*screen_w] +
		p1[x+1 + y*screen_w] +
		p1[x-1 + (y+1)*screen_w] +
		p1[x + (y+1)*screen_w] +
		p1[x+1 + (y+1)*screen_w];

	    if (c==3)
	       p2[x + y*screen_w] = 1;
	    else if (c==2)
	       p2[x + y*screen_w] = p1[x + y*screen_w];
	    else
	       p2[x + y*screen_w] = 0;
	 }
      }

      c2 = 0;

      for (y=0; y<screen_h; y++) {
	 goto1(0, y);
	 for (x=0; x<screen_w; x++) {
	    if ((x<=0) || (y<=0) || (x>=screen_w-1) || (y>=screen_h-1))
	       c = 0;
	    else
	       c = p2[x-1 + (y-1)*screen_w] +
		   p2[x + (y-1)*screen_w] +
		   p2[x+1 + (y-1)*screen_w] +
		   p2[x-1 + y*screen_w] +
		   p2[x+1 + y*screen_w] +
		   p2[x-1 + (y+1)*screen_w] +
		   p2[x + (y+1)*screen_w] +
		   p2[x+1 + (y+1)*screen_w];

	    if (p2[x+y*screen_w]) {
	       c2++;
	       switch (c) {
		  case 0: tattr(8);  break;
		  case 1: tattr(1);  break;
		  case 2: tattr(9);  break;
		  case 3: tattr(2);  break;
		  case 4: tattr(10); break;
		  case 5: tattr(4);  break;
		  case 6: tattr(13); break;
		  case 7: tattr(14); break;
		  case 8: tattr(15); break;
	       } 
	    }
	    else
	       tattr(0);

	    pch('*');
	 }
      }

      if ((count > 0) && (c2 < 24)) {
	 count = -8;
	 clear = 4;
      }

      p4 = p1;
      p1 = p2;
      p2 = p3;
      p3 = p4;

      if ((memcmp(p1, p3, screen_w*screen_h) == 0) && (count > 0))
	 count = -8;
      else if ((memcmp(p1, p2, screen_w*screen_h) == 0) && (count > 0))
	 count = -8;

      for (c2=0; c2<10; c2++) {
	 delay(32);
	 if ((input_waiting()) || (mouse_changed(NULL, NULL)) ||
	     (alt_pressed()) || (ctrl_pressed()))
	    break;
      }

   } while ((!input_waiting()) && (!mouse_changed(NULL, NULL)) && 
	    (!alt_pressed()) && (!ctrl_pressed()));

   free(p1);
   free(p2);
   free(p3);

   clear_keybuf();
   show_c();
   n_vid();
}
