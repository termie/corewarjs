/* pMARS -- a portable Memory Array Redcode Simulator
 * Copyright (C) 1993-1995 Albert Ma, Na'ndor Sieben, Stefan Strack, Mintardjo Wangsawidjaja and Martin Maierhofer
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
 * lnxdisp.c: user interface for Linux svga graphics displays
 *            mostly stolen from grxdisp.c, uidisp.c, xgraphio.c
 * $Id: lnxdisp.c,v 1.1.1.1 2000/08/20 13:29:38 iltzu Exp $
 */

#ifdef LINUXGRAPHX
#include "asm.h"
#include "lnxdisp.h"

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>

#include <vgamouse.h>

#define BLACK 		0	/* some hardcoded color constants */
#define BLUE		1
#define GREEN		2
#define CYAN		3
#define RED			4
#define LIGHTRED	12
#define YELLOW		14
#define WHITE 		15

#define BORDER_WIDTH	3	/* arena border */
#define LEFT_UPPER_X	3	/* location of the arena */
#define LEFT_UPPER_Y	30
#define CYCLE_Y		25	/* location of the cycle meter */

#define CLEAR			219	/* character for overwriting others */

#define ESC 	0x1b		/* some keyboard constants */
#define BS 		0x08
#define CR 		0x0a

#define ALT_A	150		/* keys with ALT pressed */
#define ALT_Z	175

#define F1		200	/* function keys */
#define F2		201
#define F3		202
#define F4		203
#define F5		204
#define F6		205
#define F7		206
#define F8		207
#define F9		208
#define F10		209
#define F11		210
#define F12		211

#define UP		220	/* cursor keys */
#define DOWN	221
#define RIGHT	222
#define LEFT	223

#define HOME	231		/* 'special' keys */
#define INS		232
#define DEL		233
#define END		234
#define PGUP	235
#define PGDN	236


/* static variable definitions */
static int      verticalSize;	/* # core locations in a line in the arena */
static int      size;		/* size of one core location */

static int      cycleRatio;	/* ratio of pixel of cycle meter to cycle */
static int      processRatio;	/* ratio pixel of process meter to process */

static int      mouseOK;	/* has the mouse initalization succeeded ? */

static int      posx, posy;	/* cursor position of current panel */
static int      posx1, posy1;	/* same for for panel 1 */
static int      posx2, posy2;	/* same for for panel 2 */

static int      point;		/* # input characters in panel */

static int      grwindx0, grwindy0;	/* position of panel 1 */
static int      grwindx1, grwindy1;	/* same for panel 2 */

static int      horizspace = 8;	/* horizontal/vertical extension of one */
static int      verspace = 8;	/* character */

static char     str[2] = " ";	/* needed for some output */

static int      writeColor;	/* normal output color, ie. WHITE */
static int      graphioColor;	/* current output color */
static int      curColor = 0;	/* the color used for drawing & text */

static void    *font, *blackFont;	/* colored font and black font */
static int      last_font_color;/* the color used lateley in text output */

/* variables used externally as well */
int             xDim, yDim;	/* size of the display */

int             colors[MAXWARRIOR];	/* colors of the two warriors */
int             datcolors[MAXWARRIOR];	/* death colors */
int             clearColor;	/* color used to erase, ie. BLACK */

int             splY[MAXWARRIOR];	/* location of the process meters */

int             x, y;		/* location and color of the currently */
int             col;		/* accessed core address */

int             svgaTextLines;	/* the number of text lines of cdb */

int             console_fd;	/* file descriptor for /dev/console */

struct termios  tio_orig;	/* the original terminal settings */

/* variables defined externally */
extern int      curPanel;	/* number of the panel in use */
extern ADDR_T   curAddr;	/* the current core address */

/* imported from str_???.c, these are varios error/status messages */
extern char    *pressAnyKey;
extern char    *cantReadConsole;
extern char    *selectFails;
extern char    *tcsetattrFails;
extern char    *modeNotAvail;
extern char    *tryingNext;
extern char    *noModes;
extern char    *noMouse;
extern char    *cantSetMode;

