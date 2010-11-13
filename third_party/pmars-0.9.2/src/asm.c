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
 * asm.c: assembler
 * $Id: asm.c,v 1.3 2000/12/25 00:49:07 iltzu Exp $
 *
 *    usage: int assemble(char *filename, FIELD_T warriornum);
 *     parameters:
 *      filename: name of file to be assembled
 *      warriornum: num of warrior[num]
 *     return:
 *      errorcode: its definitions are defined in global.h
 *     globals:
 *      if errorcode is not SUCCESS, errmsg[] contains the last error
 *      message and errorlevel is set
 */

#if defined(MPW)
#pragma segment Asm
#endif

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "global.h"
#include "asm.h"

#ifdef MACGRAPHX
extern void macputs(char *);
#endif

#define concat(a,b) (strlen(a)+strlen(b)<MAXALLCHAR?pstrcat((a),(b)):NULL)

/*
#define ASM_DEBUG
*/

/* *************************** external strings ************************** */

extern char *logicErr, *reference, *labelRefMsg, *groupLabel, *textMsg,
       *stackMsg, *labelMsg, *endOfChart, *afterPass, *instrMsg, *endOfPass,
       *unknown, *anonymous, *illegalAppendErr, *bufferOverflowErr, *badOffsetErr,
       *illegalConcatErr, *tooManyLabelsErr, *unopenedFORErr, *unclosedROFErr,
       *bad88FormatErr, *noInstErr, *tokenErr, *undefinedSymErr, *expectNumberErr,
       *syntaxErr, *tooManyInstrErr, *missingErr, *recursiveErr, *badExprErr,
       *missingAssertErr, *concatErr, *ignoreENDErr, *invalidAssertErr,
       *tooMuchStuffErr, *extraTokenErr, *improperPlaceErr, *invalid88Err,
       *incompleteOpErr, *redefinitionErr, *undefinedLabelErr, *discardLabelErr,
       *assertionFailErr, *tooManyMsgErr, *fileOpenErr, *fileReadErr, *notEnoughMemErr,
       *warning, *error, *inLine, *opcodeMsg, *modifierMsg, *aTerm, *bTerm,
       *currentAssertMsg, *currentFORMsg, *CURLINEErr, *paramCheckMsg,
       *errNumMsg, *warNumMsg, *duplicateMsg, *divZeroErr, *overflowErr;

#ifdef VMS                        /* VMS LSE strings ( "-D" option) */
extern char *StartDia, *Region, *AllLine, *LineQualifier, *Label, *LSEWarn,
       *LSEErr, *Message, *EndDia, *StartMod, *Opening, *EndMod;
#endif                                /* VMS */

/* ********************** macros + type definitions ********************** */

#define LOGICERROR do { fprintf(STDOUT, logicErr, __FILE__, __LINE__); \
                        Exit(PARSEERR); } while(0)

#define MEMORYERROR errprn(MLCERR, (line_st *) NULL, "")

typedef enum RType {
  RTEXT, RSTACK, RLABEL
}       RType;

typedef enum errType {
  BUFERR, M88ERR, TOKERR, SYNERR, SNFERR, F88ERR, NOPERR,
  EVLERR, EXPERR, RECERR, ANNERR, LINERR, APPERR, IGNORE,
  ZLNERR, NUMERR, IDNERR, ROFERR, FORERR, ERVERR, GRPERR,
  CHKERR, NASERR, BASERR, EXXERR, FNFERR, UDFERR, CATERR,
  DLBERR, OFSERR, DOEERR, DSKERR, MLCERR, DIVERR, OFLERR,
  MISC
}       errType;

typedef enum stateCol {
  S_OP, S_MOD_ADDR_EXP, S_MODF, S_ADDR_EXP_A,
  S_EXP_FS, S_ADDR_EXP_B, S_EXPR
}       stateCol;

typedef struct src_st {
  char   *src;
  uShrt   loc;
  struct src_st *nextsrc;
}       src_st;

typedef struct line_st {
  char   *vline;
  FIELD_T dbginfo;
  src_st *linesrc;
  struct line_st *nextline;
}       line_st;

typedef struct grp_st {
  char   *symn;
  struct grp_st *nextsym;
}       grp_st;

typedef struct ref_st {
  grp_st *grpsym;
  line_st *sline;
  uShrt   value, visit;
  RType   reftype;
  struct ref_st *nextref;
}       ref_st;

typedef struct err_st {
  uShrt   code, loc, num;
}       err_st;
/* ************************* some globals *************************** */

#ifdef NEW_MODES
char    addr_sym[] = {'#', '$', '@', '<', '>', '*', '{', '}', '\0'};
char    expr_sym[] =
{'(', ')', '/', '+', '-', '%', '!', '=', '\0'};
#else
char    addr_sym[] = {'#', '$', '@', '<', '>', '\0'};
char    expr_sym[] =
{'(', ')', '*', '/', '+', '-', '%', '!', '=', '\0'};
#endif

char   *opname[] =
{"MOV", "ADD", "SUB", "MUL", "DIV", "MOD", "JMZ",        /* op */
  "JMN", "DJN", "CMP", "SLT", "SPL", "DAT", "JMP",        /* op */
#ifdef NEW_OPCODES
  "SEQ", "SNE", "NOP",                /* ext op */
#endif
#ifdef PSPACE
  "LDP", "STP",
#endif
  "ORG", "END",                        /* pseudo-opcodes */
#ifdef SHARED_PSPACE
"PIN", ""};
#else
""};
#endif

char   *modname[] = {"A", "B", "AB", "BA", "F", "X", "I", ""};
#ifndef SERVER
char   *swname[] = {"DEBUG", "TRACE", "BREAK", "ASSERT", ""};
#else
char   *swname[] = {"ASSERT", ""};
#endif

#ifdef SHARED_PSPACE
#define PSEOPNUM 4                /* also PIN */
#else
#define PSEOPNUM 3
#endif                                /* SHARED_PSPACE */

#define OPNUM (sizeof(opname)/sizeof(opname[0])) - PSEOPNUM

#define ORGOP (OPNUM + 0)
#define ENDOP (OPNUM + 1)
#define PINOP (OPNUM + 2)
#define EQUOP (OPNUM + 3)        /* this has to be the last */

#define MODNUM ((sizeof(modname)/sizeof(modname[0])) - 1)
#define SWNUM  ((sizeof(swname)/sizeof(swname[0])) - 1)

#define ERRMAX   9                /* max error */
#define GRPMAX   7                /* max group */
#define LEXCMAX  50                /* max number of excess lines */

static char noassert;
static uChar errnum, warnum;        /* Number of error and warning */
static uChar symnum;

static ref_st *reftbl;
static grp_st *symtbl;
static src_st *srctbl;
static line_st *sline[2], *lline[2];
static err_st *errkeep;

static FIELD_T dbginfo;
static int dbgproceed;
static unsigned int curWarrior;
static unsigned int pass;
static int ierr;

static char buf[MAXALLCHAR], buf2[MAXALLCHAR];
static char token[MAXALLCHAR], outs[MAXALLCHAR];


#ifdef VMS
FILE   *dias;
#endif

/* ****************** required local prototypes ********************* */

#ifdef NEW_STYLE
static void textout(char *);
static void errprn(errType, line_st *, char *);
#else
static void textout(), errprn();
#endif

/* ***************** conforming local prototypes ******************** */

#ifdef NEW_STYLE
#ifndef SERVER
static void lineswitch(char *, uShrt, line_st *);
#endif
static int globalswitch(char *, uShrt, uShrt, uShrt);
static int trav2(char *, char *, int);
static int normalize(long);
static int blkfor(char *, char *);
static int equtbl(char *);
static int equsub(char *, char *, int, ref_st *);
static ref_st *lookup(char *);
static grp_st *addsym(char *, grp_st *);
static src_st *addlinesrc(char *, uShrt);
static void newtbl(void);
static void addpredef(char *, U32_T);
static void addpredefs(void);
static void addline(char *, src_st *, uShrt);
static void show_info(uShrt), show_lbl(void);
static void disposeline(line_st *), disposegrp(grp_st *);
static void disposetbl(ref_st *, ref_st *);
static void cleanmem(void);
static void nocmnt(char *);
static void automaton(char *, stateCol, mem_struct *);
static void dfashell(char *, mem_struct *);
static void expand(uShrt), encode(uShrt);
#else
#ifndef SERVER
static void lineswitch();
#endif
static int globalswitch(), trav2(), normalize();
static int blkfor(), equtbl(), equsub();
static ref_st *lookup();
static grp_st *addsym();
static src_st *addlinesrc();
static void newtbl(), addpredef(), addline();
static void addpredefs();
static void show_info(), show_lbl();
static void disposeline(), disposegrp(), disposetbl();
static void cleanmem(), nocmnt();
static void automaton(), dfashell(), expand(), encode();
#endif

