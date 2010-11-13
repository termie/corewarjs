/* pMARS -- a portable Memory Array Redcode Simulator
 * Copyright (C) 1993-1996 Albert Ma, Na'ndor Sieben, Stefan Strack and Mintardjo Wangsawidjaja
 * Copyright (C) 2000 Ilmari Karonen
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
 * cdb.c: debugger
 * $Id: cdb.c,v 1.3 2000/12/25 00:49:07 iltzu Exp $
 *
 * cdb - line-oriented core debugger by Stefan Strack
 */

#include <ctype.h>
#include <string.h>
#include "global.h"
#include "sim.h"
#if defined(DOS16)
#include <conio.h>
#endif
#if defined(DJGPP) && defined(DOSTXTGRAPHX)
#include <pc.h>
#endif

#ifndef SERVER

#define VERBLEN 3
#define SKIP_SPACE(s) while(isspace(*s)) ++s
#if defined(XWINGRAPHX)                /* need this string externally */
char   *CDB_PROMPT = "(cdb) ";
#else
#define CDB_PROMPT "(cdb) "
#endif
#define CMDSEP '~'                /* chain command separator */
#define CMDREP '!'                /* chain command repeator */
#define MAXSTR 255 /* 80 */        /* general buffer length */
#define MAXCMDSTR 500                /* max. chars in inputStr, lastCmdStr */
#define MAXMACROS 200                /* maximum number of macros */
#define MAXLOOPNESTING 12        /* max. nesting depth of cdb command loops
                                 * !!~..~!n */
#define TEXTLINES 23                /* number of lines to list before pausing */
#define MAXCMD 80
#define MAXARG 80
#define RANGE_T 0
#define TEXT_T 1
#define FORCE 1                        /* force STDOUT output */
#define COND 0                        /* allow '@' supression */
#define CORE 0                        /* Core list mode */
#define QUEUE 1                        /* Queue list mode */
#define WARRIOR 2                /* Warrior list mode */
#define PSP 3                        /* P-space list mode */
#define FOLD(n) ((ADDR_T) (((n) % (long) targetSize) + targetSize) % targetSize)
#define TERMINAL(s)  (! *(s))
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#define NOEOL(s) do {\
    register char *ch=(s);\
    while (*ch && (*ch != '\n') && (*ch != '\r')) ++ch;\
    *ch=0; } while(0)
#define targetSelect(index) (targetID == CORE || targetID == PSP ?\
        index : (targetID == QUEUE ?\
        queue(index) : (W-warrior==index ? progCnt : *warrior[index].taskHead)))
#ifdef NEW_STYLE
#define toupper_(x) (toupper(x))
#else
#define toupper_(x) (isalpha(x) && islower(x) ? toupper(x) : (x))
#endif

/* hash value constants */
#define NULL_H 0
#define ON_H 419
#define OF_H 411
#define ARE_H 1220
#define CAL_H 2226
#define CA_H 2214
#define CLS_H 2530
#define CLE_H 2516
#define CL_H 2511
#define CLO_H 2526
#define CON_H 2606
#define CO_H 2592
#define C_H 2187
#define DIS_H 3178
#define DI_H 3159
#define D_H 2916
#define ECH_H 3734
#define EC_H 3726
#define EDI_H 3762
#define ED_H 3753
#define E_H 3645
#define EXE_H 4298
#define EX_H 4293
#define FIL_H 4629
#define FI_H 4617
#define F_H 4374
#define GO_H 5508
#define G_H 5103
#define HAS_H 5878
#define HEL_H 5979
#define HE_H 5967
#define H_H 5832
#define IF_H 6723
#define LIS_H 9010
#define LI_H 8991
#define L_H 8748
#define LSE_H 9266
#define LS_H 9261
#define MOV_H 9904
#define MO_H 9882
#define MAC_H 9507
#define MA_H 9504
#define M_H 9477
#define PRO_H 12165
#define PR_H 12150
#define P_H 11664
#define PQU_H 12144
#define PQ_H 12123
#define PSP_H 12193
#define PS_H 12177
#define QUI_H 12969
#define QU_H 12960
#define Q_H 12393
#define REG_H 13264
#define RE_H 13257
#define R_H 13122
#define REM_H 13270
#define RES_H 13276
#define SEA_H 13987
#define SE_H 13986
#define SHE_H 14072
#define SH_H 14067
#define STE_H 14396
#define ST_H 14391
#define S_H 13851
#define SKI_H 14157
#define SK_H 14148
#define SWI_H 14481
#define SW_H 14472
#define THR_H 14814
#define TH_H 14796
#define TRA_H 15067
#define TR_H 15066
#define T_H 14580
#define UNT_H 15707
#define UN_H 15687
#define U_H 15309
#define WRI_H 17262
#define WR_H 17253
#define W_H 16767
#define WQU_H 17247
#define WQ_H 17226
#define XEC_H 17634
#define XE_H 17631
#define X_H 17496

/*
 * prototypes
 */
#ifdef NEW_STYLE
int     cdb(char *msg);
char   *get_cmd(char *prompt);
int
parse_cmd(char *cmd, char *verb, long *start,
          long *stop, char *arg);
int     subst_eval(char *expr, long *result);
void    substitute(char *in, char *repl, char *with, char *out);
void    help(void);
void    print_core(ADDR_T start, ADDR_T stop);
void    set_trace(ADDR_T start, ADDR_T stop);
void    unset_trace(ADDR_T start, ADDR_T stop);
void    print_registers(void);
void    edit_core(ADDR_T start, ADDR_T stop);
void    fill_core(ADDR_T start, ADDR_T stop);
unsigned int hash_str(char *s, int len);
void    cdb_fputs(char *s, int out);
int     wildsearch(char *pattern, char *target);
void    load_macros(char *fileName);
char   *match_macro(char *macro, char *definition);
void    print_macros(void);
void    exec_macro(char *macro);
void    bad_arg(char *argStr);
ADDR_T  queue(int index);
char   *queueview(ADDR_T index, char *buf);
char   *warriorview(ADDR_T index, char *buf);
char   *pspaceview(ADDR_T index, char *buf);
static char *(*targetview) (ADDR_T loc, char *buf) = locview;
#if defined(DOSALLGRAPHX)
extern void display_init(void);
#endif
#ifdef DOSGRXGRAPHX
extern char *grgets(char *s, int maxstr);
extern void grputs(char *s);
extern void bgi_display_close(int wait);
extern void write_menu(void);
extern void open_graphics(void);
extern void grclear(void);
extern void bgi_clear_arena(void);
extern void grupdate(int curPanel);
#endif
#ifdef DOSTXTGRAPHX
extern void SetCursor(int x, int y);
extern void switch_page(int page);
extern void aputs5(char *str, int attr);
extern char *agets5(char *str, int maxchar, int attr);
extern void clear_page5(void);
extern void text_display_clear(void);
extern void text_panel_update(int curPanel);
#endif
#ifdef CURSESGRAPHX
extern void init_curses(void);
extern void end_curses(void);
extern void winupdate(void);
#endif
#ifdef MACGRAPHX
extern void macputs(char *str);
extern void macgets(char *str, int maxchar);
extern void mac_display_close(void);
extern void mac_clear_screen(void);
extern int mac_text_lines(void);
#endif
#ifdef LINUXGRAPHX
/* all the prototypes have already been declared in global.h */
#endif
#ifdef DOS16
extern int parse(char *expr, mem_struct far * c, ADDR_T loc);
#else
extern int parse(char *expr, mem_struct * c, ADDR_T loc);
#endif

#if defined(DJGPP)
extern void sighandler(int dummy);
#if defined(DOSTXTGRAPHX) && !defined(CURSESGRAPHX)
#if defined(CONIOGRAPHX)
extern void CacheScreenUpdate(short *buf);
#define SCREENUPDATE CacheScreenUpdate
#else
#define SCREENUPDATE ScreenUpdate
#endif                                /* CONIOGRAPHX */
#endif                                /* DOSTXTGRAPHX */
#endif                                /* DJGPP */

#else
int     cdb();
char   *get_cmd();
int     parse_cmd();
int     subst_eval();
void    substitute();
void    help();
void    print_core();
void    set_trace();
void    unset_trace();
void    print_registers();
void    edit_core();
void    fill_core();
unsigned int hash_str();
void    cdb_fputs();
int     wildsearch();
void    load_macros();
char   *match_macro();
void    print_macros();
void    exec_macro();
void    bad_arg();
ADDR_T  queue();
char   *pspaceview();
char   *queueview();
char   *warriorview();
static char *(*targetview) () = locview;
char   *strstr();
extern int parse();
#ifdef CURSESGRAPHX
extern void init_curses();
extern void end_curses();
extern void winupdate();
extern char *agets5();
#endif
#endif

/* strings */
extern char *pagePrompt, *exitingCdbToFinishSimulation, *usageDisplay, *usageExecute,
       *usageMoveable, *pressAnyKeyToContinue, *usageSkip, *usageWrite,
       *closingLogfile, *cannotOpenLogfile, *openingLogfile, *unknownCommand,
       *macroStringExpansionTooLong, *EOFreadingCommandInput, *maximumLoopNestingExceeded,
       *badRepeatCountExpression, *writeErrorDiskFull, *badArgument, *helpText[],
       *roundOfCycle, *currentlyExecutingWarrior, *processesActive, *otherWarrior,
       *warriorAtAddressHasActiveProcesses, *pluralEndingOfProcess, *ofWarriorsAreAlive,
       *fillWith, *dotByItselfEndsKeyboardInput, *cannotOpenMacroFile,
       *maximumNumberOfMacrosExceeded, *outOfMacroMemory, *unknownMacro,
       *badExprErr, *divZeroErr, *overflowErr;

/* global variables */
static FILE *logfile = 0;
ADDR_T  curAddr;
static long skipCounter = 0;
unsigned char commaSep;                /* parse_cmd flag indicating 2 argument
                                 * command, also used by CALC_H */
static int silent = 0;
static ADDR_T targetSize;
static int targetID = CORE;
#if defined(DJGPP) && !defined(CURSESGRAPHX) && defined(DOSTXTGRAPHX)
extern short *Screen[];
#endif
#if defined(DOSTXTGRAPHX)
extern int screenY;
#endif

#ifdef CYCLE_CHECK
static char *nextMacro = NULL, *macroTab[MAXMACROS], macroCycle[MAXMACROS];
#else
static char *nextMacro = NULL, *macroTab[MAXMACROS];
#endif

warrior_struct *W2, *QW;
char    outs[MAXSTR + 1], buffer1[MAXSTR + 1], buffer2[MAXSTR + 1];
char   *xInpP;                        /* pointer to inputStr[], used by
                                 * input-requiring functions called by cdb() */
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
int     printAttr;
int     curPanel = 0;
ADDR_T  curAddr2;
#endif

