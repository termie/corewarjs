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
 * pmars.c: main(), toplevel, initialization, cleanup
 * $Id: pmars.c,v 1.1.1.1 2000/08/20 13:29:40 iltzu Exp $
 */

#include <stdio.h>
#if defined(unix) || defined(VMS)
#include <signal.h>
#else
#if defined(__MSDOS__)
#include <dos.h>
#endif
#endif
#include "global.h"

#if defined(LINUXGRAPHX)
#include <vga.h>
#include <vgamouse.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
#if defined(DJGPP)
#include <std.h>
#else
#ifdef DOS16
#include <io.h>
#else
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#endif
#endif
#endif

#ifdef __MAC__
#include <console.h>
#endif

#ifdef __OS2__
#include <signal.h>
void _cdecl sighandler(int);
#endif

#ifdef NEW_STYLE
void    init(void);
extern void results(FILE * outp);
void    body(void);
void    Exit(int);
int     returninfo(void);
#ifdef PSPACE
void    pspace_init(void);
#endif
#if defined(unix) || defined(__MSDOS__) || defined(VMS)
void    sighandler(int dummy);
#endif
#if defined(CURSESGRAPHX)
extern void end_curses(void);
#endif
#else
void    init();
extern void results();
void    body();
void    Exit();
int     returninfo();
#ifdef PSPACE
void    pspace_init();
#endif
#if defined(unix) || defined(__MSDOS__)
void    sighandler();
#endif
#if defined(CURSESGRAPHX)
extern void end_curses();
#endif
#endif

/* external strings */
extern char *stub386, *info01, *outOfMemory;
#if defined(LINUXGRAPHX)
extern char *cantInitSvga, *cantOpenConsole, *tcgetattrFails;
#endif

/* global variables */
int     pspP = 0;                /* contains number of P-spaces allocated, <
                                 * warriors with PIN */

#if defined(__BORLANDC__) && defined(CHECK386)
/*
 * Check for 386 or better processor; needed when compiled using 386
 * instruction set under BC. Define CHECK386 to enable processor check.
 */
void    check386(void);
#pragma startup check386        /* must be called before main() */
void
check386(void)
{
  asm     pushf
          asm pop ax
          asm and ah, 01111111 b
          asm or ah, 01000000 b
          asm push ax
          asm popf
          asm pushf
          asm pop ax
          asm and ah, 11000000 b
          asm cmp ah, 01000000 b
          asm je is386
          printf(stub386);
  Exit(NOT386);
is386:
  return;
}
#endif

#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
void
decode_vopt(option)
  int     option;
{
#if defined(XWINGRAPHX)
  extern int xDisplayType;        /* defined in xwindisp.c */

  xDisplayType = (option / 1000) % 10;
  option %= 1000;
#endif
  displayLevel = option % 10;
  displayMode = (option % 100 - displayLevel) / 10;
  displaySpeed = (option - displayMode * 10 - displayLevel) / 100;
  if (displaySpeed >= SPEEDLEVELS)
    displaySpeed = SPEEDLEVELS - 1;
#if defined(CURSESGRAPHX)
  refreshInterval = refIvalAr[displaySpeed];
#else
  loopDelay = loopDelayAr[displaySpeed];
  keyDelay = keyDelayAr[displaySpeed];
#endif
}
#endif

void
init()
{
  INITIALINST.opcode = (FIELD_T) DAT *8 + (FIELD_T) mF;
  INITIALINST.A_mode = INITIALINST.B_mode = (FIELD_T) DIRECT;
  INITIALINST.A_value = INITIALINST.B_value = INITIALINST.debuginfo = 0;
  errorcode = SUCCESS;
  errorlevel = WARNING;                /* reserve for future */
  errmsg[0] = '\0';                /* reserve for future */
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
  decode_vopt(SWITCH_v);
#endif
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
  if (!isatty(fileno(stdin)))
    inputRedirection = TRUE;
#endif
}

void
body()
{
  int     i, j;

  for (i = 0; (i < warriors) && (errorcode == SUCCESS); i++)
    if ((!assemble(warrior[i].fileName, i)) && (!SWITCH_b)) {
      fprintf(STDOUT, info01, warrior[i].name, warrior[i].instLen,
              warrior[i].authorName);
      disasm(warrior[i].instBank, warrior[i].instLen, warrior[i].offset);
      fprintf(STDOUT, "\n");
    }
#ifdef PSPACE                        /* set up pSpace */
  pspace_init();
#endif
  if (rounds && (errorcode == SUCCESS)) {
    simulator1();
    if (SWITCH_k) {
      set_reg('W', (long) warriors);        /* 'W' used in score calculation */
      if (warriors == 2)        /* standard 2-warrior game */
        fprintf(STDOUT, "%d %d\n%d %d\n", warrior[0].score[0],
             warrior[0].score[1], warrior[1].score[0], warrior[1].score[1]);
      else                        /* multiwarrior */
        for (i = 0; i < warriors; i++) {
          fprintf(STDOUT, "%d ", score(i));
          for (j = 0; j < warriors; j++) {
            fprintf(STDOUT, "%d ", warrior[i].score[j]);
          }
          fprintf(STDOUT, "%d\n", deaths(i));
        }
    } else                        /* ! SWITCH_k */
      results(stdout);
  }
}

/* called when ctrl-c is pressed; prepares for debugger entry */
#if defined(unix) || defined(__MSDOS__) || defined (__OS2__)
void
#ifdef __OS2__
        _cdecl
