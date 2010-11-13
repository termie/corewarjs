/* pMARS -- a portable Memory Array Redcode Simulator
 * Copyright (C) 1993-1995 Albert Ma, Na'ndor Sieben, Stefan Strack, Mintardjo Wangsawidjaja and Martin Maierhofer
 * Copyright (C) 2000 Philip Kendall
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
 * xwindisp.c: user interface for XWindows graphics displays
 * $Id: xwindisp.c,v 1.1.1.2 2000/09/28 11:03:56 iltzu Exp $
 */

#ifdef XWINGRAPHX

#include "config.h"
#include "asm.h"
#include "xwindisp.h"

#include "pmarsicn.h"

#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD
#include <unistd.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#define MIN_WIDTH                320        /* minimum width of the window */
#define MIN_HEIGHT                200        /* minimum height of the window */

#define MAXLENGTH                160        /* maximum output string length */
#define MAXSTR                        80        /* maximum input string
                                                 * length */

#define MAXXCOLOR                16        /* need 16 X colors */
#define BLACK                         0        /* some color constants; in fact,
                                         * these */
#define BLUE                        1        /* are indices into xColors[] below */
#define GREEN                        2
#define CYAN                        3
#define RED                        4
#define LIGHTRED                12
#define YELLOW                        14
#define WHITE                         15

 /* X names of the colors we allocate */
static char *xColorNames[MAXXCOLOR] = {
  "black", "blue3", "green3", "cyan3",
  "red3", "magenta3", "yellow3", "light gray",
  "gray", "blue1", "green1", "cyan1",
  "red1", "magenta1", "yellow1", "white"
};

 /* X color (;-) names for gray scale displays */
static char *xGrayScaleNames[MAXXCOLOR] = {
  "black", "gray60", "white", "white",
  "gray60", "gray90", "gray70", "white",
  "gray80", "gray60", "gray90", "gray80",
  "gray70", "gray70", "gray80", "white"
};

/* X color (?) names for b&w only displays */
static char *xBWColorNames[MAXXCOLOR] = {
  "black", "white", "white", "white",
  "white", "white", "white", "white",
  "white", "white", "white", "white",
  "white", "white", "white", "white"
};

static int borderWidth = 3;        /* arena border */
static int leftUpperX = 5;        /* location of the arena */
static int leftUpperY = 50;
static int cycleY = 45;                /* location of the cycle meter */

static int colors[MAXWARRIOR];        /* colors of the two warriors */
static int datcolors[MAXWARRIOR];        /* death colors */

static int splY[MAXWARRIOR];        /* location of the process meters */

static int verticalSize;        /* # core locations in a line in the arena */
static int size;                /* size of one core location */

static int cycleRatio;                /* ratio of pixel of cycle meter to cycle */
static int processRatio;        /* ratio pixel of process meter to process */

static int x, y;                /* position and color of the currently */
static int col;                        /* accessed core address */

static int posx, posy;                /* cursor position of current panel */
static int posx1, posy1;        /* same for for panel 1 */
static int posx2, posy2;        /* same for for panel 2 */

static int point;                /* # input characters in panel */

static int grwindx0, grwindy0;        /* position of panel 1 */
static int grwindx1, grwindy1;        /* same for panel 2 */

static int horizspace = 9;        /* horizontal/vertical extension of one */
static int verspace = 15;        /* character */

static char str[2] = " ";        /* needed for some output */

static int xDim, yDim;                /* size of the window */

static Display *display;        /* the central X11 structure */
static char *displayName = NULL;/* name of the display */
static Window xwindow;                /* our window */

static XFontStruct *fontInfo;        /* the font we use, default font name */
static char *fontName = "-dec-terminal-bold-*-*-*-14-*-*-*-*-*-iso8859-*";

static GC writeGC, clearGC, colorGC;        /* white/black/color GCs */

static int screenNum;                /* the screen number we are mapped on */
static int defDepth;                /* default depth of the screen */

static Colormap colormap;        /* colormap of the screen, X colors */
static unsigned long xColors[MAXXCOLOR];

 /* the events we need */
static long eventMask = ExposureMask | KeyPressMask | ButtonPressMask |
StructureNotifyMask;

static int controlPressed;        /* modifier status of last key press */
static int altPressed;

static char *geometry;                /* the geometry specification */

static int doesBs;                /* does the X server support backing store? */
static Pixmap bsPixmap;                /* if not, this is our own backing store */

/* these are used in clparse.c */
int     xMaxOptions = 3;        /* the number & names of X cmd line options */
char   *xOptions[] = {"display", "geometry", "fn"};
char   *xStorage[] = {NULL, NULL, NULL};        /* filled in clparse */
int     xDisplayType = 0;        /* 0 = color, 1 = grayscale, 2 = b&w */

/* this is used in cdb.c */
int     xWinTextLines;                /* the number of text lines */

/* these are set in pmars.c */
int     xWinArgc;                /* command line arguments */
char  **xWinArgv;

/* some variables we need from other modules */
extern int curPanel;                /* number of the panel in use */
extern ADDR_T curAddr;                /* the current core address */
extern char *CDB_PROMPT;        /* cdb's prompt */