/* ************************** Functions ***************************** */

static ref_st *
lookup(symn)
  char   *symn;
{
  ref_st *curtbl;
  grp_st *symtable;

  for (curtbl = reftbl; curtbl; curtbl = curtbl->nextref)
    for (symtable = curtbl->grpsym; symtable;
         symtable = symtable->nextsym)
      if (!strcmp(symtable->symn, symn))
        return curtbl;

  return NULL;
}

/* ******************************************************************* */

static void
newtbl()
{
  ref_st *curtbl;
  if ((curtbl = (ref_st *) MALLOC(sizeof(ref_st))) != NULL) {
    curtbl->grpsym = NULL;
    curtbl->sline = NULL;
    curtbl->visit = FALSE;        /* needed to detect recursive reference */
    curtbl->nextref = reftbl;
    reftbl = curtbl;
  } else
    MEMORYERROR;
}

/* ******************************************************************* */

static grp_st *
addsym(symn, curgroup)
  char   *symn;
  grp_st *curgroup;
{
  grp_st *symgrp;

  if ((symgrp = (grp_st *) MALLOC(sizeof(grp_st))) != NULL)
    if ((symgrp->symn = pstrdup(symn)) != NULL)
      symgrp->nextsym = curgroup;
    else {
      FREE(symgrp);
      MEMORYERROR;
    }

  return symgrp;
}

/* ******************************************************************* */

static void
addpredef(symn, value)
  char   *symn;
  U32_T   value;
{
  grp_st *lsymtbl = NULL;
  line_st *aline;

  lsymtbl = addsym(symn, lsymtbl);
  sprintf(token, "%lu", (unsigned long) value);
  newtbl();
  reftbl->grpsym = lsymtbl;
  reftbl->reftype = RTEXT;
  if (((aline = (line_st *) MALLOC(sizeof(line_st))) != NULL) &&
      ((aline->vline = pstrdup(token)) != NULL)) {
    aline->nextline = NULL;
    reftbl->sline = aline;
  } else
    MEMORYERROR;
}

/* ******************************************************************* */

static void
addpredefs()
{
  /* predefined constants */
  addpredef("CORESIZE", (U32_T) coreSize);
  addpredef("MAXPROCESSES", (U32_T) taskNum);
  addpredef("MAXCYCLES", (U32_T) cycles);
  addpredef("MAXLENGTH", (U32_T) instrLim);
  addpredef("MINDISTANCE", (U32_T) separation);
  addpredef("VERSION", (U32_T) PMARSVER);
  addpredef("WARRIORS", (U32_T) warriors);
  addpredef("ROUNDS", (U32_T) rounds);
#ifdef PSPACE
  addpredef("PSPACESIZE", (U32_T) pSpaceSize);
#endif
}

/* ******************************************************************* */

/* Add line, with sline and lline, it is possible to add line to multiple
   group of inst */
static void
addline(vline, src, lspnt)
  char   *vline;
  src_st *src;
  uShrt   lspnt;
{
  line_st *temp;
  if ((temp = (line_st *) MALLOC(sizeof(line_st))) != NULL)
    if ((temp->vline = pstrdup(vline)) != NULL) {
      temp->dbginfo = (dbginfo ? TRUE : FALSE);
      temp->linesrc = src;
      temp->nextline = NULL;
      if (sline[lspnt])                /* First come first serve */
        lline[lspnt] = lline[lspnt]->nextline = temp;
      else                        /* lline init depends on sline */
        sline[lspnt] = lline[lspnt] = temp;
    } else {
      FREE(temp);
      MEMORYERROR;
    }
}

/* ******************************************************************* */

static src_st *
addlinesrc(src, loc)
  char   *src;
  uShrt   loc;
{
  src_st *alinesrc;

  if ((alinesrc = (src_st *) MALLOC(sizeof(src_st))) == NULL)
    MEMORYERROR;
  else {
    alinesrc->src = pstrdup(src);
    alinesrc->loc = loc;
    alinesrc->nextsrc = srctbl;
    srctbl = alinesrc;
  }

  return alinesrc;
}

/* ******************************************************************* */

static void
disposeline(aline)
  line_st *aline;
{
  line_st *tmp;

  while ((tmp = aline) != NULL) {
    FREE(tmp->vline);
    aline = aline->nextline;
    FREE(tmp);
  }
}

/* ******************************************************************* */

static void
disposegrp(agrp)
  grp_st *agrp;
{
  grp_st *tmp;

  while ((tmp = agrp) != NULL) {
    FREE(tmp->symn);
    agrp = agrp->nextsym;
    FREE(tmp);
  }
}

/* ******************************************************************* */

static void
disposetbl(atbl, btbl)
  ref_st *atbl, *btbl;
{
  ref_st *tmp;

  while ((tmp = atbl) != btbl) {
    disposegrp(tmp->grpsym);
    atbl = atbl->nextref;
    FREE(tmp);
  }
}

/* ******************************************************************* */

/* clear all allocated mem */
static void
cleanmem()
{
  disposeline(sline[0]);
  disposeline(sline[1]);
  sline[0] = sline[1] = NULL;
  disposetbl(reftbl, NULL);
  reftbl = NULL;
  disposegrp(symtbl);
  symtbl = NULL;
  symnum = 0;
}

/* ******************************************************************* */

/* show symbol informations */
static void
show_lbl()
{
  ref_st *aref;
  grp_st *agrp;
  line_st *aline;

  textout(labelRefMsg);

  for (aref = reftbl; aref; aref = aref->nextref) {
    textout(groupLabel);
    buf[0] = '\0';
    if (aref->grpsym == NULL)
      textout("<NONE>\n");
    else {
      for (agrp = aref->grpsym; agrp; agrp = agrp->nextsym) {
        textout(agrp->symn);
        textout(" ");
      }
      textout("\n");
    }

    textout("Reference:\n");
    switch (aref->reftype) {
    case RTEXT:
      textout(textMsg);
      textout("\n");
      for (aline = aref->sline; aline; aline = aline->nextline) {
        textout(aline->vline);
        textout("\n");
      }
      textout("\n");
      break;
    case RSTACK:
      textout(stackMsg);
      sprintf(outs, "%5d\n\n", aref->value);
      textout(outs);
      break;
    case RLABEL:
      textout(labelMsg);
      sprintf(outs, "%5d\n\n", aref->value);
      textout(outs);
      break;
    }
  }

  textout(endOfChart);
}

/* ******************************************************************* */

/* Show information about the EQU processing */
static void
show_info(sspnt)
  uShrt   sspnt;
{
  line_st *aline;

  sprintf(outs, afterPass, pass);
  textout(outs);

  textout(instrMsg);

  for (aline = sline[sspnt]; aline; aline = aline->nextline) {
    sprintf(outs, "(%5d) %s\n", aline->linesrc->loc, aline->vline);
    textout(outs);
  }
  textout(endOfPass);
}

/* ******************************************************************* */

/* Remove trailing comment from str */
static void
nocmnt(str)
  char   *str;
{
  while (*str && (*str != com_sym))
    str++;
  *str = '\0';
}

/* ******************************************************************* */

static int
globalswitch(str, idx, loc, lspnt)
  char   *str;
  uShrt   idx, loc, lspnt;
{
  uChar   i;
  i = (uChar) idx;

  get_token(str, &i, token);
  to_upper(token);

  if (!strcmp(token, "REDCODE") && i == idx + 7)        /* no leading spaces */
    return -1;

  while (isspace(str[i]))
    i++;

  if (strcmp(token, "NAME") == 0) {
    FREE(warrior[curWarrior].name);
    if (str[i] == '\0')
      warrior[curWarrior].name = pstrdup(unknown);
    else
      warrior[curWarrior].name = pstrdup((char *) str + i);
  } else if (strcmp(token, "AUTHOR") == 0) {
    FREE(warrior[curWarrior].authorName);
    if (str[i] == '\0')
      warrior[curWarrior].authorName = pstrdup(anonymous);
    else
      warrior[curWarrior].authorName = pstrdup((char *) str + i);
  } else if (strcmp(token, "DATE") == 0) {
    FREE(warrior[curWarrior].date);
    if (str[i] == '\0')
      warrior[curWarrior].date = pstrdup("");
    else
      warrior[curWarrior].date = pstrdup((char *) str + i);
  } else if (strcmp(token, "VERSION") == 0) {
    FREE(warrior[curWarrior].version);
    if (str[i] == '\0')
      warrior[curWarrior].version = pstrdup("");
    else
      warrior[curWarrior].version = pstrdup((char *) str + i);
  } else if (str_in_set(token, swname) < SWNUM) {
    nocmnt(str + i);                /* don't remove first comment */
    addline(str, addlinesrc(str, loc), lspnt);
  }
  return 0;
}

