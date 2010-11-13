/* pMARS -- a portable Memory Array Redcode Simulator
 * Copyright (C) 1993-1996 Albert Ma, Na'ndor Sieben, Stefan Strack and Mintardjo Wangsawidjaja
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * grxdisp.c: DOS graphics mode (DJGPP/GRX) display
 * $Id: grxdisp.c,v 1.1.1.1 2000/08/20 13:29:36 iltzu Exp $
 *
 * graphics coreviewer should work on any display supported by go32
 * include this file into sim.c to replace the DISPLAY_? macros */

#include <grx.h>
#include <conio.h>
#include <mousex.h>

int     mouse_or_key(char *);
void    grclear(void);
void    bgi_clear_arena(void);

/* extern string */
extern char *pressAnyKey;

GrTextOption groption;
GrFont *grfont;
GR_graphics_modes graphdriver = GR_width_height_graphics;
int     graphmode = 0;
#define setcolor(color) groption.txo_fgcolor.v=(color)
#define outtextxy(x,y,s) GrDrawString(s,strlen(s),x,y,&groption)

#define WHITE GrWhite()
#define BLACK GrBlack()

/* to be implemented */
#define display_push(val)
#define display_die(warnum)

int     colors[MAXWARRIOR];        /* colors of the two warriors */
int     datcolors[MAXWARRIOR];        /* death colors */
int     clearColor;
int     writeColor;

#include "xgraphio.c"

#define MAXSTR 80
#define BORDER_WIDTH 3                /* arena border */
#define LEFT_UPPER_X 3                /* location of the arena */
#define LEFT_UPPER_Y 30                /* 20 */
#define CYCLE_Y 25 /* 15 */        /* location of the cycle meter */

int     cycle_Y = CYCLE_Y;        /* location of cycle meter */
int     splY[MAXWARRIOR];        /* location of the process meters */

int     VERTICAL_SIZE;                /* # of core locations in a line in the arena */
int     SIZE;                        /* size of one core location */
int     CYCLE_RATIO;
int     PROCESS_RATIO;

int     step;                        /* are we in single step mode ? */
int     i;
int     mouseOK;
int     mouseLocation;
int     newMouseLoc;
int     x, y;
int     xsize, ysize;
int     col;

extern int stepping;
extern ADDR_T curAddr;
extern int curPanel;

void    display_init();

void
        open_graphics(void);

int
xkoord(int addr)
{
  return
  (LEFT_UPPER_X + ((addr) % VERTICAL_SIZE) * (SIZE + 1));
}

int
ykoord(int addr)
{
  return
  (LEFT_UPPER_Y + ((addr) / VERTICAL_SIZE) * (SIZE + 1));
}
void
findplace(int addr)
{
  x = xkoord(addr);
  y = ykoord(addr);
  col = colors[W - warrior];
}

/*************************************************************************/

#define bgi_update_cycle_meter()\
 GrPlotNC(cycle / CYCLE_RATIO, CYCLE_Y, clearColor)

#define bgi_display_init() open_graphics()

#define bgi_display_read(addr)\
  GrPlotNC(xkoord(addr),ykoord(addr),colors[W-warrior])

#define bgi_display_dec(addr)\
do { \
  findplace(addr);\
  GrPlotNC(x  ,y,col);\
  GrPlotNC(x+1,y,col);  \
} while (0)

#define bgi_display_inc(addr)\
do { \
  findplace(addr);\
  GrPlotNC(x,y  ,col); \
  GrPlotNC(x,y+1,col); \
} while(0)

#define bgi_display_write(addr)\
do { \
  findplace(addr);\
  GrPlotNC(x+1,y,col);\
  GrPlotNC(x  ,y+1,col);\
} while(0)

#define bgi_display_exec(addr)\
do {\
    GrBoxNC(xkoord(addr),ykoord(addr),\
    xkoord(addr)+1,ykoord(addr)+1,colors[W-warrior]);\
} while(0)


#define bgi_display_spl(warrior,tasks) \
  GrPlotNC(tasks/PROCESS_RATIO,splY[warrior],colors[warrior])

#define bgi_display_dat(addr,warNum,tasks) \
  do {\
    if (displayLevel>0) {\
        GrBoxNC(xkoord(addr),ykoord(addr),\
        xkoord(addr)+1,ykoord(addr)+1,datcolors[warNum]);\
    }\
    GrPlotNC(tasks/PROCESS_RATIO,splY[warNum],clearColor);\
  } while(0)

