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
 * alldisp.c: functions for combined DOS graphics/text mode display
 * $Id: alldisp.c,v 1.1.1.1 2000/08/20 13:29:25 iltzu Exp $
 */

void    display_init(void);
void    display_read(int addr);
void    display_close(void);

void
display_init()
{
  if (displayMode == TEXT)
    text_display_init();
  else
    bgi_display_init();
}

void
display_read(addr)
{
  if (displayLevel > 3)
    if (displayMode == TEXT)
      text_display_read(addr);
    else
      bgi_display_read(addr);
}


#define display_dec(addr)\
do {\
if (displayLevel > 2)\
if (displayMode==TEXT) text_display_dec(addr);\
  else bgi_display_dec(addr);\
} while (0)

#define display_inc(addr)\
do {\
if (displayLevel > 2)\
if (displayMode==TEXT) text_display_inc(addr);\
  else bgi_display_inc(addr);\
} while (0)

#define display_write(addr)\
do {\
if (displayLevel > 1)\
if (displayMode==TEXT) text_display_write(addr);\
  else bgi_display_write(addr);\
} while (0)

#define display_exec(addr)\
do {\
if (displayLevel > 0)\
if (displayMode==TEXT) text_display_exec(addr);\
  else bgi_display_exec(addr);\
} while (0)

#define display_spl(warrior,tasks) \
do {\
  if (displayMode==TEXT) {\
    if (displayLevel > 0)\
        text_display_spl(warrior,tasks);\
  } else bgi_display_spl(warrior,tasks);\
} while (0)

#define display_dat(addr,warNum,tasks) \
do {\
  if (displayMode==TEXT) {\
    if (displayLevel > 0)\
        text_display_dat(addr);\
  } else bgi_display_dat(addr,warNum,tasks);\
} while (0)

#define display_clear() \
do {\
if (displayMode==TEXT) text_display_clear();\
  else bgi_display_clear();\
} while (0)

void
display_close()
{
  if (displayMode == TEXT)
    text_display_close();
  else
    bgi_display_close(WAIT);
}