/* ******************************************************************* */

#ifndef SERVER
static void
lineswitch(str, idx, aline)        /* line switch */
  char   *str;
  uShrt   idx;
  line_st *aline;
{
  uChar   i;
  i = (uChar) idx;
  get_token(str, &i, token);
  to_upper(token);

  if (strcmp(token, "DEBUG") == 0) {
    get_token(str, &i, token);
    to_upper(token);
    if ((dbgproceed = strcmp(token, "OFF")) != 0) {
      debugState = BREAK;
      if ((copyDebugInfo = strcmp(token, "STATIC") ? TRUE : FALSE) == TRUE)
        if (*token)
          errprn(IGNORE, aline, str);
    }
  } else if (strcmp(token, "TRACE") == 0) {
    if (dbgproceed) {
      get_token(str, &i, token);
      to_upper(token);
      if ((dbginfo = strcmp(token, "OFF") ? TRUE : FALSE) == TRUE)
        if (*token)
          errprn(IGNORE, aline, str);
    }
  } else if (strcmp(token, "BREAK") == 0) {
    if (dbgproceed) {
      get_token(str, &i, token);
      dbginfo = 2;
      if (*token)
        errprn(IGNORE, aline, str);
    }
  }
}
#endif

/* ******************************************************************* */

/* stst && wangsawm v0.4.0: output for different displays */
static void
textout(str)
  char   *str;
{
#ifdef MACGRAPHX
  macputs(str);
#else
  if (!inCdb)
    fprintf(stderr, str);
#if defined DOSALLGRAPHX
  else {
    if (displayMode == TEXT)
      aputs5(str, NORMAL_ATTR);
    else
      grputs(str);
  }
#else
#if defined DOSTXTGRAPHX
  else
    aputs5(str, NORMAL_ATTR);
#else
#if defined DOSGRXGRAPHX
  else
    grputs(str);
#else
#if defined LINUXGRAPHX
  else
    svga_puts(str);
#else
#if defined XWINGRAPHX
  else
    xWin_puts(str);
#else                                /* no display */
  else
    fprintf(stderr, str);
#endif                                /* XWINGRAPHX */
#endif                                /* LINUXGRAPHX */
#endif                                /* DOSGRXGRAPHX */
#endif                                /* DOSTXTGRAPHX */
#endif                                /* DOSALLGRAPHX */
#endif                                /* MACGRAPHX */
}

/* ******************************************************************* */

static void
errprn(code, aline, arg)
  errType code;
  line_st *aline;
  char   *arg;
{
  char    abuf[MAXALLCHAR];

  errorcode = PARSEERR;
  errorlevel = SERIOUS;

  switch (code) {
  case ANNERR:
    strcpy(abuf, illegalAppendErr);
    errorlevel = WARNING;
    break;
  case BUFERR:
    strcpy(abuf, bufferOverflowErr);
    break;
  case ERVERR:
    sprintf(abuf, illegalConcatErr, arg);
    break;
  case GRPERR:
    sprintf(abuf, tooManyLabelsErr, arg);
    break;
  case FORERR:
    strcpy(abuf, unopenedFORErr);
    break;
  case ROFERR:
    strcpy(abuf, unclosedROFErr);
    errorlevel = WARNING;
    break;
  case M88ERR:
    sprintf(abuf, bad88FormatErr, arg);
    break;
  case ZLNERR:
    strcpy(abuf, noInstErr);
    errorlevel = WARNING;
    break;
  case DLBERR:
    sprintf(abuf, discardLabelErr, arg);
    errorlevel = WARNING;
    break;
  case TOKERR:
    sprintf(abuf, tokenErr, arg);
    break;
  case SNFERR:
    sprintf(abuf, undefinedSymErr, arg);
    break;
  case NUMERR:
    strcpy(abuf, expectNumberErr);
    break;
  case SYNERR:
    strcpy(abuf, syntaxErr);
    break;
  case LINERR:
    sprintf(abuf, tooManyInstrErr, arg);
    break;
  case EXPERR:
    sprintf(abuf, missingErr, arg);
    break;
  case RECERR:
    sprintf(abuf, recursiveErr, arg);
    break;
  case EVLERR:
    strcpy(abuf, badExprErr);
    break;
  case DIVERR:
    strcpy(abuf, divZeroErr);
    break;
  case OFLERR:
    strcpy(abuf, overflowErr);
    errorlevel = WARNING;
    break;
  case NASERR:
    strcpy(abuf, missingAssertErr);
    errorlevel = WARNING;
    break;
  case OFSERR:
    strcpy(abuf, badOffsetErr);
    errorlevel = WARNING;
    break;
  case CATERR:
    sprintf(abuf, concatErr, arg);
    break;
  case DOEERR:
    strcpy(abuf, ignoreENDErr);
    errorlevel = WARNING;
    break;
  case BASERR:
    strcpy(abuf, invalidAssertErr);
    errorlevel = WARNING;
    break;
  case EXXERR:
    sprintf(abuf, tooMuchStuffErr, MAXINSTR);
    break;
  case IGNORE:
    sprintf(abuf, extraTokenErr, arg);
    errorlevel = WARNING;
    break;
  case APPERR:
    sprintf(abuf, improperPlaceErr, arg);
    break;
  case F88ERR:
    sprintf(abuf, invalid88Err, arg);
    break;
  case NOPERR:
    sprintf(abuf, incompleteOpErr, arg);
    break;
  case IDNERR:
    sprintf(abuf, redefinitionErr, arg);
    errorlevel = WARNING;
    break;
  case UDFERR:
    sprintf(abuf, undefinedLabelErr, arg);
    errorlevel = WARNING;
    break;
  case CHKERR:
    strcpy(abuf, assertionFailErr);
    break;
  case MISC:
    strcpy(abuf, arg);
    break;
  case FNFERR:
    sprintf(abuf, fileOpenErr, arg);
    break;
  case DSKERR:
    sprintf(abuf, fileReadErr, arg);
    break;
  case MLCERR:
    cleanmem();                        /* refresh memory for fprintf and system */
#ifdef __MAC__
    textout(notEnoughMemErr);
#else
    fprintf(stderr, notEnoughMemErr);
#endif
    Exit(MEMERR);
    break;
  }

  if (errorlevel == WARNING)
    warnum++;
  else
    errnum++;

  if (aline) {

    int     i = 0;

    if (aline->linesrc == NULL)
      LOGICERROR;

    while ((i < ierr) && ((errkeep[i].loc != aline->linesrc->loc) ||
                          (errkeep[i].code != code)))
      i++;

    if (i == ierr) {
      sprintf(outs, "%s", errorlevel == WARNING ? warning : error);
#ifndef VMS
      textout(outs);
#else                                /* if defined(VMS) */
      if (!SWITCH_D)
        textout(outs);
      else {
        fprintf(dias, "%s", StartDia);
        fprintf(dias, "%s %s %s %s %d %s\"%s\"\n", Region,
                warrior[curWarrior].fileName, AllLine, LineQualifier, aline->
             linesrc->loc, Label, errorlevel == WARNING ? LSEWarn : LSEErr);
      }

      if (!SWITCH_D) {
#endif
        sprintf(outs, inLine, aline->linesrc->loc, aline->linesrc->src);
        textout(outs);
        sprintf(outs, "        %s\n", abuf);
        textout(outs);
#ifdef VMS
      } else {
        fprintf(dias, "%s \"%s\"\n", Message, abuf);
        fprintf(dias, "%s", EndDia);
      }
#endif
      errkeep[ierr].num = 1;
      errkeep[ierr].loc = aline->linesrc->loc;
      errkeep[ierr++].code = code;
    } else
      errkeep[i].num++;
  } else {
    sprintf(outs, "%s:\n",
            errorlevel == WARNING ? warning : error);

#ifndef VMS
    textout(outs);
#else
    if (!SWITCH_D)
      textout(outs);
    else {
      fprintf(dias, "%s", StartDia);
      fprintf(dias, "%s %s %s %s\"%s\"\n", Region,
              warrior[curWarrior].fileName, AllLine, Label,
              errorlevel == WARNING ? LSEWarn : LSEErr);
    }
#endif
    sprintf(outs, "        %s\n", abuf);
#ifndef VMS
    textout(outs);
#else
    if (!SWITCH_D)
      textout(outs);
    else {
      fprintf(dias, "%s \"%s\"\n", Message, abuf);
      fprintf(dias, "%s", EndDia);
    }
#endif
  }

  if (ierr >= ERRMAX) {
    sprintf(outs, tooManyMsgErr);
#ifndef VMS
    textout(outs);
#else
    if (!SWITCH_D)
      textout(outs);
    else {
      fprintf(dias, "%s", StartDia);
      fprintf(dias, "Message \"%s\"\n", Message, outs);
      fprintf(dias, "%s", EndDia);
    }
#endif

    Exit(PARSEERR);

  }
  strcpy(errmsg, abuf);
}

