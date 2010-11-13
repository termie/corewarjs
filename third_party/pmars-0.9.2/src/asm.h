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
 * asm.h: header for asm.c
 * $Id: asm.h,v 1.1.1.1 2000/08/20 13:29:27 iltzu Exp $
 */

#define NONE      0
#define ADDRTOKEN 1
#define FSEPTOKEN 2
#define COMMTOKEN 3
#define MODFTOKEN 4
#define EXPRTOKEN 5
#define WSPCTOKEN 6
#define CHARTOKEN 7
#define NUMBTOKEN 8
#define APNDTOKEN 9
#define MISCTOKEN 10                /* unrecognized token */

#define sep_sym ','
#define com_sym ';'
#define mod_sym '.'
#define cat_sym '&'
#define cln_sym ':'

extern char addr_sym[], expr_sym[], spc_sym[];
extern char *opname[], *modname[];

#if defined(NEW_STYLE)
extern char *pstrdup(char *), *pstrcat(char *, char *), *pstrchr(char *, int);
extern uChar ch_in_set(uShrt, char *), skip_space(char *, uShrt);
extern uChar str_in_set(char *, char *s[]);
extern int get_token(char *, uChar *, char *);
extern void to_upper(char *);
#else
extern char *pstrdup(), *pstrcat(), *pstrchr();;
extern uChar ch_in_set(), skip_space();
extern uChar str_in_set();
extern int get_token();
extern void to_upper();
#endif