/* function prototypes */
#ifdef NEW_STYLE
static void     draw_mouse_cursor(int show, int newx, int newy);
static void     svga_outtextxy(int x, int y, char *s);
static int      conv_chr(char *buf, int *ndx, int maxndx);
static char     keypressed(int read_flag);
static void     graphio_init(void);
static void     newline(void);
static void     newchar(void);
static void     delchar(void);
static int      mouse_or_key(char *result);
static char    *special_keys(int key, char *buf);
static void     write_names(void);
#else
static void     draw_mouse_cursor();
static void     svga_outtextxy();
static int      conv_chr();
static char     keypressed();
static void     graphio_init();
static void     newline();
static void     newchar();
static void     delchar();
static int      mouse_or_key();
static char    *special_keys();
static void     write_names();
#endif

/* some convenient macros */
#define setcolor(color) vga_setcolor(curColor = color)

#define cursoron() svga_outtextxy(posx, posy, "_")
#define cursoroff() setcolor(BLACK); \
	str[0]=CLEAR; \
	svga_outtextxy(posx, posy, str); \
	setcolor(graphioColor)

#define draw_border do { \
  gl_rect( \
  LEFT_UPPER_X-BORDER_WIDTH, \
  LEFT_UPPER_Y-BORDER_WIDTH, \
  xkoord(verticalSize-1)+BORDER_WIDTH, \
  ykoord(coreSize)+BORDER_WIDTH, \
  writeColor ); } while (0)


/**********************************************************************/
/*   misc & util functions															 */
/**********************************************************************/

/*
 * adjust the position variables for a new panel
 */
void 
svga_update(newcurPanel)
  int             newcurPanel;
{
  if (curPanel == newcurPanel)
    return;
  if (curPanel == 0 && newcurPanel != 0) {
    posx1 = posx;
    posy1 = posy;
    grwindx0 = xDim / 2;
    grwindx1 = xDim - horizspace;
    svga_clear();
    posx2 = posx;
    posy2 = posy;
    curPanel = 2;
  }
  switch (newcurPanel) {
  case 0:			/* only one panel */
    if (curPanel == 1) {
      posx1 = posx;
      posy1 = posy;
    }
    grwindx0 = xDim / 2;
    grwindx1 = xDim - horizspace;
    svga_clear();
    grwindx0 = 1;
    grwindx1 = xDim - horizspace;
    posx = posx1;
    posy = posy1;
    break;
  case 1:			/* first panel */
    grwindx0 = 1;
    grwindx1 = xDim / 2 - horizspace;
    if (curPanel == 2) {
      posx2 = posx;
      posy2 = posy;
      posx = posx1;
      posy = posy1;
    }
    break;
  case 2:			/* second panel */
    grwindx0 = xDim / 2;
    grwindx1 = xDim - 10;
    switch (curPanel) {
    case 1:
      posx1 = posx;
      posy1 = posy;
      posx = posx2;
      posy = posy2;
      break;
    case 0:
      posx1 = posx;
      posy1 = posy;
      svga_clear();
    }
    break;
  }
  curPanel = newcurPanel;
}

/*
 * initialize some graphics variables
 */
static void 
graphio_init(void)
{
  verspace = 10;
  grwindy0 += 3;
  svgaTextLines = (yDim - grwindy0) / verspace;	/* used for pausing */
}

/*
 * return the x coordinate of the given core address
 */
int 
xkoord(addr)
  int             addr;
{
  return (LEFT_UPPER_X + ((addr) % verticalSize) * (size + 1));
}

/*
 * return the y coordinate of the given core address
 */
int 
ykoord(addr)
  int             addr;
{
  return (LEFT_UPPER_Y + ((addr) / verticalSize) * (size + 1));
}

/*
 * record the coordinates and the color of the given core address
 */
void 
findplace(addr)
  int             addr;
{
  x = xkoord(addr);
  y = ykoord(addr);
  col = colors[W - warrior];
}

/**********************************************************************/
/*   string output functions														 */
/**********************************************************************/

/*
 * print a newline
 */
static void 
newline()
{
  posx = grwindx0;
  posy += verspace;
  if (posy > grwindy1) {
    int             width = grwindx1 - grwindx0;
    int             height = yDim - grwindy0;

    posy -= verspace;
    gl_copybox(grwindx0, grwindy0 + verspace, width, height - verspace,
	       grwindx0, grwindy0);
    gl_fillbox(grwindx0, posy, width, yDim - posy, BLACK);
  }
}

/*
 * adjust the cursor after printing a new character
 */
static void 
newchar()
{
  posx += horizspace;
  if (posx > grwindx1 - horizspace)
    newline();
}