/* ******************************************************************* */

#define A_expr buf
#define B_expr buf2
#define vallen 8
static uChar opcode, modifier;
static uChar statefine;
static stateCol laststate;

static uShrt line, linemax;
static uChar vcont;
static uShrt dspnt;
static line_st *aline;

/* ******************************************************************* */

/* Here is the automaton */
static void
automaton(expr, state, cell)
  char   *expr;
  stateCol state;
  mem_struct *cell;
{
  uChar   idx = 0;
  char   *tmp;
  ref_st *atbl;

  switch (laststate = state) {

  case S_OP:
    statefine = FALSE;
    if (get_token(expr, &idx, token) == CHARTOKEN) {
      to_upper(token);
      if ((opcode = str_in_set(token, opname)) < OPNUM)
        automaton((char *) expr + idx, S_MOD_ADDR_EXP, cell);
      else if (opcode < EQUOP)
        automaton((char *) expr + idx, S_EXPR, cell);
      else
/* This should be toggled as error. No label should appear at this point */
        LOGICERROR;
    } else
      errprn(EXPERR, aline, opcodeMsg);
    break;

  case S_MOD_ADDR_EXP:
    statefine = FALSE;
    switch (get_token(expr, &idx, token)) {
    case MODFTOKEN:
      automaton((char *) expr + idx, S_MODF, cell);
      break;
    case ADDRTOKEN:
      cell->A_mode = (FIELD_T) ch_in_set(*token, addr_sym);
#ifdef NEW_MODES
      if (cell->A_mode > 4) {
        if (SWITCH_8)
          errprn(M88ERR, aline, "[*{}]");
        else
          cell->A_mode = SYM_TO_INDIR_A(cell->A_mode);
      }
#endif
      automaton((char *) expr + idx, S_EXP_FS, cell);
      break;
    case NUMBTOKEN:
    case EXPRTOKEN:
      automaton(expr, S_EXP_FS, cell);
      break;
    case CHARTOKEN:
      if ((atbl = lookup(token)) != NULL)
        if (atbl->visit)
          errprn(RECERR, aline, token);
        else if (atbl->reftype == RTEXT)
          if (atbl->sline) {
            atbl->visit = TRUE;
            automaton(atbl->sline->vline, S_MOD_ADDR_EXP, cell);
            atbl->visit = FALSE;
            automaton((char *) expr + idx, laststate, cell);
          } else
/* all slines should've been filled */
            LOGICERROR;
        else if ((tmp = (char *) MALLOC(sizeof(char) * vallen)) != NULL) {
          sprintf(tmp, "%d", (int) atbl->value - line);
          atbl->visit = TRUE;
          automaton(tmp, S_MOD_ADDR_EXP, cell);
          FREE(tmp);
          atbl->visit = FALSE;
          automaton((char *) expr + idx, laststate, cell);
        } else
          MEMORYERROR;
      else if (!token[1])
        automaton(expr, S_EXP_FS, cell);
      else
        errprn(SNFERR, aline, token);
      break;
    case NONE:
      break;
    default:
      errprn(APPERR, aline, token);
    }
    break;

  case S_MODF:
    statefine = FALSE;
    switch (get_token(expr, &idx, token)) {
    case CHARTOKEN:
      to_upper(token);
      if ((modifier = str_in_set(token, modname)) < MODNUM)
        automaton((char *) expr + idx, S_ADDR_EXP_A, cell);
      else
        errprn(EXPERR, aline, modifierMsg);
      break;
    case NONE:
      break;
    default:
      errprn(EXPERR, aline, modifierMsg);
    }
    break;

  case S_ADDR_EXP_A:
    statefine = FALSE;
    switch (get_token(expr, &idx, token)) {
    case ADDRTOKEN:
      cell->A_mode = (FIELD_T) ch_in_set(*token, addr_sym);
#ifdef NEW_MODES
      if (cell->A_mode > 4) {
        if (SWITCH_8)
          errprn(M88ERR, aline, "[*{}]");
        else
          cell->A_mode = SYM_TO_INDIR_A(cell->A_mode);
      }
#endif
      automaton((char *) expr + idx, S_EXP_FS, cell);
      break;
    case NUMBTOKEN:
    case EXPRTOKEN:
      automaton(expr, S_EXP_FS, cell);
      break;
    case CHARTOKEN:
      if ((atbl = lookup(token)) != NULL)
        if (atbl->visit)
          errprn(RECERR, aline, token);
        else if (atbl->reftype == RTEXT)
          if (atbl->sline) {
            atbl->visit = TRUE;
            automaton(atbl->sline->vline, S_ADDR_EXP_A, cell);
            atbl->visit = FALSE;
            automaton((char *) expr + idx, laststate, cell);
          } else
/* all slines should've been filled */
            LOGICERROR;
        else if ((tmp = (char *) MALLOC(sizeof(char) * vallen)) != NULL) {
          sprintf(tmp, "%d", (int) atbl->value - line);
          atbl->visit = TRUE;
          automaton(tmp, S_ADDR_EXP_A, cell);
          FREE(tmp);
          atbl->visit = FALSE;
          automaton((char *) expr + idx, laststate, cell);
        } else
          MEMORYERROR;
      else if (!token[1])
        automaton(expr, S_EXP_FS, cell);
      else
        errprn(SNFERR, aline, token);
      break;
    case NONE:
      break;
    default:
      errprn(EXPERR, aline, aTerm);
    }
    break;

  case S_EXP_FS:
    switch (get_token(expr, &idx, token)) {
    case FSEPTOKEN:
      automaton((char *) expr + idx, S_ADDR_EXP_B, cell);
      break;
    case ADDRTOKEN:
      if ((token[0] != '>') && (token[0] != '<') && (token[0] != '*'))
        errprn(APPERR, aline, token);
      else {
        if (!concat(A_expr, token))
          errprn(BUFERR, aline, "");
        statefine = TRUE;
        automaton((char *) expr + idx, S_EXP_FS, cell);
      }
      break;
    case NUMBTOKEN:
      if (!concat(token, " "))
        errprn(BUFERR, aline, "");
      /* FALLTHROUGH */
    case EXPRTOKEN:
      if (!concat(A_expr, token))
        errprn(BUFERR, aline, "");
      statefine = TRUE;
      automaton((char *) expr + idx, S_EXP_FS, cell);
      break;
    case CHARTOKEN:
      if ((atbl = lookup(token)) != NULL)
        if (atbl->visit)
          errprn(RECERR, aline, token);
        else if (atbl->reftype == RTEXT)
          if (atbl->sline) {
            atbl->visit = TRUE;
            automaton(atbl->sline->vline, S_EXP_FS, cell);
            atbl->visit = FALSE;
            automaton((char *) expr + idx, laststate, cell);
          } else
/* all slines should've been filled */
            LOGICERROR;
        else if ((tmp = (char *) MALLOC(sizeof(char) * vallen)) != NULL) {
          sprintf(tmp, "%d", (int) atbl->value - line);
          atbl->visit = TRUE;
          automaton(tmp, S_EXP_FS, cell);
          FREE(tmp);
          atbl->visit = FALSE;
          automaton((char *) expr + idx, laststate, cell);
        } else
          MEMORYERROR;
      else if (!token[1])        /* check if it's a register. */
        if (concat(A_expr, token)) {
          statefine = TRUE;
          automaton((char *) expr + idx, S_EXP_FS, cell);
        } else
          errprn(BUFERR, aline, "");
      else
        errprn(SNFERR, aline, token);
      break;
    case NONE:
      break;
    default:
      errprn(APPERR, aline, token);
    }
    break;

  case S_ADDR_EXP_B:
    statefine = FALSE;
    switch (get_token(expr, &idx, token)) {
    case ADDRTOKEN:
      cell->B_mode = (FIELD_T) ch_in_set(*token, addr_sym);
#ifdef NEW_MODES
      if (cell->B_mode > 4) {
        if (SWITCH_8)
          errprn(M88ERR, aline, "[*{}]");
        else
          cell->B_mode = SYM_TO_INDIR_A(cell->B_mode);
      }
#endif
      automaton((char *) expr + idx, S_EXPR, cell);
      break;
    case NUMBTOKEN:
    case EXPRTOKEN:
      automaton(expr, S_EXPR, cell);
      break;
    case CHARTOKEN:
      if ((atbl = lookup(token)) != NULL)
        if (atbl->visit)
          errprn(RECERR, aline, token);
        else if (atbl->reftype == RTEXT)
          if (atbl->sline) {
            atbl->visit = TRUE;
            automaton(atbl->sline->vline, S_ADDR_EXP_B, cell);
            atbl->visit = FALSE;
            automaton((char *) expr + idx, laststate, cell);
          } else
/* all slines should've been filled */
            LOGICERROR;
        else if ((tmp = (char *) MALLOC(sizeof(char) * vallen)) != NULL) {
          sprintf(tmp, "%d", (int) atbl->value - line);
          atbl->visit = TRUE;
          automaton(tmp, S_ADDR_EXP_B, cell);
          FREE(tmp);
          atbl->visit = FALSE;
          automaton((char *) expr + idx, laststate, cell);
        } else
          MEMORYERROR;
      else if (!token[1])
        automaton(expr, S_EXPR, cell);
      else
        errprn(SNFERR, aline, token);
      break;
    case NONE:
      break;
    default:
      errprn(EXPERR, aline, bTerm);
    }
    break;

  case S_EXPR:
    switch (get_token(expr, &idx, token)) {
    case ADDRTOKEN:
      if ((token[0] != '>') && (token[0] != '<') && (token[0] != '*'))
        errprn(APPERR, aline, token);
      else {
        if (!concat(B_expr, token))
          errprn(BUFERR, aline, "");
        statefine = TRUE;
        automaton((char *) expr + idx, S_EXPR, cell);
      }
      break;
    case NUMBTOKEN:
      if (!concat(token, " "))
        errprn(BUFERR, aline, "");
      /* FALLTHROUGH */
    case EXPRTOKEN:
      if (!concat(B_expr, token))
        errprn(BUFERR, aline, "");
      statefine = TRUE;
      automaton((char *) expr + idx, S_EXPR, cell);
      break;
    case CHARTOKEN:
      if ((atbl = lookup(token)) != NULL)
        if (atbl->visit)
          errprn(RECERR, aline, token);
        else if (atbl->reftype == RTEXT)
          if (atbl->sline) {
            atbl->visit = TRUE;
            automaton(atbl->sline->vline, S_EXPR, cell);
            atbl->visit = FALSE;
            automaton((char *) expr + idx, S_EXPR, cell);
          } else
/* all slines should've been filled */
            LOGICERROR;
        else if ((tmp = (char *) MALLOC(sizeof(char) * vallen)) != NULL) {
          if (opcode < OPNUM)
            sprintf(tmp, "%d", (int) atbl->value - line);
          else
            sprintf(tmp, "%d", (int) atbl->value);
          atbl->visit = TRUE;
          automaton(tmp, S_EXPR, cell);
          FREE(tmp);
          atbl->visit = FALSE;
          automaton((char *) expr + idx, S_EXPR, cell);
        } else
          MEMORYERROR;
      else if (!token[1])        /* check for register use */
        if (concat(B_expr, token)) {
          statefine = TRUE;
          automaton((char *) expr + idx, S_EXPR, cell);
        } else
          errprn(BUFERR, aline, "");
      else
        errprn(SNFERR, aline, token);
      break;
    case NONE:
      break;
    default:
      errprn(APPERR, aline, token);
    }
    break;
  }
}

