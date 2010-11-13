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
 * gtdisp.c: DJGPP & Watcom/DOS text display (libpc.a)
 * $Id: gtdisp.c,v 1.1.1.1 2000/08/20 13:29:37 iltzu Exp $
 */

#include <string.h>
#if !defined(WATCOM)
#ifndef _PC_H_
#include <pc.h>
#endif
#ifdef CONIOGRAPHX
#include <gppconio.h>
#endif
#endif
#include <dos.h>
#include <conio.h>

/* strings */
extern char *pressAnyKey;
extern char *cannotAllocateScreenBuffers;

int     curScreen, panelStart, panelWidth, screenSize, screenMap, screenX,
        screenY, page5x = 0, page5y = 0, page5y2 = 0, defPageY;
extern int curPanel;
short  *Screen[6];

#if defined(CONIOGRAPHX) && (!defined(WATCOM))
void    CacheScreenUpdate(short *buf);
#define SCREENUPDATE CacheScreenUpdate
#else
#define SCREENUPDATE ScreenUpdate
#endif
void    switch_page(int page);
void    SetCursor(char start, char stop);
void    scroll(int lines);
void    aputs5(char *str, int attr);
void    aputc5(char chr, int attr);
char   *agets5(char *str, int maxchar, int attr);
void    clear_page5(void);
void    display_init(void);
void    text_display_init(void);
void    text_display_close(void);
void    text_display_clear(void);
char   *xkey2macro(int xcode, char *buffer);
char   *ckey2macro(int ccode, char *buffer);
char   *ukey2macro(int ucode, char *buffer);

#define WRITE_CH 249
#define READ_CH 250
#define DEC_CH '-'
#define INC_CH '+'
#define DEF_SCREEN_Y 25
#define CURSOR_CH 219
#define KEYPRESSATTR 0xCE00
#define HELPATTR 0x0700
#define LF 10
#define CR 13
#define BS 8
#define ESC 27
#define TAB 9
#if defined(CONIOGRAPHX) && (!defined(WATCOM))
/* The SCREENUPDATE()-based display crashes on Nandor's computer; here
is an alternate display that is based on conio functions -- roughly 2.5x
slower */
#define CACHE 2
void
CacheScreenUpdate(short *buf)
{
  register int idx = 0;
  while (idx < screenSize - 1) {/* -1; otherwise window scrolls */
    if (Screen[CACHE][idx] != buf[idx]) {
      gotoxy((idx % screenX) + 1, (idx / screenX) + 1);
      textattr(buf[idx] >> 8);
      putch(buf[idx] & 0x00FF);
      Screen[CACHE][idx] = buf[idx];
    }
    ++idx;
  }
}
#endif

#if defined(WATCOM)
void
ScreenUpdate(short *scrptr)
{
  memcpy((void *) ((0xB800) << 4), scrptr, 4000);
}


void
ScreenRetrieve(short *scrptr)
{
  memcpy(scrptr, (void *) ((0xB800) << 4), 4000);
}
#endif

void
text_display_init()
{
  register int idx;
#if defined(WATCOM)
  screenX = panelWidth = 80;
  screenY = 25;
#else
  defPageY = wherey();
  screenX = panelWidth = ScreenCols();
  screenY = ScreenRows();
#endif
  panelStart = 0;
  screenSize = screenX * screenY;
  /* allocate screen buffers */
  if (!Screen[CORE_PAGE] &&
#if defined(CONIOGRAPHX) && (!defined(WATCOM))
      !(Screen[CACHE] = (short *) malloc(screenSize * sizeof(short))) ||
#endif
      !(Screen[DEF_PAGE] = (short *) malloc(screenSize * sizeof(short))) ||
      !(Screen[CORE_PAGE] = (short *) malloc(screenSize * sizeof(short))) ||
      !(Screen[CDB_PAGE] = (short *) malloc(screenSize * sizeof(short)))) {
    fprintf(stderr, cannotAllocateScreenBuffers);
    Exit(MEMERR);
  }
  for (idx = 0; idx < screenSize; idx++) {
    Screen[CORE_PAGE][idx] = (short) ' ';
    Screen[DEF_PAGE][idx] = (short) ' ';
    Screen[CDB_PAGE][idx] = (short) ' ';
  }
  /* determince wrap factor */
  screenMap = 0;
  while (coreSize > screenSize) {
    screenSize <<= 1;
    ++screenMap;
  }
  screenSize >>= screenMap;
#if (!defined(CONIOGRAPHX)) || defined(WATCOM)
  ScreenRetrieve(Screen[DEF_PAGE]);
#else
  gppconio_init();
  Screen[DEF_PAGE][screenSize - 2] = (short) '\r';
#endif
  switch_page(CORE_PAGE);
  hide_cursor();
}

