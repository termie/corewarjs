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
 * global.h: global header
 * $Id: global.h,v 1.5 2000/12/25 00:56:41 iltzu Exp $
 *
 * 10-23-98 Pentium optimized version 30% faster than the original
 *          Ken Espiritu   (kespirit@cesun1.ce.vt.edu)
 */

#include "config.h"
#include <stdio.h>
#ifndef INT_MAX
#include <limits.h>
#endif
#if defined(__STDC__) || defined(__MSDOS__)
#include <stdlib.h>
#endif

/* *********************************************************************
   System dependent definitions or declarations
   ********************************************************************* */

/* Generic Pointer type */
#if defined(NEW_STYLE)
typedef void *pointer_t;
#else
typedef char *pointer_t;
#endif

#if defined(DOS16)

/* ***** Turbo C FAR model ***** */
#if defined(__TURBOC__)

#include <alloc.h>
#define MALLOC(x)     malloc((size_t)(x))
#define REALLOC(x, y) realloc((pointer_t)(x), (size_t)(y))
#define FREE(x)       free((pointer_t)(x))

#endif                                /* __TURBOC__ */


/* ***** Microsoft/Quick C FAR model ***** */
#if defined(_MSC_VER)

/* Need to test these predefined macros */
#include <malloc.h>

#define MALLOC(x)     _fmalloc((size_t)(x))
#define REALLOC(x, y) _frealloc((pointer_t)(x), (size_t)(y))
#define FREE(x)       _ffree((pointer_t)(x))

#define DOSFARMODEL

#endif                                /* _MSC_VER */

#endif                                /* DOS16 */


/***** Others which don't have MALLOC() defined. *****/

#if !defined(MALLOC)

#define MALLOC(x)     malloc((size_t)(x))
#define REALLOC(x, y) realloc((pointer_t)(x), (size_t)(y))
#define FREE(x)       free((pointer_t)(x))

#endif                                /* ! MALLOC */

/***** Miscellaneous *****/

/* Some old compilers do not have NULL defined */
#ifndef NULL

#ifdef DOSFARMODEL
#define NULL 0L
#else
#define NULL 0
#endif                                /* DOSFARMODEL */

#endif                                /* NULL */

/* unsigned types (renamed to avoid conflict with possibly predefined types) */
typedef unsigned char uChar;
typedef unsigned short uShrt;
typedef unsigned long uLong;

/* FALSE, TRUE */
#ifndef TRUE
enum {
  FALSE, TRUE
};
#endif

/* global error output macro */
#ifdef MACGRAPHX
#define errout(s) macputs(s)
#else
#define errout(s) fputs(s,stderr)
#endif

/* ************************************************************************
   pmars global structures and definitions
   ************************************************************************ */

#if defined(__STDC__)  || defined(DOS16)
#define NEW_STYLE
#endif

/* Version and date */

#define PMARSVER  92
#define PMARSDATE "25/12/00"

#ifdef VMS                        /* Must change codes to work with VMS error
                                 * handling */
extern  PMARS_FATAL, PMARS_BADCOMLIN, PMARS_PARSEERR;
#define GRAPHERR   1                /* no grphx yet */
#define NOT386     1                /* if this error occurs, I don't wanna hear
                                 * about it. */
#define MEMERR   292                /* %SYS-E-INSFMEM */
#define SERIOUS  &PMARS_FATAL        /* %PMARS-F-FATAL */
#define FNOFOUND 98962                /* %RMS-E-FNF */
#define CLP_NOGOOD &PMARS_BADCOMLIN        /* %PMARS-E-BADCOMLIN */
#define PARSEERR &PMARS_PARSEERR/* ENDABORT */
#define USERABORT 44                /* %SYS-E-ABORT */
#else                                /* everyone else */
/* return code:
   0: success
   Negative number: System error such as insufficient space, etc
   Positive number: User error such as number too big, file not found, etc */
#define GRAPHERR  -4                /* graphic error */
#define NOT386    -3                /* trying to execute 386 code on lesser
                                 * machine */
#define MEMERR    -2                /* insufficient memory, cannot free, etc. */
#define SERIOUS   -1                /* program logic error */
#define FNOFOUND   1                /* File not found */
#define CLP_NOGOOD 2                /* command line argument error */
#define PARSEERR   3                /* File doesn't assemble correctly */
#define USERABORT  4                /* user stopped program from cdb, etc. */
#endif

/* these are used as return codes of internal functions */
#define SUCCESS    0
#define WARNING    0

/* used by eval.c */
#define OVERFLOW  1
#define OK_EXPR   0
#define BAD_EXPR -1
#define DIV_ZERO -2