/* ******************************************************************* */

static void
dfashell(expr, cell)
  char   *expr;
  mem_struct *cell;
{
  cell->A_mode = (FIELD_T) ch_in_set('$', addr_sym);
  cell->B_mode = (FIELD_T) ch_in_set('$', addr_sym);
  modifier = MODNUM;                /* marked as not used */
  A_expr[0] = B_expr[0] = '\0';

  errorcode = SUCCESS;
  automaton(expr, S_OP, cell);

  if (opcode < OPNUM) {

/* This is also represented in a NONE case in automaton function call */
    if ((statefine == FALSE) && (errorcode == SUCCESS))
      errprn(SYNERR, aline, opname[opcode]);

    else if (B_expr[0] == '\0')        /* If there's only one argument */
      switch (opcode) {
      case DAT:                /* The default in DAT statement is #0 */
        cell->B_mode = cell->A_mode;
        strcpy(B_expr, A_expr);
        cell->A_mode = (FIELD_T) ch_in_set('#', addr_sym);
        strcpy(A_expr, "0");
        break;
      case SPL:                /* The default in SPL/JMP statement is $0 */
      case JMP:
#ifdef NEW_OPCODES
      case NOP:
#endif
        cell->B_mode = (FIELD_T) ch_in_set('$', addr_sym);
        strcpy(B_expr, "0");
        break;
      default:
        errprn(NOPERR, aline, opname[opcode]);        /* opcode < OPNUM */
      }

    if (SWITCH_8) {
      switch (opcode) {
      case DAT:
        if (((cell->A_mode != (FIELD_T) IMMEDIATE) &&
             (cell->A_mode != (FIELD_T) PREDECR)) ||
            ((cell->B_mode != (FIELD_T) IMMEDIATE) &&
             (cell->B_mode != (FIELD_T) PREDECR)))
          errprn(F88ERR, aline, "DAT [#<] [#<]");
        break;
      case MOV:
      case ADD:
      case SUB:
      case CMP:
        if (cell->B_mode == (FIELD_T) IMMEDIATE) {
          sprintf(token, "%s [#$@<] [$@<]", opname[opcode]);
          errprn(F88ERR, aline, token);
        }
        break;
      case JMP:
      case JMZ:
      case JMN:
      case DJN:
      case SPL:
        if (cell->A_mode == (FIELD_T) IMMEDIATE) {
          sprintf(token, "%s [$@<] [#$@<]", opname[opcode]);
          errprn(F88ERR, aline, token);
        }
        break;
      case MUL:
      case DIV:
      case MOD:
#ifdef NEW_OPCODES
      case SEQ:
      case SNE:
      case NOP:
#endif
#ifdef PSPACE
      case LDP:
      case STP:
#endif
        errprn(M88ERR, aline, opname[opcode]);
      }

      if (modifier != MODNUM)
        errprn(M88ERR, aline, ".");        /* These two should be declared in
                                         * label */
      if ((cell->A_mode == (FIELD_T) POSTINC) ||
          (cell->B_mode == (FIELD_T) POSTINC))
        errprn(M88ERR, aline, ">");
    }
    if (modifier == MODNUM)        /* no modifier, pick default */
      switch (opcode) {
      case DAT:
#ifdef NEW_OPCODES
      case NOP:
#endif
        modifier = (FIELD_T) mF;
        break;
      case MOV:
      case CMP:
#ifdef NEW_OPCODES
      case SEQ:
      case SNE:
#endif
        if (cell->A_mode == (FIELD_T) IMMEDIATE)
          modifier = (FIELD_T) mAB;
        else if (cell->B_mode == (FIELD_T) IMMEDIATE)
          modifier = (FIELD_T) mB;
        else
          modifier = (FIELD_T) mI;
        break;
      case ADD:
      case SUB:
      case MUL:
      case DIV:
      case MOD:
        if (cell->A_mode == (FIELD_T) IMMEDIATE)
          modifier = (FIELD_T) mAB;
        else if (cell->B_mode == (FIELD_T) IMMEDIATE)
          modifier = (FIELD_T) mB;
        else
          modifier = (FIELD_T) mF;
        break;
#ifdef PSPACE
      case LDP:
      case STP:
#endif
      case SLT:
        if (cell->A_mode == (FIELD_T) IMMEDIATE)
          modifier = (FIELD_T) mAB;
        else
          modifier = (FIELD_T) mB;
        break;
      default:
        modifier = (FIELD_T) mB;
      }
    cell->opcode = (FIELD_T) (opcode << 3) + modifier;
  }
}

/* ******************************************************************* */

static int
normalize(value)
  long    value;
{
  while (value >= (long) coreSize)
    value -= (long) coreSize;
  while (value < 0)
    value += (long) coreSize;
  return ((int) value);
}

/* ******************************************************************* */
/* assemble into instBank */

