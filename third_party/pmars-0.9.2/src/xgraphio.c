/* pMARS -- a portable Memory Array Redcode Simulator
 * Copyright (C) 1993-1995 Albert Ma, Na'ndor Sieben, Stefan Strack and Mintardjo Wangsawidjaja
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
 * xgraphio.c: input/output functions for DOS DJGPP/GRX display
 *
 * $Id: xgraphio.c,v 1.1.1.1 2000/08/20 13:29:48 iltzu Exp $
 */

#include <bios.h>
/* #include <alloc.h> */
#include <string.h>
#include <conio.h>
#include <dos.h>

#define ESC 27
#define BS 8
#define LF 10
#define CR 13
#define CLEAR 219
#define MAXLENGTH 160
#define HORIZSPACE 8

int     posx, posy;                /* for current panel */
int     posx1, posy1;                /* for panel 1 */
int     posx2, posy2;                /* for panel 2 */
int     scrollx1, scrollx2;
char    outstr[MAXLENGTH];
int     point;
int     grwindx0;
int     grwindy0;
int     grwindx1;
int     grwindy1;
int     verspace = 8;
int     scrollPixel;                /* how many pixels in a scrolled line */
int     scrollLines;                /* hom many lines to scroll           */
int     lastLine;                /* last scrolled line                 */
int     bgiTextLines;                /* how many lines has cdb */
char    str[2] = " ";
int     graphioColor;
extern int curPanel;

extern char *ckey2macro(int ccode, char *buf);
extern char *ukey2macro(int, char *);
extern char *xkey2macro(int, char *);

void    grputs(char *);

void 
grupdate(int newcurPanel)
{
  if (curPanel == newcurPanel)
    return;
  if (curPanel == 0 && newcurPanel != 0) {
    posx1 = posx;
    posy1 = posy;
    grwindx0 = GrMaxX() / 2;
    grwindx1 = GrMaxX() - HORIZSPACE;
    grclear();
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
    grwindx0 = GrMaxX() / 2;
    grwindx1 = GrMaxX() - HORIZSPACE;
    grclear();
    grwindx0 = 1;
    grwindx1 = GrMaxX() - HORIZSPACE;
    posx = posx1;
    posy = posy1;
    scrollx1 = 1;
    scrollx2 = 79;
    break;
  case 1:                        /* first panel */
    grwindx0 = 1;
    grwindx1 = GrMaxX() / 2 - HORIZSPACE;
    if (curPanel == 2) {
      posx2 = posx;
      posy2 = posy;
      posx = posx1;
      posy = posy1;
    }
    scrollx1 = 1;
    scrollx2 = 39;
    break;
  case 2:                        /* second panel */
    grwindx0 = GrMaxX() / 2;
    grwindx1 = GrMaxX() - 10;
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
      grclear();
    }
    scrollx1 = 40;
    scrollx2 = 79;
    break;
  }
  curPanel = newcurPanel;
}

void 
graphio_init(void)
{
  switch (graphdriver) {
    default:scrollPixel = 16;
    lastLine = 29;
    verspace = 10;                /* 8 */
    break;
/*      case EGA : scrollPixel= 14;
     lastLine = 24;
     verspace = 8;
         break;
      case CGA : scrollPixel= 8;
         lastLine = 24;
         verspace = 8;
*/
  }
  scrollLines = (GrMaxY() - grwindy0) / scrollPixel;
  grwindy0 = 1 + GrMaxY() - scrollPixel * (scrollLines);
  bgiTextLines = (GrMaxY() - grwindy0) / verspace;        /* used for pausing in
                                                         * cdb */
}


void
grclear()
{
  posx = grwindx0;
  posy = grwindy0;
/*  setfillstyle(EMPTY_FILL, WHITE); */
  GrFilledBox(grwindx0, grwindy0, grwindx1 + HORIZSPACE - 1, GrMaxY(), GrBlack());
}

void
newline(void)
{
  union REGS regs;
  posx = grwindx0;
  posy += verspace;
  if (posy > grwindy1) {
    posy -= verspace;
    switch (graphdriver) {
    default:
      GrBitBlt(NULL, grwindx0, grwindy0, NULL, grwindx0, grwindy0 + verspace,
               grwindx1 + HORIZSPACE - 1, GrMaxX(), GrWRITE);
      GrFilledBox(grwindx0, posy, grwindx1 + HORIZSPACE - 1, GrMaxY(), GrBlack());
/*    case VGA :
    case EGA :
    case CGA :

  regs.x.ax=0x0600+ 1 ;
  regs.x.bx= 0;
  regs.x.cx=( (lastLine-scrollLines+1) <<8)+ scrollx1 ;
  regs.x.dx=( lastLine <<8)+ scrollx2 ;
  int86(0x10,&regs,&regs);
  posy -= (scrollPixel-verspace) ;
     break;
      default:
      grclear();
*/
    }
  }
}

void
newchar(void)
{
  posx += HORIZSPACE;
  if (posx > grwindx1 - HORIZSPACE)
    newline();
}

#define cursoron() outtextxy(posx, posy, "_")

#define cursoroff() setcolor(BLACK); \
str[0]=CLEAR;                        \
outtextxy(posx, posy, str);          \
setcolor(graphioColor)

void
delchar(void)
{
  if (point) {
    cursoroff();
    posx -= HORIZSPACE;
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

char   *
grgets(char *result, int maxchar)
{
  if (inputRedirection) {
    return fgets(result, maxchar, stdin);
  } else {
    int     lo;
    result[0] = 0;
    point = 0;
    cursoron();

    do {
      if (mouse_or_key(result))
        return result;
      lo = getch();
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
/*       case '\t':
    cursoroff();
    grputs("    ");
    lo=' ';    I'd like tab to be a macro for switch, Nandor */
        default:
          if (lo >= 32 && lo < 128) {
            result[point++] = lo;
            if (point > maxchar - 1)
              lo = CR;
            cursoroff();
            str[0] = lo;
            outtextxy(posx, posy, str);
            newchar();
            cursoron();
          } else if (lo > 127) {
            ukey2macro(lo, result);
            cursoroff();
            grputs(result);
            return result;
          } else {
            ckey2macro(lo, result);
            cursoroff();
            grputs(result);
            return result;
          }
        }

      } else {
        xkey2macro(getch(), result);
        cursoroff();
        grputs(result);                /* Stefan you might want to change this,
                                 * Nandor */
        return result;
      }
    } while (lo != CR);
    result[point++] = '\n';
    result[point] = 0;
    if (point)
      return result;
    else
      return NULL;
  }                                /* !inputRedirection */
}

void
grputs(char *sss)
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
      outtextxy(posx, posy, str);
      newchar();
    }
    ++sss;
  }
}



/*
 * void main(void) {       char result[160]; int             graphdriver =
 * DETECT; int             graphmode;
 *
 * initgraph(&graphdriver, &graphmode, ""); grclear();
 *
 * grgets(result,10); grputs("first\nsecond"); grclear(); }
 *
 */