#if !defined(NEW_STYLE) && !defined(VMS)
char   *
strstr(cs, ct)
  char   *cs, *ct;
{
  char   *p, *q, *r;

  for (p = cs; *p != 0; ++p) {
    for (q = p, r = ct; *r != 0; ++q, ++r) {
      if (*q != *r)
        goto next_sub;
    }
    return p;
next_sub:;
  }
  return NULL;
}
#endif
/*---------------------------------------------------------------------------
 cdb - main debugger loop, command dispatcher
 ---------------------------------------------------------------------------*/
int
cdb(message)
  char   *message;
{
  char   *cmdStr, *fnStr, verbStr[MAXCMD + 1], argStr[MAXARG + 1];
  static long start, stop;
  int     argType, returnValue;
  unsigned int i;

  /* skip execution */
  if (skipCounter > 0) {
    --skipCounter;
    return STEP;
  }
  inCdb = TRUE;
  /* do some display related stuff */
#if defined(DOSALLGRAPHX)
  if (displayMode == TEXT) {
    switch_page(CDB_PAGE);
    /* show_cursor();  */
  } else
    write_menu();
#else
#if defined(DOSTXTGRAPHX)
  switch_page(CDB_PAGE);
  /* show_cursor(); */
#else
#if defined(DOSGRXGRAPHX)
  write_menu();
#else
#if defined(LINUXGRAPHX)
  svga_write_menu();
#else
#if defined(XWINGRAPHX)
  xWin_write_menu();
#endif
#endif
#endif
#endif
#endif

  W2 = W->nextWarrior;
  if (targetID == QUEUE) {
    curAddr = 0;
    QW = W;
    targetSize = (QW->tasks ? QW->tasks : 1);        /* at least 1, otherwise
                                                 * we'll get an exception in
                                                 * pqueue mode */
#ifdef QUEUEDEBUG
    for (i = 0; i < targetSize; i++)
      printf("%d ", queue(i));
    printf("\n");
#endif
  } else if (targetID == WARRIOR) {
    curAddr = W - warrior;
    /* targetSize = warriors; */
  } else if (targetID == PSP) {
    QW = W;
  } else {
    curAddr = progCnt;
    targetSize = coreSize;
  }

  cdb_fputs(message, COND);
  print_core(curAddr, curAddr);
  do {                                /* command interpreter loop */
    cmdStr = get_cmd(CDB_PROMPT);        /* get (chained) command */
    argType = parse_cmd(cmdStr, verbStr, &start, &stop, argStr);
    switch (hash_str(verbStr, VERBLEN)) {        /* decode on hash value for
                                                 * speed */
    case CAL_H:
    case CA_H:                        /* calculate */
      if (argType == RANGE_T) {
        if (!commaSep)
          sprintf(outs, "%ld\n", start);
        else
          sprintf(outs, "%ld,%ld\n", start, stop);
        cdb_fputs(outs, COND);
      } else
        bad_arg(argStr);
      break;
    case CLS_H:
    case CLE_H:
    case CL_H:
#if defined(DOSALLGRAPHX)
      if (displayMode == TEXT)
        clear_page5();
      else
        grclear();
#else
#if defined(DOSTXTGRAPHX)
      clear_page5();
#else
#if defined(DOSGRXGRAPHX)
      grclear();
#else
#ifdef DOS16
      clrscr();
#else
#if defined(MACGRAPHX)
      mac_clear_screen();
#else
#if defined(LINUXGRAPHX)
      svga_clear();
#else
#if defined(XWINGRAPHX)
      xWin_clear();
#else
      cdb_fputs(CLRSCR, COND);        /* escape sequence for your terminal */
#endif                                /* XWINGRAPHX */
#endif                                /* LINUXGRAPHX */
#endif                                /* MACGRAPHX */
#endif                                /* MSDOS */
#endif                                /* GRX */
#endif                                /* TXT */
#endif                                /* ALL */
      break;
#if defined(DOSGRXGRAPHX) || defined(DOSTXTGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
    case CLO_H:                /* close display panel 2 */
      if (curPanel == 2) {
        i = curAddr;
        curAddr = curAddr2;
        curAddr2 = i;
      }
#if defined(DOSALLGRAPHX)
      if (displayMode == TEXT)
        text_panel_update(0);
      else
        grupdate(0);
#else
#if defined(DOSTXTGRAPHX)
      text_panel_update(0);
#else
#if defined(LINUXGRAPHX)
      svga_update(0);
#else
#if defined(XWINGRAPHX)
      xWin_update(0);
#else
      grupdate(0);
#endif                                /* XWINGRAPHX */
#endif                                /* LINUXGRAPHX */
#endif                                /* DOSTXTGRAPHX */
#endif                                /* DOSALLGRAPHX */
      break;
#endif                                /* DOSTXTGRAPHX || DOSGRXGRAPHX */
    case CON_H:
    case CO_H:
    case C_H:
      cdb_fputs(exitingCdbToFinishSimulation, COND);
      if (logfile) {
        fclose(logfile);
        logfile = NULL;
      }
      if (*argStr && ((start = FOLD(start)) == FOLD(stop)))
        progCnt = start;
      returnValue = NOBREAK;
      inCdb = FALSE;
      break;
#if defined(CURSESGRAPHX) || defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) \
    || defined(LINUXGRAPHX) || defined(XWINGRAPHX)
    case DIS_H:
    case DI_H:
    case D_H:
      if (argStr) {
        if (((i = hash_str(argStr, 3)) == CLE_H) || (i == CLS_H)) {
#if defined(DOSALLGRAPHX)
          if (displayMode == TEXT)
            text_display_clear();
          else
            bgi_clear_arena();
#else
#if defined(DOSTXTGRAPHX)
          text_display_clear();
#else
#if defined(DOSGRXGRAPHX)
          bgi_clear_arena();
#else
#if defined(LINUXGRAPHX)
          svga_clear_arena();
#endif                                /* SVGA */
#endif                                /* GRX */
#endif                                /* TXT */
#endif                                /* ALL */
        } else {
          if ((i = hash_str(argStr, 2)) == ON_H)
            displayLevel = 3;
          else if (i == OF_H)
            displayLevel = 0;
          else if (FOLD(start) == FOLD(stop)) {
#if defined(DOSALLGRAPHX)
            if (displayMode != TEXT)
              bgi_display_close(NOWAIT);
#else
#if defined(LINUXGRAPHX)
            svga_display_close(NOWAIT);
#endif
#endif
            decode_vopt(start);
#if defined(DOSALLGRAPHX)
            display_init();
            if (displayMode == TEXT)
              switch_page(CDB_PAGE);
#else
#if defined(LINUXGRAPHX)
            svga_open_graphics();
#endif
#endif
#if defined(XWINGRAPHX)
            xWin_resize();
#endif
          }
        }
      } else
        cdb_fputs(usageDisplay, FORCE);
      break;
#endif
    case ECH_H:
    case EC_H:
      sprintf(outs, "%s\n", argStr);
      cdb_fputs(outs, COND);
      break;
    case EDI_H:
    case ED_H:
    case E_H:
      curAddr = FOLD(stop);
      edit_core(FOLD(start), curAddr);
      break;
    case EXE_H:
    case EX_H:
    case XEC_H:
    case XE_H:
    case X_H:
      if ((start = FOLD(start)) == FOLD(stop)) {
        W->taskHead = W->taskTail;
        W->tasks = 1;
        progCnt = start;
        returnValue = STEP;
        inCdb = FALSE;
      } else
        cdb_fputs(usageExecute, FORCE);
      break;
    case FIL_H:
    case FI_H:
    case F_H:
      fill_core(FOLD(start), FOLD(stop));
      break;
    case GO_H:
    case G_H:
      if (*argStr && ((start = FOLD(start)) == FOLD(stop)))
        progCnt = start;
      returnValue = BREAK;
      inCdb = FALSE;
      break;
    case HAS_H:                /* HASH: developer hook for adding new *
                                 * commands */
      for (i = 0; argStr[i]; ++i)
        argStr[i] = toupper_(argStr[i]);
      sprintf(outs, "#define %s_H %d\n", argStr, hash_str(argStr, VERBLEN));
      cdb_fputs(outs, COND);
      break;
    case HEL_H:
    case HE_H:
    case H_H:
      help();
      break;
    case IF_H:
      if (argType == RANGE_T) {
        if (!start)
          cmdMod = SKIP;
      } else
        bad_arg(argStr);
      break;
    case LIS_H:
    case LI_H:
    case L_H:
    case NULL_H:                /* no command, use default */
      if (argType == RANGE_T) {
        curAddr = FOLD(stop);
        print_core(FOLD(start), curAddr);
      } else if (argStr[0] != CMDREP && argStr[1] != CMDREP)        /* kludge to make !!~!!
                                                                 * work */
        bad_arg(argStr);
      break;
#ifdef VMS
    case LSE_H:
    case LS_H:
      cdb_fputs("Currently not implemented\n", FORCE);
      break;
#endif
    case MAC_H:
    case MA_H:
    case M_H:
      /* filename ? */
      for (fnStr = argStr; *fnStr && (*fnStr != ',') && !isspace(*fnStr); ++fnStr);
      while (*fnStr && ((*fnStr == ',') || isspace(*fnStr)))
        *fnStr++ = 0;
      if (!macroTab[0] || *fnStr)
        load_macros(fnStr);
      if (!*argStr)
        print_macros();
      else
        exec_macro(argStr);
      break;
    case MOV_H:
    case MO_H:
      if ((i = hash_str(argStr, 2)) == ON_H)
        copyDebugInfo = TRUE;
      else if (i == OF_H)
        copyDebugInfo = FALSE;
      else
        cdb_fputs(usageMoveable, FORCE);
      break;
    case PRO_H:
    case PR_H:
    case P_H:
#if defined(DOSALLGRAPHX)
      if (displayMode == TEXT)
        results(NULL);
      else
        results(STDOUT);
#else
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
      results(NULL);
#else
      results(STDOUT);
#endif
#endif
      if (logfile)
        results(logfile);
      break;
    case PQU_H:
    case PQ_H:
      if ((i = hash_str(argStr, 2)) == OF_H) {
        curAddr = targetSelect(curAddr);
        targetSize = coreSize;
        targetID = CORE;
        targetview = locview;
      } else {
        if (*argStr && start == stop && start >= 1 && start <= warriors)
          QW = warrior + start - 1;
        else
          QW = W;
        curAddr = 0;
        targetSize = (QW->tasks ? QW->tasks : 1);
        targetID = QUEUE;
        targetview = queueview;
      }
      print_core(curAddr, curAddr);
      break;
#ifdef PSPACE
    case PSP_H:                /* P-space list mode */
    case PS_H:
      if ((i = hash_str(argStr, 2)) == OF_H) {
        curAddr = targetSelect(curAddr);
        targetSize = coreSize;
        targetID = CORE;
        targetview = locview;
      } else {
        if (*argStr && start == stop && start >= 1 && start <= warriors)
          QW = warrior + start - 1;
        else
          QW = W;
        curAddr = 0;
        targetSize = pSpaceSize;
        targetID = PSP;
        targetview = pspaceview;
      }
      print_core(curAddr, curAddr);
      break;
#endif
    case QUI_H:
    case QU_H:
    case Q_H:
      if (logfile) {
        fclose(logfile);
        logfile = NULL;
      }
#if defined(DOSALLGRAPHX)
      if (displayMode != TEXT)
        bgi_display_close(NOWAIT);
      else {
        show_cursor();
        switch_page(DEF_PAGE);
      }
#else
#if defined(DOSGRXGRAPHX)
      bgi_display_close(NOWAIT);
#else
#if defined(DOSTXTGRAPHX)
      switch_page(DEF_PAGE);
      show_cursor();
#else
#if defined(MACGRAPHX)
      mac_display_close();
#else
#if defined(LINUXGRAPHX)
      svga_display_close(NOWAIT);
#else
#if defined(XWINGRAPHX)
      xWin_display_close(NOWAIT);
#endif
#endif
#endif
#endif
#endif
#endif
#if defined(__MAC__) || defined(__AMIGA__)
      /* DOS taskQueue may not be free'd because of segment wrap-around */
      if (alloc_p) {
        free(memory);
        free(taskQueue);
        alloc_p = 0;
      }
#endif

      Exit(USERABORT);

    case REM_H:                /* macro comment */
      break;
    case REG_H:
    case RE_H:
    case R_H:
      print_registers();
      break;
    case RES_H:                /* reset command chain/macro execution */
      cmdMod = RESET;
      break;
    case SEA_H:                /* wildcard search for pattern in disassembly */
    case SE_H:
      for (i = 0; argStr[i]; ++i)
        argStr[i] = toupper_(argStr[i]);
      for (i = curAddr + 1; !cmdMod && i != curAddr; ++i) {
        if (i == targetSize)
          i = 0;
        if (!wildsearch(argStr, (*targetview) (i, outs))) {
          curAddr = i;
          break;
        }
      }
      cdb_fputs((*targetview) (curAddr, outs), COND);
      break;
#if !defined(__MAC__) && !defined(XWINGRAPHX)
    case SHE_H:                /* execute shell (command) */
    case SH_H:
#if defined(DOSALLGRAPHX)
      if (displayMode == TEXT) {
        switch_page(DEF_PAGE);
        show_cursor();
      } else {
        bgi_display_close(NOWAIT);
        displayMode = GRX;
      }
#else
#if defined(DOSGRXGRAPHX)
      bgi_display_close(NOWAIT);
#else
#if defined(DOSTXTGRAPHX)
      switch_page(DEF_PAGE);
      show_cursor();
#if defined(CURSESGRAPHX) && !defined(__PDCURSES__)
      endwin();
#endif
#else
#if defined(LINUXGRAPHX)
      svga_display_close(NOWAIT);
#endif
#endif
#endif
#endif
      system(argStr);
#if defined(DOSALLGRAPHX)
      if (displayMode == TEXT) {
        switch_page(CDB_PAGE);
        hide_cursor();
      } else
        open_graphics();
#else
#if defined(DOSGRXGRAPHX)
      open_graphics();
#else
#if defined(LINUXGRAPHX)
      printf(pressAnyKeyToContinue);
      fflush(stdout);
      svga_getch();
      svga_open_graphics();
#else
#if defined(DOSTXTGRAPHX)
#if defined(CURSESGRAPHX)
      printf(pressAnyKeyToContinue);
      getch();
      clear_page5();
#endif
      switch_page(CDB_PAGE);
      hide_cursor();
#endif
#endif
#endif
#endif
      break;
#endif
    case SKI_H:
    case SK_H:
      if (start == stop) {        /* we don't fold here */
        skipCounter = start;
        returnValue = STEP;
        inCdb = FALSE;
      } else
        cdb_fputs(usageSkip, FORCE);
      break;
    case STE_H:
    case ST_H:
    case S_H:
      if (*argStr && ((start = FOLD(start)) == FOLD(stop)))
        progCnt = start;
      returnValue = STEP;
      inCdb = FALSE;
      break;
#if defined(DOSGRXGRAPHX) || defined(DOSTXTGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
    case SWI_H:                /* switch display panels */
    case SW_H:
      if (*argStr && (start == stop) && ((start == 1) || (start == 2))) {
        if (curPanel == start)
          break;
        else
          i = start;
      } else if (curPanel < 2)
        i = 2;
      else
        i = 1;
#if defined(DOSALLGRAPHX)
      if (displayMode == TEXT)
        text_panel_update(i);
      else
        grupdate(i);
#else
#if defined(DOSTXTGRAPHX)
      text_panel_update(i);
#else
#if defined(LINUXGRAPHX)
      svga_update(i);
#else
#if defined(XWINGRAPHX)
      xWin_update(i);
#else
      grupdate(i);
#endif                                /* XWINGRAPHX */
#endif                                /* LINUXGRAPHX */
#endif                                /* DOSTXTGRAPHX */
#endif                                /* DOSALLGRAPHX */
      i = curAddr;
      curAddr = curAddr2;
      curAddr2 = i;
      break;
#endif                                /* DOSTXTGRAPHX || DOSGRXGRAPHX */
    case TRA_H:
    case TR_H:
    case T_H:
      if (argType == RANGE_T)
        set_trace(FOLD(start), FOLD(stop));
      else
        bad_arg(argStr);
      break;
    case THR_H:
    case TH_H:
      if (*argStr && ((start = FOLD(start)) == FOLD(stop)))
        progCnt = start;
      skipCounter = (W->tasks * warriors) - 1;
      returnValue = STEP;
      inCdb = FALSE;
      break;
    case UNT_H:
    case UN_H:
    case U_H:
      if (argType == RANGE_T)
        unset_trace(FOLD(start), FOLD(stop));
      else
        bad_arg(argStr);
      break;
    case WRI_H:
    case WR_H:
    case W_H:
      if (TERMINAL(argStr) && !logfile)
        cdb_fputs(usageWrite, FORCE);
      if (logfile) {
        cdb_fputs(closingLogfile, COND);
        fclose(logfile);
        logfile = NULL;
      }
      if (!TERMINAL(argStr)) {
        if ((logfile = fopen(argStr, "a")) == NULL)
          cdb_fputs(cannotOpenLogfile, FORCE);
        else
          cdb_fputs(openingLogfile, COND);
      }
      break;
    case WQU_H:
    case WQ_H:
      if ((i = hash_str(argStr, 2)) == OF_H) {
        curAddr = targetSelect(curAddr);
        targetSize = coreSize;
        targetID = CORE;
        targetview = locview;
      } else {
        curAddr = W - warrior;
        targetSize = warriors;
        targetID = WARRIOR;
        targetview = warriorview;
      }
      print_core(curAddr, curAddr);
      break;
    default:
      sprintf(outs, unknownCommand, cmdStr);
      cdb_fputs(outs, FORCE);
      break;
    }                                /* switch */
  } while (inCdb);                /* do */
#if defined(DOSALLGRAPHX)
  if (displayMode == TEXT) {
    switch_page(CORE_PAGE);
    hide_cursor();
  } else
    write_menu();
#else
#if defined(DOSTXTGRAPHX)
  switch_page(CORE_PAGE);
  hide_cursor();
#else
#if defined(DOSGRXGRAPHX)
  write_menu();
#else
#if defined(LINUXGRAPHX)
  svga_write_menu();
#else
#if defined(XWINGRAPHX)
  xWin_write_menu();
#endif
#endif
#endif
#endif
#endif
  return returnValue;
}
/*---------------------------------------------------------------------------
 queue - return address at process queue position
 ---------------------------------------------------------------------------*/