void
bgi_clear_arena()
{
  GrFilledBox(
              LEFT_UPPER_X - BORDER_WIDTH + 1,
              LEFT_UPPER_Y - BORDER_WIDTH + 1,
              xkoord(VERTICAL_SIZE - 1) + BORDER_WIDTH - 1,
              ykoord(coreSize) + BORDER_WIDTH - 1, GrBlack());
}

void
bgi_display_clear()
{
  int     dummy;
  bgi_clear_arena();
  for (dummy = 0; dummy < warriors; dummy++) {
    GrLine(0, splY[dummy], GrMaxX(), splY[dummy], clearColor);
    GrPlot(1 + taskNum / PROCESS_RATIO, splY[dummy], colors[dummy]);
    GrLine(1, splY[dummy], warrior[dummy].tasks / PROCESS_RATIO,
           splY[dummy], colors[dummy]);
  }
  GrLine(1, CYCLE_Y, GrMaxX(), CYCLE_Y, clearColor);
  GrLine(1, CYCLE_Y, cycle / CYCLE_RATIO - 1, CYCLE_Y, writeColor);
}

void
bgi_display_close(int wait)
{
  MouseUnInit();
  if (wait == WAIT && displayLevel) {
    grputs(pressAnyKey);
    if (inputRedirection)
      while (!kbhit());
    else
      getch();
  }
  textmode(LASTMODE);
  displayMode = TEXT;                /* to display results in textmode */
#ifdef DOSTXTGRAPHX
  ScreenUpdate(Screen[DEF_PAGE]);        /* restore text screen */
  show_cursor();
  gotoxy(1, defPageY);
#endif
}

/**************************************************************************/
/* this function is called by grgets() in xgraphio to handle mouse events */

int
mouse_or_key(char *result)
{
  int     x, y, hi, lo;
  MouseEvent evt;
  MouseWarp(xkoord(curAddr) + 1, ykoord(curAddr) + 1);
  MouseDisplayCursor();
  while (!kbhit()) {
    if (mouseOK == mouseOK) {        /* check if there is a mouse on the system */
      MouseGetEvent(M_BUTTON_DOWN | M_POLL, &evt);
      if (evt.flags & M_BUTTON_DOWN) {
        x = evt.x - LEFT_UPPER_X + (SIZE) / 2;
        y = evt.y - LEFT_UPPER_Y + (SIZE) / 2;
        newMouseLoc = x / (SIZE + 1) + (y / (SIZE + 1)) * VERTICAL_SIZE;
        if ((newMouseLoc >= 0) && (newMouseLoc < coreSize)) {
          curAddr = newMouseLoc;
          if (evt.flags & M_LEFT_DOWN)
            strcpy(result, " m mousel\n");
          else if (evt.flags & M_MIDDLE_DOWN)
            strcpy(result, " m mousem\n");
          else if (evt.flags & M_RIGHT_DOWN)
            strcpy(result, " m mouser\n");
          MouseEraseCursor();
          return 1;
        }
      }
    }
  }
  MouseEraseCursor();
  return 0;
}


#define draw_border do {            \
  GrBox(                                          \
  LEFT_UPPER_X-BORDER_WIDTH,                          \
  LEFT_UPPER_Y-BORDER_WIDTH,                          \
  xkoord(VERTICAL_SIZE-1)+BORDER_WIDTH,               \
  ykoord(coreSize)+BORDER_WIDTH,writeColor ); } while (0)

void
write_menu(void)
{
  int     y;
  int     i, j;
#define BUTTON RED
  char    s[7];
  y = ykoord(coreSize) + BORDER_WIDTH + 2;
  setcolor(writeColor);
  outtextxy(10, y, "<");
  setcolor(BUTTON);
  s[0] = CLEAR;
  s[1] = 0;
  for (i = 0; i < SPEEDLEVELS - displaySpeed; i++)
    outtextxy(20 + i * 10, y, s);
  setcolor(YELLOW);
  for (j = 0; j < displaySpeed; j++)
    outtextxy(20 + i * 10 + j * 10, y, s);
  setcolor(writeColor);
  outtextxy(20 + i * 10 + j * 10, y, "> ");
  for (i = 0; i < 5; i++) {
    sprintf(s, "%d ", i);
    if (displayLevel == i)
      setcolor(BUTTON);
    else
      setcolor(writeColor);
    outtextxy(170 + i * 10, y, s);
  }
  if (inCdb)
    setcolor(BUTTON);
  else
    setcolor(writeColor);
  outtextxy(260, y, "Debug ");
  setcolor(writeColor);
  if (xsize > 320) {
    if (inCdb) {
      setcolor(clearColor);
    }
#if defined(DOSALLGRAPHX)
    outtextxy(310, y, "space textVideo Quit");
#else
    outtextxy(310, y, "space Quit");
#endif
  }
  setcolor(writeColor);
}