/* imported from str_???.c, these are varios error/status messages */
extern char *pressAnyKey;
extern char *cantAllocMem;
extern char *cantConnect;
extern char *structureAllocFails;
extern char *cantOpenFont;
extern char *noFixedFont;
extern char *noBackingStore;
extern char *needColorDisplay;
extern char *colorNotFound;
extern char *noColorAvailable;
extern char *privateMap;
extern char *invalidGeom;

/* nice macros for displaying/hiding the cursor */
#define cursoron() xWin_outtextxy(posx, posy, "_")
#define cursoroff() xWin_cleartextxy(posx, posy, clearGC)

/* function prototypes */
#ifdef NEW_STYLE
static void setcolor(int c);
static void xWin_cleartextxy(int x, int y, GC gc);
static void xWin_outtextxy(int x, int y, char *s);
static unsigned long conv_key(XEvent * event);
static unsigned long xWin_getch(void);
static void graphio_init(void);
static void newline(void);
static void newchar(void);
static void
        delchar(void);
static int mouse_or_key(char *result, unsigned long *key);
static char *special_keys(unsigned long key, char *buf);
static int xkoord(int addr);
static int ykoord(int addr);
static void findplace(int addr);
static void xWin_clear_arena(void);
static void write_names(void);
static void redraw(void);
static void my_err(char *s);
static int make_int(char *s);
static void
parse_geometry(XSizeHints * sizeHints,
               int *x, int *y, int *w, int *h);
static void alloc_colors(void);
static void get_gc(void);
static void handle_event(XEvent * event);
static void init_xwin(void);
static void draw_border(void);
#else
static void setcolor();
static void xWin_cleartextxy();
static void xWin_outtextxy();
static unsigned long conv_key();
static unsigned long xWin_getch();
static void graphio_init();
static void newline();
static void newchar();
static void
        delchar();
static int mouse_or_key();
static char *special_keys();
static int xkoord();
static int ykoord();
static void findplace();
static void xWin_clear_arena();
static void write_names();
static void redraw();
static void my_err();
static int make_int();
static void parse_geometry();
static void alloc_colors();
static void get_gc();
static void handle_event();
static void init_xwin();
static void draw_border();
#endif

/**********************************************************************/
/*   misc and util functions                                                                                                                 */
/**********************************************************************/

/*
 * print the given string to stderr and terminate
 */
static void
my_err(s)
  char   *s;
{
  fprintf(stderr, s);
  Exit(1);
}

/*
 * convert the given string into a number
 */
static int
make_int(s)
  char   *s;
{
  char   *err;
  int     r;

  r = (int) strtol(s, &err, 10);
  if (*err != '\0')
    my_err(invalidGeom);
  return r;
}

/*
 * parse the string specifying the geometry and return it in the
 * parameters, set sizeHint's flags member to indicate the parameters
 * specified
 */
static void
parse_geometry(sizeHints, x, y, w, h)
  XSizeHints *sizeHints;
  int    *x, *y, *w, *h;
{
  char   *g = geometry, *s, *t;
  char    c;

  sizeHints->flags = 0;

  if (g[0] != '+' && g[0] != '-') {        /* first parse width & height */
    sizeHints->flags |= USSize;

    if ((s = strchr(g + 1, 'x')) == NULL)
      my_err(invalidGeom);
    *s = '\0';
    *w = make_int(g);
    if (*w < MIN_WIDTH)
      *w = MIN_WIDTH;
    g = s + 1;

    if (strchr(g + 1, '+') == NULL && strchr(g + 1, '-') == NULL) {
      *h = make_int(g);
      if (*h < MIN_HEIGHT)
        *h = MIN_HEIGHT;
      return;
    }
    s = strchr(g + 1, '+');
    t = strchr(g + 1, '-');
    if (s == NULL)
      s = t;
    else if (t != NULL)
      if (t < s)
        s = t;
    if (s == NULL)
      my_err(invalidGeom);        /* this should never happen ! */

    c = *s;
    *s = '\0';
    *h = make_int(g);
    if (*h < MIN_HEIGHT)
      *h = MIN_HEIGHT;

    *s = c;
    g = s;
  }
  sizeHints->flags |= USPosition;

  if ((s = strchr(g + 1, '+')) == NULL && (s = strchr(g + 1, '-')) == NULL)
    my_err(invalidGeom);
  c = *s;
  *s = '\0';
  *x = make_int(g);

  *s = c;
  *y = make_int(s);
}

/*
 * adjust the position variables for a new panel
 */
void
xWin_update(newcurPanel)
  int     newcurPanel;
{
  if (curPanel == newcurPanel)
    return;
  if (curPanel == 0 && newcurPanel != 0) {
    posx1 = posx;
    posy1 = posy;
    grwindx0 = xDim / 2;
    grwindx1 = xDim - horizspace;
    xWin_clear();
    posx2 = posx;
    posy2 = posy;
    curPanel = 2;
  }
  switch (newcurPanel) {
  case 0:                        /* only one panel */
    if (curPanel == 1) {
      posx1 = posx;
      posy1 = posy;
    }
    grwindx0 = xDim / 2;
    grwindx1 = xDim - horizspace;
    xWin_clear();
    grwindx0 = 1;
    grwindx1 = xDim - horizspace;
    posx = posx1;
    posy = posy1;
    break;
  case 1:                        /* first panel */
    grwindx0 = 1;
    grwindx1 = xDim / 2 - horizspace;
    if (curPanel == 2) {
      posx2 = posx;
      posy2 = posy;
      posx = posx1;
      posy = posy1;
    }
    break;
  case 2:                        /* second panel */
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
      xWin_clear();
    }
    break;
  }
  curPanel = newcurPanel;
}