/*
 * delete a character by overwriting it with black
 */
static void 
delchar()
{
  if (point) {
    cursoroff();
    posx -= horizspace;
    if (posx < grwindx0) {
      posx = grwindx0;
      cursoron();
    } else {
      --point;
      cursoroff();
      cursoron();
    }
  }
}

/*
 * draw the given text at the current address
 */
void 
svga_puts(sss)
  char           *sss;
{
  if (printAttr) {
    graphioColor = colors[printAttr - 1];
    setcolor(graphioColor);
  } else {
    graphioColor = WHITE;
    setcolor(graphioColor);
  }

  while (*sss) {
    switch (*sss) {
    case '\n':
      newline();
      break;
    default:
      str[0] = *sss;
      svga_outtextxy(posx, posy, str);
      newchar();
    }
    ++sss;
  }
}

/*
 * write the menu line
 */
void 
svga_write_menu()
{
  int             y, i, j;
  char            s[7];

  y = ykoord(coreSize) + BORDER_WIDTH + 2;

  setcolor(writeColor);
  svga_outtextxy(10, y, "<");

  s[0] = CLEAR;
  s[1] = '\0';

  setcolor(RED);
  for (i = 0; i < SPEEDLEVELS - displaySpeed; i++)
    svga_outtextxy(20 + i * 10, y, s);

  setcolor(YELLOW);
  for (j = 0; j < displaySpeed; j++)
    svga_outtextxy(20 + i * 10 + j * 10, y, s);

  setcolor(writeColor);
  svga_outtextxy(20 + i * 10 + j * 10, y, "> ");

  for (i = 0; i < 5; i++) {
    sprintf(s, "%d ", i);
    if (displayLevel == i)
      setcolor(RED);
    else
      setcolor(writeColor);
    svga_outtextxy(170 + i * 10, y, s);
  }

  if (inCdb)
    setcolor(RED);
  else
    setcolor(writeColor);
  svga_outtextxy(260, y, "Debug ");

  if (xDim > 320) {		/* this is too simple */
    if (inCdb)
      setcolor(clearColor);
    else
      setcolor(writeColor);
    svga_outtextxy(310, y, "space Quit");
  }
  setcolor(writeColor);
}

/*
 * display the names of the warriors
 */
void 
write_names()
{
  if (warriors <= 2) {
    setcolor(colors[0]);
    svga_outtextxy(1, 0, warrior[0].name);

    if (warriors == 2) {
      setcolor(colors[1]);
      svga_outtextxy(140, 0, warrior[1].name);
    }
  }
}

/**********************************************************************/
/*   keyboard and mouse handling routines										 */
/**********************************************************************/

/*
 * redraw the mouse cursor (if show is false, hide it) at the new
 * location (newx/newy)
 */
void 
draw_mouse_cursor(show, newx, newy)
  int             show, newx, newy;
{
  static int      state = 0;	/* currently showing ? */
  static int      xpos, ypos;	/* current mouse position */
  static char     data[100];	/* saved screen image */

  if (state && !show) {
    gl_putbox(xpos - 1, ypos - 1, 3, 3, data);
    state = 0;
    xpos = ypos = -1;		/* force a redraw the next time */
    return;
  }
  if (state && (newx != xpos || newy != ypos))
    gl_putbox(xpos - 1, ypos - 1, 3, 3, data);
  state = show;
  if (!state || (newx == xpos && newy == ypos))
    return;
  xpos = newx;
  ypos = newy;
  gl_getbox(xpos - 1, ypos - 1, 3, 3, data);
  gl_fillbox(xpos - 1, ypos - 1, 3, 3, WHITE);
}

/*
 * convert a sequence of ASCII characters contained in buf into
 * a 'key' (eg. 'A' or F12	or PGUP) -- this is ugly code at its best
 * -- and I mean it ;-)
 */