void
text_display_clear()
{
  register int idx = 0;
  while (idx < screenSize)
    Screen[CORE_PAGE][idx++] = (short) ' ';
}

#define text_display_read(addr)\
        Screen[CORE_PAGE][addr >> screenMap] = READ_CH + (((W-warrior)+1) << 8)
#define text_display_write(addr)\
        Screen[CORE_PAGE][addr >> screenMap] = WRITE_CH + (((W-warrior)+1) << 8)
#define text_display_dec(addr)\
        Screen[CORE_PAGE][addr >> screenMap] = DEC_CH + (((W-warrior)+1) << 8)
#define text_display_inc(addr)\
        Screen[CORE_PAGE][addr >> screenMap] = INC_CH + (((W-warrior)+1) << 8)
#define text_display_exec(addr)\
    Screen[CORE_PAGE][addr >> screenMap] = '0' + (W-warrior) + (((W-warrior)+1) << 12)
#define text_display_spl(warrior,tasks)
#define text_display_dat(addr)\
        Screen[CORE_PAGE][addr >> screenMap] = '0' + (W-warrior) +\
             (((((W-warrior)+1) << 4) + 0x0F + 0x80) << 8)

void
text_display_close()
{
  register short *ptr
  = Screen[CORE_PAGE] + (screenX >> 1) - 8 + (screenY >> 1) * screenX;

  if (displayLevel) {
    while (*pressAnyKey)        /* destructive, cannot be used again */
      *ptr++ = *pressAnyKey++ | KEYPRESSATTR;
    SCREENUPDATE(Screen[CORE_PAGE]);
    if (inputRedirection)
      while (!kbhit());
    else
      getch();
  }
  switch_page(DEF_PAGE);
  show_cursor();
}

void
text_panel_update(int newPanel)
{
  int     tmp;

  switch (newPanel) {
  case 0:
    if (curPanel == 2) {
      clear_page5();
      page5y = page5y2;
      page5y2 = 0;
    } else if (curPanel == 1) {
      panelStart = panelWidth = page5x = screenX >> 1;
      tmp = page5y;
      clear_page5();
      page5y = tmp;
    }
    panelWidth = screenX;
    panelStart = page5x = 0;
    break;
  case 1:
    panelStart = page5x = 0;
    if (curPanel == 2) {
      tmp = page5y;
      page5y = page5y2;
      page5y2 = tmp;
    }
    break;
  case 2:
    panelStart = panelWidth = page5x = screenX >> 1;
    if (curPanel < 2) {
      tmp = page5y;
      page5y = page5y2;
      page5y2 = tmp;
    }
    if (curPanel == 0)
      clear_page5();
    break;
  }
  curPanel = newPanel;
}

void
switch_page(int num)
{
  if (Screen[num])
    SCREENUPDATE(Screen[num]);
}

void
SetCursor(char start, char end)
{
#if defined(WATCOM)
  union REGS regs;
  regs.x.eax = 0x0100;
  regs.x.ecx = (start * 256) + end;
  int386(0x10, &regs, &regs);
#else
  union REGS regs;
  regs.x.ax = 0x0100;
  regs.x.cx = (start * 256) + end;
  int86(0x10, &regs, &regs);
#endif
}


void
aputs5(char *text, int attr)
{                                /* direct write to screen5 with attribute */
  register int x = page5x, y = page5y;

  while (*text) {
    if ((*text == '\n') || (x == panelStart + panelWidth)) {
      Screen[CDB_PAGE][x + y * screenX] = ' ' | attr;
      if (y == screenY - 1) {
        scroll(1);
        x = panelStart;
      } else {
        ++y;
        x = panelStart;
      }
    }
    if (*text != '\n')
      Screen[CDB_PAGE][(x++) + y * screenX] = *text | attr;
    ++text;
  }
  page5x = x;
  page5y = y;
  SCREENUPDATE(Screen[CDB_PAGE]);
}
void
aputc5(char chr, int attr)
{                                /* direct write to screen5 with attribute */
  register int x = page5x, y = page5y;

  if ((chr == '\n') || (x == panelStart + panelWidth)) {
    Screen[CDB_PAGE][x + y * screenX] = ' ' | attr;
    if (y == screenY - 1) {
      scroll(1);
      x = panelStart;
    } else {
      ++y;
      x = panelStart;
    }
  }
  if (chr != '\n')
    Screen[CDB_PAGE][(x++) + y * screenX] = chr | attr;
  page5x = x;
  page5y = y;
  SCREENUPDATE(Screen[CDB_PAGE]);
}

