/* pMARS -- a portable Memory Array Redcode Simulator
 * Copyright (C) 1993-1996 Albert Ma, Na'ndor Sieben, Stefan Strack and Mintardjo Wangsawidjaja
 * Copyright (C) 2000 Philip Kendall and Ilmari Karonen
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
 * str_eng.c: externalized messages for easy translation (ENGLISH version)
 * $Id: str_eng.c,v 1.5 2000/12/25 00:49:08 iltzu Exp $
 */

#include "global.h"

#if (PMARSLANG == ENGLISH)

#ifndef _EXENAME
#define _EXENAME "pmars"
#endif

/*
 * Strings from sim.c:
 */
#ifdef VMS                        /* Make error messages conform to the VMS
                                 * standard. */
char   *outOfMemory = "%%SYS-E-INSFMEM, insufficient memory\n";
char   *warriorTerminated = "%%PMARS-I-WARTER, warrior %d: %s terminated\n";
char   *fatalErrorInSimulator = "%%PMARS-F-FATAL, fatal error in simulator. Please report this error.\n";
char   *warriorTerminatedEndOfRound = "%%PMARS-I-WARTEREND, warrior %d: %s terminated - End of round %d\n";
char   *endOfRound = "%%PMARS-I-ENDOFROU, end of round %d\n";
#else
char   *outOfMemory = "Out of memory\n";
char   *warriorTerminated = "Warrior %d: %s terminated\n";
char   *fatalErrorInSimulator = "Fatal error in simulator. Please report this error.\n";
char   *warriorTerminatedEndOfRound = "Warrior %d: %s terminated - End of round %d\n";
char   *endOfRound = "End of round %d\n";
#endif

/*
 * Strings from cdb.c:
 */

