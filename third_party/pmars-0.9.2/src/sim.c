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
 * sim.c: simulator
 * $Id: sim.c,v 1.3 2000/12/25 00:49:08 iltzu Exp $
 *
 * 10-23-98 Pentium optimized version 30% faster than the original
 *          Ken Espiritu
 */

#include "global.h"
#include "sim.h"
#include <time.h>

#ifdef unix
#include <signal.h>
#endif
#ifdef DOS16
#include <dos.h>
#endif
#ifdef DJGPP
#include <pc.h>
extern void sighandler(int dummy);
#endif


#ifdef TRACEGRAPHX
#include "trace.c"
#else
#if defined(MACGRAPHX)
#include "macdisp.c"
#else
#if defined(OS2PMGRAPHX)
#include "pmdisp.h"
#else
#if defined(GRAPHX)

#ifdef CURSESGRAPHX
#include "curdisp.c"
#else

#ifdef DOSTXTGRAPHX
#ifdef DJGPP
#include "gtdisp.c"
#else
#ifdef WATCOM
#include "wtdisp.c"
#else
#include "tdisp.c"
#endif
#endif                                /* DJGPP */
#endif                                /* DOSTXTGRAPHX */

#ifdef DOSGRXGRAPHX
#ifdef DJGPP
#include "grxdisp.c"
#else
#ifdef WATCOM
#include "wgdisp.c"
#else
#include "bgidisp.c"
#endif
#endif                                /* DJGPP */
#endif                                /* DOSGRXGRAPHX */

#ifdef DOSALLGRAPHX
#include "alldisp.c"
#endif

#endif                                /* !CURSESGRAPHX */
#ifdef LINUXGRAPHX
#include "lnxdisp.c"
#else
#ifdef XWINGRAPHX
#include "xwindisp.c"
#else
#include "uidisp.c"
#endif
#endif

#else                                /* !GRAPHX */
#define display_init()
#define display_clear()
#define display_read(addr)
#define display_write(addr)
#define display_dec(addr)
#define display_inc(addr)
#define display_exec(addr)
#define display_spl(warrior,tasks)
#define display_dat(address,warrior,tasks)
#define display_die(warnum)
#define display_close()
#define display_cycle()
#define display_push(val)
#endif                                /* TRACEGRAPHX */
#endif
#endif
#endif

#ifdef DOS16
#define push(val) *W->taskTail++=(val)
#else
#define push(val) \
do { \
 *W->taskTail=(val); \
  if (++W->taskTail==endQueue) W->taskTail=taskQueue;\
  display_push(val);\
  } while (0)
#endif

#ifdef PSPACE
#define get_pspace(idx) (((idx) % pSpaceSize) ?\
	    *(pSpace[W->pSpaceIndex] + ((idx) % pSpaceSize)) : W->lastResult)
#define set_pspace(idx,value) do {\
	    if ((idx) % pSpaceSize) \
		*(pSpace[W->pSpaceIndex] + ((idx) % pSpaceSize)) = value;\
	    else W->lastResult = value;\
	    } while(0)
#endif

#define OP(opcode,modifier) (opcode<<3)+modifier
#define ADDMOD(A,B,C) do { if ((C=(int) A+B)>=coreSize) C-=coreSize; } \
while (0)
#define SUBMOD(A,B,C) do { if ((C=(int) A-B)<0) C+=coreSize; } \
while (0)

#ifdef NEW_STYLE
extern int posit(void);
extern void npos(void);
extern S32_T rng(S32_T seed);
#else
extern int posit();
extern void npos();
extern S32_T rng();
#endif

/* strings */
extern char *outOfMemory;
extern char *warriorTerminated;
extern char *fatalErrorInSimulator;
extern char *warriorTerminatedEndOfRound;
extern char *endOfRound;

warrior_struct *W;                /* indicate which warrior is running */
U32_T   totaltask;                /* size of the taskQueue */
ADDR_T FAR *endQueue;
ADDR_T FAR *taskQueue;
ADDR_T  progCnt;                /* program counter */

  mem_struct FAR *destPtr;        /* pointer used to copy program to core */
  mem_struct FAR *tempPtr;        /* temporary pointer used in op decode phase */

  mem_struct IR;                /* current instruction and A cell */
#ifdef NEW_MODES
//  mem_struct IRA;                /* A/B_field hold A-field of A/B-pointer
//                                 * necessary for '}' mode */
ADDR_T AA_Value, AB_Value;
#endif

mem_struct FAR *memory;

long    cycle;
int     round;

char    alloc_p = 0;                /* indicate whether memory has been allocated */
int     warriorsLeft;                /* number of warriors still left in core */

warrior_struct *endWar;                /* end of the warriors array */

/*--------------------*/
#ifdef NEW_STYLE
S32_T
checksum_warriors(void)
#else
S32_T
checksum_warriors()
#endif