char   *
agets5(char *buf, int maxchar, int attr)
{
  if (inputRedirection)
    return fgets(buf, maxchar, stdin);
  else {
    register int bufIdx = 0, c;
    int     idx;

    Screen[CDB_PAGE][page5x + page5y * screenX] = CURSOR_CH | attr | 0x8000;
    SCREENUPDATE(Screen[CDB_PAGE]);
    while (((c = getch()) != EOF) && (bufIdx < maxchar - 1)) {
      switch (c) {
      case 0:                        /* extended key */
#if 1
        aputs5(xkey2macro(getch(), buf + bufIdx), attr);
#else
        xkey2macro(getch(), buf + bufIdx);
        aputc5('\n', attr);
#endif
        return buf;
/*  case TAB:
      aputs5(strcpy(buf + bufIdx, "    "), attr);
      bufIdx += 4;
      break;  */
      case BS:                        /* backspace */
        if ((page5x > panelStart) && bufIdx) {
          Screen[CDB_PAGE][page5x + page5y * screenX] = ' ' | attr;
          --page5x;
          --bufIdx;
          Screen[CDB_PAGE][page5x + page5y * screenX] = CURSOR_CH | attr | 0x8000;
          SCREENUPDATE(Screen[CDB_PAGE]);
        }
        break;
      case ESC:                /* erase line */
        while ((page5x > panelStart) && bufIdx) {
          Screen[CDB_PAGE][page5x + page5y * screenX] = ' ' | attr;
          --page5x;
          --bufIdx;
          Screen[CDB_PAGE][page5x + page5y * screenX] = CURSOR_CH | attr | 0x8000;
          SCREENUPDATE(Screen[CDB_PAGE]);
        }
        break;
      case LF:
      case CR:
        goto done;
      default:
        if (c >= 32 && c <= 127) {
          aputc5(c, attr);
          buf[bufIdx++] = c;
        } else if (c > 127) {
#if 1
          aputs5(ukey2macro(c, buf + bufIdx), attr);
#else
          ukey2macro(c, buf + bufIdx);
          aputc5('\n', attr);
#endif
          return buf;
        } else {                /* control key */
#if 1
          aputs5(ckey2macro(c, buf + bufIdx), attr);
#else
          ckey2macro(c, buf + bufIdx);
          aputc5('\n', attr);
#endif
          return buf;
        }
        break;
      }
      Screen[CDB_PAGE][page5x + page5y * screenX] = CURSOR_CH | attr | 0x8000;
      SCREENUPDATE(Screen[CDB_PAGE]);
    }
done:
    aputc5('\n', attr);
    buf[bufIdx++] = '\n';
    buf[bufIdx] = 0;
    if (bufIdx)
      return buf;
    else
      return NULL;
  }                                /* !inputRedirection */
}

void
scroll(int lines)
{
  register int line, col;

  for (line = lines; line < screenY; ++line)
    for (col = panelStart; col < panelStart + panelWidth; ++col)
      Screen[CDB_PAGE][col + (line - lines) * screenX] = Screen[CDB_PAGE][col + line * screenX];
  for (line = screenY - lines; line < screenY; ++line)
    for (col = panelStart; col < panelStart + panelWidth; ++col)
      Screen[CDB_PAGE][col + line * screenX] = (short) ' ';
  SCREENUPDATE(Screen[CDB_PAGE]);
#if 0
  union REGS regs;
  regs.x.ax = 0x0600 + lines;
  regs.x.bx = 0;
  regs.x.cx = panelStart;
  regs.x.dx = ((screenY - 1) << 8) + (panelStart + panelWidth - 1);
  int86(0x10, &regs, &regs);
#endif
}

void
clear_page5(void)
{
  register int line, col;

  for (line = 0; line < screenY; ++line)
    for (col = panelStart; col < panelStart + panelWidth; ++col)
      Screen[CDB_PAGE][col + line * screenX] = (short) ' ';
  SCREENUPDATE(Screen[CDB_PAGE]);
  page5x = panelStart;
  page5y = 0;
}

#if ! defined(DOSGRXGRAPHX)
#define display_init()      text_display_init()
#define display_clear()     text_display_clear()
#define display_read(addr) do { if (displayLevel > 3)\
        text_display_read(addr); } while(0)
#define display_write(addr) do { if (displayLevel > 1)\
        text_display_write(addr); } while(0)
#define display_dec(addr) do { if (displayLevel > 2)\
        text_display_dec(addr); } while(0)
#define display_inc(addr) do { if (displayLevel > 2)\
        text_display_inc(addr); } while(0)
#define display_exec(addr) do { if (displayLevel > 0)\
        text_display_exec(addr); } while(0)
#define display_spl(warrior,tasks)
#define display_dat(addr,warNum,tasks) do { if (displayLevel > 0)\
        text_display_dat(addr); } while(0)
#define display_close()     text_display_close()
#define display_push(val)
#define display_die(warnum)
#endif