#ifndef SERVER
char   *pagePrompt = " RET for more, q to quit, a for all ";
char   *exitingCdbToFinishSimulation = "Exiting cdb to finish simulation\n";
char   *usageDisplay = "Usage: display clear|on|off|nnn\n";
char   *usageExecute = "Usage: execute [address]\n";
char   *usageMoveable = "Usage: moveable on|off\n";
char   *pressAnyKeyToContinue = "\nPress any key to continue ";
char   *usageSkip = "Usage: skip [count]\n";
char   *usageWrite = "Usage: write logfile\n";
char   *closingLogfile = "Closing logfile\n";
char   *cannotOpenLogfile = "Cannot open logfile\n";
char   *openingLogfile = "Opening logfile\n";
char   *unknownCommand = "Unknown command \"%s\"\n";
char   *macroStringExpansionTooLong = "Macro string expansion too long\n";
char   *EOFreadingCommandInput = "EOF reading command input\n";
char   *maximumLoopNestingExceeded = "Maximum loop nesting exceeded\n";
char   *badRepeatCountExpression = "Bad repeat count expression\n";
char   *writeErrorDiskFull = "Write error - disk full?\n";
char   *badArgument = "Bad argument \"%s\"\n";
char   *helpText[] = {
  "Command groups:\n",
  "Executing:    continue execute  go       quit     skip     step     thread\n",
  "Listing:      list     search\n",
  "Tracing:      trace    untrace  moveable\n",
  "Editing:      edit     fill\n",
  "Information:  help     progress registers\n",
  "Macro:        calc     echo     if       macro    remark   reset\n",
#if defined(DOSGRXGRAPHX) || defined(DOSTXTGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
  "Display:      clear    close    display  switch\n",
#else
  "Display:      clear\n",
#endif
#if defined(__MAC__) || defined(XWINGRAPHX)
  "Others:       pqueue   wqueue   pspace   write\n",
#else
  "Others:       pqueue   wqueue   pspace   shell    write\n",
#endif
  "\n",
  "Command description, shortest abbreviation in ():\n",
  "(ca)lc expr1[,expr2]   calculate expression(s) and echo result(s)\n",
  "(cl)ear                clear cdb screen\n",
#if defined(DOSGRXGRAPHX) || defined(DOSTXTGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
  "(clo)se                close right cdb panel and make left full-screen\n",
#endif
  "(c)ontinue *           exit cdb and finish simulation\n",
#if defined(CURSESGRAPHX) || defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) \
    || defined(LINUXGRAPHX) || defined(XWINGRAPHX)
  "(d)isplay clear|on|off|nnn\n",
  "                       clear, turn core display on/off or set -v option to nnn\n",
#endif
  "(ec)ho string          print string (for commenting macros)\n",
  "(e)dit [range]         edit core addresses\n",
  "(e(x))ecute [address]  reset process queue to [address] and step\n",
  "(f)ill [range]         fill core addresses with instruction\n",
  "(g)o *                 execute until next trace\n",
  "(h)elp                 show this command summary\n",
  "(if) expression        execute next command in chain if expression is not 0\n",
  "(l)ist [range]         list core addresses\n",
  "(m)acro [name][,file]  execute macro [name] (from [file], [user]=keyboard)\n",
  "(m)acro                list macros\n",
  "(mo)veable on|off      enable/disable copying debug info\n",
  "(pq)ueue [1|2|off]     enter/exit process queue debug mode\n",
#ifdef PSPACE
  "(ps)pace [1|2|off]     enter/exit pspace debug mode\n",
#endif
  "(p)rogress             show result of simulation so far\n",
  "(q)uit                 quit program\n",
  "(r)egisters            show simulator state\n",
  "(rem)ark string        macro comment\n",
  "(res)et                terminate macro/command chain processing\n",
  "(se)arch pattern       search for wildcarded pattern in core\n",
#if !defined(__MAC__) && !defined(XWINGRAPHX)
  "(sh)ell [command]      execute OS command\n",
#endif
  "(sk)ip [count]         execute [count] steps silently\n",
  "(s)tep *               execute next queued instruction\n",
#if defined(DOSGRXGRAPHX) || defined(DOSTXTGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
  "(sw)itch [1|2]         switch to left (1), right (2), or other cdb panel\n",
#endif
  "(th)read *             step through current thread\n",
  "(t)race [range]        set trace of core addresses\n",
  "(u)ntrace [range]      clear trace of core addresses\n",
  "(w)rite logfile        write all following output to logfile\n",
  "(w)rite                close logfile\n",
  "(wq)ueue [off]         enter/exit warrior queue debug mode\n",
  "* [address]            set PC to [address], then continue|go|..\n",
  "range: { e | e,e | e, | ,e | , }\n",
  "    e: { [0-9]* | . | $ | A | B | PC | PC1 | PC2 | LINES |\n",
  "         +e | -e | e+e | e-e | e*e | e/e | e%e | !e | e==e | e!=e |\n",
  "         e<e | e>e | e<=e | e>=e | e&&e | e||e | [C-Z] | [C-Z]=e | (e) }\n",
  "macro definition: macro-name=command-chain\n",
  "command-chain: link[~link]*        link: command | !! | ![e]\n",
  "!!: loop start, ![e]: loop end, e: iterations\n",
""};

char   *roundOfCycle = "Round %-3d of %-10d Cycle %ld\n";
char   *currentlyExecutingWarrior = "Currently executing warrior %d: %-30s\n";
char   *processesActive = "Processes active %-6d Process queue:\n";
char   *otherWarrior = "Other warrior: %-30s\n";
char   *warriorAtAddressHasActiveProcesses =
"Warrior %d: %s at address [%d] has %d active process%s\n";
char   *pluralEndingOfProcess = "es";
char   *ofWarriorsAreAlive = "%d of %d warriors are alive\n";
char   *fillWith = "Fill with: ";
char   *dotByItselfEndsKeyboardInput = "[\".\" by itself ends keyboard input]\n";
char   *cannotOpenMacroFile = "Cannot open macro file \"%s\"\n";
char   *maximumNumberOfMacrosExceeded = "Maximum number of macros exceeded\n";
char   *outOfMacroMemory = "Out of memory allocating macro memory\n";
char   *unknownMacro = "Unknown macro \"%s\"\n";
#endif                                /* !SERVER */

char   *nameByAuthorScores = "%s by %s scores %d\n";
char   *resultsAre = "  Results:";
char   *resultsWLT = "Results: %d %d %d\n";

/*
 * display files
 */

#ifdef DOSTXTGRAPHX
#ifdef CURSESGRAPHX
char   *cannotAllocateVirtualScreen = "Cannot allocate memory for virtual screen";
char   *cannotOpenNewWindow = "Cannot open new window";
char   *errorInCurdispc = "Error in curdisp.c";
char   *cannotAllocateWindowMemory = "Cannot allocate memory for window";
char   *cursesError = "Curses error\n";
#else
#if defined(DJGPP) || defined(WATCOM)
char   *cannotAllocateScreenBuffers = "Cannot allocate textmode screen buffers\n";
#endif                                /* DJGPP */
#endif                                /* CURSESGRAPHX */
#endif                                /* DOSTXTGRAPHX */