ADDR_T
queue(index)
  int     index;
{
  if (!index)
    return progCnt;
  else
#ifdef DOS16
    return *(QW->taskHead + index - 1);
#else
    return *(QW->taskHead + ((QW->taskHead + index - 1 >= endQueue) ?
                             (index - 1 - totaltask) : index - 1));
#endif
}
/*---------------------------------------------------------------------------
 get_cmd - get command from user or from command chain (cmd1~cmd2~cmd3..)
         returns null-term cmd?
         <return> recalls last cmd (chain)
         <space>.... supresses saving cmd (chain) to last-cmd
         ~~ (empty cmd) repeats last cmd in chain (also ~\0)
         ~ ~ is same as \n (used in print/edit/fill_core())
        !e repeats the chain before e times (no e, repeat forever)
        expands macro "flat" by a rather obscure method
 Warning: this is probably the worst spaghetti code in pmars; change at your
        your own risk!
 ---------------------------------------------------------------------------*/
char   *
get_cmd(prompt)
  char   *prompt;
{
  static int curCmd = 0, nextCmd = 0, loopNesting;
  static long repeating = 0, loopStack[MAXLOOPNESTING];
  static char holding[MAXCMDSTR + 1], inputStr[MAXCMDSTR + 1], lastCmdStr[MAXCMDSTR + 1];
  int     inpPtr, marking, loopStart, conLine, i;
  char   *rv, *ptr;                /* return value for *gets */

  if (cmdMod == RESET)                /* "reset" command was issued to terminate
                                 * macro */
    cmdMod = curCmd = nextCmd = 0;
new_input:

#if 0
  printf("entering with inputStr+(curCmd=%d)=\"%s\",inputStr+(nextCmd=%d)=\"%s\"\n",
         curCmd, inputStr + curCmd, nextCmd, inputStr + nextCmd);
#endif
  if (nextMacro) {                /* last command was "macro <name>"" */
#if defined(CURSESGRAPHX)
    winupdate();
#else
#if defined(DJGPP)
#if defined(DOSALLGRAPHX)
    if (displayLevel == TEXT)
      SCREENUPDATE(Screen[CDB_PAGE]);
#else
#if defined(DOSTXTGRAPHX)
    SCREENUPDATE(Screen[CDB_PAGE]);
#endif                                /* DOSTXTGRAPHX */
#endif                                /* DOSALLGRAPHX */
#endif                                /* DJGPP */
#endif                                /* CURSESGRAPHX */
    if (strlen(inputStr + nextCmd) + nextCmd + strlen(nextMacro)
        <= MAXCMDSTR) {                /* check for buffer overflow */
      if (nextCmd) {                /* save remaining chain for later appending */
        *holding = CMDSEP;
        strcpy(holding + 1, inputStr + nextCmd);
      } else
        *holding = 0;
#ifdef CYCLE_CHECK
      if ((tmpEnd = curCmd + strlen(nextMacro)) > macroEnd) {
        /* done with last macro, clear cycle-check array */
        for (macroIdx = 0; macroTab[macroIdx]; ++macroIdx)
          macroCycle[macroIdx] = 0;
        macroEnd = tmpEnd;
      }
#endif
      nextCmd = curCmd;                /* reset */
      strcpy(inputStr + curCmd, nextMacro);
      /* insert macro string in place of "macro <name>" command */
      strcat(inputStr + curCmd, holding);
      nextMacro = NULL;
    } else {
      cdb_fputs(macroStringExpansionTooLong, FORCE);
      nextCmd = curCmd = 0;
      nextMacro = NULL;
      goto new_input;
    }
  } else if (!nextCmd) {
    /* command string exhausted; get new from user */
    for (loopNesting = 0; loopNesting < MAXLOOPNESTING; ++loopNesting)
      loopStack[loopNesting] = 0;
    loopNesting = 0;
    repeating = 0;
#ifdef CYCLE_CHECK
    /* clear cycle-check array */
    for (macroIdx = 0; macroTab[macroIdx]; ++macroIdx)
      macroCycle[macroIdx] = 0;
#endif
    cdb_fputs(prompt, FORCE);        /* display prompt */
    conLine = FALSE;
    i = 0;                        /* buffer index */
    do {                        /* while not continuation line */
#if defined(CURSESGRAPHX)
      winupdate();
#else
#if defined(DJGPP)
#if defined(DOSALLGRAPHX)
      if (displayLevel == TEXT)
        SCREENUPDATE(Screen[CDB_PAGE]);
#else
#if defined(DOSTXTGRAPHX)
      SCREENUPDATE(Screen[CDB_PAGE]);
#endif                                /* DOSTXTGRAPHX */
#endif                                /* DOSALLGRAPHX */
#endif                                /* DJGPP */
#endif                                /* CURSESGRAPHX */
#if defined(DOSALLGRAPHX)
      if (displayMode == TEXT)
        rv = agets5(inputStr + i, MAXCMDSTR - i, NORMAL_ATTR);        /* read command line */
      else
        rv = grgets(inputStr + i, MAXCMDSTR - i);
#else
#if defined(DOSTXTGRAPHX)
      rv = agets5(inputStr + i, MAXCMDSTR - i, NORMAL_ATTR);        /* read command line */
#else
#if defined(DOSGRXGRAPHX)
      rv = grgets(inputStr + i, MAXCMDSTR - i);
#else
#if defined(MACGRAPHX)
      rv = macgets(inputStr + i, MAXCMDSTR - i);
#else
#if defined(LINUXGRAPHX)
      rv = svga_gets(inputStr + i, MAXCMDSTR - i);
#else
#if defined(XWINGRAPHX)
      rv = xWin_gets(inputStr + i, MAXCMDSTR - i);
#else
      rv = fgets(inputStr + i, MAXCMDSTR - i + 1, stdin);
#endif
#endif
#endif
#endif
#endif
#endif
      if (!rv) {
        cdb_fputs(EOFreadingCommandInput, FORCE);
        strcpy(inputStr, "con\n");        /* leave cdb */
        break;
      }
      if (logfile)
        fputs(inputStr + i, logfile);        /* echo to logfile if logging */
      NOEOL(inputStr + i);
      if (inputStr[i = strlen(inputStr) - 1] == '\\') {
        conLine = TRUE;
        inputStr[i] = 0;
      } else
        conLine = FALSE;
    } while (conLine == TRUE);

    if (TERMINAL(inputStr))        /* if return was pressed, recall last command */
      strcpy(inputStr, lastCmdStr);
    /* if leading space, don't save as last command */
    else if (!isspace(inputStr[0]))
      strcpy(lastCmdStr, inputStr);
    nextCmd = curCmd = 0;
#ifdef CYCLE_CHECK
    macroEnd = 0;
#endif
  }                                /* if (! nextCmd) */
#if defined(CURSESGRAPHX)
  else
    winupdate();
#else
#if defined(DJGPP)
#if defined(DOSALLGRAPHX)
  else if (displayLevel == TEXT)
    SCREENUPDATE(Screen[CDB_PAGE]);
#else
#if defined(DOSTXTGRAPHX)
  else
    SCREENUPDATE(Screen[CDB_PAGE]);
#endif                                /* DOSTXTGRAPHX */
#endif                                /* DOSALLGRAPHX */
#endif                                /* DJGPP */
#endif                                /* CURSESGRAPHX */
  /* advance to next ~,! or \0 */
advance:
  marking = 0;
  for (inpPtr = nextCmd; inputStr[inpPtr] && (inputStr[inpPtr] != CMDSEP)
       && ((nextCmd == 0 && inputStr[inpPtr + 1] != CMDREP) ||
           inputStr[inpPtr] != CMDREP || marking); ++inpPtr)
    if (!isspace(inputStr[inpPtr]))
      marking = 1;

  if (!inputStr[inpPtr] && (nextCmd != inpPtr)) {
    /* last (or only) cmd in chain */
    curCmd = nextCmd;
    nextCmd = 0;
  } else if ((nextCmd == inpPtr) && (inputStr[inpPtr] != CMDREP)) {
    /* ~[~\0] = empty cmd, recall last in chain */
    if (inputStr[inpPtr])        /* ~~ */
      nextCmd = inpPtr + 1;
    else                        /* ~\0 */
      nextCmd = 0;
    if (inpPtr > 0)
      inputStr[inpPtr - 1] = ' ';
    inputStr[inpPtr] = 0;
  } else if (inputStr[inpPtr] == CMDSEP) {
    curCmd = nextCmd;
    nextCmd = inpPtr + 1;
    inputStr[inpPtr] = 0;
  } else {                        /* if (inputStr[inpPtr] == CMDREP) */
    register int i, j;

    if (inputStr[inpPtr + 1] == CMDREP) {        /* !! loop start */
      i = inpPtr + 2;
      if (cmdMod == SKIP) {        /* skip next block !!....!n */
        cmdMod = 0;
        marking = 1;
        for (; inputStr[i]; ++i)
          if (inputStr[i] == CMDREP) {
            if (inputStr[i + 1] == CMDREP) {        /* possible loop start!! */
              loopStart = 1;
            } else
              loopStart = 0;
            for (j = i - 1; isspace(inputStr[j]); --j);
            if (inputStr[j] == CMDSEP) {        /* ~  ! */
              if (loopStart)
                ++marking;
              else
                --marking;
              if (!marking && !loopStart)
                break;
              else if (loopStart)
                i = i + 2;
            } else if (loopStart)
              i = i + 2;
          }
        loopStack[loopNesting] = repeating;        /* undo stack pushing */
        repeating = loopStack[--loopNesting];
      }
      /* advance to next ~ or \0 */
      for (; inputStr[i] && (inputStr[i] != CMDSEP); ++i);
      if (inputStr[i]) {
        nextCmd = i + 1;
        loopStack[loopNesting] = repeating;
        if (loopNesting < MAXLOOPNESTING) {
          repeating = loopStack[++loopNesting];
          goto advance;
        } else {
          cdb_fputs(maximumLoopNestingExceeded, FORCE);
          curCmd = nextCmd = 0;
          goto new_input;
        }
      } else {
        nextCmd = 0;
        goto new_input;
      }
    } else {                        /* !# loop end */
#if defined(DOS16) && !defined(DJGPP)
      putc('\r', stdout);        /* to check for Ctrl-C press */
#else
#if defined(DJGPP)
      if (kbhit())
        sighandler(0);
#endif
#endif
      if (cmdMod == SKIP) {
        cmdMod = 0;
        repeating = 1;
      }
      if (!repeating) {
        repeating = -1;                /* flag for "empty repeat count" */
        for (i = inpPtr + 1; inputStr[i] && inputStr[i] != CMDSEP; ++i)
          if (!isspace(inputStr[i]))
            repeating = 0L;
        if ((j = (inputStr[i] == CMDSEP)) == 1)
          inputStr[i] = 0;
        for (ptr = inputStr + inpPtr + 1; *ptr; ++ptr)        /* capitalize */
          *ptr = toupper(*ptr);
        if (!repeating && subst_eval(inputStr + inpPtr + 1, &repeating) == TEXT_T) {
          cdb_fputs(badRepeatCountExpression, FORCE);
          curCmd = nextCmd = 0;
          goto new_input;
        }
        if (j)
          inputStr[i] = CMDSEP;
      }
      if (!--repeating) {        /* done */
        /* advance to next ~ or \0 */
        for (i = inpPtr + 1; inputStr[i] && (inputStr[i] != CMDSEP); ++i);
        if (inputStr[i]) {
          nextCmd = i + 1;
          loopStack[loopNesting] = repeating;
          if (loopNesting)
            repeating = loopStack[--loopNesting];
#if 0
          else {
            cdb_fputs(maximumLoopNestingExceeded, FORCE);
            curCmd = nextCmd = 0;
            goto new_input;
          }
#endif
          goto advance;
        } else {
          nextCmd = 0;
          goto new_input;
        }
      }
      marking = 1;
      for (i = inpPtr - 1; i >= 0; --i)
        if (!inputStr[i]) {        /* reverse */
          inputStr[i] = CMDSEP;
          nextCmd = i + 1;
        } else if (inputStr[i] == CMDREP) {
          if (inputStr[i - 1] == CMDREP) {        /* possible loop start!! */
            loopStart = 1;
            --i;
          } else
            loopStart = 0;
          for (j = i - 1; isspace(inputStr[j]) && j >= 0; --j);
          if (!inputStr[j] || inputStr[j] == CMDSEP) {        /* ~  ! */
            inputStr[j] = CMDSEP;
            if (loopStart)
              --marking;
            else
              ++marking;
            if ((!marking && loopStart) || !loopNesting)
              break;
            else {
              nextCmd = j + 1;
              i = j;                /* continue past inner nested loop */
            }
          } else
            i = j;                /* not a loop, continue reversing */
        }
      if (inputStr[i] == CMDREP) {
        for (++i; inputStr[i] && (inputStr[i] != CMDSEP); ++i);
        if (inputStr[i])
          curCmd = i + 1;
      } else
        curCmd = 0;
      if (nextCmd > 0)
        inputStr[nextCmd - 1] = 0;
    }                                /* !# loop end */
  }
  if (cmdMod == SKIP) {
    cmdMod = 0;
    goto new_input;
  }
  return inputStr + curCmd;
}
/*---------------------------------------------------------------------------
 cdb_fputs(str, wout) - screen output with file logging
    wout==FORCE     always output to STDOUT
    wout==COND      output to STDOUT may be supressed by '@' or '&'
 ---------------------------------------------------------------------------*/
