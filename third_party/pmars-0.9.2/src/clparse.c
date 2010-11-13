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
 * clparse.c: command line parser
 * $Id: clparse.c,v 1.5 2000/12/25 00:49:08 iltzu Exp $
 */

/*******************************************************************
 *                                                                 *
 * int parse_param(int argc,char *argv[])                          *
 *                                                                 *
 * Command line parameter parse                                    *
 *                                                                 *
 * if filename# exists then warrior[#].filename contains           *
 * the filename otherwise it contains NULL                         *
 * number of warriors in warrior                                   *
 *                                                                 *
 * returns 0 if it's successfull                                   *
 * if there is an error in the command line it gives an            *
 * error message and returns a non-zero value (exit neccessary)    *
 *                                                                 *
 *******************************************************************/

#include <string.h>
#include <ctype.h>
#include "global.h"

#define CLP_MAXSTRLEN  80
#define CLP_ADDR ADDR_T
#define CLP_LONG long
#define CLP_INT int

typedef enum {
  clp_int, clp_addr, clp_long, clp_bool, clp_str
}       clp_dtype_t;

typedef struct clp_option_struct {
  char    word;                        /* sign for the switch */
  pointer_t storage;
  CLP_LONG min;
  CLP_LONG max;
  CLP_LONG def;
  clp_dtype_t dtype;
  char   *description;
}       clp_opt_t;

/* externalized strings */

extern char *credits_screen1, *credits_screen2, *credits_screen3, *usage_screen,
       *optionsAre, *readingStdin, *readOptionsFromFile, *standardInput,
       *errorIsLocatedIn, *unknownOption, *badArgumentForSwitch, *optionMustBeInTheRange, *tooManyParameters,
       *cannotOpenParameterFile, *optRounds, *optEnterDebugger, *optDisabledInServerVersion,
       *optCoreSize, *optBrief, *optCycles, *optVerboseAssembly, *optProcesses,
       *optKotH, *optLength, *opt88, *optDistance, *optFixedSeries, *optFixedPosition,
       *optSort, *optView, *optScoreFormula, *optDIAOutput, *noWarriorFile,
       *fFExclusive, *coreSizeTooSmall, *dLessThanl, *FLessThand,
       *outOfMemory, *badScoreFormula, *optPSpaceSize, *pSpaceTooBig,
       *optPermutate, *permutateMultiWarrior;

#if defined(XWINGRAPHX)
extern char *badArgumentForXSwitch, *optXOpt[];
#endif

#ifdef MACGRAPHX
void
        macputs(char *outputstr);
#endif

#ifdef NEW_STYLE
void
        strip(char *argv[]);
void
        print_usage(clp_opt_t clopt[]);
int
clp_parse(clp_opt_t clopt[],
          FILE * filep);
  int
          parse_param(int argc, char **argv);
  int
          next_input(FILE * filep, char *inputs);

#else
void    print_usage();
int     next_input();
int     clp_parse();
int     parse_param();

#endif

static char *describe[] = {"#", "#", "#", " ", "$"};

#define  record(pword,pdtype,pstorage,pmin,pmax,pdef,pdescription)\
     options[optI].word        = pword;    \
     options[optI].dtype       = pdtype;   \
     options[optI].storage     = (pointer_t) pstorage; \
     options[optI].min         = (CLP_LONG) pmin;     \
     options[optI].max         = (CLP_LONG) pmax;     \
     options[optI].def         = (CLP_LONG) pdef;     \
     options[optI++].description = pdescription

static char outs[MAXALLCHAR];
static int printed_usage;        /* to show the help screen only once */
int     argc;
char  **argv;

/*******************************************************************
 * credits and usage screen are now in str_???.c                                       *
 *******************************************************************/

#if defined (XWINGRAPHX)

/*********************************************
 * check for standard X command line options *
 *********************************************/

extern char *xOptions[];
extern char *xStorage[];
extern int xMaxOptions;

static clp_opt_t xOpt;

int
xwin_decode(inputs, clip)
  char   *inputs;
  clp_opt_t **clip;
{
  int     i;

  if (inputs[0] != '-')
    return 0;
  for (i = 0; i < xMaxOptions; i++)
    if (strcmp(&inputs[1], xOptions[i]) == 0)
      break;
  if (i == xMaxOptions)
    return 0;

  xOpt.word = 'x';
  xOpt.dtype = clp_str;
  xOpt.storage = (pointer_t) & xStorage[i];
  xOpt.def = i;                        /* remember which option this was */
  xOpt.description = optXOpt[i];
  *clip = &xOpt;

  return 1;
}
#endif

/*******************************************************************/

void
print_usage(clopt)
  clp_opt_t clopt[];

