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
 * disasm.c: functions to turn core cell data into printable string
 * $Id: disasm.c,v 1.1.1.1 2000/08/20 13:29:32 iltzu Exp $
 */

#include "global.h"
#include "asm.h"
#include "sim.h"

#define denormalize(x) \
    ((x) > coreSize / 2 ? ((int) (x) - coreSize) : (int) (x))

#ifdef DOS16
typedef mem_struct FAR mem_st;
#else
typedef mem_struct mem_st;
#endif

/* whether INITIALINST is displayed or not */
#define HIDE 0
#define SHOW 1

/* ******************************************************************** */

char   *
cellview(cell, outp, emptyDisp)
#ifdef DOS16
  mem_struct far *cell;
#else
  mem_struct *cell;
#endif
  char   *outp;
  int     emptyDisp;
{
  FIELD_T opcode, modifier;

  opcode = ((FIELD_T) (cell->opcode & 0xf8)) >> 3;
  modifier = (cell->opcode & 0x07);

  if ((emptyDisp == SHOW) || (cell->opcode != INITIALINST.opcode) ||
      (cell->A_mode != INITIALINST.A_mode) || (cell->A_value != INITIALINST.A_value) ||
      (cell->B_mode != INITIALINST.B_mode) || (cell->B_value != INITIALINST.B_value) ||
      (cell->debuginfo != INITIALINST.debuginfo))
    sprintf(outp, "%3s%c%-2s %c%6d, %c%6d %4s",
            opname[opcode],
            SWITCH_8 ? ' ' : '.', SWITCH_8 ? "  " : modname[modifier],
#ifdef NEW_MODES
            INDIR_A(cell->A_mode) ? addr_sym[INDIR_A_TO_SYM(cell->A_mode)] :
#endif
            addr_sym[cell->A_mode], denormalize(cell->A_value),
#ifdef NEW_MODES
            INDIR_A(cell->B_mode) ? addr_sym[INDIR_A_TO_SYM(cell->B_mode)] :
#endif
            addr_sym[cell->B_mode], denormalize(cell->B_value),
#ifdef SERVER
            "");
#else
            cell->debuginfo ? "T" : "");
#endif
  else
    *outp = 0;

  return (outp);
}

/* ******************************************************************** */

char   *
locview(loc, outp)
  ADDR_T  loc;
  char   *outp;
{
  char    buf[MAXALLCHAR];
  sprintf(outp, "%05d   %s\n", loc, cellview(memory + loc, buf, HIDE));
  return (outp);
}

/* ******************************************************************** */

void
disasm(cells, n, offset)
  mem_struct *cells;
  ADDR_T  n, offset;
{
  ADDR_T  i;
  char    buf[MAXALLCHAR];

  if (SHOW && !SWITCH_8 && (offset >= 0) && (offset < n))
    fprintf(STDOUT, "%-6s %3s%3s  %6s\n", "", "ORG", "", "START");

  for (i = 0; i < n; ++i)
    fprintf(STDOUT, "%-6s %s\n", i == offset ? "START" : "",
#ifdef DOS16
            cellview((mem_struct far *) cells + i, buf, SHOW));
#else
            cellview((mem_struct *) cells + i, buf, SHOW));
#endif

  if (SHOW && SWITCH_8 && (offset >= 0) && (offset < n))
    fprintf(STDOUT, "%-6s %3s%3s  %6s\n", "", "END", "", "START");
}