/* strings shared between displays */
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
char   *pressAnyKey = "Press any key ..";
#endif
#ifdef DOSGRXGRAPHX
#ifdef DJGPP
/* grxdisp.c strings here */
#else
/* bgidisp.c strings here */
#endif                                /* DJGPP */
#endif                                /* DOSGRXGRAPHX */

#ifdef LINUXGRAPHX
/* lnxdisp.c strings here */
char   *cantReadConsole = "Can't read /dev/console";
char   *selectFails = "Select on /dev/console fails";
char   *tcsetattrFails = "tcsetattr() fails";
char   *modeNotAvail = "Mode %dx%dx256 not available, trying 640x480x256";
char   *tryingNext = ", trying 320x200x256";
char   *noModes = "\nCan't even open 320x200x256, giving up.\n";
char   *noMouse = "Can't initialize mouse support: %s\n";
char   *cantSetMode = "Can't set graphics mode (%dx%s)\n";
#endif                                /* LINUXGRAPHX */

#ifdef XWINGRAPHX
/* xwindisp.c strings here */
char   *cantAllocMem = "Can't allocate memory\n";
char   *cantConnect = "Can't connect to X server \"%s\"\n";
char   *structureAllocFails = "Structure Allocation fails\n";
char   *cantOpenFont = "Can't open font \"%s\", trying \"fixed\"\n";
char   *noFixedFont = "Can't even open font \"fixed\", giving up\n";
char   *noBackingStore = "X Server doesn't support backing store\n";
char   *needColorDisplay = "Need a color display\n";
char   *colorNotFound = "Color name \"%s\" not found in database\n";
char   *noColorAvailable = "Can't allocate color, all cells allocated and no one matches\n";
char   *privateMap = "Can't allocate color \"%s\"; switching to private colormap\n";
char   *invalidGeom = "Invalid geometry specification\n";
#endif                                /* XWINGRAPHX */

/* pmars.c */

#if defined(__BORLANDC__) && defined(CHECK386)
char   *stub386 = "This binary was compiled for dos 386 or better.\n";
#endif
char   *info01 = "Program \"%s\" (length %d) by \"%s\"\n\n";
#if defined(LINUXGRAPHX)
char   *cantInitSvga = "Can't initialize svga library.\n";
char   *cantOpenConsole = "Can't open /dev/console";
char   *tcgetattrFails = "tcgetattr() fails";
#endif

/* asm.c */