static int 
conv_chr(buf, ndx, maxndx)
  char           *buf;
  int            *ndx;
  int             maxndx;
{
  int             i = *ndx;
  int             ondx = *ndx;

  if (*ndx >= maxndx)
    return 0;
  *ndx += 1;
  if (buf[i++] != ESC)
    return buf[ondx];

  if (i < maxndx) {
    if (buf[i++] != '[') {	/* must be Alt-a to Alt-z */
      if (buf[i - 1] >= 'a' && buf[i - 1] <= 'z') {
	*ndx = i;
	return ALT_A + buf[i - 1] - 'a';
      } else
	return buf[ondx];
    }
    if (i >= maxndx)
      return buf[ondx];
    if (buf[i++] == '[') {	/* must be F1 through F5 */
      if (i >= maxndx)
	return buf[ondx];
      *ndx = i + 1;
      switch (buf[i]) {
      case 'A':
	return F1;
      case 'B':
	return F2;
      case 'C':
	return F3;
      case 'D':
	return F4;
      case 'E':
	return F5;
      default:
	return 0;
      }
    } else if (buf[i - 1] >= '1' && buf[i - 1] <= '6') {	/* must be HOME etc */
      if (i >= maxndx)
	return buf[ondx];
      if (buf[i] != '~') {	/* could as well be F6 to F12 */
	if (i + 1 >= maxndx)
	  return buf[ondx];
	if (buf[i + 1] != '~')
	  return buf[ondx];
	if (buf[i - 1] == '1') {
	  *ndx = i + 2;
	  switch (buf[i]) {
	  case '7':
	    return F6;
	  case '8':
	    return F7;
	  case '9':
	    return F8;
	  default:
	    return 0;
	  }
	} else if (buf[i - 1] == '2') {
	  *ndx = i + 2;
	  switch (buf[i]) {
	  case '0':
	    return F9;
	  case '1':
	    return F10;
	  case '3':
	    return F11;
	  case '4':
	    return F12;
	  default:
	    return 0;
	  }
	} else
	  return buf[ondx];
      }
      *ndx = i-- + 1;
      switch (buf[i]) {
      case '1':
	return HOME;
      case '2':
	return INS;
      case '3':
	return DEL;
      case '4':
	return END;
      case '5':
	return PGUP;
      case '6':
	return PGDN;
      default:
	return 0;
      }
    } else {			/* must be CURSOR keys */
      *ndx = i--;
      switch (buf[i]) {
      case 'A':
	return UP;
      case 'B':
	return DOWN;
      case 'C':
	return RIGHT;
      case 'D':
	return LEFT;
      default:
	return 0;
      }
    }
  }
  return buf[ondx];
}

/*
 * wait for a key and return its symbolic representation (eg. 'A', F12, ..)
 */
int 
svga_getch()
{
  int             x, max = 0;
  char            buffer[20];
  int             ndx = 0;

  if (ndx < max)
    return conv_chr(buffer, &ndx, max);

  for (;;) {
    if ((x = read(console_fd, buffer, 20)) == -1) {
      if (errno == EINTR)
	continue;
      perror(cantReadConsole);
      Exit(1);
    }
    break;
  }
  max = x;
  ndx = 0;
  return conv_chr(buffer, &ndx, max);
}

/*
 * check if a key has been pressed and return it if read_flag is set,
 * otherwise leave it unread. Return 0 if no key has been pressed.
 */
static char 
keypressed(read_flag)
  int             read_flag;
{
  fd_set          set;
  struct timeval  timeout;
  int             x;
  char            buf[10];

  FD_ZERO(&set);
  FD_SET(console_fd, &set);
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  for (;;) {
    if ((x = select(FD_SETSIZE, &set, NULL, NULL, &timeout)) == -1) {
      if (errno == EINTR)
	continue;
      perror(selectFails);
      Exit(1);
    }
    break;
  }

  if (x == 0)
    return 0;
  if (!read_flag)
    return -1;
  if (read(console_fd, buf, 1) == -1) {
    perror(cantReadConsole);
    Exit(1);
  }
  return buf[0];
}

/*
 * check for mouse and key events, return 1 and set result if a mouse
 * button gets pressed, return 0 if a keyboard event occurs
 */