/*
 * initialize some graphics variables
 */
static void
graphio_init()
{
  size = 4;                        /* Size of a given location, feel free to
                                 * make it bigger */
  do {
    --size;                        /* decrease the size to fit */
    verticalSize = (xDim - 2 * leftUpperX) / (size + 1);
  } while ((ykoord(coreSize) > yDim - 20) && size > 0);

  if ((cycleRatio = 2 * warriors * cycles / yDim) == 0)
    cycleRatio = 1;
  if ((processRatio = taskNum / yDim) == 0)
    processRatio = 1;

  verspace = fontInfo->ascent + fontInfo->descent;

  grwindy0 = ykoord(coreSize) + 2 * borderWidth + verspace;
  grwindy1 = yDim - verspace;

  xWinTextLines = (yDim - grwindy0) / verspace;        /* used for pausing in cdb */
}

/*
 * return the x coordinate of the given core address
 */
int
xkoord(addr)
  int     addr;
{
  return (leftUpperX + ((addr) % verticalSize) * (size + 1));
}

/*
 * return the y coordinate of the given core address
 */
int
ykoord(addr)
  int     addr;
{
  return (leftUpperY + ((addr) / verticalSize) * (size + 1));
}

/*
 * record the coordinates and the color of the given core address
 */
void
findplace(addr)
  int     addr;
{
  x = xkoord(addr);
  y = ykoord(addr);
  col = colors[W - warrior];
}

/*
 * check for a color display and allocate the colors we need
 */
static void
alloc_colors()
{
  XColor  c;
  int     i = 5;
  XVisualInfo vInfo;
  char  **colorNames;

  defDepth = DefaultDepth(display, screenNum);

  while (!XMatchVisualInfo(display, screenNum, defDepth, i--, &vInfo));

  if (i >= StaticColor)
    colorNames = xColorNames;
  else if (i == GrayScale)
    colorNames = xGrayScaleNames;
  else
    colorNames = xBWColorNames;

  /*
   * override autodetected display by user- supplied choice if possible
   */
  if (xDisplayType == 1) {
    if (i >= GrayScale)
      colorNames = xGrayScaleNames;
  } else if (xDisplayType >= 2)
    colorNames = xBWColorNames;

  for (i = 0; i < MAXXCOLOR; i++) {
    if (!XParseColor(display, colormap, colorNames[i], &c)) {
      fprintf(stderr, colorNotFound, colorNames[i]);
      Exit(1);
    }
    /* If the color allocation fails and we are using the default
       colormap, switch to a private colormap and continue */
    if (!XAllocColor(display, colormap, &c)) {
      if(colormap == DefaultColormap(display, screenNum)) {
	fprintf(stderr, privateMap, colorNames[i]);
	colormap = XCopyColormapAndFree(display, colormap);
	i--;
      } else {
	my_err(noColorAvailable);
      }
    }
    xColors[i] = c.pixel;
  }
}

/*
 * allocate the GCs we need
 */
static void
get_gc()
{
  XGCValues values;

  writeGC = XCreateGC(display, xwindow, 0, &values);
  XSetFont(display, writeGC, fontInfo->fid);
  XSetForeground(display, writeGC, xColors[WHITE]);
  XSetBackground(display, writeGC, xColors[BLACK]);
  XSetLineAttributes(display, writeGC, 0, LineSolid, CapButt, JoinBevel);

  clearGC = XCreateGC(display, xwindow, 0, &values);
  XSetFont(display, clearGC, fontInfo->fid);
  XSetForeground(display, clearGC, xColors[BLACK]);
  XSetBackground(display, clearGC, xColors[BLACK]);
  XSetLineAttributes(display, clearGC, 0, LineSolid, CapButt, JoinBevel);

  colorGC = XCreateGC(display, xwindow, 0, &values);
  XSetFont(display, colorGC, fontInfo->fid);
  XSetForeground(display, colorGC, xColors[WHITE]);
  XSetBackground(display, colorGC, xColors[BLACK]);
  XSetLineAttributes(display, colorGC, 0, LineSolid, CapButt, JoinBevel);
}

/**********************************************************************/
/*   string output functions                                                                                                                 */
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
    int     width = grwindx1 - grwindx0;
    int     height = yDim - grwindy0;

    posy -= verspace;
    XCopyArea(display, (doesBs ? xwindow : bsPixmap), xwindow, writeGC,
            grwindx0, grwindy0 + verspace, width + 1, height - verspace + 1,
              grwindx0, grwindy0);

    XFillRectangle(display, xwindow, clearGC,
                   grwindx0, posy, width + 1, yDim - posy);

    if (!doesBs) {
      XCopyArea(display, bsPixmap, bsPixmap, writeGC,
            grwindx0, grwindy0 + verspace, width + 1, height - verspace + 1,
                grwindx0, grwindy0);

      XFillRectangle(display, bsPixmap, clearGC,
                     grwindx0, posy, width + 1, yDim - posy);
    }
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
xWin_puts(sss)
  char   *sss;
{
  if (printAttr)
    setcolor(colors[printAttr - 1]);
  else
    setcolor(WHITE);

  while (*sss) {
    switch (*sss) {
    case '\n':
      newline();
      break;
    default:
      str[0] = *sss;
      xWin_outtextxy(posx, posy, str);
      newchar();
    }
    ++sss;
  }
}

