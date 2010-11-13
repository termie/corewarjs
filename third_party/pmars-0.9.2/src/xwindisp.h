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
 * xwindisp.h: user interface for X11 graphical displays
 * $Id: xwindisp.h,v 1.1.1.1 2000/08/20 13:29:50 iltzu Exp $
 */

#ifdef XWINGRAPHX

#ifndef XWINDISP_H
#define XWINDISP_H

/* extern prototype declaration */
#ifdef NEW_STYLE
extern void     xWin_open_graphics(void);
extern void     xWin_display_cycle(void);
extern void     xWin_display_clear(void);
extern void     xWin_display_read(int addr);
extern void     xWin_display_dec(int addr);
extern void     xWin_display_inc(int addr);
extern void     xWin_display_write(int addr);
extern void     xWin_display_exec(int addr);
extern void     xWin_display_spl(int warrior, int tasks);
extern void     xWin_display_dat(int addr, int warNum, int tasks);
#else
extern void     xWin_open_graphics();
extern void     xWin_display_cycle();
extern void     xWin_display_clear();
extern void     xWin_display_read();
extern void     xWin_display_dec();
extern void     xWin_display_inc();
extern void     xWin_display_write();
extern void     xWin_display_exec();
extern void     xWin_display_spl();
extern void     xWin_display_dat();
#endif

/* functional macros needed by sim.c */
#define display_die(warnum)
#define display_push(val)

#define display_init()  xWin_open_graphics()
#define display_clear() xWin_display_clear()
#define display_close() xWin_display_close(WAIT)
#define display_cycle() xWin_display_cycle()

#define display_read(addr) do { if (displayLevel > 3)\
  xWin_display_read(addr); } while(0)
#define display_write(addr) do { if (displayLevel > 1)\
  xWin_display_write(addr); } while(0)
#define display_dec(addr) do { if (displayLevel > 2)\
  xWin_display_dec(addr); } while (0)
#define display_inc(addr)  do { if (displayLevel > 2)\
  xWin_display_inc(addr); } while(0)
#define display_exec(addr) do { if (displayLevel > 0)\
  xWin_display_exec(addr); } while(0)
#define display_spl(warrior,tasks) xWin_display_spl(warrior,tasks)
#define display_dat(addr,warnum,tasks) xWin_display_dat(addr,warnum,tasks)

#endif				/* XWINDISP_H */
#endif				/* defined XWINGRAPHX */