#endif
sighandler(dummy)
  int     dummy;
{
#ifndef SERVER
  debugState = STEP;
  cmdMod = RESET;                /* terminate any macros in process */
  /*
   * some machines (SPARCstation, RS6000) seem to lose the handler address
   * after first signal trap -> need to call signal() again
   */
#if !defined(__OS2__) && !defined(DJGPP)
#ifdef SIGINT
  signal(SIGINT, sighandler);
#endif
#if 0
  signal(SIGTSTP, sighandler);
#endif
#endif                                /* os2 or djgpp */
  return;
#else
  Exit(USERABORT);
#endif
}
#endif                                /* unix */


void
Exit(errorcode)
  int     errorcode;
{
#if defined(CURSESGRAPHX)
  end_curses();                        /* Restore terminal to sane mode */
#else
#if defined(__MAC__)
  while (!button());
#endif
#endif
#ifdef __OS2__                        /* jk - exit() overreacts... could be Borland
                                 * bug */
#ifdef OS2PMGRAPHX
  _endthread();                        /* this should be called ONLY from a
                                 * secondary thread */
#else
  _exit(errorcode);                /* jk - exit() overreacts... could be Borland
                                 * bug */
#endif
#else
#ifdef LINUXGRAPHX
  if (vga_getcurrentmode() != TEXT)
    svga_display_close(0);
  fflush(stdout);
#endif
  exit(SWITCH_Q >= 0 ? returninfo() : errorcode);
#endif
}

int
main(argc, argv)
  int     argc;
  char  **argv;
{
#if defined(unix) && !defined(DJGPP)
#ifdef SIGINT
  signal(SIGINT, sighandler);
#endif
#if 0
  signal(SIGTSTP, sighandler);
#endif
#else
#if defined(__MSDOS__) && !defined(DJGPP)
  ctrlbrk((int (*) (void)) sighandler);
#endif
#endif                                /* ifdef unix */
#ifdef __OS2__
  signal(SIGINT, sighandler);
#endif
#ifdef __MAC__
  /* console init for MAC */
  extern int ccommand(char ***);

  argc = ccommand(&argv);
#endif
#if defined(LINUXGRAPHX)
  if (vga_init() == -1) {
    fprintf(stderr, cantInitSvga);
    exit(1);
  }
  if ((console_fd = open("/dev/console", O_RDONLY)) == -1) {
    perror(cantOpenConsole);
    exit(1);
  }
  if (tcgetattr(console_fd, &tio_orig) == -1) {
    perror(tcgetattrFails);
    exit(1);
  }
#endif
#if defined(XWINGRAPHX)
  xWinArgc = argc;
  xWinArgv = argv;
#endif

  if ((errorcode = parse_param(argc, argv)) == 0) {
    init();
#ifdef OS2PMGRAPHX                /* jk */
    pm_body();
#else
    body();
#endif
    Exit(errorcode);
  }
  return SWITCH_Q >= 0 ? returninfo() : errorcode;
}

/* return exitcode based on SWITCH_Q setting, useful mainly in scripts */
int
returninfo()
{
#ifdef GRAPHX
#define VARIANT 2
#else
#ifdef SERVER
#define VARIANT 0
#else
#define VARIANT 1
#endif
#endif

  if (SWITCH_Q >= 1 && SWITCH_Q <= 999) {
    int     idxV[MAXWARRIOR], scrV[MAXWARRIOR], i;

    set_reg('W', (long) warriors);        /* 'W' used in score calculation */
    for (i = 0; i < warriors; ++i) {
      scrV[i] = score(i);
      idxV[i] = i;
    }
    if (SWITCH_o)
      sort_by_score(idxV, scrV);
    if (SWITCH_Q <= warriors)
      return scrV[idxV[SWITCH_Q - 1]];
    else if (SWITCH_Q >= 101 && SWITCH_Q <= 100 + warriors)
      return idxV[SWITCH_Q - 101] + 1;
    else
      return -1;                /* unknown -Q argument */
  } else
    switch (SWITCH_Q) {
    case 1000:
      return PMARSVER;                /* version */
    case 1001:
      return VARIANT;                /* SERVER, GRAPHX, etc. */
    case 1002:
      return PMARSVER * 10 + VARIANT;        /* a combination of the two */
    case 1003:                        /* core address size (int or short) */
      return sizeof(ADDR_T);
    case 2000:                        /* how many warriors share a P-space */
#ifdef PSPACE
      return warriors - pspP + 1;
#else
      return -1;
#endif
    default:
      return -1;                /* unknown -Q argument */
    }
}

#ifdef PSPACE
void
pspace_init()
{
  int     i, j;
  for (i = 0; i < warriors; ++i) {
    if (warrior[i].pSpaceIndex == UNSHARED)
      warrior[i].pSpaceIndex = pspP++;
    else if (warrior[i].pSpaceIndex == PIN_APPEARED) {        /* PIN appeared and
                                                         * unmatched */
      warrior[i].pSpaceIndex = pspP;
      for (j = i + 1; j < warriors; ++j)
        if (warrior[i].pSpaceIDNumber == warrior[j].pSpaceIDNumber)
          warrior[j].pSpaceIndex = pspP;
      ++pspP;
    }                                /* else: pSpace already shared, don't
                                 * increment pspP */
  }
  /* allocate space */
  for (i = 0; i < warriors; ++i) {
    warrior[i].lastResult = coreSize - 1;        /* unlikely number */
    if (pSpace[j = warrior[i].pSpaceIndex] == NULL &&
#ifdef DOS16
        ((pSpace[j] = (ADDR_T far *) farcalloc(pSpaceSize, sizeof(ADDR_T))) == NULL)) {
#else
    ((pSpace[j] = (ADDR_T *) calloc(pSpaceSize, sizeof(ADDR_T))) == NULL)) {
#endif
      errout(outOfMemory);
      Exit(MEMERR);
    }
  }
}                                /* pspace_init() */
#endif