/*
 * write the menu line
 */
void
xWin_write_menu()
{
  int     y, i, j;
  char    s[7];

  y = ykoord(coreSize) + borderWidth + 2;

  setcolor(WHITE);
  xWin_outtextxy(10, y, "<");

  setcolor(RED);
  for (i = 0; i < SPEEDLEVELS - displaySpeed; i++)
    xWin_cleartextxy(20 + i * 10, y, colorGC);

  setcolor(YELLOW);
  for (j = 0; j < displaySpeed; j++)
    xWin_cleartextxy(20 + i * 10 + j * 10, y, colorGC);

  setcolor(WHITE);
  xWin_outtextxy(20 + i * 10 + j * 10, y, "> ");

  for (i = 0; i < 5; i++) {
    sprintf(s, "%d ", i);
    if (displayLevel == i)
      setcolor(RED);
    else
      setcolor(WHITE);
    xWin_outtextxy(170 + i * 10, y, s);
  }

  if (inCdb)
    setcolor(RED);
  else
    setcolor(WHITE);
  xWin_outtextxy(260, y, "Debug ");

  if (xDim > 400) {                /* this is only an estimate */
    if (inCdb)
      setcolor(BLACK);
    else
      setcolor(WHITE);
    xWin_outtextxy(310, y, "space Quit");
  }
  setcolor(WHITE);
}

/*
 * display the names of the warriors
 */
void
write_names()
{
  if (warriors <= 2) {
    setcolor(colors[0]);
    xWin_outtextxy(1, 0, warrior[0].name);

    if (warriors == 2) {
      setcolor(colors[1]);
      xWin_outtextxy(140, 0, warrior[1].name);
    }
  }
}

/**********************************************************************/
/*   keyboard and mouse handling routines                                                                                 */
/**********************************************************************/

/*
 * convert a KeyPress event to an integer representing the key
 */
static unsigned long
conv_key(event)
  XEvent *event;
{
  KeySym  keysym;
  static unsigned char buffer[20];
  XComposeStatus compose;
  int     count;

  count = XLookupString(&event->xkey, buffer, 20, &keysym, &compose);

  controlPressed = (event->xkey.state & ControlMask);
  altPressed = (event->xkey.state & Mod1Mask);

  if (count == 1 && buffer[0] >= 32 && buffer[0] <= 127)
    return buffer[0];
  else
    return (unsigned long) keysym;
}

/*
 * wait for a keypress, return the key
 */
static unsigned long
xWin_getch()
{
  XEvent  event;

  for (;;) {
    XNextEvent(display, &event);
    if (event.type == KeyPress)
      return conv_key(&event);
    handle_event(&event);
  }
}

/*
 * check for mouse and key events, return 1 and set result if a mouse
 * button gets pressed, return 0 and the key if a keyboard event
 * occurs
 */
static int
mouse_or_key(result, key)
  char   *result;
  unsigned long *key;
{
  XEvent  event;
  int     x, y;
  int     mouseLoc;

  XWarpPointer(display, xwindow, xwindow, 0, 0, xDim + 1, yDim + 1,
               xkoord(curAddr) + 1, ykoord(curAddr) + 1);

  for (;;) {
    XNextEvent(display, &event);

    switch (event.type) {
    case KeyPress:
      *key = conv_key(&event);
      return 0;

    case ButtonPress:
      switch (event.xbutton.button) {
      case Button1:
        strcpy(result, " m mousel\n");
        break;
      case Button2:
        strcpy(result, " m mouser\n");
        break;
      case Button3:
        strcpy(result, " m mousem\n");
        break;
      default:
        continue;
      }
      x = event.xbutton.x - leftUpperX + size / 2;
      y = event.xbutton.y - leftUpperY + size / 2;

      mouseLoc = x / (size + 1) + (y / (size + 1)) * verticalSize;
      if ((mouseLoc >= 0) && (mouseLoc < coreSize)) {
        curAddr = mouseLoc;
        return 1;
      }
      break;

    default:
      handle_event(&event);
    }
  }

  return 0;
}

/*
 * handle 'special' keys (ie. function/control/alt keys)
 */
