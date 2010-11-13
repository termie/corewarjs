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
 * $Id: token.c,v 1.1.1.1 2000/08/20 13:29:46 iltzu Exp $
 */

#include <ctype.h>
#include <string.h>

#include "global.h"
#include "asm.h"

/* **************************** Prototype ******************************** */

#ifdef NEW_STYLE
#define toupper_(x) (toupper(x))
#else
#define toupper_(x) (isalpha(x) && islower(x) ? toupper(x) : (x))
#endif

/* *************************** definitions ******************************* */

#define is_addr(ch)  pstrchr(addr_sym, (int) ch)
#define is_expr(ch)  pstrchr(expr_sym, (int) ch)
#define is_alpha(ch) (isalpha(ch) || (ch) == '_')

/* ********************************************************************** */

char   *
pstrchr(s, c)
  char   *s;
  int     c;
{
  do {
    if ((int) *s == c)
      return s;
  } while (*s++);

  return NULL;
}

/* ********************************************************************** */

char   *
pstrdup(s)
  char   *s;
{
  char   *p, *q;
  register int i;

  for (q = s, i = 0; *q; q++)
    i++;

  if ((p = (char *) MALLOC(sizeof(char) * (i + 1))) != NULL) {
    q = p;
    while (*s)
      *q++ = *s++;
    *q = '\0';
  }
  return p;
}

/* ********************************************************************** */

char   *
pstrcat(s1, s2)
  char   *s1, *s2;
{
  register char *p = s1;

  while (*p)
    p++;
  while (*s2)
    *p++ = *s2++;
  *p = '\0';

  return s1;
}

/* ********************************************************************** */

/* return src of char in charset. charset is a string */
uChar 
ch_in_set(c, s)
  uShrt   c;
  char   *s;
{
  char    cc;
  register char a;
  register uChar i;

  cc = (char) c;
  for (i = 0; ((a = s[i]) != '\0') && (a != cc); i++);
  return (i);
}

/* ********************************************************************** */

/*
 * return src of str in charset. charset is a string set. case is significant
 */
uChar 
str_in_set(str, s)
  char   *str, *s[];
{
  register uChar i;
  for (i = 0; *s[i] && strcmp(str, s[i]); i++);
  return (i);
}

/* ********************************************************************** */

/* return next char which is non-whitespace char */
uChar 
skip_space(str, i)
  char   *str;
  uShrt   i;
{
  register uChar idx;
  idx = (uChar) i;
  while (isspace(str[idx]))
    idx++;
  return (idx);
}

/* ********************************************************************** */

void 
to_upper(str)
  char   *str;
{
  while ((*str = toupper_(*str)) != '\0')
    str++;
}

/* ********************************************************************** */

/* Get token which depends on the first letter. token need to be allocated. */
int 
get_token(str, curIndex, token)
  char   *str, *token;
  uChar  *curIndex;
{
  register uChar src, dst = 0;
  register int ch;                /* int for ctype compliance */
  int     tokenType;

  src = skip_space(str, (uShrt) * curIndex);

  if (str[src])
    if (isdigit(ch = str[src])) {        /* Grab the whole digit */
      while (isdigit(str[src]))
        token[dst++] = str[src++];
      tokenType = NUMBTOKEN;
    }
  /* Grab the whole identifier. There would be special treatment to modifiers */
    else if (is_alpha(ch)) {
      for (; ((ch = str[src]) != '\0') && (is_alpha(ch) || isdigit(ch));)
        token[dst++] = str[src++];
      tokenType = CHARTOKEN;
    }
  /*
   * The following would accept only one single char. The order should
   * reflect the frequency of the symbols being used
   */
    else {
      /* Is operator symbol ? */
      if (is_expr(ch))
        tokenType = EXPRTOKEN;
      /* Is addressing mode symbol ? */
      else if (is_addr(ch))
        tokenType = ADDRTOKEN;
      /* Is concatenation symbol ? */
      /* currently force so that there is no double '&' */
      else if (ch == cat_sym)
        if (str[src + 1] == '&') {
          token[dst++] = str[src++];
          tokenType = EXPRTOKEN;
        } else
          tokenType = APNDTOKEN;
      /* comment symbol ? */
      else if (ch == com_sym)
        tokenType = COMMTOKEN;
      /* field separator symbol ? */
      else if (ch == sep_sym)
        tokenType = FSEPTOKEN;
      /* modifier symbol ? */
      else if (ch == mod_sym)
        tokenType = MODFTOKEN;
      else if ((ch == '|') && (str[src + 1] == '|')) {
        token[dst++] = str[src++];
        tokenType = EXPRTOKEN;
      } else
        tokenType = MISCTOKEN;

      token[dst++] = str[src++];
    }
  else
    tokenType = NONE;

  token[dst] = '\0';
  *curIndex = src;

  return (tokenType);
}