char   *logicErr = "Error in %s. Line: %d\n";
char   *labelRefMsg = "Label references:\n";
char   *groupLabel = "Group label(s):\n";
char   *textMsg = "(TEXT):";
char   *stackMsg = "(STACK):";
char   *labelMsg = "(LABEL):";
char   *endOfChart = "***END*OF*CHART***\n\n";
char   *afterPass = "After pass %d\n";
char   *instrMsg = "Instruction (physical line, instr):\n\n";
char   *endOfPass = "\n***END*OF*PASS***\n\n";
char   *unknown = "Unknown";
char   *anonymous = "Anonymous";
char   *illegalAppendErr = "Attempting to append a string to an undefined label";
char   *bufferOverflowErr = "Buffer overflow. Substitution is too complex";
char   *illegalConcatErr = "Illegal use of string concatenation '%s'";
char   *tooManyLabelsErr = "Too many labels for a declaration (last: '%s')";
char   *unopenedFORErr = "Unopened FOR";
char   *unclosedROFErr = "Unclosed ROF";
char   *bad88FormatErr = "Bad '88 format at token '%s'";
char   *badOffsetErr = "Execution starts from outside of the program";
char   *noInstErr = "No instructions";
char   *spaceRedErr = "Putting space between ';' and 'redcode' is useless";
char   *tokenErr = "Unrecognized or improper placement of token: '%s'";
char   *terminatedRedErr = "Source code terminated by ';redcode'";
char   *undefinedSymErr = "Undefined label or symbol: '%s'";
char   *expectNumberErr = "Expecting a number";
char   *syntaxErr = "Syntax error";
char   *discardLabelErr = "Discarding these labels: '%s'";
char   *tooManyInstrErr = "Too many instructions (about %s more)";
char   *missingErr = "Missing '%s'";
char   *recursiveErr = "Recursive reference of label '%s'";
char   *badExprErr = "Bad expression";
char   *divZeroErr = "Division by zero";
char   *overflowErr = "Arithmetic overflow detected";
char   *missingAssertErr = "Missing ';assert'. Warrior may not work with the current setting";
char   *concatErr = "Unable to derefer and concatenate symbol '%s'";
char   *ignoreENDErr = "Both opcodes ORG and END are used. Ignoring END";
char   *invalidAssertErr = "Invalid ';assert' parameter";
char   *tooMuchStuffErr = "Lines generated by the source exceed the limit %d\n\tIgnoring code generation";
char   *extraTokenErr = "Ignored, extra tokens in line '%s'";
char   *improperPlaceErr = "Improper placement of '%s'";
char   *invalid88Err = "Invalid '88 format. Proper format: '%s'";
char   *incompleteOpErr = "Incomplete operand at instruction '%s'";
char   *redefinitionErr = "Ignored, redefinition of label '%s'";
char   *undefinedLabelErr = "Undefined label '%s'";
char   *assertionFailErr = "Assertion in this line fails";
char   *tooManyMsgErr = "\nToo many errors or warnings.\nProgram aborted.\n";
char   *fileOpenErr = "Unable to open file '%s'";
char   *fileReadErr = "Unable to read file '%s'";
char   *notEnoughMemErr = "MALLOC() fails\nProgram aborted\n";
char   *warning = "Warning";
char   *error = "Error";
char   *inLine = " in line %d: '%s'\n";
char   *opcodeMsg = "opcode";
char   *modifierMsg = "modifier";
char   *aTerm = "A-term";
char   *bTerm = "B-term";
char   *currentAssertMsg = "Current parameter for ';assert': %s\n";
char   *currentFORMsg = "Current parameter for FOR: %s\n";
char   *CURLINEErr = "CURLINE is a reserved keyword";
char   *paramCheckMsg = "Parameter checking:\n";
char   *errNumMsg = "Number of errors: %d\n";
char   *warNumMsg = "Number of warnings: %d\n";
char   *duplicateMsg = "Duplicate errors/warnings found in line %d (%d)\n";

/*
 *  Strings from clparse.c:
 */

char   *credits_screen1 =
"pMARS v%d.%d.%d, %s, corewar simulator with ICWS'94 extensions\n";
char   *credits_screen2 =
"Copyright (C) 1993-95 Albert Ma, Na'ndor Sieben, Stefan Strack and Mintardjo Wangsaw\n";

#ifdef SERVER
char   *credits_screen3 =
"SERVER version without debugger\n";
#else
#ifdef DOSALLGRAPHX
char   *credits_screen3 =
"Graphics/Textmode display version by Na'ndor Sieben and Stefan Strack\n";
#else
#ifdef DOSGRXGRAPHX
char   *credits_screen3 =
"Graphics display version by Na'ndor Sieben\n";
#else
#ifdef CURSESGRAPHX
char   *credits_screen3 =
"Curses display version by Mintardjo Wangsaw and Stefan Strack\n";
#else
#ifdef DOSTXTGRAPHX
char   *credits_screen3 =
"Textmode display version by Stefan Strack\n";
#else
#ifdef MACGRAPHX
char   *credits_screen3 =
"Macintosh display version by Alex MacAulay\n";
#else
#ifdef LINUXGRAPHX
char   *credits_screen3 =
"Linux/SVGA display version by Martin Maierhofer\n";
#else
char   *credits_screen3 = "";
#endif                                /* LINUXGRAPHX */
#endif                                /* MACGRAPHX */
#endif                                /* DOSTXTGRAPHX */
#endif                                /* CURSESGRAPHX */
#endif                                /* DOSGRXGRAPHX */
#endif                                /* DOSALLGRAPHX */
#endif                                /* SERVER */