static void
encode(sspnt)
  uShrt   sspnt;
{
  int evalerrA, evalerrB;
  long    resultA, resultB;
  mem_struct *base;

  if (line <= MAXINSTR) {

    if (line > (uShrt) instrLim) {
      sprintf(buf, "%d", (int) line - instrLim);
      errprn(LINERR, (line_st *) NULL, buf);
    }
    warrior[curWarrior].instLen = line;

    if (line)
      if ((warrior[curWarrior].instBank = base = (mem_struct *)
           MALLOC((line + 1) * sizeof(mem_struct))) != NULL) {
        for (aline = sline[sspnt], line = 0; aline; aline = aline->nextline) {
          dfashell(aline->vline, (mem_struct *) base + line);
          if (errnum == 0) {
            if (*A_expr == '\0')
              strcpy(A_expr, "0");
            if (*B_expr == '\0')
              strcpy(B_expr, "0");

            if ((opcode == ENDOP) || (opcode == ORGOP)
#ifdef SHARED_PSPACE
                || (opcode == PINOP)
#endif
              )
              if ((evalerrA = eval_expr(B_expr, &resultB)) < OK_EXPR) {
                if (evalerrA == DIV_ZERO)
                  errprn(DIVERR, aline, "");
                else
                  errprn(EVLERR, aline, "");
              } else {                /* by absolute */
                if (evalerrA == OVERFLOW)
                  errprn(OFLERR, aline, "");

                if ((opcode == ORGOP || opcode == PINOP) && SWITCH_8)
                  errprn(M88ERR, aline, opname[opcode]);

                if (opcode == ORGOP)
                  warrior[curWarrior].offset = normalize(resultB);
#ifdef SHARED_PSPACE
                else if (opcode == PINOP) {
                  warrior[curWarrior].pSpaceIDNumber = resultB;        /* not an address, no
                                                                 * need to normalize */
                  warrior[curWarrior].pSpaceIndex = PIN_APPEARED;        /* to indicate PIN has
                                                                         * been set */
                }
#endif
                else if (resultB)
                  if (warrior[curWarrior].offset)
                    errprn(DOEERR, aline, "");
                  else
                    warrior[curWarrior].offset = normalize(resultB);
                /* else ignore 'end' with parameter == 0L */
              }
            else if ((evalerrA = eval_expr(A_expr, &resultA)) < OK_EXPR) {
              if (evalerrA == DIV_ZERO)
                errprn(DIVERR, aline, "");
              else
                errprn(EVLERR, aline, "");
            } else if ((evalerrB = eval_expr(B_expr, &resultB)) < OK_EXPR) {
              if (evalerrB == DIV_ZERO)
                errprn(DIVERR, aline, "");
              else
                errprn(EVLERR, aline, "");
            } else {
              if (evalerrA == OVERFLOW || evalerrB == OVERFLOW)
                errprn(OFLERR, aline, "");
              base[line].A_value = (ADDR_T) normalize(resultA);
              base[line].B_value = (ADDR_T) normalize(resultB);
              if ((base[line++].debuginfo = (FIELD_T) aline->dbginfo) != 0)
                debugState = BREAK;
            }
          }
        }
        if ((warrior[curWarrior].offset < 0) ||
            (warrior[curWarrior].offset >= warrior[curWarrior].instLen))
          errprn(OFSERR, (line_st *) NULL, "");
      } else
        MEMORYERROR;
    else
      errprn(ZLNERR, (line_st *) NULL, "");
  } else
    errprn(EXXERR, (line_st *) NULL, "");
}

/* ******************************************************************* */

#define SNIL 0
#define SLBL 1
#define SVAL 2
#define SCOM 3
#define SPSE 4
#define SFOR 5
#define SROF 6
#define SERR -1

/* ******************************************************************* */

static int
blkfor(expr, dest)
  char   *expr, *dest;
{
  int evalerr;
  line_st *cline;
  long    result;
  ref_st *atbl, *ptbl;
  grp_st *forSymGr;

  if (symtbl) {
    forSymGr = addsym(symtbl->symn, NULL);
    if (symtbl->nextsym) {
      newtbl();
      reftbl->reftype = RLABEL;
      reftbl->grpsym = symtbl->nextsym;
      reftbl->value = line;
    }
    symtbl = NULL;
    symnum = 0;
  } else
    forSymGr = NULL;

  *dest = '\0';
  trav2(expr, dest, SVAL);

  if (SWITCH_V) {
    sprintf(outs, currentFORMsg, dest);
    textout(outs);
  }
  if ((evalerr = eval_expr(dest, &result)) < OK_EXPR) {
    if (evalerr == DIV_ZERO)
      errprn(DIVERR, aline, "");
    else
      errprn(EVLERR, aline, "");
  } else if (result <= 0L) {
    if (evalerr == OVERFLOW)
      errprn(OFLERR, aline, "");
    statefine++;
  } else {
    if (evalerr == OVERFLOW)
      errprn(OFLERR, aline, "");

    newtbl();
    reftbl->reftype = RSTACK;
    reftbl->grpsym = forSymGr;
    reftbl->visit = 1;
    atbl = reftbl;

    cline = aline;
    for (atbl->value = 1; vcont && atbl->value <= (uShrt) result && aline;
         atbl->value++)
      for (aline = cline; vcont;)
        if (aline->nextline) {
          int     r;

          aline = aline->nextline;
          *dest = '\0';
          if ((r = trav2(aline->vline, dest, SNIL)) == SROF)
            break;
          else if (r == SCOM) {
            addline(dest, aline->linesrc, dspnt);
            if (dbginfo == 3)
              dbginfo = 0;
          }
        } else {
          errprn(ROFERR, (line_st *) NULL, "");
          vcont = FALSE;
        }

    for (ptbl = NULL, atbl = reftbl; atbl; ptbl = atbl, atbl = atbl->nextref)
      if (atbl->reftype == RSTACK)
        break;

    if (ptbl)
      ptbl->nextref = atbl->nextref;
    else
      reftbl = atbl->nextref;

    disposetbl(atbl, atbl->nextref);
  }

  return SFOR;
}

/* ******************************************************************* */

static int
equtbl(expr)
  char   *expr;
{
  line_st *cline, *pline = NULL;
  uChar   i;

  if (symtbl) {

    newtbl();
    reftbl->reftype = RTEXT;
    reftbl->grpsym = symtbl;
    symtbl = NULL;
    symnum = 0;

    if (((cline = (line_st *) MALLOC(sizeof(line_st))) != NULL) &&
        ((cline->vline = pstrdup(expr)) != NULL)) {
      cline->linesrc = aline->linesrc;
      cline->nextline = NULL;
      pline = reftbl->sline = cline;
    } else
      MEMORYERROR;

    while (aline->nextline) {

      i = 0;
      get_token(aline->nextline->vline, &i, token);
      to_upper(token);
      if (strcmp(token, "EQU") == 0) {
        aline = aline->nextline;

        if (((cline = (line_st *) MALLOC(sizeof(line_st))) != NULL) &&
            ((cline->vline = pstrdup((char *) aline->vline + i)) != NULL)) {
          cline->linesrc = aline->linesrc;
          cline->nextline = NULL;
          pline = pline->nextline = cline;
        } else
          MEMORYERROR;
      }
#ifndef SERVER
      else if (*token == com_sym) {
        aline = aline->nextline;
        lineswitch(aline->vline, i, aline);
      }
#endif
      else
        break;
    }
  } else
    errprn(ANNERR, aline, "");

  return SVAL;
}

/* ******************************************************************* */

static int
equsub(expr, dest, wdecl, tbl)
  char   *expr, *dest;
  int     wdecl;
  ref_st *tbl;
{
  line_st *cline;

  tbl->visit = 1;
  cline = aline;
  aline = tbl->sline;
  wdecl = trav2(aline->vline, dest, wdecl);

  while (aline->nextline && vcont) {
    if (statefine == 0)
      if (wdecl == SCOM) {
        addline(dest, aline->linesrc, dspnt);
        if (dbginfo == 3)
          dbginfo = 0;
      }
    aline = aline->nextline;
    *dest = '\0';
    wdecl = trav2(aline->vline, dest, SNIL);
  }

  aline = cline;
  if (isspace(*expr))
    concat(dest, " ");

  tbl->visit = 0;
  return trav2(expr, dest, wdecl);
}

/* ******************************************************************* */

