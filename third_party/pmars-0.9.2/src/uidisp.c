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
 * uidisp.c: common user interface for DOS graphics/text displays
 * $Id: uidisp.c,v 1.1.1.1 2000/08/20 13:29:47 iltzu Exp $
 */

#ifdef NEW_STYLE
char   *ckey2macro(int ccode, char *buf);
char   *ukey2macro(int ucode, char *buf);
char   *xkey2macro(int code, char *buf);
#else
char   *ckey2macro();
char   *ukey2macro();
char   *xkey2macro();
#endif
#if ! defined(CURSESGRAPHX)
#if defined(DJGPP) || defined(WATCOM)
#define KEYPRESSED kbhit()
#else
#include <bios.h>
#define KEYPRESSED bioskey(1)
#endif                                /* DJGPP */

void
display_cycle(void)
{
  unsigned long counter;
  int     ch;

  counter = loopDelay;
  while (--counter);

  if (!(cycle & keyDelay)) {

#if defined(DJGPP) || defined(WATCOM)
#if defined(DOSALLGRAPHX)
    if (displayMode == TEXT)
#ifdef CONIOGRAPHX
      CacheScreenUpdate(Screen[CORE_PAGE]);
#else
      ScreenUpdate((void *) Screen[CORE_PAGE]);
#endif
#else
#if defined(DOSTXTGRAPHX)
#ifdef CONIOGRAPHX
    CacheScreenUpdate(Screen[CORE_PAGE]);
#else
    ScreenUpdate((void *) Screen[CORE_PAGE]);
#endif
#endif                                /* DOSTXTGRAPHX */
#endif                                /* DOSALLGRAPHX */
#endif                                /* DJGPP */

#if defined(DOSALLGRAPHX)
    if (displayMode != TEXT)
      bgi_update_cycle_meter();
#else
#if defined(DOSGRXGRAPHX)
    bgi_update_cycle_meter();
#endif
#endif
    if (KEYPRESSED && !inputRedirection) {
      switch (ch = getch()) {
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
        debugState = STEP;
         /* stepping = FALSE; */ break;
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
#if defined(DOSALLGRAPHX)
        if (displayMode == TEXT)
          text_display_clear();
        else
          bgi_clear_arena();
#else
#if defined(DOSTXTGRAPHX)
        text_display_clear();
#else
        bgi_clear_arena();
#endif
#endif
        break;
      case 27:                        /* escape */
      case 'q':
        display_close();
        Exit(USERABORT);
#if defined(DOSALLGRAPHX)
      case 'v':
        if (displayMode == TEXT)
          displayMode = GRX
            ;
        else
          bgi_display_close(NOWAIT);
        display_init();
        break;
#endif
#if 0
#if defined(DOSALLGRAPHX)
      case '?':
      case 'h':
        if (displayMode == TEXT)
          disp_help();
        break;
#else
#if defined(DOSTXTGRAPHX)
      case '?':
      case 'h':
        disp_help();
        break;
#endif
#endif
#endif                                /* 0 */
      default:
        if (ch > 32 && ch < 128)
          ch += 128;
        ungetch(ch);
        debugState = STEP;
        break;
      }
#if defined(DOSALLGRAPHX)
      if (displayMode != TEXT)
        write_menu();
#else
#if defined(DOSGRXGRAPHX)
      write_menu();
#endif
#endif
    }
  }
}

/* convert an extended key scan code high byte to a macro string "m [name]" */
char   *
xkey2macro(code, buf)
  int     code;
  char   *buf;
{
#define INS 82
#define DEL 83
#define UP 72
#define DOWN 80
#define LEFT 75
#define RIGHT 77
#define PGUP 73
#define PGDN 81
#define HOME 71
#define END 79
  static char *altCode = "qwertyuiop[]  asdfghjkl;'  \\zxcvbnm ./";
  switch (code) {
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
  default:
    if ((code >= 59) && (code <= 68))        /* F-keys */
      sprintf(buf, " m f%d\n", code - 58);
    else if ((code >= 16) && (code <= 53) && (altCode[code - 16] != ' '))
      sprintf(buf, " m alt-%c\n", altCode[code - 16]);
    else
      sprintf(buf, " m xkey(%d)\n", code);
    break;
  }
  return buf;
}

/* convert a printable character (+128) into a macro string "m key-?" */
char   *
ukey2macro(ucode, buf)
  int     ucode;
  char   *buf;
{
  sprintf(buf, " m key-%c\n", ucode - 128);
  return buf;
}
#endif                                /* if ! defined (CURSESGRAPHX) */

/* convert a control character (asc < 32) into a macro string "m ctrl-?" */
char   *
ckey2macro(ccode, buf)
  int     ccode;
  char   *buf;
{
  sprintf(buf, " m ctrl-%c\n", ccode + 96);
  return buf;
}
