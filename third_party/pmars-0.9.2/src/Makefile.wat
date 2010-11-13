# File: makefile.wat
# Purpose: Makefile support for pmars 0.8 with Watcom compiler
# From: akemi@netcom.com (David Boeren)

# Configuration options:
#
# No.   Name            Incompatible with   Description
# (1)   /DSERVER        2                   disables cdb debugger (koth server 
#                                           version)
# (2)   /DGRAPHX        1                   enables platform specific core 
#                                           graphics
# (3)   /DEXT94                             ICWS'94 + SEQ,SNE,NOP,*,{,}
# (4)   /DSMALLMEM                          16-bit addresses, less memory
# (5)   /DPERMUTATE                         enables -P switch

CC = wcc386
CFLAGS  = /mf /wx /s /oneatx /DWATCOM /DEXT94 /DGRAPHX /DDOSTXTGRAPHX /DPERMUTATE

OBJS = pmars.obj asm.obj eval.obj disasm.obj cdb.obj pos.obj &
       clparse.obj global.obj token.obj str_eng.obj sim.obj

# Use whichever of these two you wish...
pmars.exe: $(OBJS) pmars.lnk makefile config.h global.h
#   wlink system pmodew @pmars.lnk
   wlink system dos4g @pmars.lnk
   @erase pmars.lnk


asm.obj: asm.h
cdb.obj: sim.h
disasm.obj: asm.h sim.h
pos.obj: sim.h
sim.obj: sim.h alldisp.c uidisp.c gtdisp.c grxdisp.c xgraphio.c wtdisp.c
token.obj: asm.h


pmars.lnk: makefile
    %create  pmars.lnk
    @%append pmars.lnk OPTION CASEEXACT
    @%append pmars.lnk name pmars
    @for %i in ($(OBJS)) do @%append pmars.lnk file %i

.c.obj:
    $(CC) $(CFLAGS) $[*.c