void
cdb_fputs(str, wout)
  char   *str;
  int     wout;
{
  if ((silent != 2 || wout == FORCE) && logfile && (fputs(str, logfile) == EOF))
#if defined(DOSALLGRAPHX)
    if (displayMode == TEXT)
      aputs5(writeErrorDiskFull, NORMAL_ATTR);
    else
      grputs(writeErrorDiskFull);
#else
#if defined(DOSTXTGRAPHX)
    aputs5(writeErrorDiskFull, NORMAL_ATTR);
#else
#if defined(DOSGRXGRAPHX)
    grputs(writeErrorDiskFull);
#else
#if defined(MACGRAPHX)
    macputs(writeErrorDiskFull);
#else
#if defined(LINUXGRAPHX)
    svga_puts(writeErrorDiskFull);
#else
#if defined(XWINGRAPHX)
    xWin_puts(writeErrorDiskFull);
#else
    fputs(writeErrorDiskFull, stderr);
#endif
#endif
#endif
#endif
#endif
#endif

  if ((!silent) || (wout == FORCE))
#if defined(DOSALLGRAPHX)
    if (displayMode == TEXT) {
      if (printAttr)
        aputs5(str, printAttr << 12);
      else
        aputs5(str, NORMAL_ATTR);
    } else
      grputs(str);
  printAttr = 0;
#else
#if defined(DOSTXTGRAPHX)
    if (printAttr)
      aputs5(str, printAttr << 12);
    else
      aputs5(str, NORMAL_ATTR);
  printAttr = 0;
#else
#if defined(DOSGRXGRAPHX)
    grputs(str);
  printAttr = 0;
#else
#if defined(MACGRAPHX)
    macputs(str);
#else
#if defined(LINUXGRAPHX)
    svga_puts(str);
  printAttr = 0;
#else
#if defined(XWINGRAPHX)
    xWin_puts(str);
  printAttr = 0;
#else
    fputs(str, STDOUT);
#endif
#endif
#endif
#endif
#endif
#endif
}
/*---------------------------------------------------------------------------
 bad_arg - report argument error
 ---------------------------------------------------------------------------*/