static char *
special_keys(key, buf)
  unsigned long key;
  char   *buf;
{
  if (controlPressed && key < 128) {
    sprintf(buf, " m ctrl-%c\n", (char) key);
    return buf;
  }
  if (altPressed && key < 128) {
    sprintf(buf, " m alt-%c\n", (char) key);
    return buf;
  }
  switch (key) {
  case XK_Tab:
    strcpy(buf, " m ctrl-i\n");
    break;
  case XK_Insert:
    strcpy(buf, " m ins\n");
    break;
  case XK_Delete:
    strcpy(buf, " m del\n");
    break;
  case XK_Up:
    strcpy(buf, " m up\n");
    break;
  case XK_Down:
    strcpy(buf, " m down\n");
    break;
  case XK_Left:
    strcpy(buf, " m left\n");
    break;
  case XK_Right:
    strcpy(buf, " m right\n");
    break;
#ifdef XK_Page_Up
  case XK_Page_Up:
    strcpy(buf, " m pgup\n");
    break;
#else
#ifdef XK_Prior
  case XK_Prior:
    strcpy(buf, " m pgup\n");
    break;
#endif
#endif
#ifdef XK_Page_Down
  case XK_Page_Down:
    strcpy(buf, " m pgdn\n");
    break;
#else
#ifdef XK_Next
  case XK_Next:
    strcpy(buf, " m pgdn\n");
    break;
#endif
#endif
  case XK_Begin:
    strcpy(buf, " m home\n");
    break;
  case XK_End:
    strcpy(buf, " m end\n");
    break;
  case XK_F1:
    strcpy(buf, " m f1\n");
    break;
  case XK_F2:
    strcpy(buf, " m f2\n");
    break;
  case XK_F3:
    strcpy(buf, " m f3\n");
    break;
  case XK_F4:
    strcpy(buf, " m f4\n");
    break;
  case XK_F5:
    strcpy(buf, " m f5\n");
    break;
  case XK_F6:
    strcpy(buf, " m f6\n");
    break;
  case XK_F7:
    strcpy(buf, " m f7\n");
    break;
  case XK_F8:
    strcpy(buf, " m f8\n");
    break;
  case XK_F9:
    strcpy(buf, " m f9\n");
    break;
  case XK_F10:
    strcpy(buf, " m f10\n");
    break;
  case XK_F11:
    strcpy(buf, " m f11\n");
    break;
  case XK_F12:
    strcpy(buf, " m f12\n");
    break;
  default:
    return NULL;
  }
  return buf;
}

/*
 * read a line from the keyboard
 */
char   *
xWin_gets(result, maxchar)
  char   *result;
  int     maxchar;
{
  if (inputRedirection) {
    return fgets(result, maxchar, stdin);
  } else {
    unsigned long lo;
    result[0] = 0;
    point = 0;
    cursoron();

    do {
      if (mouse_or_key(result, &lo))
        return result;
      if (lo) {
        if (special_keys(lo, result) != NULL) {
          cursoroff();
          xWin_puts(result);
          return result;
        }
        switch (lo) {
        case XK_BackSpace:
        case XK_Delete:
          delchar();
          break;
        case XK_Escape:
          while (point)
            delchar();
          break;
        case XK_Return:
        case XK_KP_Enter:
        case XK_Linefeed:
          cursoroff();
          newline();
          break;
        default:
          if (lo >= 32 && lo < 128) {
            result[point++] = lo;
            if (point > maxchar - 1)
              lo = XK_Return;
            cursoroff();
            str[0] = lo;
            xWin_outtextxy(posx, posy, str);
            newchar();
            cursoron();
          }
        }
      }
    } while (lo != XK_Return && lo != XK_KP_Enter && lo != XK_Linefeed);
    result[point++] = '\n';
    result[point] = 0;
    if (point)
      return result;
    else
      return NULL;
  }                                /* !inputRedirection */
}

/**********************************************************************/
/*   event handling                                                                                                                                         */
/**********************************************************************/

/*
 * called eg. after resizing, this function redraws the window 'from
 * scratch'
 */
static void
redraw()
{
  XClearWindow(display, xwindow);
  if (!doesBs)
    XFillRectangle(display, bsPixmap, clearGC, 0, 0, xDim + 1, yDim + 1);

  draw_border();
  setcolor(WHITE);
  xWin_write_menu();
  xWin_display_clear();

  curPanel = -1;                /* one panel only */
  xWin_update(0);
  xWin_clear();

  write_names();

  if (inCdb) {
    point = 0;
    xWin_puts(CDB_PROMPT);
    cursoron();
  }
}

/*
 * handle the 'standard' events, ie. ConfigureNotify, Expose, (Un)MapNotify
 */
static void
handle_event(event)
  XEvent *event;
{
  int     width, height;
  static int mapped = 0;
  int     need_redraw = 0;
  XEvent  ev;

  switch (event->type) {
  case Expose:
    if (doesBs) {
      if (event->xexpose.count != 0)
        break;
      if (mapped)
        redraw();
    } else {
      int     x, y, w, h;

      x = event->xexpose.x;
      y = event->xexpose.y;
      w = event->xexpose.width;
      h = event->xexpose.height;

      XCopyArea(display, bsPixmap, xwindow, writeGC, x, y, w, h, x, y);
    }
    break;

  case ConfigureNotify:
    width = event->xconfigure.width;
    height = event->xconfigure.height;

    if (width == xDim && height == yDim)
      return;

    if (!doesBs) {
      XFreePixmap(display, bsPixmap);
      bsPixmap = XCreatePixmap(display, xwindow, width, height,
                               defDepth);
    }
    xDim = width;
    yDim = height;
    graphio_init();

    if (mapped)
      redraw();
    break;

  case MapNotify:
    mapped = 1;
    break;

  case UnmapNotify:                /* wait for next map event */
    for (;;) {
      XNextEvent(display, &ev);
      if (ev.type == MapNotify)
        break;

      if (ev.type == ConfigureNotify) {
        width = ev.xconfigure.width;
        height = ev.xconfigure.height;

        if (width != xDim || height != yDim) {
          if (!doesBs) {
            XFreePixmap(display, bsPixmap);
            bsPixmap = XCreatePixmap(display, xwindow, width, height,
                                     defDepth);
          }
          xDim = width;
          yDim = height;
          graphio_init();
          need_redraw = 1;
        }
      }
    }
    if (need_redraw)
      redraw();
    break;

#if 0                                /* debugging only */
  default:
    printf("Got unknown event: %d\n", event->type);
    break;
#endif
  }
}