/* recursively traverse the buffer */
/* buf[] has to be "" */
static int
trav2(buffer, dest, wdecl)
  char   *buffer, *dest;
  int     wdecl;
{
  int evalerr;
  uChar   idxp = 0;
  ref_st *tbl;

  switch (get_token(buffer, &idxp, token)) {

  case NONE:
    return wdecl;

  case COMMTOKEN:
    if ((wdecl == SNIL) && (statefine == 0)) {

      uChar   idx;

      idx = idxp;
      get_token(buffer, &idx, token);
      to_upper(token);
      if (strcmp(token, "ASSERT") == 0) {
        long    result;

        trav2((char *) buffer + idx, dest, SVAL);

        if (SWITCH_V) {
          sprintf(outs, currentAssertMsg, dest);
          textout(outs);
        }
        if ((evalerr = eval_expr(dest, &result)) < OK_EXPR)
          errprn(BASERR, aline, "");
        else {
          if (evalerr == OVERFLOW)
            errprn(OFLERR, aline, "");
          noassert = FALSE;
          if (result == 0L)
            errprn(CHKERR, aline, "");
        }
      }
#ifndef SERVER
      else
        lineswitch(buffer, idxp, aline);
#endif
    }
    return wdecl;

  case CHARTOKEN:

    strcpy(buf, token);
    to_upper(buf);

    if (strcmp(buf, "ROF") == 0)
      if (statefine)
        statefine--;
      else if (wdecl <= SLBL)
        return SROF;
      else
        errprn(APPERR, aline, token);

    else if (strcmp(buf, "FOR") == 0)
      if (statefine)
        statefine++;
      else if (wdecl <= SLBL)
        return blkfor((char *) buffer + idxp, dest);
      else
        errprn(APPERR, aline, token);

    else if (strcmp(token, "CURLINE") == 0)
      if (statefine)
        return SNIL;
      else if (wdecl > SLBL) {
        sprintf(buf, "%d", line);
        if (isspace(buffer[idxp]))
          concat(buf, " ");
        if (concat(dest, buf))
          return (trav2((char *) buffer + idxp, dest, wdecl));
        else
          errprn(BUFERR, aline, "");
      } else
        errprn(MISC, aline, CURLINEErr);

    else if (strcmp(buf, "EQU") == 0)
      if (statefine)
        return SNIL;
      else if (wdecl <= SLBL)
        return equtbl((char *) buffer + idxp);
      else
        errprn(APPERR, aline, token);

    else if ((opcode = str_in_set(buf, opname)) < EQUOP)
      if (statefine)
        return SNIL;

      else if (wdecl <= SLBL) {

        uChar   j, op;

        for (j = 0; buf[j]; j++);

        while (buffer[idxp] && !isspace(buffer[idxp]))
          buf[j++] = buffer[idxp++];

        if (isspace(buffer[idxp])) {
          buf[j++] = ' ';
          buf[j] = '\0';
        }
        if (!concat(dest, buf))
          errprn(BUFERR, aline, "");

        else {
          op = opcode;
          if ((wdecl = trav2((char *) buffer + idxp, dest,
                             op < OPNUM ? SVAL : SPSE)) != SERR) {

            if (symtbl) {
              newtbl();
              reftbl->reftype = RLABEL;
              reftbl->value = line;
              reftbl->grpsym = symtbl;
              symtbl = NULL;
              symnum = 0;
            }
            if (op < OPNUM) {
              line++;
              if (dbginfo == 2)
                dbginfo++;
            } else if (op == ENDOP)
              vcont = FALSE;

            return SCOM;
          } else
            return SERR;
        }
      } else
        errprn(APPERR, aline, token);

    else if (statefine == 0) {

      char    tmp = 1;

      while ((buffer[idxp] == cat_sym) && (isalpha(buffer[idxp + 1])) &&
             (idxp++, get_token(buffer, &idxp, buf) == CHARTOKEN) && tmp)
        if (((tbl = lookup(buf)) != NULL) && (tbl->reftype == RSTACK)) {
          sprintf(buf, "%02u", (unsigned int) tbl->value);
          if (!concat(token, buf))
            errprn(BUFERR, aline, "");
        } else {
          errprn(CATERR, aline, buf);
          tmp = 0;
        }

      if ((tbl = lookup(token)) != NULL)
        if (tbl->reftype == RTEXT)
          if (tbl->visit)
            errprn(RECERR, aline, token);
          else
            return equsub((char *) buffer + idxp, dest, wdecl, tbl);

        else if (wdecl > SLBL) {
          if (tbl->reftype == RSTACK)
            sprintf(buf, "%02u", (unsigned int) tbl->value);
          else if (wdecl == SPSE)
            sprintf(buf, "%d", tbl->value);        /* absolute value */
          else
            sprintf(buf, "%d", tbl->value - line);        /* relative value */

          if (isspace(buffer[idxp]))
            concat(buf, " ");

          if (concat(dest, buf))
            return (trav2((char *) buffer + idxp, dest, wdecl));
          else
            errprn(BUFERR, aline, "");
        } else
          errprn(IDNERR, aline, token);

      else if (wdecl <= SLBL) {
        if (symnum < GRPMAX) {
          symtbl = addsym(token, symtbl);
          symnum++;
        } else
          errprn(GRPERR, aline, token);

        if (buffer[idxp] == cln_sym)        /* ignore a colon after a line-label */
          idxp++;

        /* traverse the rest of buffer */
        return trav2((char *) buffer + idxp, dest, SLBL);
      } else {
        if (isspace(buffer[idxp]))
          concat(token, " ");
        if (concat(dest, token))
          return (trav2((char *) buffer + idxp, dest, wdecl));
        else
          errprn(BUFERR, aline, "");
      }
    } else
      return trav2((char *) buffer + idxp, dest, SNIL);
    break;
  default:
    if (statefine)
      return SNIL;
    else if (wdecl <= SLBL)
      errprn(TOKERR, aline, token);
    else {
      if (concat(dest, token))
        return trav2((char *) buffer + idxp, dest, wdecl);
      else
        errprn(BUFERR, aline, "");
    }
  }
  return SERR;
}

/* ******************************************************************* */

/* collect and expand equ */
static void
expand(sspnt)
  uShrt   sspnt;
{
  dspnt = 1 - sspnt;
  disposeline(sline[dspnt]);
  sline[dspnt] = NULL;

  vcont = TRUE;
  statefine = 0;
  linemax = (uShrt) instrLim + LEXCMAX;

  aline = sline[sspnt];
  while (aline && vcont) {
    buf2[0] = '\0';
    switch (trav2(aline->vline, buf2, SNIL)) {
    case SCOM:
      addline(buf2, aline->linesrc, dspnt);
      if (dbginfo == 3)
        dbginfo = 0;
      break;
    case SFOR:
      break;
    case SROF:
      errprn(FORERR, aline, "");
    }
    aline = aline->nextline;
  }
}

/* ******************************************************************* */

int
parse(expr, cell, loc)
  char   *expr;
  mem_struct *cell;
  ADDR_T  loc;
{
  int evalerrA, evalerrB;
  long    resultA, resultB;
  mem_struct tmp;
  ADDR_T  dloc;
  uChar   i = 0;
  uShrt   sspnt = 0;

  errnum = warnum = 0;
  ierr = 0;
  aline = NULL;
  dloc = loc;
  switch (get_token(expr, &i, token)) {
  case CHARTOKEN:
    nocmnt(expr);
    addline(expr, addlinesrc(expr, (uShrt) loc), sspnt);
    break;
  case COMMTOKEN:
    globalswitch(expr, (uShrt) i, (uShrt) loc, sspnt);
    break;
  default:
    errprn(TOKERR, aline, token);
  }

  line = (uShrt) dloc;
  expand(sspnt);
  disposeline(sline[sspnt]);
  sline[sspnt] = NULL;
  sspnt = 1 - sspnt;

  for (aline = sline[sspnt], line = (uShrt) loc; aline;
       aline = aline->nextline) {
    dfashell(aline->vline, &tmp);
    if (errnum == 0)
      if (opcode < OPNUM)
        if ((evalerrA = eval_expr(A_expr, &resultA)) < OK_EXPR) {
          if (evalerrA == DIV_ZERO)
            errprn(DIVERR, aline, "");
          else
            errprn(EVLERR, aline, "");
        } else if ((evalerrB = eval_expr(B_expr, &resultB)) < OK_EXPR) {
          if (evalerrB == DIV_ZERO)
            errprn(DIVERR, aline, "");
          else
            errprn(EVLERR, aline, "");
        } else {
          if (evalerrA == OVERFLOW || evalerrB == OVERFLOW)
            errprn(OFLERR, aline, "");
          cell->opcode = tmp.opcode;
          cell->A_mode = tmp.A_mode;
          cell->B_mode = tmp.B_mode;
          cell->A_value = (ADDR_T) normalize(resultA);
          cell->B_value = (ADDR_T) normalize(resultB);
          if ((cell->debuginfo = (dbginfo ? TRUE : FALSE)) != FALSE)
            debugState = BREAK;
          cell++, line++;
          if (++dloc >= coreSize) {
            dloc -= coreSize;
            cell -= coreSize;
          }
        }
  }
  disposeline(sline[0]);
  disposeline(sline[1]);
  sline[0] = sline[1] = NULL;

  while (srctbl) {
    src_st *tmp;

    tmp = srctbl;
    srctbl = srctbl->nextsrc;
    FREE(tmp->src);
    FREE(tmp);
  }

  if (dloc < loc)
    dloc = coreSize - loc + dloc;
  else
    dloc -= loc;

  if (errnum)
    return (-1);
  else
    return (dloc);
}