/* used by cdb.c */
#define NOBREAK 0
#define BREAK   1
#define STEP    2

#define SKIP    1                /* cmdMod settings for cdb: skip next command */
#define RESET   2                /* clear command queue */

#define UNSHARED -1                /* P-space is private */
#define PIN_APPEARED -2

/* used by sim.c and asm.c */
#ifdef NEW_MODES
#define INDIR_A(x) (0x80 & (x))
#define RAW_MODE(x) (0x7F & (x))
#define SYM_TO_INDIR_A(x) ( ((x) - 3) | 0x80)        /* turns index into
                                                 * addr_sym[] to INDIR_A code */
#define INDIR_A_TO_SYM(x) ( RAW_MODE(x) + 3 )        /* vice versa */
#endif

/* used by many */
#define STDOUT stdout

#ifdef DOS16
#define MAXCORESIZE   8192
#else
#ifdef SMALLMEM
#define MAXCORESIZE   65535
#else
#define MAXCORESIZE   ((INT_MAX>>1)+1)
#endif
#endif

#define MAXTASKNUM    INT_MAX
#define MAXROUND      INT_MAX
#define MAXCYCLE      LONG_MAX
#ifdef DOS16
#define MAXWARRIOR         8
#else
#define MAXWARRIOR        36
#endif
#define MAXINSTR         1000

#define MAXSEPARATION MAXCORESIZE/MAXWARRIOR

#define MAXALLCHAR 256

/* The following holds the order in which opcodes, modifiers, and addr_modes
   are represented as in parser. The enumerated field should start from zero */
enum addr_mode {
  IMMEDIATE,                        /* # */
  DIRECT,                        /* $ */
  INDIRECT,                        /* @ */
  PREDECR,                        /* < */
  POSTINC                        /* > */
};

enum op {
  MOV, ADD, SUB, MUL, DIV, MOD, JMZ,
  JMN, DJN, CMP, SLT, SPL, DAT, JMP,
  SEQ,  SNE, NOP, LDP, STP
};                                /* has to match asm.c:opname[] */

enum modifier {
  mA,                                /* .A */
  mB,                                /* .B */
  mAB,                                /* .AB */
  mBA,                                /* .BA */
  mF,                                /* .F */
  mX,                                /* .X */
  mI                                /* .I */
};


#ifdef SMALLMEM
typedef unsigned short ADDR_T;
#define ISNEG(x) ((x)==0xFFFF)        /* use for unsigned ADDR_T */
#else
typedef int ADDR_T;
#define ISNEG(x) ((x)<0)
#endif

typedef unsigned char FIELD_T;
typedef unsigned long U32_T;        /* unsigned long (32 bits) */
typedef long S32_T;


/* Memory structure */
typedef struct mem_struct {
  ADDR_T  A_value, B_value;
  FIELD_T opcode;
  FIELD_T A_mode, B_mode;
  FIELD_T debuginfo;

}       mem_struct;

/* Warrior structure */
typedef struct warrior_struct {
  long    pSpaceIDNumber;
#ifdef DOS16
  ADDR_T far *taskHead, far * taskTail;
#else
  ADDR_T *taskHead, *taskTail;
#endif
  int     tasks;
  ADDR_T  lastResult;
  int     pSpaceIndex;
  ADDR_T  position;                /* load position in core */
  int     instLen;                /* Length of instBank */
  int     offset;                /* Offset value specified by 'ORG' or 'END'.
                                 * 0 is default */
  short   score[MAXWARRIOR * 2 - 1];

  char   *name;                        /* warrior name */
  char   *version;
  char   *date;
  char   *fileName;                /* file name */
  char   *authorName;                /* author name */
  mem_struct *instBank;

  struct warrior_struct *nextWarrior;

}       warrior_struct;

/* ***********************************************************************
   pmars global variable declarations
   *********************************************************************** */

extern int errorcode;
extern int errorlevel;
extern char errmsg[MAXALLCHAR];

/* Some parameters */
extern int warriors;
extern ADDR_T coreSize;
extern int taskNum;
extern ADDR_T instrLim;
extern ADDR_T separation;
extern int rounds;
extern long cycles;

extern int cmdMod;
extern S32_T seed;

extern int SWITCH_b;
extern int SWITCH_e;
extern int SWITCH_k;
extern int SWITCH_8;
extern int SWITCH_f;
extern ADDR_T SWITCH_F;
extern int SWITCH_V;
extern int SWITCH_o;
extern int SWITCH_Q;
extern char *SWITCH_eq;
#ifdef VMS
extern int SWITCH_D;
#endif
#ifdef PERMUTATE
extern int SWITCH_P;
#endif