#ifdef VMS
char   *usage_screen =
"Ported to VMS by Nathan Summers\n\nUsage:\n   " _EXENAME " [options] file1 [files ..]\n   The special file - stands for sys$input.  It must be in quotes if it is the\n   last command on the line.\n\nAll uppercase options must be in quotes.\n";
#else
#if ! defined(CURSESGRAPHX) && defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX)
char   *usage_screen =
"Usage:\n   pmarsv [options] file1 [files ..]\n   The special file - stands for standard input\n\n";
#else
char   *usage_screen =
"Usage:\n   pmars [options] file1 [files ..]\n   The special file - stands for standard input\n\n";
#endif
#endif


char   *optionsAre = "Options:\n";
#ifdef VMS
char   *readingStdin = "[Reading from sys$input until Ctrl-Z or \"$\"]\n";
#else
char   *readingStdin = "[Reading from standard input until EOF or \"$\"]\n";
#endif                                /* VMS */
char   *readOptionsFromFile = "  -@ $ Read options from file $\n";
#ifdef VMS
char   *standardInput = "sys$input";
#else
char   *standardInput = "standard input";
#endif                                /* VMS */
char   *errorIsLocatedIn = "The error is located in \"%s\"\n";
char   *unknownOption = "\nUnknown option \"%s\"\n";
char   *badArgumentForSwitch = "\nBad argument for \"-%c\" switch\n";
#if defined(XWINGRAPHX)
char   *badArgumentForXSwitch = "\nBad argument for \"-%s\" switch\n";
#endif
char   *optionMustBeInTheRange = "\nOption \"-%c\" value must be in the range ";
char   *tooManyParameters = "\nToo many parameters \"%s\"\n";
char   *cannotOpenParameterFile = "\nCannot open parameter file\n";
char   *optRounds = "Rounds to play [1]";
char   *optEnterDebugger = "Enter debugger";
char   *optDisabledInServerVersion = "(disabled in SERVER version)";
char   *optCoreSize = "Size of core [8000]";
char   *optBrief = "Brief mode (no source listings)";
char   *optCycles = "Cycles until tie [80000]";
char   *optVerboseAssembly = "Verbose assembly";
char   *optProcesses = "Max. processes [8000]";
char   *optKotH = "Output in KotH format";
char   *optLength = "Max. warrior length [100]";
char   *opt88 = "Enforce ICWS'88 rules";
char   *optDistance = "Min. warriors distance";
char   *optFixedSeries = "Fixed position series";
char   *optFixedPosition = "Fixed position of warrior #2";
char   *optSort = "Sort result output by score";
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX)\
    || defined(XWINGRAPHX)
char   *optView = "Coreview mode [103]";
#endif
char   *optScoreFormula = "Score formula $ [(W*W-1)/S]";
#ifdef VMS
char   *optDIAOutput = "Generate DIA diagnostics";
#endif
#ifdef PSPACE
char   *optPSpaceSize = "Size of P-space [1/16th core]";
#endif
#ifdef PERMUTATE
char   *optPermutate = "Permutate starting positions";
#endif
#if defined(XWINGRAPHX)
char   *optXOpt[] = {
  "Display to connect to",
  "Geometry of the window",
  "X font to use"
};
#endif
char   *noWarriorFile = "\nNo warrior file specified\n";
char   *fFExclusive = "\nOnly one of -f and -F can be given\n";
char   *coreSizeTooSmall = "\nCore size is too small\n";
char   *dLessThanl = "\nWarrior distance cannot be smaller than warrior length\n";
char   *FLessThand = "\nPosition of warrior #2 cannot be smaller than warrior distance\n";
char   *badScoreFormula = "\nBad score formula\n";
#ifdef PSPACE
char   *pSpaceTooBig = "\nP-space is bigger than core\n";
#endif
#ifdef PERMUTATE
char   *permutateMultiWarrior = "\nPermutation cannot be used in multiwarrior battles\n";
#endif
/* These strings are used by the VMS Language Sensitive Editor */
#ifdef VMS
char   *StartDia = "START DIAGNOSTIC\n";
char   *Region = "REGION";
char   *AllLine = "/COL=(1,65535)";
char   *LineQualifier = "/LINE=";
char   *Label = "/LABEL=";
char   *LSEWarn = "Warning";
char   *LSEErr = "Error";
char   *Message = "MESSAGE";
char   *EndDia = "END DIAGNOSTIC\n";
char   *StartMod = "START MODULE\n";
char   *Opening = "Opening";
char   *EndMod = "END MODULE\n";
#endif                                /* VMS */

#endif                                /* PMARSLANG == ENGLISH */