{
  int     shuffle = 0;                /* XOR with position-dependent int */
  S32_T   checksum = 0;                /* accumulator */
  mem_struct *source;
  mem_struct *last;

  /*
   * create a checksum for RNG seed based on warrior code; debuginfo is not
   * used.
   */
  W = warrior;
  do {
    source = W->instBank;
    last = source + W->instLen;
    while (source != last) {
      checksum += source->opcode ^ (shuffle++);
      checksum += source->A_mode ^ (shuffle++);
      checksum += source->B_mode ^ (shuffle++);
      checksum += source->A_value ^ (shuffle++);
      checksum += source->B_value ^ (shuffle++);
      ++source;
    }
    ++W;
  } while (W < endWar);
  return checksum;
}

void
simulator1()
{
#ifdef PERMUTATE
  int permidx = 0, permtmp, *permbuf = NULL;
#endif
  /* range for random number generator */
  warrior_struct *oldW;                /* the previous living warrior to execute */
  ADDR_T  positions = coreSize + 1 - (separation << 1);
  ADDR_T  coreSize1 = coreSize - 1;
  warrior_struct *starter = warrior;        /* pointer to warrior that starts
					 * round */
  U32_T   cycles2 = warriors * cycles;
#ifndef SERVER
  char    outs[60];                /* for cdb() entering message */
#endif
  mem_struct *sourcePtr;        /* pointer used to copy program to core */
  mem_struct *endPtr;                /* pointer used to copy program to core */
register  int     temp;                        /* general purpose temporary variable */
  int     addrA, addrB;                /* A and B pointers */
#ifdef TRACEGRAPHX
  int     temp2;
#endif
  ADDR_T FAR *tempPtr2;
#ifdef NEW_MODES
  ADDR_T FAR *offsPtr;                /* temporary pointer used in op decode phase */
#endif

  endWar = warrior + warriors;

#ifdef PERMUTATE
  if (SWITCH_P) {
    permbuf = (int *) malloc((size_t)(warriors * positions * sizeof(int)));
    if (!permbuf) {
      errout(outOfMemory);
      Exit(MEMERR);
    }
  }
#endif

#ifdef DOS16
  if (!alloc_p) {
    memory = farmalloc((long) coreSize * sizeof(mem_struct));
    if (!memory) {
      errout(outOfMemory);
      Exit(MEMERR);
    }
    taskQueue = farmalloc(65528L);        /* allocate an entire 64K segment */
    if (!taskQueue) {
      farfree(memory);
      errout(outOfMemory);
      Exit(MEMERR);
    }
    /* zero the offset so the weird segment hack works */
    memory = (mem_struct far *) MK_FP(FP_SEG(memory), 0);
    taskQueue = (ADDR_T far *) MK_FP(FP_SEG(taskQueue), 0);
    alloc_p = 1;
    endQueue = taskQueue + totaltask;        /* memory; */
  }
#else
  if (!alloc_p) {
    memory = (mem_struct *) malloc((size_t) coreSize * sizeof(mem_struct));
    if (!memory) {
      errout(outOfMemory);
      Exit(MEMERR);
    }
    totaltask = (U32_T) taskNum *warriors + 1;
    taskQueue = (ADDR_T *) malloc((size_t) totaltask * sizeof(ADDR_T));
    if (!taskQueue) {
      free(memory);
      errout(outOfMemory);
      Exit(MEMERR);
    }
    alloc_p = 1;
    endQueue = taskQueue + totaltask;
  }
#endif
  if (SWITCH_e)
    debugState = STEP;                /* automatically enter debugger */
  if (!debugState)
    copyDebugInfo = TRUE;        /* this makes things a little faster */
  seed = SWITCH_F ?
    (SWITCH_F - separation) :        /* seed from argument */
  /* seed from either checksum or time */
    rng(SWITCH_f ? checksum_warriors() : time(0));
#ifdef PERMUTATE
  if (SWITCH_F && SWITCH_P)
    seed *= warriors; /* get table index from position */
#endif

  display_init();
  round = 1;
  do {                                /* each round */
#if defined(DOS16) && !defined(SERVER) && !defined(DOSTXTGRAPHX) && !defined(DOSGRXGRAPHX) && !defined(DJGPP)
    fputc('\r', stdout);        /* enable interruption by Ctrl-C */
#else
#if defined(DJGPP) && !defined(SERVER) && !defined(DOSTXTGRAPHX) && !defined(DOSGRXGRAPHX) && !defined(CURSESGRAPHX)
    if (kbhit())
      sighandler(0);
#endif
#endif
    warriorsLeft = warriors;
    cycle = cycles2;
    if (warriors > 1) {
      if (warriors == 2) {
#ifdef PERMUTATE
        if (SWITCH_P) {
          if (permidx == 0) { /* initialize buffer */
            for (;permidx < warriors*positions; permidx++)
              permbuf[permidx] = permidx;
          }
          permtmp = seed % permidx;
          warrior[1].position = separation + permbuf[permtmp]/warriors;
          starter = warrior + permbuf[permtmp] % warriors;
          permbuf[permtmp] = permbuf[--permidx];
        } else
#endif
	  warrior[1].position = separation + seed % positions;
	seed = rng(seed);
      } else {
	if (posit())
	  npos();                /* use back-up positioning algo npos if posit
				 * fails */
      }
    }
    /* create nextWarrior links each round */
    /* leave oldW pointing to last warrior */
    for (oldW = warrior; oldW < endWar - 1; ++oldW)
      oldW->nextWarrior = oldW + 1;
    oldW->nextWarrior = warrior;
    W = starter;
    if (starter != warrior)
      oldW = starter - 1;
    addrA = warrior[0].instLen;
    /* clear the core following warrior 0 */
    do {
      memory[addrA] = INITIALINST;
    } while (++addrA < coreSize);
    tempPtr2 = endQueue - taskNum - 1;
    temp = 0;
    do {
      /* initialize head, tail, and taskQueue */
      W->taskHead = tempPtr2;
      W->taskTail = tempPtr2 + 1;
      *tempPtr2 = W->position + W->offset;
      W->tasks = 1;
      tempPtr2 -= taskNum;
      destPtr = memory + W->position;
      sourcePtr = W->instBank;
      endPtr = sourcePtr + W->instLen;
      /* copy the warriors to core */
      while (sourcePtr != endPtr) {
	*destPtr++ = *sourcePtr++;
      }
      display_spl(temp, 1);
      W = W->nextWarrior;
    } while (++temp < warriors);

    display_clear();
    /* the inner loop of execution */
    do {                        /* each cycle */
      display_cycle();
     // progCnt = *(W->taskHead++);
     // IR = memory[progCnt];        /* copy instruction into register */
	IR = memory[(progCnt= *(W->taskHead++))];        
#ifndef DOS16
      if (W->taskHead == endQueue)
	W->taskHead = taskQueue;
#endif
#ifndef SERVER
      if (debugState && ((debugState == STEP) || memory[progCnt].debuginfo))
	debugState = cdb("");
#endif
#ifdef NEW_MODES
      AB_Value = IR.A_value;        /* necessary if B-mode is immediate */
#endif

      /*
       * evaluate A operand.  This is the hardest part of the entire
       * simulator code.  Obfuscated C code at it's finest
       */
if (IR.A_mode != (FIELD_T) IMMEDIATE)
{
	ADDMOD(IR.A_value, progCnt, addrA);
	tempPtr = &memory[addrA];

	if (IR.A_mode != (FIELD_T) DIRECT)
	{
		#ifdef NEW_MODES
		if (INDIR_A(IR.A_mode))
		{
			IR.A_mode = RAW_MODE(IR.A_mode);
			offsPtr = &(tempPtr->A_value);
		} else
			offsPtr = &(tempPtr->B_value);

		temp = *offsPtr;
		#endif
		if (IR.A_mode == (FIELD_T) PREDECR)
		{
			if (--temp < 0)
				temp = coreSize1;
			#ifdef NEW_MODES
			*offsPtr = temp;
			#else
			tempPtr->B_value = temp;
			#endif
			display_dec(addrA);
		}
		/* jk - added os2 part */
		#ifdef GRAPHX
		display_inc(addrA);
		#else
		#if defined(TRACEGRAPHX)
		temp2 = addrA;        /* make the trace accurate */
		#endif
		#endif
		ADDMOD(temp, addrA, addrA);

		destPtr = &memory[addrA];       //new
		#ifdef NEW_MODES
		AA_Value = destPtr->A_value;
		#endif
		IR.A_value = destPtr->B_value;

		if (IR.A_mode == (FIELD_T) POSTINC)
		{
			if (++temp == coreSize)
				temp = 0;
			#ifdef NEW_MODES
			*offsPtr = temp;
			#else
			tempPtr->B_value = temp;
			#endif
			#ifdef TRACEGRAPHX
			display_inc(temp2);
			#endif
		}
	} else
	{
	#ifdef NEW_MODES

	IR.A_value = tempPtr->B_value;
	AA_Value = tempPtr->A_value;

	#else
	IR.A_value = temp = tempPtr->B_value;
	#endif
	}
} else
{
	#ifdef NEW_MODES
	AA_Value = IR.A_value;
	#endif
	addrA = progCnt;

	IR.A_value = IR.B_value;
}

      /*
       * evaluate B operand.  This is the hardest part of the entire
       * simulator code.  Obfuscated C code at it's finest
       */
 //     addrB = progCnt;
if (IR.B_mode != (FIELD_T) IMMEDIATE)
{
	ADDMOD(IR.B_value, progCnt, addrB);
	 tempPtr = &memory[addrB];

	if (IR.B_mode != (FIELD_T) DIRECT)
	{
		#ifdef NEW_MODES
		if (INDIR_A(IR.B_mode))
		{
			IR.B_mode = RAW_MODE(IR.B_mode);
			offsPtr = &(tempPtr->A_value);
		} else
			offsPtr = &(tempPtr->B_value);
		temp = *offsPtr;
		#endif
		if (IR.B_mode == (FIELD_T) PREDECR)
		{
			if (--temp < 0)
				temp = coreSize1;
			#ifdef NEW_MODES
			*offsPtr = temp;
			#else
			tempPtr->B_value = temp;
			#endif
			display_dec(addrB);
		}
		/* jk - added os2 part */
		#ifdef GRAPHX
		display_inc(addrB);
		#else
		#if defined(TRACEGRAPHX)
		temp2 = addrB;
		#endif
		#endif

		ADDMOD(temp, addrB, addrB);

		destPtr = &memory[addrB];
		#ifdef NEW_MODES
		AB_Value = destPtr->A_value;
		#endif

		IR.B_value = destPtr->B_value;

		if (IR.B_mode == (FIELD_T) POSTINC)
		{
			if (++temp == coreSize)
				temp = 0;
			#ifdef NEW_MODES
			*offsPtr = temp;
			#else
			tempPtr->B_value = temp;
			#endif
			#ifdef TRACEGRAPHX
			display_inc(temp2);
			#endif
		}
	} else
	{
	#ifdef NEW_MODES
	AB_Value = tempPtr->A_value;

	IR.B_value = tempPtr->B_value;
	#else
	IR.B_value = temp = tempPtr->B_value;
	#endif
	}
} else
      addrB = progCnt;
      /*
       * addrA holds the A-pointer IR.A_value holds the B operand of the
       * A-pointer cell. temp holds the B-pointer IR.B_value holds the B
       * operand of the B-pointer cell.
       */
#ifdef NEW_MODES
      /*
       * IRA_A_value holds the A operand of the A-pointer cell IRA.B_value
       * holds the A operand of the B-pointer cell.
       */

#define ADDRA_AVALUE AA_Value
#define ADDRB_AVALUE AB_Value
#else
#define ADDRA_AVALUE memory[addrA].A_value
#define ADDRB_AVALUE memory[addrB].A_value
#endif

      /* execute it! */
      display_exec(progCnt);
      switch (IR.opcode) {

      case OP(MOV, mA):
	display_read(addrA);
	memory[addrB].A_value = ADDRA_AVALUE;
	display_write(addrB);
	break;

      case OP(MOV, mF):
	memory[addrB].A_value = ADDRA_AVALUE;
	/* FALLTHRU */
      case OP(MOV, mB):
	display_read(addrA);
	memory[addrB].B_value = IR.A_value;
	display_write(addrB);
	break;

      case OP(MOV, mAB):
	display_read(addrA);
	memory[addrB].B_value = ADDRA_AVALUE;
	display_write(addrB);
	break;

      case OP(MOV, mX):
	memory[addrB].B_value = ADDRA_AVALUE;
	/* FALLTHRU */
      case OP(MOV, mBA):
	display_read(addrA);
	memory[addrB].A_value = IR.A_value;
	display_write(addrB);
	break;

#ifdef PERMUTATE
  if (SWITCH_P) {
    permbuf = (int *) malloc((size_t)(warriors * positions * sizeof(int)));
    if (!permbuf) {
      errout(outOfMemory);
      Exit(MEMERR);
    }
  }
#endif

      case OP(ADD, mA):
	display_read(addrA);
	ADDMOD(ADDRB_AVALUE, ADDRA_AVALUE, temp);
	memory[addrB].A_value = temp;
	display_write(addrB);
	break;

      case OP(ADD, mI):
      case OP(ADD, mF):
	ADDMOD(ADDRB_AVALUE, ADDRA_AVALUE, temp);
	memory[addrB].A_value = temp;
	/* FALLTHRU */
      case OP(ADD, mB):
	display_read(addrA);
	ADDMOD(IR.B_value, IR.A_value, temp);
	memory[addrB].B_value = temp;
	display_write(addrB);
	break;

      case OP(ADD, mAB):
	display_read(addrA);
	ADDMOD(IR.B_value, ADDRA_AVALUE, temp);
	memory[addrB].B_value = temp;
	display_write(addrB);
	break;

      case OP(ADD, mX):
	ADDMOD(IR.B_value, ADDRA_AVALUE, temp);
	memory[addrB].B_value = temp;
	/* FALLTHRU */
      case OP(ADD, mBA):
	display_read(addrA);
	ADDMOD(ADDRB_AVALUE, IR.A_value, temp);
	memory[addrB].A_value = temp;
	display_write(addrB);
	break;


      case OP(SUB, mA):
	display_read(addrA);
	SUBMOD(ADDRB_AVALUE, ADDRA_AVALUE, temp);
	memory[addrB].A_value = temp;
	display_write(addrB);
	break;

      case OP(SUB, mI):
      case OP(SUB, mF):
	SUBMOD(ADDRB_AVALUE, ADDRA_AVALUE, temp);
	memory[addrB].A_value = temp;
	/* FALLTHRU */
      case OP(SUB, mB):
	display_read(addrA);
	SUBMOD(IR.B_value, IR.A_value, temp);
	memory[addrB].B_value = temp;
	display_write(addrB);
	break;

      case OP(SUB, mAB):
	display_read(addrA);
	SUBMOD(IR.B_value, ADDRA_AVALUE, temp);
	memory[addrB].B_value = temp;
	display_write(addrB);
	break;

      case OP(SUB, mX):
	SUBMOD(IR.B_value, ADDRA_AVALUE, temp);
	memory[addrB].B_value = temp;
	/* FALLTHRU */
      case OP(SUB, mBA):
	display_read(addrA);
	SUBMOD(ADDRB_AVALUE, IR.A_value, temp);
	memory[addrB].A_value = temp;
	display_write(addrB);
	break;


	/* the cast prevents overflow */
      case OP(MUL, mA):
	display_read(addrA);
	memory[addrB].A_value =
	  (U32_T) ADDRB_AVALUE *ADDRA_AVALUE % coreSize;
	display_write(addrB);
	break;

      case OP(MUL, mI):
      case OP(MUL, mF):
	memory[addrB].A_value =
	  (U32_T) ADDRB_AVALUE *ADDRA_AVALUE % coreSize;
	/* FALLTHRU */
      case OP(MUL, mB):
	display_read(addrA);
	memory[addrB].B_value =
	  (U32_T) IR.B_value * IR.A_value % coreSize;
	display_write(addrB);
	break;

      case OP(MUL, mAB):
	display_read(addrA);
	memory[addrB].B_value =
	  (U32_T) IR.B_value * ADDRA_AVALUE % coreSize;
	display_write(addrB);
	break;

      case OP(MUL, mX):
	memory[addrB].B_value =
	  (U32_T) IR.B_value * ADDRA_AVALUE % coreSize;
	/* FALLTHRU */
      case OP(MUL, mBA):
	display_read(addrA);
	memory[addrB].A_value =
	  (U32_T) ADDRB_AVALUE *IR.A_value % coreSize;
	display_write(addrB);
	break;


      case OP(DIV, mA):
	display_read(addrA);
	if (!ADDRA_AVALUE)
	  goto die;
	memory[addrB].A_value = ADDRB_AVALUE / ADDRA_AVALUE;
	display_write(addrB);
	break;

      case OP(DIV, mB):
	display_read(addrA);
	if (!IR.A_value)
	  goto die;
	memory[addrB].B_value = IR.B_value / IR.A_value;
	display_write(addrB);
	break;

      case OP(DIV, mAB):
	display_read(addrA);
	if (!ADDRA_AVALUE)
	  goto die;
	memory[addrB].B_value = IR.B_value / ADDRA_AVALUE;
	display_write(addrB);
	break;

      case OP(DIV, mBA):
	display_read(addrA);
	if (!IR.A_value)
	  goto die;
	memory[addrB].A_value = ADDRB_AVALUE / IR.A_value;
	display_write(addrB);
	break;

      case OP(DIV, mI):
      case OP(DIV, mF):
	display_read(addrA);
	if (ADDRA_AVALUE) {
	  memory[addrB].A_value = ADDRB_AVALUE / ADDRA_AVALUE;
	  display_write(addrB);
	  if (!IR.A_value)
	    goto die;
	  memory[addrB].B_value = IR.B_value / IR.A_value;
	  display_write(addrB);
	  break;
	} else {
	  if (!IR.A_value)
	    goto die;
	  memory[addrB].B_value = IR.B_value / IR.A_value;
	  display_write(addrB);
	  goto die;
	}
      case OP(DIV, mX):
	display_read(addrA);
	if (IR.A_value) {
	  memory[addrB].A_value = ADDRB_AVALUE / IR.A_value;
	  display_write(addrB);
	  if (!ADDRA_AVALUE)
	    goto die;
	  memory[addrB].B_value = IR.B_value / ADDRA_AVALUE;
	  display_write(addrB);
	  break;
	} else {
	  if (!ADDRA_AVALUE)
	    goto die;
	  memory[addrB].B_value = IR.B_value / ADDRA_AVALUE;
	  display_write(addrB);
	  goto die;
	}

      case OP(MOD, mA):
	display_read(addrA);
	if (!ADDRA_AVALUE)
	  goto die;
	memory[addrB].A_value = ADDRB_AVALUE % ADDRA_AVALUE;
	display_write(addrB);
	break;

      case OP(MOD, mB):
	display_read(addrA);
	if (!IR.A_value)
	  goto die;
	memory[addrB].B_value = IR.B_value % IR.A_value;
	display_write(addrB);
	break;

      case OP(MOD, mAB):
	display_read(addrA);
	if (!ADDRA_AVALUE)
	  goto die;
	memory[addrB].B_value = IR.B_value % ADDRA_AVALUE;
	display_write(addrB);
	break;

      case OP(MOD, mBA):
	display_read(addrA);
	if (!IR.A_value)
	  goto die;
	memory[addrB].A_value = ADDRB_AVALUE % IR.A_value;
	display_write(addrB);
	break;

      case OP(MOD, mI):
      case OP(MOD, mF):
	display_read(addrA);
	if (ADDRA_AVALUE) {
	  memory[addrB].A_value = ADDRB_AVALUE % ADDRA_AVALUE;
	  display_write(addrB);
	  if (!IR.A_value)
	    goto die;
	  memory[addrB].B_value = IR.B_value % IR.A_value;
	  display_write(addrB);
	  break;
	} else {
	  if (!IR.A_value)
	    goto die;
	  memory[addrB].B_value = IR.B_value % IR.A_value;
	  display_write(addrB);
	  goto die;
	}

      case OP(MOD, mX):
	display_read(addrA);
	if (IR.A_value) {
	  memory[addrB].A_value = ADDRB_AVALUE % IR.A_value;
	  display_write(addrB);
	  if (!ADDRA_AVALUE)
	    goto die;
	  memory[addrB].B_value = IR.B_value % ADDRA_AVALUE;
	  display_write(addrB);
	  break;
	} else {
	  if (!ADDRA_AVALUE)
	    goto die;
	  memory[addrB].B_value = IR.B_value % ADDRA_AVALUE;
	  display_write(addrB);
	  goto die;
	}


      case OP(JMZ, mA):
      case OP(JMZ, mBA):
	display_read(addrB);
	if (ADDRB_AVALUE)
	  break;
	push(addrA);
	goto nopush;

      case OP(JMZ, mF):
      case OP(JMZ, mX):
      case OP(JMZ, mI):
	display_read(addrB);
	if (ADDRB_AVALUE)
	  break;
	/* FALLTHRU */
      case OP(JMZ, mB):
      case OP(JMZ, mAB):
	display_read(addrB);
	if (IR.B_value)
	  break;
	push(addrA);
	goto nopush;


      case OP(JMN, mA):
      case OP(JMN, mBA):
	display_read(addrB);
	if (!ADDRB_AVALUE)
	  break;
	push(addrA);
	goto nopush;

      case OP(JMN, mB):
      case OP(JMN, mAB):
	display_read(addrB);
	if (!IR.B_value)
	  break;
	push(addrA);
	goto nopush;

      case OP(JMN, mF):
      case OP(JMN, mX):
      case OP(JMN, mI):
	display_read(addrB);
	if (!ADDRB_AVALUE && !IR.B_value)
	  break;
	push(addrA);
	goto nopush;

      case OP(DJN, mA):
      case OP(DJN, mBA):
#ifdef NEW_MODES
	if (ISNEG(--memory[addrB].A_value))
	  memory[addrB].A_value = coreSize1;
	display_dec(addrB);
	if (AB_Value == 1)
	  break;
#else
	if (!--memory[addrB].A_value)
	  break;
	display_dec(addrB);
	if (ISNEG(memory[addrB].A_value))
	  memory[addrB].A_value = coreSize1;
#endif
	push(addrA);
	goto nopush;


      case OP(DJN, mB):
      case OP(DJN, mAB):
	if (ISNEG(--memory[addrB].B_value))
	  memory[addrB].B_value = coreSize1;
	display_dec(addrB);
	if (IR.B_value == 1)
	  break;
	push(addrA);
	goto nopush;

      case OP(DJN, mF):
      case OP(DJN, mI):
      case OP(DJN, mX):
	if (ISNEG(--memory[addrB].B_value))
	  memory[addrB].B_value = coreSize1;
	if (ISNEG(--memory[addrB].A_value))
	  memory[addrB].A_value = coreSize1;
	display_dec(addrB);
#ifdef NEW_MODES
	if ((AB_Value == 1) && (IR.B_value == 1))
	  break;
#else
	if ((!memory[addrB].A_value) && (IR.B_value == 1))
	  break;
#endif
	push(addrA);
	goto nopush;

#ifdef NEW_OPCODES
      case OP(SEQ, mA):
#endif
      case OP(CMP, mA):
	display_read(addrB);
	display_read(addrA);
	if (ADDRB_AVALUE != ADDRA_AVALUE)
	  break;
	ADDMOD(progCnt, 2, temp);
	goto pushtemp;

#ifdef NEW_OPCODES
      case OP(SEQ, mI):
#endif
      case OP(CMP, mI):
	display_read(addrB);
	display_read(addrA);
	if ((memory[addrB].opcode != memory[addrA].opcode) ||
	    (memory[addrB].A_mode != memory[addrA].A_mode) ||
	    (memory[addrB].B_mode != memory[addrA].B_mode))
	  break;
	/* FALLTHRU */
#ifdef NEW_OPCODES
      case OP(SEQ, mF):
#endif
      case OP(CMP, mF):
	display_read(addrB);
	display_read(addrA);
	if (ADDRB_AVALUE != ADDRA_AVALUE)
	  break;
	/* FALLTHRU */
#ifdef NEW_OPCODES
      case OP(SEQ, mB):
#endif
      case OP(CMP, mB):
	display_read(addrB);
	display_read(addrA);
	if (IR.B_value != IR.A_value)
	  break;
	ADDMOD(progCnt, 2, temp);
	goto pushtemp;

#ifdef NEW_OPCODES
      case OP(SEQ, mAB):
#endif
      case OP(CMP, mAB):
	display_read(addrB);
	display_read(addrA);
	if (IR.B_value != ADDRA_AVALUE)
	  break;
	ADDMOD(progCnt, 2, temp);
	goto pushtemp;

#ifdef NEW_OPCODES
      case OP(SEQ, mX):
#endif
      case OP(CMP, mX):
	display_read(addrB);
	display_read(addrA);
	if (IR.B_value != ADDRA_AVALUE)
	  break;
	/* FALLTHRU */
#ifdef NEW_OPCODES
      case OP(SEQ, mBA):
#endif
      case OP(CMP, mBA):
	display_read(addrB);
	display_read(addrA);
	if (ADDRB_AVALUE != IR.A_value)
	  break;
	ADDMOD(progCnt, 2, temp);
	goto pushtemp;

#ifdef NEW_OPCODES
      case OP(SNE, mA):
	display_read(addrB);
	display_read(addrA);
	if (ADDRB_AVALUE != ADDRA_AVALUE)
	  goto skip;
	break;
    skip:
	ADDMOD(progCnt, 2, temp);
	goto pushtemp;

      case OP(SNE, mI):
	display_read(addrB);
	display_read(addrA);
	if ((memory[addrB].opcode != memory[addrA].opcode) ||
	    (memory[addrB].A_mode != memory[addrA].A_mode) ||
	    (memory[addrB].B_mode != memory[addrA].B_mode))
	  goto skip;
	/* FALLTHRU */
      case OP(SNE, mF):
	display_read(addrB);
	display_read(addrA);
	if (ADDRB_AVALUE != ADDRA_AVALUE)
	  goto skip;
	/* FALLTHRU */
      case OP(SNE, mB):
	display_read(addrB);
	display_read(addrA);
	if (IR.B_value != IR.A_value)
	  goto skip;
	break;

      case OP(SNE, mAB):
	display_read(addrB);
	display_read(addrA);
	if (IR.B_value != ADDRA_AVALUE)
	  goto skip;
	break;

      case OP(SNE, mX):
	display_read(addrB);
	display_read(addrA);
	if (IR.B_value != ADDRA_AVALUE)
	  goto skip;
	/* FALLTHRU */
      case OP(SNE, mBA):
	display_read(addrB);
	display_read(addrA);
	if (ADDRB_AVALUE != IR.A_value)
	  goto skip;
	break;
#endif

      case OP(SLT, mA):
	display_read(addrB);
	display_read(addrA);
	if (ADDRB_AVALUE <= ADDRA_AVALUE)
	  break;
	ADDMOD(progCnt, 2, temp);
	goto pushtemp;

      case OP(SLT, mF):
      case OP(SLT, mI):
	display_read(addrB);
	display_read(addrA);
	if (ADDRB_AVALUE <= ADDRA_AVALUE)
	  break;
	/* FALLTHRU */
      case OP(SLT, mB):
	display_read(addrB);
	display_read(addrA);
	if (IR.B_value <= IR.A_value)
	  break;
	ADDMOD(progCnt, 2, temp);
	goto pushtemp;

      case OP(SLT, mAB):
	display_read(addrB);
	display_read(addrA);
	if (IR.B_value <= ADDRA_AVALUE)
	  break;
	ADDMOD(progCnt, 2, temp);
	goto pushtemp;

      case OP(SLT, mX):
	display_read(addrB);
	display_read(addrA);
	if (IR.B_value <= ADDRA_AVALUE)
	  break;
	/* FALLTHRU */
      case OP(SLT, mBA):
	display_read(addrB);
	display_read(addrA);
	if (ADDRB_AVALUE <= IR.A_value)
	  break;
	ADDMOD(progCnt, 2, temp);
	goto pushtemp;


      case OP(JMP, mA):
      case OP(JMP, mB):
      case OP(JMP, mAB):
      case OP(JMP, mBA):
      case OP(JMP, mF):
      case OP(JMP, mX):
      case OP(JMP, mI):
	push(addrA);
	goto nopush;


      case OP(SPL, mA):
      case OP(SPL, mB):
      case OP(SPL, mAB):
      case OP(SPL, mBA):
      case OP(SPL, mF):
      case OP(SPL, mX):
      case OP(SPL, mI):
//        temp = progCnt + 1;
	if ((temp = progCnt + 1) == coreSize)
	  temp = 0;
	push(temp);

	if (W->tasks >= taskNum)
	  goto nopush;
	++W->tasks;
	display_spl(W - warrior, W->tasks);
	push(addrA);
	goto nopush;


      case OP(DAT, mA):
      case OP(DAT, mB):
      case OP(DAT, mAB):
      case OP(DAT, mBA):
      case OP(DAT, mF):
      case OP(DAT, mX):
      case OP(DAT, mI):
    die:
	display_dat(progCnt, W - warrior, W->tasks);
	if (--W->tasks)
	  goto nopush;
	display_die(W - warrior);
	W->score[warriorsLeft + warriors - 2]++;
	cycle = cycle - 1 - (cycle - 1) / (warriorsLeft--);
	if (warriorsLeft < 2)
	  goto nextround;        /* can't use break because in switch */

#ifndef SERVER
	if (debugState == BREAK) {
	  sprintf(outs, warriorTerminated, W - warrior, W->name);
	  debugState = cdb(outs);
	}
#endif                                /* SERVER */
	oldW->nextWarrior = W = W->nextWarrior;
	continue;

	/* $EXT$ insert code for new opcodes here */

#ifdef NEW_OPCODES
      case OP(NOP, mA):
      case OP(NOP, mB):
      case OP(NOP, mAB):
      case OP(NOP, mBA):
      case OP(NOP, mF):
      case OP(NOP, mX):
      case OP(NOP, mI):
	break;
#endif

#ifdef PSPACE
      case OP(LDP, mA):
	display_read(addrA);
	memory[addrB].A_value = get_pspace(ADDRA_AVALUE);
	display_write(addrB);
	break;

      case OP(LDP, mF):
      case OP(LDP, mX):
      case OP(LDP, mI):
      case OP(LDP, mB):
	display_read(addrA);
	memory[addrB].B_value = get_pspace(IR.A_value);
	display_write(addrB);
	break;

      case OP(LDP, mAB):
	display_read(addrA);
	memory[addrB].B_value = get_pspace(ADDRA_AVALUE);
	display_write(addrB);
	break;

      case OP(LDP, mBA):
	display_read(addrA);
	memory[addrB].A_value = get_pspace(IR.A_value);
	display_write(addrB);
	break;

      case OP(STP, mA):
	display_read(addrA);
	set_pspace(ADDRB_AVALUE, ADDRA_AVALUE);
	break;

      case OP(STP, mF):
      case OP(STP, mX):
      case OP(STP, mI):
      case OP(STP, mB):
	display_read(addrA);
	set_pspace(IR.B_value, IR.A_value);
	break;

      case OP(STP, mAB):
	display_read(addrA);
	set_pspace(IR.B_value, ADDRA_AVALUE);
	break;

      case OP(STP, mBA):
	display_read(addrA);
	set_pspace(ADDRB_AVALUE, IR.A_value);
	break;
#endif                                /* PSPACE */
      case OP(MOV, mI):
	display_read(addrA);
#ifndef SERVER
	if (!copyDebugInfo)
	  temp = memory[addrB].debuginfo;
#endif
	memory[addrB] = memory[addrA];
#ifndef SERVER
	if (!copyDebugInfo)
	  memory[addrB].debuginfo = temp;
#endif
	memory[addrB].B_value = IR.A_value;
#ifdef NEW_MODES
	memory[addrB].A_value = AA_Value;
#endif
	display_write(addrB);
//        break;
#if 0
      default:
	errout(fatalErrorInSimulator);
#ifdef PERMUTATE
        if(permbuf) {
          free(permbuf);
          permbuf = NULL;
        }
#endif
#ifndef DOS16
	/* DOS taskQueue may not be free'd because of segment wrap-around */
	free(memory);
	free(taskQueue);
	alloc_p = 0;
#endif
	Exit(SERIOUS);
#endif
      }                                /* end switch */

      /* push the next instruction onto queue */
 //     temp = progCnt + 1;
      if ((temp=progCnt + 1) == coreSize)
	temp = 0;
  pushtemp:                        /* push whatever's in temp onto queue */
      push(temp);

  nopush:                        /* just go to the next warrior/cycle */
      oldW = W;
      W = W->nextWarrior;
//      --cycle;
    } while (--cycle);                /* next cycle */
nextround:
    for (temp = 0; temp < warriors; temp++) {
      if (warrior[temp].tasks) {
	warrior[temp].score[warriorsLeft - 1]++;
#ifdef PSPACE
	warrior[temp].lastResult = warriorsLeft;
      } else
	warrior[temp].lastResult = 0;
#else
      }
#endif
    }
    if (starter == endWar - 1)
      starter = warrior;
    else
      ++starter;
#ifndef SERVER
    if (debugState == BREAK) {
      if (warriorsLeft == 1 && warriors != 1)
	sprintf(outs, warriorTerminatedEndOfRound, W - warrior, W->name, round);
      else
	sprintf(outs, endOfRound, round);
      debugState = cdb(outs);
    }
#endif
  } while (++round <= rounds);

  display_close();
#ifdef PERMUTATE
  if(permbuf) {
    free(permbuf);
    permbuf = NULL;
  }
#endif
#ifndef DOS16
  /* DOS taskQueue may not be free'd because of segment wrap-around */
  free(memory);
  free(taskQueue);
  alloc_p = 0;
#endif
}