/**********************************************************************/
/*   graphics utility functions                                                                                                         */
/**********************************************************************/

/*
 * set the color to be used in subsequent graphics operations
 */
void
setcolor(c)
  int     c;
{
  XSetForeground(display, colorGC, xColors[c]);
}

/*
 * draw a border around the arena
 */
static void
draw_border()
{
  int     x = leftUpperX - borderWidth;
  int     y = leftUpperY - borderWidth;
  int     w = xkoord(verticalSize - 1) + borderWidth - x;
  int     h = ykoord(coreSize) + borderWidth - y;

  XDrawRectangle(display, xwindow, writeGC, x, y, w, h);
  if (!doesBs)
    XDrawRectangle(display, bsPixmap, writeGC, x, y, w, h);
}

/*
 * 'clear' the character at position (x,y) with the current color
 */
void
xWin_cleartextxy(x, y, gc)
  int     x, y;
  GC      gc;
{
  XFillRectangle(display, xwindow, gc, x, y,
                 fontInfo->max_bounds.width + 1,
                 fontInfo->ascent + fontInfo->descent + 1);
  if (!doesBs)
    XFillRectangle(display, bsPixmap, gc, x, y,
                   fontInfo->max_bounds.width + 1,
                   fontInfo->ascent + fontInfo->descent + 1);
}

/*
 * draw specified text at location (x,y) - being the upper left corner
 * of the text
 */
void
xWin_outtextxy(x, y, s)
  int     x, y;
  char   *s;
{
  int     y_baseline = y + fontInfo->ascent;

  XDrawString(display, xwindow, colorGC, x, y_baseline, s, strlen(s));
  if (!doesBs)
    XDrawString(display, bsPixmap, colorGC, x, y_baseline, s, strlen(s));
}

/*
 * clear the current panel by filling it with black and reset the
 * cursor position.
 */
void
xWin_clear()
{
  posx = grwindx0;
  posy = grwindy0;

  XFillRectangle(display, xwindow, clearGC, grwindx0, grwindy0,
                 grwindx1 + horizspace - grwindx0 + 1, yDim - grwindy0 + 1);
  if (!doesBs)
    XFillRectangle(display, bsPixmap, clearGC, grwindx0, grwindy0,
                 grwindx1 + horizspace - grwindx0 + 1, yDim - grwindy0 + 1);
}

/*
 * clear the arena
 */
void
xWin_clear_arena()
{
  int     x = leftUpperX - borderWidth + 1;
  int     y = leftUpperY - borderWidth + 1;

  XFillRectangle(display, xwindow, clearGC,
                 x, y, xkoord(verticalSize - 1) + borderWidth - x,
                 ykoord(coreSize) + borderWidth - y);
  if (!doesBs)
    XFillRectangle(display, bsPixmap, clearGC,
                   x, y, xkoord(verticalSize - 1) + borderWidth - x,
                   ykoord(coreSize) + borderWidth - y);
}

/*
 * clear the arena and display the process meters and cycle meters
 */
void
xWin_display_clear()
{
  int     i;

  xWin_clear_arena();

  for (i = 0; i < warriors; i++) {
    XDrawLine(display, xwindow, clearGC,
              0, splY[i], xDim, splY[i]);
    if (!doesBs)
      XDrawLine(display, bsPixmap, clearGC,
                0, splY[i], xDim, splY[i]);

    setcolor(colors[i]);
    XDrawPoint(display, xwindow, colorGC,
               1 + taskNum / processRatio, splY[i]);
    if (!doesBs)
      XDrawPoint(display, bsPixmap, colorGC,
                 1 + taskNum / processRatio, splY[i]);

    XDrawLine(display, xwindow, colorGC,
              1, splY[i], warrior[i].tasks / processRatio, splY[i]);
    if (!doesBs)
      XDrawLine(display, bsPixmap, colorGC,
                1, splY[i], warrior[i].tasks / processRatio, splY[i]);
  }

  XDrawLine(display, xwindow, clearGC, 1, cycleY, xDim, cycleY);
  XDrawLine(display, xwindow, writeGC,
            1, cycleY, cycle / cycleRatio, cycleY);
  if (!doesBs) {
    XDrawLine(display, bsPixmap, clearGC, 1, cycleY, xDim, cycleY);
    XDrawLine(display, bsPixmap, writeGC,
              1, cycleY, cycle / cycleRatio, cycleY);
  }
}

/*
 * one more cycle has passed, update the display and check for keyboard
 * input
 */