static int 
mouse_or_key(result)
  char           *result;
{
  int             x, y, mx, my, button;
  int             newLoc;
  static int      lastbutton;

  if (!mouseOK)
    return 0;

  mouse_setposition(xkoord(curAddr) + 1, ykoord(curAddr) + 1);
  draw_mouse_cursor(1, xkoord(curAddr) + 1, ykoord(curAddr) + 1);

  while (!keypressed(0)) {
    mouse_update();
    button = mouse_getbutton();
    mx = mouse_getx();
    my = mouse_gety();
    draw_mouse_cursor(1, mx, my);

    if (button != lastbutton) {
      lastbutton = button;
      if (button == 0)
	continue;
      x = mx - LEFT_UPPER_X + (size) / 2;
      y = my - LEFT_UPPER_Y + (size) / 2;
      newLoc = x / (size + 1) + (y / (size + 1)) * verticalSize;
      if ((newLoc >= 0) && (newLoc < coreSize)) {
	curAddr = newLoc;
	if (button & MOUSE_LEFTBUTTON)
	  strcpy(result, " m mousel\n");
	else if (button & MOUSE_MIDDLEBUTTON)
	  strcpy(result, " m mousem\n");
	else if (button & MOUSE_RIGHTBUTTON)
	  strcpy(result, " m mouser\n");
	draw_mouse_cursor(0, 0, 0);
	return 1;
      }
    }
  }
  draw_mouse_cursor(0, 0, 0);
  return 0;
}

/*
 * handle 'special' keys (ie. function/control/alt keys)
 */
static char    *
special_keys(key, buf)
  int             key;
  char           *buf;
{
  if (key < 32) {
    sprintf(buf, " m ctrl-%c\n", key + 96);
    return buf;
  }
  if (key >= ALT_A && key <= ALT_Z) {
    sprintf(buf, " m alt-%c\n", key + 'a' - ALT_A);
    return buf;
  }
  switch (key) {
  case INS:
    strcpy(buf, " m ins\n");
    break;
  case DEL:
    strcpy(buf, " m del\n");
    break;
  case UP:
    strcpy(buf, " m up\n");
    break;
  case DOWN:
    strcpy(buf, " m down\n");
    break;
  case LEFT:
    strcpy(buf, " m left\n");
    break;
  case RIGHT:
    strcpy(buf, " m right\n");
    break;
  case PGUP:
    strcpy(buf, " m pgup\n");
    break;
  case PGDN:
    strcpy(buf, " m pgdn\n");
    break;
  case HOME:
    strcpy(buf, " m home\n");
    break;
  case END:
    strcpy(buf, " m end\n");
    break;
  case F1:
    strcpy(buf, " m f1\n");
    break;
  case F2:
    strcpy(buf, " m f2\n");
    break;
  case F3:
    strcpy(buf, " m f3\n");
    break;
  case F4:
    strcpy(buf, " m f4\n");
    break;
  case F5:
    strcpy(buf, " m f5\n");
    break;
  case F6:
    strcpy(buf, " m f6\n");
    break;
  case F7:
    strcpy(buf, " m f7\n");
    break;
  case F8:
    strcpy(buf, " m f8\n");
    break;
  case F9:
    strcpy(buf, " m f9\n");
    break;
  case F10:
    strcpy(buf, " m f10\n");
    break;
  case F11:
    strcpy(buf, " m f11\n");
    break;
  case F12:
    strcpy(buf, " m f12\n");
    break;
  default:
    buf[0] = '\n';
    buf[1] = '\0';
    break;
  }
  return buf;
}

/*
 * read a line from the keyboard
 */
char           *
svga_gets(result, maxchar)
  char           *result;
  int             maxchar;
{
  if (inputRedirection) {
    return fgets(result, maxchar, stdin);
  } else {
    int             lo;
    result[0] = 0;
    point = 0;
    cursoron();

    do {
      if (mouse_or_key(result))
	return result;
      lo = svga_getch();
      if (lo) {
	switch (lo) {
	case BS:
	  delchar();
	  break;
	case ESC:
	  while (point)
	    delchar();
	  break;
	case CR:
	  cursoroff();
	  newline();
	  break;
	default:
	  if (lo >= 32 && lo < 128) {
	    result[point++] = lo;
	    if (point > maxchar - 1)
	      lo = CR;
	    cursoroff();
	    str[0] = lo;
	    svga_outtextxy(posx, posy, str);
	    newchar();
	    cursoron();
	  } else {
	    special_keys(lo, result);
	    cursoroff();
	    svga_puts(result);
	    return result;
	  }
	}
      }
    } while (lo != CR);
    result[point++] = '\n';
    result[point] = 0;
    if (point)
      return result;
    else
      return NULL;
  }				/* !inputRedirection */
}

/**********************************************************************/
/*   graphics utility functions													 */
/**********************************************************************/

/*
 * draw a rectangle given the two opposite points
 */