/* ******************************************************************* */

static char stdinstart = 0;

int
assemble(fName, aWarrior)
  char   *fName;
  int     aWarrior;
{
  FILE   *infp;
  uChar   cont = TRUE, conLine = FALSE, i;
  uShrt   lines;                /* logical and physical lines */
  uShrt   spnt = 0;                /* index/pointer to sline and lline */

#ifdef VMS
  char    DIAfilename[256], *temp;
#endif

#ifdef ASM_DEBUG
  printf("Entering assemble sub\n");
#endif

  errorlevel = WARNING;
  errorcode = SUCCESS;
  *errmsg = '\0';
  errnum = warnum = 0;

  pass = 0;

  srctbl = NULL;
  sline[0] = sline[1] = NULL;
  reftbl = NULL;
  symtbl = NULL;
  symnum = 0;

  lines = 0;
  ierr = 0;

  if ((errkeep = (err_st *) MALLOC(sizeof(err_st) * ERRMAX)) == NULL)
    MEMORYERROR;

  curWarrior = aWarrior;
  warrior[curWarrior].name = pstrdup(unknown);
  warrior[curWarrior].authorName = pstrdup(anonymous);
  warrior[curWarrior].date = pstrdup("");
  warrior[curWarrior].version = pstrdup("");
#ifdef PSPACE
  warrior[curWarrior].pSpaceIndex = UNSHARED;        /* tag */
#endif
  /* These two inits turn out to be neccessary */
  warrior[curWarrior].instBank = NULL;
  warrior[curWarrior].instLen = 0;

#ifdef VMS
  if (SWITCH_D) {
    temp = strstr(warrior[curWarrior].fileName, "]");        /* Look for dir spec */
    if (temp == NULL) {
      temp = strstr(warrior[curWarrior].fileName, ":");
      if (temp == NULL)
        temp = warrior[curWarrior].fileName;
      else
        temp++;                        /* Bypass ":" */
    } else
      temp++;                        /* bypass "]" */
    strcpy(DIAfilename, temp);
    temp = strstr(DIAfilename, ".");
    if (temp == NULL)
      strcat(DIAfilename, ".DIA");
    else
      strcpy(temp, ".DIA");
    printf("%s %s.", Opening, DIAfilename);
    dias = fopen(DIAfilename, "w");
    fprintf(dias, "%s", StartMod);
  }
#endif

  dbgproceed = TRUE;
  dbginfo = FALSE;
  noassert = TRUE;

  addpredefs();

#ifdef ASM_DEBUG
  printf("Entering file reading module\n");
#endif

  if ((*fName == '\0') || (infp = fopen(fName, "r")) != NULL) {

    char    pstart;

    if (*fName == '\0') {
      infp = stdin;
      pstart = stdinstart;
    } else
      pstart = 0;

    lines = 0;
    aline = NULL;

    while (cont) {

      /*
       * Read characters until newline or EOF encountered. newline is
       * excluded. Need to be non-strict boolean evaluation
       */
      *buf = '\0';
      i = 0;                        /* pointer to line buffer start */
      do {
        if (fgets(buf + i, MAXALLCHAR - i, infp)) {
          for (; buf[i]; i++)
            if (buf[i] == '\n' || buf[i] == '\r')
              break;
          buf[i] = 0;
          if (buf[i - 1] == '\\') {        /* line continued */
            conLine = TRUE;
            buf[--i] = 0;        /* reset */
          } else
            conLine = FALSE;
        } else if (ferror(infp)) {
          errprn(DSKERR, (line_st *) NULL, fName);
          fclose(infp);
          return PARSEERR;
        } else
          cont = (feof(infp) == 0);
      } while (conLine == TRUE);
      lines++;
      i = 0;

      switch (get_token(buf, &i, token)) {
        /*
         * COMMTOKEN before any non-whitespace chars may contains switches.
         * We treat them first and therefore we don't have to save it in
         * sline
         */
      case COMMTOKEN:
        if (globalswitch(buf, (uShrt) i, lines, spnt))        /* REDCODE? */
          switch (pstart) {
          case 0:
            disposeline(sline[spnt]);
            sline[spnt] = lline[spnt] = NULL;
            pstart++;
            break;
          case 1:
            pstart++;
            /* fallthru */
          case 2:
            cont = 0;
            break;
          }
        break;
#if 0                                /* no longer scanning for END, ;redcode req'd
                                 * between warriors in stdin */
      case CHARTOKEN:
        nocmnt(buf);
        do {
          to_upper(token);
          if (strcmp(token, "END") == 0) {
            cont = 0;
            break;
          }
        } while (get_token(buf, &i, token) != NONE);
        addline(buf, addlinesrc(buf, lines), spnt);
        break;
#endif
      case NONE:
        break;
      default:
        nocmnt(buf);                /* saving some space */
        addline(buf, addlinesrc(buf, lines), spnt);
        break;
      }
    }

    if (pstart)
      pstart--;

    if (*fName)
      fclose(infp);
    else
      stdinstart = pstart;        /* save value for next stdin reference */

    if (SWITCH_V) {
      show_info(spnt);
      textout(paramCheckMsg);
    }
#ifdef ASM_DEBUG
    printf("Entering pass 1 (expand and shrink)\n");
#endif

    line = 0;
    expand(spnt);

    if (symtbl) {
      grp_st *tmp;

      *buf = '\0';
      for (tmp = symtbl; tmp; tmp = tmp->nextsym) {
        if (*buf)
          if (!concat(buf, " "))
            break;
        if (!concat(buf, tmp->symn))
          break;
      }

      errprn(DLBERR, (line_st *) NULL, buf);
      disposegrp(symtbl);        /* discount any symtbl with empty reference */
      symtbl = NULL;
      symnum = 0;
    }
    spnt = 1 - spnt;
    pass++;

    if (noassert)
      errprn(NASERR, (line_st *) NULL, "");

    if (SWITCH_V) {
      textout("\n");
      show_info(spnt);
      show_lbl();
    }
#ifdef ASM_DEBUG
    printf("Entering pass 2 (parsing and loading)\n");
#endif
    encode(spnt);

    dbginfo = FALSE;
    dbgproceed = TRUE;

#ifdef ASM_DEBUG
    printf("Disclaiming all temporary storage\n");
#endif
    /* release all allocated memory */
    cleanmem();
    while (srctbl) {
      src_st *tmp;

      tmp = srctbl;
      srctbl = srctbl->nextsrc;
      FREE(tmp->src);
      FREE(tmp);
    }

    /* leave it for cdb() */
    addpredefs();

    if (errnum) {
      FREE(warrior[curWarrior].instBank);
      warrior[curWarrior].instBank = NULL;
      warrior[curWarrior].instLen = 0;
    }
#ifndef SERVER
    if (errnum + warnum) {
      if (*warrior[curWarrior].fileName)
        sprintf(outs, "\nSource: filename '%s'\n", warrior[curWarrior].fileName);
      else
        sprintf(outs, "\nSource: standard input (%s by %s)\n",
                warrior[curWarrior].name, warrior[curWarrior].authorName);
      textout(outs);
    }
#endif
    if (errnum) {
      sprintf(outs, errNumMsg, errnum);
      textout(outs);
    }
    if (warnum) {
      sprintf(outs, warNumMsg, warnum);
      textout(outs);
    }
    while (ierr)
      if (errkeep[--ierr].num > 1) {
        sprintf(outs, duplicateMsg, errkeep[ierr].loc, errkeep[ierr].num);
        textout(outs);
      }
    if (errnum + warnum) {
      sprintf(outs, "\n");
      textout(outs);
    }
  } else
    errprn(FNFERR, (line_st *) NULL, fName);

  if (errnum)
    errorcode = PARSEERR;
  else
    errorcode = SUCCESS;

  reset_regs();
#ifdef VMS
  if (SWITCH_D) {
    fprintf(dias, "%s", EndMod);
    fclose(dias);
  }
#endif
  return (errorcode);
}