void
xWin_display_cycle()
{
  unsigned long ch = 0;
  int     key = 0;
  XEvent  event;

#ifdef HAVE_USLEEP
  if (loopDelay > 1)
    usleep(loopDelay);
#else
  unsigned long ctr = loopDelay;
  while (ctr--);
#endif

  if (!(cycle & keyDelay)) {
    XDrawPoint(display, xwindow, clearGC,
               cycle / cycleRatio, cycleY);
    if (!doesBs)
      XDrawPoint(display, bsPixmap, clearGC,
                 cycle / cycleRatio, cycleY);

    while (XCheckMaskEvent(display, eventMask, &event)) {
      if (event.type == KeyPress) {
        key = 1;
        ch = conv_key(&event);
        break;
      }
      handle_event(&event);
    }

    if (!inputRedirection && key) {
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
        sighandler(0);                /* ??? debugState = STEP; *//* stepping =
                                 * FALSE; */
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
        xWin_clear_arena();
        break;

      case XK_Escape:
      case 'q':
        display_close();
        Exit(USERABORT);

      default:
        if (ch < 128)
          debugState = STEP;
        break;
      }
      xWin_write_menu();
    }
  }
}

/**********************************************************************/
/*   display functions indicating the core action                                                         */
/**********************************************************************/

/*
 * display a read access at the given address
 */
void
xWin_display_read(addr)
  int     addr;
{
  setcolor(colors[W - warrior]);
  XDrawPoint(display, xwindow, colorGC, xkoord(addr), ykoord(addr));
  if (!doesBs)
    XDrawPoint(display, bsPixmap, colorGC, xkoord(addr), ykoord(addr));
}

/*
 * display a decrement access at the given address
 */
void
xWin_display_dec(addr)
  int     addr;
{
  findplace(addr);
  setcolor(col);
  XDrawPoint(display, xwindow, colorGC, x, y);
  XDrawPoint(display, xwindow, colorGC, x + 1, y);
  if (!doesBs) {
    XDrawPoint(display, bsPixmap, colorGC, x, y);
    XDrawPoint(display, bsPixmap, colorGC, x + 1, y);
  }
}

/*
 * display an increment access at the given address
 */
void
xWin_display_inc(addr)
  int     addr;
{
  findplace(addr);
  setcolor(col);
  XDrawPoint(display, xwindow, colorGC, x, y);
  XDrawPoint(display, xwindow, colorGC, x, y + 1);
  if (!doesBs) {
    XDrawPoint(display, bsPixmap, colorGC, x, y);
    XDrawPoint(display, bsPixmap, colorGC, x, y + 1);
  }
}

/*
 * display a write access at the given address
 */
void
xWin_display_write(addr)
  int     addr;
{
  findplace(addr);
  setcolor(col);
  XDrawPoint(display, xwindow, colorGC, x + 1, y);
  XDrawPoint(display, xwindow, colorGC, x, y + 1);
  XDrawPoint(display, xwindow, colorGC, x + 1, y);
  if (!doesBs) {
    XDrawPoint(display, bsPixmap, colorGC, x + 1, y);
    XDrawPoint(display, bsPixmap, colorGC, x, y + 1);
    XDrawPoint(display, bsPixmap, colorGC, x + 1, y);
  }
}

/*
 * display an execute access at the given address
 */
void
xWin_display_exec(addr)
  int     addr;
{
  setcolor(colors[W - warrior]);
  XDrawRectangle(display, xwindow, colorGC,
                 xkoord(addr), ykoord(addr), 1, 1);
  if (!doesBs)
    XDrawRectangle(display, bsPixmap, colorGC,
                   xkoord(addr), ykoord(addr), 1, 1);
}

/*
 * display a split access at the given address
 */
void
xWin_display_spl(warrior, tasks)
  int     warrior, tasks;
{
  setcolor(colors[warrior]);
  XDrawPoint(display, xwindow, colorGC, tasks / processRatio, splY[warrior]);
  if (!doesBs)
    XDrawPoint(display, bsPixmap, colorGC, tasks / processRatio, splY[warrior]);
}

/*
 * display a dat access at the given address
 */
void
xWin_display_dat(addr, warNum, tasks)
  int     addr, warNum, tasks;
{
  if (displayLevel > 0) {
    setcolor(datcolors[warNum]);
    XFillRectangle(display, xwindow, colorGC,
                   xkoord(addr), ykoord(addr), 2, 2);
    if (!doesBs)
      XFillRectangle(display, bsPixmap, colorGC,
                     xkoord(addr), ykoord(addr), 2, 2);
  }
  XDrawPoint(display, xwindow, clearGC, tasks / processRatio, splY[warNum]);
  if (!doesBs)
    XDrawPoint(display, bsPixmap, clearGC,
               tasks / processRatio, splY[warNum]);
}

/**********************************************************************/
/*   initializing and closing the display                                                                                 */
/**********************************************************************/

/*
 * resize the window
 */
void
xWin_resize(void)
{
  int     xsize, ysize;

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

  if (xDim == xsize && yDim == ysize)
    return;

  XResizeWindow(display, xwindow, xsize, ysize);
}

/*
 * close the display
 */
void
xWin_display_close(wait)
  int     wait;
{
  if (wait == WAIT) {
    xWin_puts(pressAnyKey);
    xWin_getch();
  }
  if (display != NULL)
    XCloseDisplay(display);
  display = NULL;
}

/*
 * set up the X environment, open the display, create the window and
 * set various attributes
 */