void
bad_arg(argStr)
  char   *argStr;
{
  sprintf(outs, badArgument, argStr);
  cdb_fputs(outs, FORCE);
}
/*---------------------------------------------------------------------------
 hash_str(str,length) - calculate hash value for length chars of str
 length<=3 to fit in int
 ---------------------------------------------------------------------------*/
unsigned int
hash_str(str, length)
  char   *str;
  int     length;
{
  int     hash = 0, mul, i, j;

  for (i = 0; i < length && str[i]; ++i) {
    for (j = i + 1, mul = 1; j < length; ++j)
      mul *= 'Z' - 'A' + 2;
    hash += (toupper_(str[i]) - 'A' + 1) * mul;
  }
  return (hash);
}
/*---------------------------------------------------------------------------
 substitute(in,repl,with,out) - substitute all "repl" in "in" with "with"
 ---------------------------------------------------------------------------*/
void
substitute(in, repl, with, out)
  char   *in, *repl, *with, *out;
{
  /* caller is using all three global buffers */
  char    out2[MAXARG + 1];
  char   *pos;

  if ((pos = strstr(in, repl)) != NULL) {
    substitute(pos + strlen(repl), repl, with, out2);
    *pos = 0;
    strcat(in, with);
    strcat(in, out2);
  }
  strcpy(out, in);
  return;
}
/*---------------------------------------------------------------------------
 subst_eval(inpStr,result) - evaluate range field argument after substitution
 of special address symbols. Currently handles:
 .   current address
 $   targetSize-1 = -1
 A   A-value of current instruction
 B   B-value of current instruction
 PC  progCnt of current warrior
 PC# progCnt of warrior1,..
 LINES number of cdb display lines

 (others can be easily added)
 ---------------------------------------------------------------------------*/
int
subst_eval(inpStr, result)
  char   *inpStr;
  long   *result;
{
  int evalerr;
  char    buf[2][MAXARG + 1], outs2[MAXARG + 1], *pos;
  int     bi1 = 0, bi2 = 1, tmp, i;
#define SWITCHBI do {bi1 = !bi1; bi2 = !bi2;} while(0)

  if (TERMINAL(inpStr))
    return RANGE_T;

  /* relative address +-expr is just .+-expr */
  if ((*inpStr == '-') || (*inpStr == '+')) {
    buf[bi1][0] = '.';
    buf[bi1][1] = 0;
    strcat(buf[bi1], inpStr);
  } else
    strcpy(buf[bi1], inpStr);

  sprintf(outs, "%d", curAddr);
  substitute(buf[bi1], ".", outs, buf[bi2]);

  SWITCHBI;
  sprintf(outs, "%d", (targetID == QUEUE && !QW->tasks ?
                       targetSize - 2 : targetSize - 1));
  substitute(buf[bi1], "$", outs, buf[bi2]);

  /* short cut: skip replacements if there are no letters in buffer */
  for (pos = buf[bi2]; *pos && !isupper(*pos); ++pos);
  if (*pos) {

    SWITCHBI;
    sprintf(outs, "%d", targetID == PSP ? QW->pSpaceIndex : memory[targetSelect(curAddr)].A_value);
    substitute(buf[bi1], "A", outs, buf[bi2]);

    SWITCHBI;
    sprintf(outs, "%d", targetID == PSP ? QW->pSpaceIndex : memory[targetSelect(curAddr)].B_value);
    substitute(buf[bi1], "B", outs, buf[bi2]);

    for (i = warriors - 1; i >= 0; --i) {
      sprintf(outs, "%d", (targetID == QUEUE || targetID == PSP ?
                           0 : (targetID == WARRIOR ?
                 i : (W - warrior == i ? progCnt : *warrior[i].taskHead))));
      sprintf(outs2, "PC%d", i + 1);
      SWITCHBI;
      substitute(buf[bi1], outs2, outs, buf[bi2]);
    }
    if (warriors < MAXWARRIOR) {/* PCN where N==warriors is PC */
      sprintf(outs, "%d", (targetID == QUEUE || targetID == PSP ?
                           0 : (targetID == WARRIOR ?
                                W - warrior : progCnt)));
      sprintf(outs2, "PC%d", warriors);
      SWITCHBI;
      substitute(buf[bi1], outs2, outs, buf[bi2]);
    }
    SWITCHBI;
    sprintf(outs, "%d", (targetID == QUEUE || targetID == PSP ?
                         0 : (targetID == WARRIOR ?
                              W - warrior : progCnt)));
    substitute(buf[bi1], "PC", outs, buf[bi2]);
    SWITCHBI;
    sprintf(outs, "%d", (cycle + (warriorsLeft ? warriorsLeft : 1) - 1) /
            (warriorsLeft ? warriorsLeft : 1));
    substitute(buf[bi1], "CYCLE", outs, buf[bi2]);
    SWITCHBI;
    sprintf(outs, "%d", round);
    substitute(buf[bi1], "ROUND", outs, buf[bi2]);

    SWITCHBI;
#if defined(DOSALLGRAPHX)
    if (displayMode != TEXT)
      sprintf(outs, "%d", bgiTextLines - 1);
    else
      sprintf(outs, "%d", screenY - 2);
#else
#if defined(DOSGRXGRAPHX)
    sprintf(outs, "%d", bgiTextLines - 1);
#else
#if defined(CURSESGRAPHX)
    sprintf(outs, "%d", LINES);
#else
#if defined(DOSTXTGRAPHX)
    sprintf(outs, "%d", screenY - 2);
#else
#if defined(MACGRAPHX)
    sprintf(outs, "%d", mac_text_lines());
#else
#if defined(LINUXGRAPHX)
    sprintf(outs, "%d", svgaTextLines);
#else
#if defined(XWINGRAPHX)
    sprintf(outs, "%d", xWinTextLines);
#else
    sprintf(outs, "%d", TEXTLINES);
#endif
#endif
#endif
#endif
#endif
#endif
#endif
    substitute(buf[bi1], "LINES", outs, buf[bi2]);

  }                                /* if (*pos) */
  if ((evalerr = eval_expr(buf[bi2], result)) >= OK_EXPR) {
    if (evalerr == OVERFLOW) {
      cdb_fputs(overflowErr, COND);
      cdb_fputs("\n", COND);
    }
    return RANGE_T;
  } else {
    return TEXT_T;                /* this is somewhat poorly handled */
  }
}
/*---------------------------------------------------------------------------
 parse_cmd - into verb,range,argument string
 ---------------------------------------------------------------------------*/
int
parse_cmd(inpStr, verbStr, start, stop, argStr)
  char   *inpStr, *verbStr, *argStr;
  long   *start, *stop;
{
  register S32_T i;
  long    result1, result2;

  SKIP_SPACE(inpStr);

  if (*inpStr == '@') {
    inpStr++;
    silent = 1;
    SKIP_SPACE(inpStr);                /* advance to argument */
  } else if (*inpStr == '&') {        /* even logfile output is disabled */
    inpStr++;
    silent = 2;
    SKIP_SPACE(inpStr);                /* advance to argument */
  } else
    silent = 0;

  /* get verb, inpStr starts with non-whitespace char */
  for (i = 0; isalpha(*inpStr) && i < MAXARG; ++i, ++inpStr)
    verbStr[i] = *inpStr;
  verbStr[i] = 0;

  SKIP_SPACE(inpStr);                /* advance to argument */

  /* if string argument, we're done */
  strncpy(argStr, inpStr, MAXARG);

  /* we don't want trailing spaces in file names */
  for (i = strlen(argStr) - 1; i >= 0 && isspace(*(argStr + i)); --i)
    *(argStr + i) = 0;

  /* range or address? - scan first value */
  for (i = 0; (*inpStr != ',') && !TERMINAL(inpStr); ++i, ++inpStr)
    buffer1[i] = toupper_(*inpStr);
  buffer1[i] = 0;
  if (*inpStr == ',') {                /* remember if we encountered a comma */
    commaSep = 1;
    ++inpStr;                        /* and spool ahead */
  } else
    commaSep = 0;
  SKIP_SPACE(inpStr);
  for (i = 0; !TERMINAL(inpStr); ++i, ++inpStr)        /* scan 2nd value */
    buffer2[i] = toupper_(*inpStr);
  buffer2[i] = 0;

  /* replace all special address symbols and evaluate expressions */
  if ((subst_eval(buffer1, &result1) == TEXT_T)
      || (subst_eval(buffer2, &result2) == TEXT_T))
    return TEXT_T;

  if (!(*buffer1) && !(*buffer2)) {        /* _ , */
    return RANGE_T;
  } else if ((*buffer1) && (*buffer2)) {        /* 1,2 */
    *start = result1;
    *stop = result2;
    return RANGE_T;
  } else if ((*buffer1) && !(*buffer2) && commaSep) {        /* 1, */
    *start = result1;
    *stop = curAddr;
    return RANGE_T;
  } else if ((*buffer1)) {        /* 1 */
    *start = result1;
    *stop = result1;
    return RANGE_T;
  } else if ((*buffer2)) {        /* ,2 */
    *start = curAddr;
    *stop = result2;
    return RANGE_T;
  } else
    return TEXT_T;
}
/*---------------------------------------------------------------------------
 help - show command help
 ---------------------------------------------------------------------------*/