{
  clp_opt_t *clip;
  int     i;

  if (!printed_usage) {
    sprintf(outs, credits_screen1, PMARSVER / 100, (PMARSVER % 100) / 10,
            PMARSVER % 10, PMARSDATE);
    errout(outs);
    errout(credits_screen2);
    errout(credits_screen3);
    errout(usage_screen);
    errout(optionsAre);
    for (i = 0, clip = clopt; clip->storage != NULL; ++clip) {
      if (clip->description != NULL) {
        sprintf(outs, "  -%c %s %-30s",
                clip->word, describe[(int) clip->dtype], clip->description);
        errout(outs);
        if (i++ % 2)
          errout("\n");
      }
    }
#ifndef MACGRAPHX
    errout(readOptionsFromFile);
#endif
#if defined(XWINGRAPHX)
    for (i = 0; i < xMaxOptions; i++) {
      sprintf(outs, "  -%-8s $ %-24s", xOptions[i], optXOpt[i]);
      errout(outs);
      if (i % 2)
        errout("\n");
    }
#endif
    printed_usage = TRUE;
  }
}

/******************************************************
 * read from the command line, command file, or stdin *
 ******************************************************/

int
next_input(filep, inputs)
  FILE   *filep;
  char   *inputs;
{
  int     i;
  if (filep == NULL) {                /* read from command line */
    if (argc) {
      strcpy(inputs, *argv);
      --argc;
      ++argv;
    } else
      *inputs = '\0';                /* read from command file or stdin */
  } else {
    *inputs = '\0';
    /* if (!feof(filep))  */
    fscanf(filep, "%s", inputs);
    if (*inputs == '"') {        /* quoted string */
      *inputs = ' ';
      i = strlen(inputs);
      if (inputs[i - 1] == '"')
        inputs[i - 1] = '\0';
      else
        fscanf(filep, "%[^\"]%*[\"]", inputs + i);
    }
    while (*inputs == ';') {
      fgets(inputs, 100, filep);
      *inputs = '\0';
      fscanf(filep, "%s", inputs);
    }
  }
  if (!strcmp(inputs, "$"))        /* end of input marker */
    return 0;
  return strlen(inputs);
}

/************************************************************************
 * Read the options (called recursively) This is the heart of clparse.c *
 ************************************************************************/

int
clp_parse(clopt, filep)
  clp_opt_t clopt[];
  FILE   *filep;