static void
init_xwin()
{
  int     xsize, ysize, x = 0, y = 0;
  XSizeHints *sizeHints;
  XWMHints *wmHints;
  XClassHint *classHints;
  char    winName[100];
  char   *ptrwinName = &winName[0];
  char   *iconName = "pMARS";
  XTextProperty xwinName;
  XTextProperty xiconName;
  XSetWindowAttributes setwinAttr;

  switch (displayMode) {        /* check for window size set with -v */
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
  /* allocate various hint structures */
  if ((sizeHints = XAllocSizeHints()) == NULL)
    my_err(cantAllocMem);
  if ((wmHints = XAllocWMHints()) == NULL)
    my_err(cantAllocMem);
  if ((classHints = XAllocClassHint()) == NULL)
    my_err(cantAllocMem);

  /*
   * try to connect to the server, xStorage[0] contains the display name set
   * in the command line
   */
  if (xStorage[0] != NULL)
    displayName = xStorage[0];
  if ((display = XOpenDisplay(displayName)) == NULL) {
    fprintf(stderr, cantConnect, XDisplayName(displayName));
    Exit(1);
  }
  screenNum = DefaultScreen(display);
  colormap = DefaultColormap(display, screenNum);
  alloc_colors();

  /*
   * try to load the indicated font, fall back to "fixed" as a last resort
   */
  if (xStorage[2] != NULL)
    fontName = xStorage[2];
  if ((fontInfo = XLoadQueryFont(display, fontName)) == NULL) {
    fprintf(stderr, cantOpenFont, fontName);

    if ((fontInfo = XLoadQueryFont(display, "fixed")) == NULL)
      my_err(noFixedFont);
  }
  horizspace = fontInfo->max_bounds.width;
  verspace = fontInfo->ascent + fontInfo->descent;

  /* parse the geometry specification (if any) */
  sizeHints->flags = 0;
  geometry = xStorage[1];
  if (geometry != NULL)
    parse_geometry(sizeHints, &x, &y, &xsize, &ysize);
  xDim = xsize;
  yDim = ysize;

  /* and now create the window */
  xwindow = XCreateSimpleWindow(display, RootWindow(display, screenNum),
                     x, y, xsize, ysize, 0, xColors[WHITE], xColors[BLACK]);
  /* If we needed to change to a private colormap, install it
     here */
  if(colormap != DefaultColormap(display, screenNum))
    XSetWindowColormap(display, xwindow, colormap);

  get_gc();                        /* allocate the GCs */

  /* does the X server support backing store? */
  if (DoesBackingStore(ScreenOfDisplay(display, screenNum)) == NotUseful) {
    doesBs = 0;
    bsPixmap = XCreatePixmap(display, xwindow, xDim, yDim, defDepth);
    XFillRectangle(display, bsPixmap, clearGC, 0, 0, xDim + 1, yDim + 1);
  } else {
    doesBs = 1;
    setwinAttr.backing_store = WhenMapped;
    XChangeWindowAttributes(display, xwindow, CWBackingStore, &setwinAttr);
  }

  /* set the minimum size of the window */
  sizeHints->flags |= PMinSize;
  sizeHints->min_width = MIN_WIDTH;
  sizeHints->min_height = MIN_HEIGHT;
  if (geometry == NULL)
    sizeHints->flags |= PSize;

  /* window and icon name */
  sprintf(winName, "pMARS %d.%d (%s) X11 version", PMARSVER / 10,
          PMARSVER % 10, PMARSDATE);
  if (XStringListToTextProperty(&ptrwinName, 1, &xwinName) == 0)
    my_err(structureAllocFails);
  if (XStringListToTextProperty(&iconName, 1, &xiconName) == 0)
    my_err(structureAllocFails);

  /*
   * we want keyboard input and want to start as a 'normal' window
   */
  wmHints->flags = StateHint | InputHint | IconPixmapHint;
  wmHints->initial_state = NormalState;
  wmHints->input = True;
  wmHints->icon_pixmap = XCreateBitmapFromData(display, xwindow,
                            pmarsicn_bits, pmarsicn_width, pmarsicn_height);

  /* resource & class names */
  classHints->res_name = "pmars";
  classHints->res_class = "pmars";

  /*
   * let the window manager know of our preferences
   */
  XSetWMProperties(display, xwindow, &xwinName, &xiconName, xWinArgv,
                   xWinArgc, sizeHints, wmHints, classHints);

  /* select the events we need */
  XSelectInput(display, xwindow, eventMask);

  /* select a new mouse cursor */
  XDefineCursor(display, xwindow,
                XCreateFontCursor(display, XC_top_left_arrow));

  /* and let the window appear */
  XMapWindow(display, xwindow);
}

/*
 * get the X interface up and running
 */
void
xWin_open_graphics()
{
  XEvent  event;
  int     i;

  init_xwin();                        /* create the window */

  if (warriors <= 2) {
    splY[0] = fontInfo->ascent + fontInfo->descent + 1;
    splY[1] = splY[0] + 2;
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

  leftUpperY = splY[warriors - 1] + 15;
  cycleY = leftUpperY - 10;

  graphio_init();

  for (;;) {                        /* wait for the first expose event */
    XNextEvent(display, &event);
    if (event.type == Expose)
      break;
    handle_event(&event);
  }
  redraw();
}

#endif                                /* XWINGRAPHX */