void
help()
{
  int     count = 0, showLines, helpIdx;
#if defined(DOSALLGRAPHX)
  if (displayMode != TEXT)
    showLines = bgiTextLines - 1;
  else
    showLines = screenY - 2;
#else
#if defined(DOSGRXGRAPHX)
  showLines = bgiTextLines - 1;
#else
#if defined(LINUXGRAPHX)
  showLines = svgaTextLines - 1;
#else
#if defined(XWINGRAPHX)
  showLines = xWinTextLines - 1;
#else
#if defined(CURSESGRAPHX)
  showLines = LINES - 1;
#else
#if defined(DOSTXTGRAPHX)
  showLines = screenY - 2;
#else
#if defined(MACGRAPHX)
  showLines = mac_text_lines();
#else
  showLines = TEXTLINES;
#endif
#endif
#endif
#endif
#endif
#endif
#endif

  for (helpIdx = 0; *helpText[helpIdx]; ++helpIdx) {
    if ((!silent) && (++count == showLines)) {
      xInpP = get_cmd(pagePrompt);
      SKIP_SPACE(xInpP);
      if ((*xInpP == 'q') || (*xInpP == 'Q')) {
        cdb_fputs("\n", COND);
        break;                        /* sin! break out of loop efficiently */
      }
      if ((*xInpP != 'a') && (*xInpP != 'A'))
        count = 0;
    }
    cdb_fputs(helpText[helpIdx], COND);
  }

}
/*---------------------------------------------------------------------------
 print_core - list core addresses in range start-stop
 ---------------------------------------------------------------------------*/
void
print_core(start, stop)
  ADDR_T  start, stop;
{
  int     count = 0;
  int     showLines;
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
  int     i;
#endif
#if defined(DOSALLGRAPHX)
  if (displayMode != TEXT)
    showLines = bgiTextLines - 1;
  else
    showLines = screenY - 2;;
#else
#if defined(DOSGRXGRAPHX)
  showLines = bgiTextLines - 1;
#else
#if defined(LINUXGRAPHX)
  showLines = svgaTextLines - 1;
#else
#if defined(XWINGRAPHX)
  showLines = xWinTextLines - 1;
#else
#if defined(CURSESGRAPHX)
  showLines = LINES - 1;
#else
#if defined(DOSTXTGRAPHX)
  showLines = screenY - 2;
#else
#if defined(MACGRAPHX)
  showLines = mac_text_lines();
#else
  showLines = TEXTLINES;
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
  if (targetID == QUEUE && start == 0)
    printAttr = QW - warrior + 1;
  else if (targetID == WARRIOR && start == W - warrior)
    printAttr = W - warrior + 1;
  else if (targetID == CORE)
    for (i = 0; i < warriors; ++i)
      if (warrior[i].tasks && (W - warrior == i ? progCnt : *warrior[i].taskHead) == start) {
        printAttr = i + 1;
        break;
      }
#endif                                /* to distinguish these by reverse video,
                                 * etc. */
  cdb_fputs((*targetview) (start, outs), COND);
  for (; start != stop;) {
    if (++start == targetSize)
      start = 0;
    if ((!silent) && (++count == showLines) && (start != stop)) {
      xInpP = get_cmd(pagePrompt);
      SKIP_SPACE(xInpP);
      if ((*xInpP == 'q') || (*xInpP == 'Q')) {
        cdb_fputs("\n", COND);
        break;                        /* sin! break out of loop efficiently */
      }
      if ((*xInpP != 'a') && (*xInpP != 'A'))
        count = 0;
    }
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
    if (targetID == QUEUE && start == 0)
      printAttr = QW - warrior + 1;
    else if (targetID == WARRIOR && start == W - warrior)
      printAttr = W - warrior + 1;
    else if (targetID == CORE)
      for (i = 0; i < warriors; ++i)
        if (warrior[i].tasks && (W - warrior == i ? progCnt : *warrior[i].taskHead) == start) {
          printAttr = i + 1;
          break;
        }
#endif                                /* to distinguish these by reverse video,
                                 * etc. */
    cdb_fputs((*targetview) (start, outs), COND);
  }
}
/*---------------------------------------------------------------------------
 set_trace - set trace bit of addresses in range start-stop
 ---------------------------------------------------------------------------*/
void
set_trace(start, stop)
  ADDR_T  start, stop;
{
  do {
    if (start == targetSize)
      start = 0;
    memory[targetSelect(start)].debuginfo |= 1;
  } while (start++ != stop);
}
/*---------------------------------------------------------------------------
 unset_trace - clear trace bit of addresses in range start-stop
 ---------------------------------------------------------------------------*/
void
unset_trace(start, stop)
  ADDR_T  start, stop;
{
  do {
    if (start == targetSize)
      start = 0;
    memory[targetSelect(start)].debuginfo &= ~1;
  } while (start++ != stop);
}
/*---------------------------------------------------------------------------
 print_registers - show simulator status
 ---------------------------------------------------------------------------*/
void
print_registers()
{
#ifdef DOS16
  ADDR_T far *thisProc;
#else
  ADDR_T *thisProc;
#endif
  int     nFuture, nPast, count, taskHalf = (coreSize <= 10000 ? 7 : 5);

  sprintf(outs, roundOfCycle, round, rounds,
          (cycle + (warriorsLeft ? warriorsLeft : 1) - 1) /
          (warriorsLeft ? warriorsLeft : 1));
  cdb_fputs(outs, COND);
  sprintf(outs, currentlyExecutingWarrior, W - warrior, W->name);
  cdb_fputs(outs, COND);
  sprintf(outs, processesActive, W->tasks);
  cdb_fputs(outs, COND);

  if (W->tasks > (taskHalf * 2)) {
    nPast = nFuture = taskHalf;
    cdb_fputs(".. ", COND);
  } else {
    nPast = W->tasks >> 1;
    nFuture = W->tasks - nPast;
  }
  if (W->taskTail < taskQueue + nPast)
    thisProc = W->taskTail + (totaltask - nPast);
  else
    thisProc = W->taskTail - nPast;
  for (; thisProc < W->taskTail;) {
    sprintf(outs, "%d ", *thisProc);
    cdb_fputs(outs, COND);
#ifdef DOS16
    ++thisProc;
#else
    if (++thisProc == endQueue)
      thisProc = taskQueue;
#endif
  }
  if (W->tasks) {
    sprintf(outs, "[%d]-> ", progCnt);
    cdb_fputs(outs, COND);
    for (thisProc = W->taskHead, count = 1; count < nFuture; count++) {
      sprintf(outs, "%d ", *thisProc);
      cdb_fputs(outs, COND);
#ifdef DOS16
      ++thisProc;
#else
      if (++thisProc == endQueue)
        thisProc = taskQueue;
#endif
    }
  }
  if (W->tasks > (taskHalf * 2))
    cdb_fputs("..", COND);
  cdb_fputs("\n", COND);
#ifdef PSPACE
  sprintf(outs, "P-space[%d]: <%d> ", W->pSpaceIndex, W->lastResult);
  cdb_fputs(outs, COND);
  for (count = 1; count < 10; ++count) {
    sprintf(outs, "%d ", *(pSpace[W->pSpaceIndex] + count));
    cdb_fputs(outs, COND);
  }
  cdb_fputs("..\n", COND);
#endif
  if (warriors == 2) {
    sprintf(outs, otherWarrior, W2->name);
    cdb_fputs(outs, COND);
    sprintf(outs, processesActive, W2->tasks);
    cdb_fputs(outs, COND);
    if (W2->tasks < (taskHalf * 2)) {
      nPast = W2->tasks >> 1;
      nFuture = W2->tasks - nPast;
    } else {
      nPast = nFuture = taskHalf;
      cdb_fputs(".. ", COND);
    }
    if (W2->taskTail < taskQueue + nPast)
      thisProc = W2->taskTail + (totaltask - nPast);
    else
      thisProc = W2->taskTail - nPast;
    for (; thisProc < W2->taskTail;) {
      sprintf(outs, "%d ", *thisProc);
      cdb_fputs(outs, COND);
#ifdef DOS16
      ++thisProc;
#else
      if (++thisProc == endQueue)
        thisProc = taskQueue;
#endif
    }
    thisProc = W2->taskHead;
    sprintf(outs, "[%d]-> ", *thisProc);
    cdb_fputs(outs, COND);
    for (count = 1; count < nFuture; count++) {
#ifdef DOS16
      ++thisProc;
#else
      if (++thisProc == endQueue)
        thisProc = taskQueue;
#endif
      sprintf(outs, "%d ", *thisProc);
      cdb_fputs(outs, COND);
    }
    if (W2->tasks > (taskHalf * 2))
      cdb_fputs("..", COND);
    cdb_fputs("\n", COND);
#ifdef PSPACE
    sprintf(outs, "P-space[%d]: <%d> ", W2->pSpaceIndex, W2->lastResult);
    cdb_fputs(outs, COND);
    for (count = 1; count < 10; ++count) {
      sprintf(outs, "%d ", *(pSpace[W2->pSpaceIndex] + count));
      cdb_fputs(outs, COND);
    }
    cdb_fputs("..\n", COND);
#endif

  } else if (warriors > 2) {
    warrior_struct *TW;
    for (TW = warrior; TW < warrior + warriors; ++TW)
      if (TW != W) {
        sprintf(outs, warriorAtAddressHasActiveProcesses, TW - warrior,
                TW->name, *TW->taskHead, TW->tasks,
                (TW->tasks == 1 ? "" : pluralEndingOfProcess));
        cdb_fputs(outs, COND);
      }
    sprintf(outs, ofWarriorsAreAlive, warriorsLeft, warriors);
    cdb_fputs(outs, COND);
  }
}
/*---------------------------------------------------------------------------
 edit_core - modify range of core addresses
 ---------------------------------------------------------------------------*/