{

  enum {
    NONE, OPTION, ARGUMENT, RANGE, TOO_MANY, MEMORY, FILENAME, OLDERROR
  }       code;
  clp_opt_t *clip;
  unsigned bool_switch;
  char    inputs[CLP_MAXSTRLEN];
  FILE   *newFile;

  code = NONE;                        /* no error so far */
  while (next_input(filep, inputs)) {
    if ((inputs[0] == '-') && inputs[1]) {        /* option ? */
      int     i;

      bool_switch = 1;                /* no argument needed for this option */
      for (i = 1; inputs[i] && (bool_switch); ++i) {
        for (clip = clopt;        /* find which option is this */
             (clip->storage != NULL) &&
             (inputs[i] != clip->word);
             ++clip);
#if defined(XWINGRAPHX)
        if (!xwin_decode(inputs, &clip) && inputs[i] != clip->word) {
#else
        if (inputs[i] != clip->word) {        /* option not found       */
#endif
#ifndef MACGRAPHX
          if (inputs[i] == '@') {        /* included command file? */
            if (inputs[i + 1] != '\0') {
              code = FILENAME;        /* must have a parameter */
              goto ERROR;
            } else {
              bool_switch = 0;
              if (next_input(filep, inputs)) {
                if (!strcmp(inputs, "-")) {
                  newFile = stdin;
                  fprintf(stderr, readingStdin);
                } else {
                  if ((newFile = fopen(inputs, "r")) == NULL) {
                    code = FILENAME;        /* command file not found */
                    goto ERROR;
                  }
                }
              } else {
                code = FILENAME;/* no parameter for -@ */
                goto ERROR;
              }
            }
            if ((clp_parse(clopt, newFile)) != NONE) {
              code = OLDERROR;
              if (newFile == stdin)
                strcpy(inputs, standardInput);
              goto ERROR;        /* error in command file */
            }
          } else                /* no such option   */
#endif                                /* MACGRAPHX */
          {
            code = OPTION;
            goto ERROR;
          }
        } else {                /* option found */
          if (clip->dtype == clp_bool)
            clip->def = !(clip->def);
          else {                /* not a bool switch, value needed */
            bool_switch = 0;
#if defined (XWINGRAPHX)
            if (clip->word != 'x' && inputs[i + 1] != '\0') {
#else
            if (inputs[i + 1] != '\0') {
#endif
              code = ARGUMENT;        /* middle option  */
              goto ERROR;        /* but we need an argument */
            } else {
              if (!next_input(filep, inputs)) {
                code = ARGUMENT;/* no argument */
                goto ERROR;
              } else if (clip->dtype == clp_str) {
                if ((*(char **) clip->storage =
                     (char *)
                     malloc((strlen(inputs) + 1) * sizeof(char))) != NULL) {
                  strcpy(*(char **) clip->storage, inputs);
                } else {
                  code = MEMORY;
                  goto ERROR;
                }
              } else {
                char   *idx;
                clip->def = atol(inputs);
                for (idx = inputs; *idx && isdigit(*idx); ++idx);
                if (*idx) {
                  code = ARGUMENT;
                  goto ERROR;
                }
                if (clip->def < (clip->min) ||
                    clip->def > (clip->max)) {
                  code = RANGE;
                  goto ERROR;
                }
              }
            }
          }
        }
      }
    } else {
      if (warriors < MAXWARRIOR) {
        int     many;

        code = NONE;
#ifndef MACGRAPHX
        many = strlen(inputs) - 1;
        if (inputs[many] == '-') {        /* warrior from stdin */
          if (!many) {
            many++;
          } else {                /* number before - ? */
            inputs[many] = 0;        /* kill the - sign */
            many = atoi(inputs);/* how many warriors from stdin */
            if (!many) {
              code = OPTION;
              goto ERROR;
            }
          }
          *inputs = '\0';
        } else
#endif
          many = 1;
        for (; many; many--) {
          if ((warrior[warriors].fileName =
            (char *) malloc((strlen(inputs) + 1) * sizeof(char))) != NULL) {
            strcpy(warrior[warriors++].fileName, inputs);
          } else {
            code = MEMORY;
            goto ERROR;
          }
        }
      } else {
        code = TOO_MANY;
        goto ERROR;
      }
    }
  }
  return (0);

ERROR:

  if (code != OLDERROR)
    print_usage(clopt);
  switch (code) {
  case NONE:
    break;
  case OLDERROR:
    sprintf(outs, errorIsLocatedIn, inputs);
    errout(outs);
    break;
  case OPTION:
    sprintf(outs, unknownOption, inputs);
    errout(outs);
    break;
  case ARGUMENT:
#if defined(XWINGRAPHX)
    if (clip->word == 'x')
      sprintf(outs, badArgumentForXSwitch, xOptions[clip->def]);
    else
#endif
      sprintf(outs, badArgumentForSwitch, clip->word);
    errout(outs);
    break;
  case RANGE:
    sprintf(outs, optionMustBeInTheRange, clip->word);
    errout(outs);
    sprintf(outs, "%ld - %ld\n", (clip->min), (clip->max));
    errout(outs);
    break;
  case TOO_MANY:
    sprintf(outs, tooManyParameters, inputs);
    errout(outs);
    break;
  case MEMORY:
    sprintf(outs, outOfMemory);
    errout(outs);
    break;
  case FILENAME:
    sprintf(outs, cannotOpenParameterFile);
    errout(outs);
    break;
  }
  return (CLP_NOGOOD);
}

int
#ifdef PARSETEST
main(largc, largv)
#else
parse_param(largc, largv)
#endif

  int     largc;
  char  **largv;
{

  int     result;
  clp_opt_t *clip;
  int     i;
  long    dummy;

  /*******************************************************************
  * command line parameters and options                              *
  ********************************************************************/

#define OPTNUM 21                /* don't forget to increase when adding new
                                 * options */
  static clp_opt_t options[OPTNUM];
  int     optI = 0;                /* used by record() macro */

#ifdef PERMUTATE
  record('r', clp_int, &rounds, 0, MAXROUND,
         -1, optRounds);
#else
  record('r', clp_int, &rounds, 0, MAXROUND,
         DEFAULTROUNDS, optRounds);
#endif
#ifndef SERVER
  record('e', clp_bool, &SWITCH_e, 0, 1,
         0, optEnterDebugger);
#else
  record('e', clp_bool, &SWITCH_e, 0, 1,
         0, optDisabledInServerVersion);
#endif
  record('s', clp_addr, &coreSize, 1, MAXCORESIZE,
         DEFAULTCORESIZE, optCoreSize);
  record('b', clp_bool, &SWITCH_b, 0, 1,
         0, optBrief);
  record('c', clp_long, &cycles, 1, MAXCYCLE,
         DEFAULTCYCLES, optCycles);
  record('V', clp_bool, &SWITCH_V, 0, 1, 0, optVerboseAssembly);
  record('p', clp_int, &taskNum, 1, MAXTASKNUM,
         DEFAULTTASKNUM, optProcesses);
  record('k', clp_bool, &SWITCH_k, 0, 1,
         0, optKotH);
  record('l', clp_addr, &instrLim, 1, MAXINSTR,
         DEFAULTINSTRLIM, optLength);
  record('8', clp_bool, &SWITCH_8, 0, 1,
         0, opt88);
  record('d', clp_addr, &separation, 1, MAXSEPARATION,
         0, optDistance);
  record('f', clp_bool, &SWITCH_f, 0, 1,
         0, optFixedSeries);
  record('F', clp_addr, &SWITCH_F, 0, MAXCORESIZE,
         0, optFixedPosition);
  record('o', clp_bool, &SWITCH_o, 0, 1, 0,
         optSort);
#if defined(PSPACE)
  record('S', clp_addr, &pSpaceSize, 1,
         MAXCORESIZE, 0, optPSpaceSize);
#endif
#ifdef PERMUTATE
  record('P', clp_bool, &SWITCH_P, 0, 1,
	 0, optPermutate);
#endif
  record('=', clp_str, &SWITCH_eq, 0, 0, 0, optScoreFormula);
  record('Q', clp_int, &SWITCH_Q, -1, INT_MAX, -1, NULL);
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX)  || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
#if defined(XWINGRAPHX)
#define V_MAX        2994
#else
#define V_MAX        994
#endif
  record('v', clp_int, &SWITCH_v, 0, V_MAX,
         103, optView);
#undef V_MAX
#endif
#if defined(VMS)
  record('D', clp_bool, &SWITCH_D, 0, 1, 0,
         optDIAOutput);
#endif
  record((char) 0, (clp_dtype_t) 0, NULL, 0, 0,
         0, NULL);
  /*******************************************************************/
  /* initializing default values                                     */
  for (i = 0; i < MAXWARRIOR; i++) {
    warrior[i].fileName = NULL;
    warrior[i].fileName = NULL;
  }

  /*******************************************************************/
  argv = largv;
  argc = largc;
  ++argv;
  --argc;                        /* skip program name */
  warriors = 0;
  printed_usage = FALSE;
  result = clp_parse(options, NULL);
  if (!result) {                /* so far so good */
    {
      for (clip = options; clip->storage != NULL; ++clip) {
        if (clip->dtype == clp_long)
          *(CLP_LONG *) clip->storage = clip->def;
        else if (clip->dtype == clp_int)
          *(CLP_INT *) clip->storage = (CLP_INT) clip->def;
        else if (clip->dtype == clp_addr)
          *(CLP_ADDR *) clip->storage = (CLP_ADDR) clip->def;
        else if (clip->dtype == clp_bool)
          *(CLP_INT *) clip->storage = (CLP_INT) clip->def;
      }

      if (SWITCH_f && SWITCH_F) {
        print_usage(options);
        errout(fFExclusive);
        result = CLP_NOGOOD;
      }
      if (separation == 0)
        separation = instrLim;
      else if (separation < instrLim) {
        print_usage(options);
        errout(dLessThanl);
        result = CLP_NOGOOD;
      }
      if (coreSize < (warriors * separation)) {
        print_usage(options);
        errout(coreSizeTooSmall);
        result = CLP_NOGOOD;
      }
      if ((SWITCH_F) && (separation > SWITCH_F)) {
        print_usage(options);
        errout(FLessThand);
        result = CLP_NOGOOD;
      }
      set_reg('W', (long) warriors);
      for (i = 1; i <= warriors; ++i) {
        set_reg('S', (long) i);
        if (eval_expr(SWITCH_eq, &dummy) < OK_EXPR) {
          print_usage(options);
          errout(badScoreFormula);
          result = CLP_NOGOOD;
          break;
        }
      }
#ifdef PSPACE
      if (pSpaceSize == 0)        /* make default, at least 1/16th of coresize */
        for (i = 16; i > 0; --i)
          if (!(coreSize % i)) {
            pSpaceSize = coreSize / i;
            break;
          }
      if (pSpaceSize > coreSize) {
	print_usage(options);
	errout(pSpaceTooBig);
	result = CLP_NOGOOD;
      }
#endif
#ifdef PERMUTATE
      if (SWITCH_P) {
	if (warriors != 2) {
	  print_usage(options);
	  errout(permutateMultiWarrior);
	  result = CLP_NOGOOD;
	} else if (rounds < 0) /* all possible combinations */
	    rounds = 2*coreSize-4*separation+2;
      } else if (rounds < 0 )
	rounds = DEFAULTROUNDS;
#endif
      /* further checks of the values */

#ifndef OS2PMGRAPHX                /* jk - we can load files after the fact... */
      if (warrior[0].fileName == NULL) {
        print_usage(options);
        errout(noWarriorFile);
        result = CLP_NOGOOD;
      }
#endif
    }
  }
  return result;
}