void 
gl_rect(x, y, xx, yy, c)
  int             x, y, xx, yy, c;
{
  gl_hline(x, y, xx, c);
  gl_hline(x, yy, xx, c);
  gl_line(x, y, x, yy, c);
  gl_line(xx, y, xx, yy, c);
}

/*
 * draw specified text at location (x,y) - being the upper left corner
 * of the text
 */
static void 
svga_outtextxy(x, y, s)
  int             x, y;
  char           *s;
{
  if (curColor == BLACK) {
    gl_setfont(8, 8, blackFont);
    gl_write(x, y, s);
    gl_setfont(8, 8, font);
    return;
  }
  if (curColor != last_font_color) {
    gl_colorfont(8, 8, curColor, font);
    gl_setfont(8, 8, font);
    last_font_color = curColor;
  }
  gl_write(x, y, s);
}

/*
 * clear the current panel by filling it with black and reset the
 * cursor position.
 */
void 
svga_clear()
{
  posx = grwindx0;
  posy = grwindy0;

  gl_fillbox(grwindx0, grwindy0, grwindx1 + horizspace - grwindx0,
	     yDim - grwindy0, BLACK);
}

/*
 * clear the arena
 */
void 
svga_clear_arena()
{
  int             x = LEFT_UPPER_X - BORDER_WIDTH + 1;
  int             y = LEFT_UPPER_Y - BORDER_WIDTH + 1;

  gl_fillbox(x, y, xkoord(verticalSize - 1) + BORDER_WIDTH - 1 - x,
	     ykoord(coreSize) + BORDER_WIDTH - 1 - y, BLACK);
}

/*
 * clear the arena and display the process meters and cycle meters
 */
void 
svga_display_clear()
{
  int             i;

  svga_clear_arena();
  for (i = 0; i < warriors; i++) {
    gl_line(0, splY[i], xDim, splY[i], clearColor);
    gl_setpixel(1 + taskNum / processRatio, splY[i], colors[i]);
    gl_line(1, splY[i], warrior[i].tasks / processRatio, splY[i], colors[i]);
  }
  gl_line(1, CYCLE_Y, xDim, CYCLE_Y, clearColor);
  gl_line(1, CYCLE_Y, cycle / cycleRatio, CYCLE_Y, writeColor);
}

/*
 * one more cycle has passed, update the display and check for keyboard
 * input
 */
void 
svga_display_cycle()
{
  int             ch;

#ifdef HAVE_USLEEP
  if (loopDelay > 1)
    usleep(loopDelay);
#else
  unsigned long   ctr = loopDelay;
  while (ctr--);
#endif

  if (!(cycle & keyDelay)) {
    gl_setpixel(cycle / cycleRatio, CYCLE_Y, clearColor);

    if (!inputRedirection && ((ch = keypressed(1)) != 0)) {
      switch (ch) {
      case '0':
	displayLevel = 0;
	break;
      case '1':
	displayLevel = 1;
	break;
      case '2':
	displayLevel = 2;
	break;
      case '3':
	displayLevel = 3;
	break;
      case '4':
	displayLevel = 4;
	break;

      case 'd':
	debugState = STEP;	/* stepping = FALSE; */
	break;

      case '>':
	if (displaySpeed > 0) {
	  --displaySpeed;
	  loopDelay = loopDelayAr[displaySpeed];
	  keyDelay = keyDelayAr[displaySpeed];
	}
	break;
      case '<':
	if (displaySpeed < SPEEDLEVELS - 1) {
	  ++displaySpeed;
	  loopDelay = loopDelayAr[displaySpeed];
	  keyDelay = keyDelayAr[displaySpeed];
	}
	break;

      case ' ':
      case 'r':
	svga_clear_arena();
	break;

      case 27:
      case 'q':
	display_close();
	Exit(USERABORT);

      default:
	debugState = STEP;
	break;
      }
      svga_write_menu();
    }
  }
}

/**********************************************************************/
/*   initializing and closing the display										 */
/**********************************************************************/

/*
 * close the display
 */
void 
svga_display_close(wait)
  int             wait;
{
  if (wait == WAIT) {
    svga_puts(pressAnyKey);
    svga_getch();
  }
  mouse_close();
  vga_setmode(TEXT);
  if (tcsetattr(console_fd, TCSANOW, &tio_orig) == -1) {
    perror(tcsetattrFails);
    Exit(1);
  }
  free(font);
  free(blackFont);
}