void
edit_core(start, stop)
  ADDR_T  start, stop;
{
  int evalerr;
  long    result;
  long    sstart, sstop;

  sstart = start;
  sstop = (start <= stop) ? stop : (long) stop + targetSize;

  do {
    cdb_fputs((*targetview) (start, outs), COND);
    if (targetID == QUEUE)
      sprintf(outs, "%-4d->%05d   ", start, queue(start));
    else if (targetID == WARRIOR)
      sprintf(outs, "#%-2d:  %05d   ", start,
              (W - warrior == start ? progCnt : *warrior[start].taskHead));
    else if (targetID == PSP)
      sprintf(outs, "[%4d]=", start);
    else                        /* targetID == CORE */
      sprintf(outs, "%05d   ", start);

    cdb_fputs(outs, COND);
    xInpP = get_cmd("");
    SKIP_SPACE(xInpP);

    if (!TERMINAL(xInpP)) {
      if (targetID == PSP) {        /* edit P-cell */
        if ((evalerr = eval_expr(xInpP, &result)) < OK_EXPR) {
          if (evalerr == DIV_ZERO)
            cdb_fputs(divZeroErr, FORCE);
          else
            cdb_fputs(badExprErr, FORCE);
          cdb_fputs("\n", FORCE);
          return;
        }
        if (evalerr == OVERFLOW) {
          cdb_fputs(overflowErr, COND);
          cdb_fputs("\n", COND);
        }
        result = ((ADDR_T) (((result) % (long) coreSize) + coreSize) % coreSize);
        if (start)
          *(pSpace[QW->pSpaceIndex] + start) = result;
        else
          QW->lastResult = result;
        result = 1;
      } else if ((result = parse(xInpP, memory + targetSelect(start),
                                 targetSelect(start))) < 0)
        return;
    } result = 1;                /* empty input */

    sstart += (long) result;
    while (result--) {
      cdb_fputs((*targetview) (start++, outs), COND);
      if (start == targetSize)
        start -= targetSize;
    }
  } while (sstart <= sstop);
}
/*---------------------------------------------------------------------------
 fill_core - fill range of core addresses with instruction
 ---------------------------------------------------------------------------*/
void
fill_core(start, stop)
  ADDR_T  start, stop;
{
  int evalerr;
  long    sstart, sstop;
  int     result;
  long    evalres;

  sstart = start;
  sstop = (start <= stop ? stop : (long) stop + targetSize);

  cdb_fputs(fillWith, COND);
  xInpP = get_cmd("");
  SKIP_SPACE(xInpP);
  if (targetID == PSP) {
    if ((evalerr = eval_expr(xInpP, &evalres)) < OK_EXPR) {
      if (evalerr == DIV_ZERO)
        cdb_fputs(divZeroErr, FORCE);
      else
        cdb_fputs(badExprErr, FORCE);
      cdb_fputs("\n", FORCE);
      return;
    }
    if (evalerr == OVERFLOW) {
      cdb_fputs(overflowErr, COND);
      cdb_fputs("\n", COND);
    }
    evalres = ((ADDR_T) (((evalres) % (long) coreSize) + coreSize) % coreSize);
  }
  if (!TERMINAL(xInpP))
    do {
      if (targetID == PSP) {
        if (start)
          *(pSpace[W->pSpaceIndex] + start) = evalres;
        else
          W->lastResult = evalres;
        result = 1;
      } else if ((result = parse(xInpP, memory + targetSelect(start), targetSelect(start))) <= 0)
        return;

      sstart += (long) result;
      while (result--) {
        if (++start == targetSize)
          start -= targetSize;
      }
    } while (sstart <= sstop);
}
/*---------------------------------------------------------------------------
 wildsearch() - search pattern in target with wildcards
        returns 0 if match, 1 if not
        spaces are not significant
        wildcards: * matches any number of chars in target
                   ? matches one char in target
 ---------------------------------------------------------------------------*/
int
wildsearch(pattern, target)
  char   *pattern, *target;
{
  register char *pat = pattern, *tar = target;        /* local copies */
  while (1) {
    SKIP_SPACE(pat);                /* remove this and the next line to make */
    SKIP_SPACE(tar);                /* the search space-sensitive */
    if (TERMINAL(pat))
      return 0;                        /* match! */
    else if (*pat == '*')
      return wildsearch(++pat, tar);
    else if ((*pat == *tar) || (*pat == '?')) {
      ++pat;
      ++tar;
    } else if (TERMINAL(tar))
      return 1;                        /* no match */
    else {                        /* mismatch, reset pattern to start */
      pat = pattern;
      ++tar;
    }
  }                                /* while (1) */
}
/*---------------------------------------------------------------------------
 load_macros() - load macros from file
 ---------------------------------------------------------------------------*/
void
load_macros(fnStr)
  char   *fnStr;
{
  int     i, j, tabIdx, macroLen, conLine;
  FILE   *mfp;
  char   *rv;

  if (!*fnStr)
    strcpy(++fnStr, "pmars.mac");
  if (!strcmp(fnStr, "user")) {        /* special symbol, read from stdin */
    mfp = stdin;
    cdb_fputs(dotByItselfEndsKeyboardInput, COND);
#if defined(CURSESGRAPHX)
    winupdate();
#else
#if defined(DJGPP)
#if defined(DOSALLGRAPHX)
    if (displayLevel == TEXT)
      SCREENUPDATE(Screen[CDB_PAGE]);
#else
#if defined(DOSTXTGRAPHX)
    SCREENUPDATE(Screen[CDB_PAGE]);
#endif                                /* DOSTXTGRAPHX */
#endif                                /* DOSALLGRAPHX */
#endif                                /* DJGPP */
#endif
  } else if ((mfp = fopen(fnStr, "r")) == NULL) {
    sprintf(outs, cannotOpenMacroFile, fnStr);
    cdb_fputs(outs, FORCE);
    return;
  }
  while (1) {
    i = 0;                        /* buffer index */
    conLine = FALSE;
    do {
#if defined(DOSALLGRAPHX)
      if ((mfp == stdin) && (displayMode == TEXT))
        rv = agets5(outs + i, MAXSTR - i, NORMAL_ATTR);
      else if (mfp == stdin)
        rv = grgets(outs + i, MAXSTR - i);
      else
#else
#if defined(DOSTXTGRAPHX)
      if (mfp == stdin)
        rv = agets5(outs + i, MAXSTR - i, NORMAL_ATTR);
      else
#else
#if defined(DOSGRXGRAPHX)
      if (mfp == stdin)
        rv = grgets(outs + i, MAXSTR - i);
      else
#else
#if defined(LINUXGRAPHX)
      if (mfp == stdin)
        rv = svga_gets(outs + i, MAXSTR - i);
      else
#else
#if defined(XWINGRAPHX)
      if (mfp == stdin)
        rv = xWin_gets(outs + i, MAXSTR - i);
      else
#endif
#endif
#endif
#endif
#endif
        rv = fgets(outs + i, MAXSTR - i + 1, mfp);
      for (; outs[i]; i++)
        if (outs[i] == '\n' || outs[i] == '\r')
          break;
      outs[i] = 0;
      if (outs[i - 1] == '\\') {/* line continued */
        conLine = TRUE;
        outs[--i] = 0;                /* reset */
      } else
        conLine = FALSE;
    } while (conLine == TRUE);

    if (!rv || ((outs[0] == '.') && (outs[1] <= 13)))
      break;                        /* user input */
    /* capitalize name and "normalize" */
    for (i = 0, j = 0; outs[i] && outs[i] != '='; ++i)
      if (!isspace(outs[i]))
        buffer1[j++] = toupper_(outs[i]);
    if (outs[i]) {
      if ((macroLen = strlen(outs)) == MAXSTR) {
        buffer1[j] = 0;
        sprintf(buffer1 + j + 1, " echo Macro %s is too long (>= %d chars)~reset",
                buffer1, MAXSTR);
        buffer1[j] = '=';
      } else {
        while (outs[i])
          buffer1[j++] = outs[i++];
        buffer1[j] = 0;
      }
    } else
      continue;                        /* we hit a "comment" line -> ignore */

    for (tabIdx = 0; (tabIdx < MAXMACROS) && macroTab[tabIdx]
         && !match_macro(buffer1, macroTab[tabIdx]); ++tabIdx);

    if (tabIdx == MAXMACROS) {
      cdb_fputs(maximumNumberOfMacrosExceeded, FORCE);
      break;
    }
    if ((macroTab[tabIdx] &&        /* previous macro with same name exists */
         ((macroTab[tabIdx] = (char *) realloc(macroTab[tabIdx], macroLen + 1)) == NULL))
        || (!macroTab[tabIdx] &&
            ((macroTab[tabIdx] = (char *) malloc(macroLen + 1)) == NULL))) {
      cdb_fputs(outOfMacroMemory, FORCE);
      break;
    }
#if 0
    if (!macroTab[tabIdx] &&
        ((macroTab[tabIdx] = (char *) malloc(macroLen + 1)) == NULL)) {
      cdb_fputs(outOfMacroMemory, FORCE);
      break;
    }
#endif
    strcpy(macroTab[tabIdx], buffer1);
  }
  if (mfp != stdin)
    fclose(mfp);
  return;
}
/*---------------------------------------------------------------------------
 match_macro - compare macro name to macro definition, return ptr to
        first char after "=" in definition if match, NULL if not.
 ---------------------------------------------------------------------------*/
char   *
match_macro(match, macroDef)
  char   *match, *macroDef;
{
  register int i;

  for (i = 0; (match[i] == macroDef[i]) && (macroDef[i] != '='); ++i);
  if ((macroDef[i] == '=') && (!match[i] || (match[i] == '=')))
    return macroDef + i + 1;
  else
    return NULL;
}
/*---------------------------------------------------------------------------
 print_macros - list all macros currently in memory
 ---------------------------------------------------------------------------*/