void
write_names(void)
{
  GrFilledBox(0, 0, 30, 10, GrBlack());        /* clear unwanted ctrl-c */
  if (warriors <= 2) {
    setcolor(colors[0]);
    outtextxy(1, 0, warrior[0].name);
    if (warriors == 2) {
      setcolor(colors[1]);
      outtextxy(140, 0, warrior[1].name);
    }
  }
}

void
open_graphics(void)
{
  int     i;

#ifdef DOSTXTGRAPHX
  if (!Screen[DEF_PAGE])
    text_display_init();        /* to save DOS text screen for later restore */
#endif

  switch (displayMode) {
  case 1:
    xsize = 640;
    ysize = 480;
    break;
  case 2:
    xsize = 800;
    ysize = 600;
    break;
  case 3:
    xsize = 1024;
    ysize = 768;
    break;
  case 4:
    xsize = 640;
    ysize = 200;
    break;
  case 5:
    xsize = 640;
    ysize = 350;
    break;
  case 6:
    xsize = 320;
    ysize = 200;
    break;
  default:
    xsize = 640;
    ysize = 480;
    break;
  }

  GrSetMode(graphdriver, xsize, ysize);

  grfont = GrLoadFont("@:pc8x8.fnt");

  memset(&groption, 0, sizeof(groption));
  groption.txo_font = grfont;
  groption.txo_xalign = GR_ALIGN_LEFT;
  groption.txo_yalign = GR_ALIGN_TOP;
  groption.txo_xmag = 1;
  groption.txo_ymag = 1;
  groption.txo_direct = GR_TEXT_RIGHT;
  groption.txo_fgcolor.v = GrWhite();
  groption.txo_bgcolor.v = GrBlack();

  clearColor = BLACK;
  writeColor = WHITE;

  SIZE = 4;                        /* Size of a given location, feel free to
                                 * make it bigger */
  do {
    --SIZE;                        /* decrease the size to fit */
    VERTICAL_SIZE = (GrMaxX() - 2 * LEFT_UPPER_X) / (SIZE + 1);
  } while ((ykoord(coreSize) > GrMaxY() - 20) && SIZE > 0);

  if ((CYCLE_RATIO = 2 * warriors * cycles / GrMaxY()) == 0)
    CYCLE_RATIO = 1;
  if ((PROCESS_RATIO = taskNum / GrMaxY()) == 0)
    PROCESS_RATIO = 1;

  draw_border;

  setcolor(writeColor);
  write_menu();
  bgi_display_clear();

/* initializing corelist variables and mouse */

  step = 0;
  MouseInit();
  mouseLocation = -1;
  newMouseLoc = 0;

/* initializing graphio parameters */

  grwindy0 = ykoord(coreSize) + 2 * BORDER_WIDTH + 6;
  grwindy1 = GrMaxY() - verspace;

  curPanel = -1;                /* one panel only */
  grupdate(0);
  graphio_init();
  grclear();

/* initializing meter locations and colors */

  if (warriors <= 2) {
    splY[0] = 9;
    splY[1] = 11;
    colors[0] = GREEN;
    colors[1] = LIGHTRED;
    datcolors[0] = GREEN + 1;
    datcolors[1] = LIGHTRED + 1;
  } else {
    for (i = 0; i < 21; i++) {
      if (warriors <= 10) {
        splY[i] = 2 * i + 1;
      } else {
        splY[i] = i + 1;
      }
    }
    for (i = 0; i < MAXWARRIOR; i++) {
      datcolors[i] = colors[i] = i % 15 + 1;
    }
  }

  write_names();

}

#if ! defined(DOSTXTGRAPHX)
#define display_init()      bgi_display_init()
#define display_clear()     bgi_display_clear()
#define display_read(addr) do { if (displayLevel > 3)\
  bgi_display_read(addr); } while(0)
#define display_write(addr) do { if (displayLevel > 1)\
  bgi_display_write(addr); } while(0)
#define display_dec(addr) do { if (displayLevel > 2)\
  bgi_display_dec(addr); } while (0)
#define display_inc(addr)  do { if (displayLevel > 2)\
  bgi_display_inc(addr); } while(0)
#define display_exec(addr) do { if (displayLevel > 0)\
  bgi_display_exec(addr); } while(0)
#define display_spl(warrior,tasks) bgi_display_spl(warrior,tasks)
#define display_dat(addr,warnum,tasks) bgi_display_dat(addr,warnum,tasks)
#define display_close()     bgi_display_close(WAIT)
#endif