/*
 * try to open the graphicmode, save the current terminal settings for
 * later restoration, initialize variables
 */
void 
svga_open_graphics()
{
  int             i, gMode;
  int             xsize, ysize;
  struct termios  tionew;

  switch (displayMode) {
  case 1:
    xsize = 640;
    ysize = 480;
    gMode = G640x480x256;
    break;
  case 2:
    xsize = 800;
    ysize = 600;
    gMode = G800x600x256;
    break;
  case 3:
    xsize = 1024;
    ysize = 768;
    gMode = G1024x768x256;
    break;
/*		case 4:  xsize=640; ysize=200; gMode=G640x200x16; break;
		case 5:  xsize=640; ysize=350; gMode=G640x350x16; break; */
  case 6:
    xsize = 320;
    ysize = 200;
    gMode = G320x200x256;
    break;
  default:
    xsize = 640;
    ysize = 480;
    gMode = G640x480x256;
    break;
  }

  if (!vga_hasmode(gMode)) {
    fprintf(stderr, modeNotAvail, xsize, ysize);
    gMode = G640x480x256;
    xsize = 640;
    ysize = 480;
    if (!vga_hasmode(gMode)) {
      fprintf(stderr, tryingNext);
      gMode = G320x200x256;
      xsize = 320;
      ysize = 200;
      if (!vga_hasmode(gMode)) {
	fprintf(stderr, noModes);
	exit(1);
      }
    }
    fprintf(stderr, "\n");
  }
  tionew = tio_orig;
  tionew.c_lflag &= ~(ICANON | ECHO);
  tionew.c_lflag |= ISIG;
  tionew.c_cc[VMIN] = 1;
  if (tcsetattr(console_fd, TCSAFLUSH, &tionew) == -1) {
    perror(tcsetattrFails);
    exit(1);
  }
  if (mouse_init("/dev/mouse", vga_getmousetype(),
		 MOUSE_DEFAULTSAMPLERATE) == -1) {
    fprintf(stderr, noMouse, strerror(errno));
    mouseOK = 0;
  } else
    mouseOK = 1;

  if (vga_setmode(gMode) == -1) {
    fprintf(stderr, cantSetMode, xsize, ysize);
    Exit(1);
  }
  gl_setcontextvga(gMode);
  gl_enableclipping();

  font = malloc(256 * 8 * 8 * BYTESPERPIXEL);
  gl_expandfont(8, 8, WHITE, gl_font8x8, font);
  gl_setfont(8, 8, font);
  last_font_color = WHITE;

  blackFont = malloc(256 * 8 * 8 * BYTESPERPIXEL);
  gl_expandfont(8, 8, BLACK, gl_font8x8, blackFont);

  xDim = vga_getxdim() - 1;
  yDim = vga_getydim() - 1;

  if (mouseOK) {
    mouse_setxrange(0, WIDTH - 1);
    mouse_setyrange(0, HEIGHT - 1);
    mouse_setwrap(MOUSE_NOWRAP);
  }
  clearColor = BLACK;
  writeColor = WHITE;

  size = 4;			/* Size of a given location, feel free to
				 * make it bigger */
  do {
    --size;			/* decrease the size to fit */
    verticalSize = (xDim - 2 * LEFT_UPPER_X) / (size + 1);
  } while ((ykoord(coreSize) > yDim - 20) && size > 0);

  if ((cycleRatio = 2 * warriors * cycles / yDim) == 0)
    cycleRatio = 1;
  if ((processRatio = taskNum / yDim) == 0)
    processRatio = 1;

  /* initializing graphio parameters */
  grwindy0 = ykoord(coreSize) + 2 * BORDER_WIDTH + 6;
  grwindy1 = yDim - verspace;
  graphio_init();

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
      if (warriors <= 10)
	splY[i] = 2 * i + 1;
      else
	splY[i] = i + 1;
    }
    for (i = 0; i < MAXWARRIOR; i++) {
      datcolors[i] = colors[i] = i % 15 + 1;
    }
  }

  /* now draw some stuff */
  draw_border;
  setcolor(writeColor);
  svga_write_menu();
  svga_display_clear();

  curPanel = -1;		/* one panel only */
  svga_update(0);
  svga_clear();

  write_names();
}

#endif				/* LINUXGRAPHX */