void
print_macros()
{
  int     count = 0, showLines, macroIdx;
#if defined(DOSALLGRAPHX)
  if (displayMode != TEXT)
    showLines = bgiTextLines - 1;
  else
    showLines = screenY - 2;
#else
#if defined(DOSGRXGRAPHX)
  showLines = bgiTextLines - 1;
#else
#if defined(LINUXGRAPHX)
  showLines = svgaTextLines - 1;
#else
#if defined(XWINGRAPHX)
  showLines = xWinTextLines - 1;
#else
#if defined(CURSESGRAPHX)
  showLines = LINES - 1;
#else
#if defined(DOSTXTGRAPHX)
  showLines = screenY - 2;
#else
#if defined(MACGRAPHX)
  showLines = mac_text_lines();
#else
  showLines = TEXTLINES;
#endif
#endif
#endif
#endif
#endif
#endif
#endif

  for (macroIdx = 0; macroTab[macroIdx]; ++macroIdx) {
    if ((!silent) && (++count == showLines)) {
      xInpP = get_cmd(pagePrompt);
      SKIP_SPACE(xInpP);
      if ((*xInpP == 'q') || (*xInpP == 'Q')) {
        cdb_fputs("\n", COND);
        break;                        /* sin! break out of loop efficiently */
      }
      if ((*xInpP != 'a') && (*xInpP != 'A'))
        count = 0;
    }
    cdb_fputs(macroTab[macroIdx], COND);
    cdb_fputs("\n", COND);
  }
}
/*---------------------------------------------------------------------------
 exec_macro - execute macro by queuing as a command line
 ---------------------------------------------------------------------------*/
void
exec_macro(macro)
  char   *macro;
{
  int     macroIdx;

  for (macroIdx = 0; macro[macroIdx]; ++macroIdx)        /* make capital */
    macro[macroIdx] = toupper_(macro[macroIdx]);

  for (macroIdx = 0; macroTab[macroIdx]; ++macroIdx)
    if ((nextMacro = match_macro(macro, macroTab[macroIdx])) != NULL)
      break;
  if (!nextMacro) {
    sprintf(outs, unknownMacro, macro);
    cdb_fputs(outs, FORCE);
  }
#ifdef CYCLE_CHECK
  else if (macroCycle[macroIdx]) {        /* detected circular macro defintion,
                                         * abort */
    cdb_fputs("Circular macro definition\n", FORCE);
    nextMacro = NULL;
  } else
    macroCycle[macroIdx] = 1;        /* mark as visited */
#endif
}
/*---------------------------------------------------------------------------
 queueview - locview for queue mode: return instruction at queue(loc)
 ---------------------------------------------------------------------------*/
char   *
queueview(loc, outp)
  ADDR_T  loc;
  char   *outp;
{
  char    buf[MAXSTR];
  ADDR_T  qloc = queue(loc);

  sprintf(outp, "%-4d->%05d   %s\n", loc, qloc, cellview(memory + qloc, buf, 0));
  return (outp);
}

/*---------------------------------------------------------------------------
 warriorview - locview for warrior mode: return instruction at warrior[loc].taskHead
 ---------------------------------------------------------------------------*/
char   *
warriorview(loc, outp)
  ADDR_T  loc;
  char   *outp;
{
  char    buf[MAXSTR];
  ADDR_T  wloc = (W - warrior == loc ? progCnt : *warrior[loc].taskHead);

  sprintf(outp, "#%-2d:  %05d   %s\n", loc, wloc, cellview(memory + wloc, buf, 0));
  return (outp);
}

char
       *
pspaceview(loc, outp)
  ADDR_T  loc;
  char   *outp;
{
  loc %= pSpaceSize;
  sprintf(outp, "[%4d]=%-5d\n", loc, loc ? *(pSpace[QW->pSpaceIndex] + loc) :
          QW->lastResult);
  return outp;
}
#endif

int
score(warnum)
  int     warnum;
{
  int     surv, accu = 0;
  long    res;

  for (surv = 1; surv <= warriors; ++surv) {
    set_reg('S', (long) surv);
    if (eval_expr(SWITCH_eq, &res) < OK_EXPR)
      return INT_MIN;                /* hopefully clparse will catch errors
                                 * earlier */
    else
      accu += warrior[warnum].score[surv - 1] * res;
  }
  return accu;
}

/* total the number of deaths for warrior[warnum]; the "order of death"
   array is currently neither reported nor used for the score calculation */
int
deaths(warnum)
  int     warnum;
{
  int     i, accu = 0;
  for (i = warriors; i < 2 * warriors - 1; ++i)
    accu += warrior[warnum].score[i];
  return accu;
}

void
sort_by_score(idxV, scrV)
  int    *idxV, *scrV;
{
  int     sorted, tmp, i;
  do {
    sorted = 1;
    for (i = 0; i < warriors - 1; ++i)
      if (scrV[idxV[i]] < scrV[idxV[i + 1]]) {
        tmp = idxV[i];
        idxV[i] = idxV[i + 1];
        idxV[i + 1] = tmp;
        sorted = 0;
      }
  } while (!sorted);
}

extern char *nameByAuthorScores;
extern char *resultsAre;
extern char *resultsWLT;

void
results(outp)
  FILE   *outp;
{
  int     idxV[MAXWARRIOR], scrV[MAXWARRIOR];
  int     i, j;
  char    outs[80];
  long    saveW, saveS;

  eval_expr("W", &saveW);        /* save these registers if in use */
  eval_expr("S", &saveS);
  set_reg('W', (long) warriors);/* 'W' used in score calculation */

  for (i = 0; i < warriors; ++i) {
    idxV[i] = i;
    scrV[i] = score(i);
  }
  if (SWITCH_o)
    sort_by_score(idxV, scrV);
#if defined(DOSALLGRAPHX)
  if ((displayMode == TEXT) && !outp) {
    for (i = 0; i < warriors; ++i) {
      sprintf(outs, nameByAuthorScores, warrior[idxV[i]].name, warrior[idxV[i]].authorName,
              scrV[idxV[i]]);
      aputs5(outs, NORMAL_ATTR);
      if (warriors > 2) {
        aputs5(resultsAre, NORMAL_ATTR);
        for (j = 0; j < warriors; ++j) {
          sprintf(outs, " %d", warrior[idxV[i]].score[j]);
          aputs5(outs, NORMAL_ATTR);
        }
        sprintf(outs, " %d\n", deaths(idxV[i]));
        aputs5(outs, NORMAL_ATTR);
      }
    }
    if (warriors == 2) {
      sprintf(outs, resultsWLT, warrior[idxV[0]].score[0],
              warrior[idxV[0]].score[2], warrior[idxV[0]].score[1]);
      aputs5(outs, NORMAL_ATTR);
    }
  } else if ((displayMode == TEXT) || (outp != STDOUT)) {
    for (i = 0; i < warriors; ++i) {
      fprintf(outp, nameByAuthorScores, warrior[idxV[i]].name, warrior[idxV[i]].authorName,
              scrV[idxV[i]]);
      if (warriors > 2) {
        fprintf(outp, resultsAre);
        for (j = 0; j < warriors; ++j) {
          fprintf(outp, " %d", warrior[idxV[i]].score[j]);
        }
        fprintf(outp, " %d\n", deaths(idxV[i]));
      }
    }
    if (warriors == 2) {
      fprintf(outp, resultsWLT, warrior[idxV[0]].score[0],
              warrior[idxV[0]].score[2], warrior[idxV[0]].score[1]);
    }
  } else {
    for (i = 0; i < warriors; ++i) {
      sprintf(outs, nameByAuthorScores, warrior[idxV[i]].name, warrior[idxV[i]].authorName,
              scrV[idxV[i]]);
      grputs(outs);
      if (warriors > 2) {
        grputs(resultsAre);
        for (j = 0; j < warriors; ++j) {
          sprintf(outs, " %d", warrior[idxV[i]].score[j]);
          grputs(outs);
        }
        sprintf(outs, " %d\n", deaths(idxV[i]));
        grputs(outs);
      }
    }
    if (warriors == 2) {
      sprintf(outs, resultsWLT, warrior[idxV[0]].score[0],
              warrior[idxV[0]].score[2], warrior[idxV[0]].score[1]);
      grputs(outs);
    }
  }
#else
#if defined(DOSGRXGRAPHX) || defined(DOSTXTGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
#if defined(DOSGRXGRAPHX)
#define OUTTEXT(s) grputs(s)
#else
#if defined(LINUXGRAPHX)
#define OUTTEXT(s) svga_puts(s)
#else
#if defined(XWINGRAPHX)
#define OUTTEXT(s) xWin_puts(s)
#else
#define OUTTEXT(s) aputs5(s, NORMAL_ATTR)
#endif
#endif
#endif

  if (!outp) {
    for (i = 0; i < warriors; ++i) {
      sprintf(outs, nameByAuthorScores, warrior[idxV[i]].name, warrior[idxV[i]].authorName,
              scrV[idxV[i]]);
      OUTTEXT(outs);
      if (warriors > 2) {
        OUTTEXT(resultsAre);
        for (j = 0; j < warriors; ++j) {
          sprintf(outs, " %d", warrior[idxV[i]].score[j]);
          OUTTEXT(outs);
        }
        OUTTEXT("\n");
        sprintf(outs, " %d\n", deaths(idxV[i]));
        OUTTEXT(outs);
      }
    }
    if (warriors == 2) {
      sprintf(outs, resultsWLT, warrior[idxV[0]].score[0],
              warrior[idxV[0]].score[2], warrior[idxV[0]].score[1]);
      OUTTEXT(outs);
    }
  } else {
    for (i = 0; i < warriors; ++i) {
      fprintf(outp, nameByAuthorScores, warrior[idxV[i]].name, warrior[idxV[i]].authorName,
              scrV[idxV[i]]);
      if (warriors > 2) {
        fprintf(outp, resultsAre);
        for (j = 0; j < warriors; ++j) {
          fprintf(outp, " %d", warrior[idxV[i]].score[j]);
        }
        fprintf(outp, " %d\n", deaths(idxV[i]));
      }
    }
    if (warriors == 2) {
      fprintf(outp, resultsWLT, warrior[idxV[0]].score[0],
              warrior[idxV[0]].score[2], warrior[idxV[0]].score[1]);
    }
  }
#else
  for (i = 0; i < warriors; ++i) {
    fprintf(outp, nameByAuthorScores, warrior[idxV[i]].name, warrior[idxV[i]].authorName,
            scrV[idxV[i]]);
    if (warriors > 2) {
      fprintf(outp, resultsAre);
      for (j = 0; j < warriors; ++j) {
        fprintf(outp, " %d", warrior[idxV[i]].score[j]);
      }
      fprintf(outp, " %d\n", deaths(idxV[i]));
    }
  }
  if (warriors == 2) {
    fprintf(outp, resultsWLT, warrior[idxV[0]].score[0],
            warrior[idxV[0]].score[2], warrior[idxV[0]].score[1]);
  }
#endif
#endif
  set_reg('W', saveW);                /* restore previous value of register */
  set_reg('S', saveS);
}
