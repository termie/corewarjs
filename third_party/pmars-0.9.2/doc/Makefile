.SUFFIXES: .man .doc .6

all: man doc

man: pmars.man
doc: pmars.doc
pmars.doc: pmars.man
pmars.man: pmars.6

.6.man:
	@echo Building $*.man - online manual
	@nroff -e -man $*.6 > $*.man

.man.doc:
	@echo Building $*.doc - printable manual
	@cp $*.man $*.doc
	@ex $*.doc < makedoc.ex > /dev/null