extern int inCdb;
extern int debugState;
extern int copyDebugInfo;
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
extern int inputRedirection;
#endif
#if defined(XWINGRAPHX)
extern int xWinArgc;
extern char **xWinArgv;
#endif
extern mem_struct INITIALINST;        /* initialize to DAT.F $0,$0 */

extern warrior_struct warrior[MAXWARRIOR];
#ifdef DOS16
extern ADDR_T far *pSpace[MAXWARRIOR];
#else
extern ADDR_T *pSpace[MAXWARRIOR];
#endif
extern ADDR_T pSpaceSize;

/* ***********************************************************************
   display define's, declarations and typedefs
   *********************************************************************** */

#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)

#if !defined(LINUXGRAPHX)        /* vga.h already defines TEXT to be 0 */
#define         TEXT 0
#endif
#define         GRX 1
#define         SPEEDLEVELS 9
#define         NORMAL_ATTR 0x0700
extern int displayLevel;
extern int displayMode;
extern int displaySpeed;
extern int SWITCH_v;

#if defined(LINUXGRAPHX)        /* needed for correct keyboard handling */
extern struct termios tio_orig;
extern int console_fd;
#endif

#if defined(CURSESGRAPHX)
#include <curses.h>
#if defined(A_NORMAL) || defined(A_BOLD) || defined(A_REVERSE)
#define ATTRIBUTE
#endif                                /* A_* */

extern int refreshInterval;
extern int refIvalAr[SPEEDLEVELS];

#else

extern int keyDelay;
extern int keyDelayAr[SPEEDLEVELS];
extern unsigned long loopDelay;
extern unsigned long loopDelayAr[SPEEDLEVELS];

#endif                                /* CURSESGRAPHX */
#endif                                /* DOSTXTGRAPHX and DOSGRXGRAPHX and
                                 * LINUXGRAPHX */


/* ***********************************************************************
   function prototypes
   *********************************************************************** */

#ifdef NEW_STYLE

extern int
        parse_param(int argc, char *argv[]);
extern int eval_expr(char *expr, long *result);
extern int assemble(char *fName, int aWarrior);
extern void disasm(mem_struct * cells, ADDR_T n, ADDR_T offset);
extern void simulator1(void);
extern char *locview(ADDR_T loc, char *outp);
extern int cdb(char *msg);
extern int score(int warnum);
extern void sort_by_score(int *idxV, int *scrV);
extern int deaths(int warnum);
extern void results(FILE * outp);
extern void sort_by_score();
extern void Exit(int code);
extern void reset_regs(void);
extern void set_reg(char regChr, long val);

#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
extern void decode_vopt(int option);
#ifndef LINUXGRAPHX
extern void grputs(char *str);
extern void aputs5(char *str, int attribute);
#endif

#if defined(LINUXGRAPHX)
extern char *svga_gets(char *s, int maxstr);
extern void svga_puts(char *s);
extern void svga_display_close(int wait);
extern void svga_write_menu(void);
extern void svga_open_graphics(void);
extern void svga_clear(void);
extern void svga_update(int curPanel);
extern void svga_clear_arena(void);
extern int svga_getch(void);
#endif

#if defined(XWINGRAPHX)
extern void xWin_puts(char *s);
extern void xWin_write_menu(void);
extern void xWin_clear(void);
extern void xWin_update(int curPanel);
extern void xWin_display_close(int wait);
extern char *xWin_gets(char *s, int maxstr);
extern void xWin_resize(void);
#endif
#endif

#ifdef DOS16
extern char *cellview(mem_struct far * cell, char *outp, int emptyDisp);
#else
extern char *cellview(mem_struct * cell, char *outp, int emptyDisp);
#endif

#else

extern int
        parse_param();
extern int
        eval_expr();
extern int assemble();
extern void disasm();
extern void simulator1();
extern char *locview();
extern char *cellview();
extern int cdb();
extern int score();
extern int deaths();
extern void results();
extern void Exit();
extern void reset_regs();
extern void set_reg();

#if defined(CURSESGRAPHX)
extern void decode_vopt();
extern void aputs5();
#endif

#if defined (LINUXGRAPHX)
extern char *svga_gets();
extern void svga_puts();
extern void svga_display_close();
extern void svga_write_menu();
extern void svga_open_graphics();
extern void svga_clear();
extern void svga_update();
extern void svga_clear_arena();
extern int svga_getch();
#endif

#if defined(XWINGRAPHX)
extern void xWin_puts();
extern void xWin_write_menu();
extern void xWin_clear();
extern void xWin_update();
extern void xWin_display_close();
extern char *xWin_gets();
extern void xWin_resize();
#endif

#endif
