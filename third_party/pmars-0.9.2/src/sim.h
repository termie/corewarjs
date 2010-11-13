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
 * sim.h: header for sim.c, cdb.c and display files
 * $Id: sim.h,v 1.1.1.1 2000/08/20 13:29:44 iltzu Exp $
 */

/*
 * IR=memory[progCnt]
 * for the currently executing warrior
 * progCnt is the current cell being executed (operand decoding has not
 * occured yet)
 * W->taskHead is the next instruction in the queue (after the current inst)
 * W->tasks is the number of processes running
 * for the other warrior:  W->nextWarrior
 * W->nextWarrior->taskHead is the next instruction in the queue
 * W->nextWarrior->tasks is the number of processes running
 */

#ifndef SIM_INCLUDED
#define SIM_INCLUDED
#endif

#ifdef DOSTXTGRAPHX
#define         HELP_PAGE 3
#define         CDB_PAGE 5
#define         CORE_PAGE 1
#define         DEF_PAGE 0
#define         NORMAL_ATTR 0x0700
#ifdef CURSESGRAPHX
#define         show_cursor()
#define         hide_cursor()
#else
#define         show_cursor() SetCursor(7,8)
#define         hide_cursor() SetCursor(-1,-1)
#endif
#endif
#ifdef DOSGRXGRAPHX
extern int bgiTextLines;
extern int printAttr;
#define         WAIT 1
#define         NOWAIT 0
#endif

#ifdef LINUXGRAPHX
extern int svgaTextLines;
extern int printAttr;
#define         WAIT 1
#define         NOWAIT 0
#endif

#ifdef XWINGRAPHX
extern int xWinTextLines;
extern int printAttr;
#define         WAIT 1
#define         NOWAIT 0
#endif

#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
#define         PC1_ATTR 1
#define         PC2_ATTR 2
#endif                                /* tells grputs that print_core() is printing
                                 * PC1 or 2 */

#if defined(DOSALLGRAPHX)
extern int displayMode;
#endif

#ifdef DOS16
#define FAR far
#else
#define FAR
#endif

extern int round;
extern long cycle;
extern ADDR_T progCnt;                /* program counter */
extern warrior_struct *W;        /* indicate which warrior is running */
extern char alloc_p;
extern int warriorsLeft;

extern ADDR_T FAR *endQueue;
extern mem_struct FAR *memory;
extern ADDR_T FAR *taskQueue;
extern warrior_struct *endWar;
extern U32_T totaltask;
