README for pMARS version 0.9.0 - portable corewar system with ICWS'94 extensions

____________________
What is Core War?

    Core War is a game in which two or more virus-like programs fight against
    each other in a simulated memory space or core. Core War programs are
    written in an assembly language called Redcode which is interpreted by a
    Core War simulator or MARS (Memory Array Redcode Simulator). The object of
    the game is to prevent the other program(s) from executing. For more
    information about Core War check out the usenet newsgroup rec.games.corewar
    and its FAQ list ftp://rtfm.mit.edu/pub/usenet/games/corewar-faq
    or go to http://www.koth.org/.

____________________
pMARS highlights

    * portable, run it on your Mac at home or VAX at work
    * free and comes with source
    * core displays for DOS, Mac and UNIX
    * implements a new redcode dialect, ICWS'94, while remaining compatible
      with ICWS'88
    * powerful redcode extensions: multi-line EQUates, FOR/ROF text repetition
    * one of the fastest simulators written in a high level language
    * full-featured, programmable debugger
    * runs the automated tournament "KotH" at http://www.koth.org and
      http://www.ecst.csuchico.edu/~pizza/koth/ and the annual ICWS tournaments

____________________
Documentation

    pmars.6 is the nroff-source for UNIX man pages. You can install it in
    /usr/man/man6 for use with the UNIX "man" command or format it with nroff
    -man. pmars.doc contains man pages without control characters that have
    been formatted for printing.

    doc/primer.94 and doc/primer.cdb contain short introductions to the ICWS'94
    draft and the cdb debugger respectively. redcode.ref is a quick reference to
    the redcode syntax supported by pMARS. CONTRIB has guidelines for porting
    pMARS to new platforms and contributing new display code.

____________________
Compiling the source

    There are a number of C preprocessor symbols that control which version
    of pMARS is compiled. To enable an option, include -DSYMBOLNAME in CFLAGS
    of the makefile or uncomment the relevant section in config.h.

    GRAPHX
        This option enables a platform-specific graphical core display.

    SERVER
        Disables the debugger for a non-interactive tournament version. The
        pMARS program that runs the KotH email tournaments is compiled with
        SERVER enabled.

    EXT94
        Enables the experimental opcodes SEQ, SNE and NOP, as well as the
        A-field relative addressing modes *, {, and }. This option should
        usually be enabled. EXT94 also enables the P-space extensions LDP,
        STP and PIN.

    SMALLMEM
        makes all addresses 16-bit as opposed to the usual 32-bit wide. This
        limits core size to 65535, but also drastically reduces the memory
        footprint of the program. We found that SMALLMEM reduces the
        simulation speed of pMARS on most CPUs, with the exception of those
        with a very small primary cache.

    There are other compile directives described in config.h, in particular
    some that fine-tune the UNIX curses display.

    pMARS has been tested with various ANSI and non-ANSI C compilers. If
    you can't get it to run or you had to change the source extensively,
    contact the authors with a full description of the problems and
    diffs to the source if applicable.

____________________
Platforms

    UNIX
        A standard UNIX makefile is provided. If you specify the GRAPHX
        directive, a character-based display using the curses library is
        built.  On some systems, it may be necessary to remove -ltermcap from
        the LIB variable in the makefile.

    UNIX/X11
        If you specify the XWINGRAPHX directive, the X-Windows display
        version of pMARS is compiled. You also need to change the link
        library by uncommenting the "LIB = -lX11" line in makefile. X11
        pMARS has a few new command line options that are described in
        pmars.doc.

    LINUX
        The Linux/SVGA version of pMARS has been derived from the DOS
        graphical version and therefore should behave very much like that
        one.

        Some notes for compiling the Linux/SVGA version:
        * You will need the Linux SVGA library (libvga) version 1.12 or above
            to compile pMARS for Linux (it may work with older libraries, but
            I have not had an opportunity to test it).
        * If you #define GRAPHX in config.h or in the makefile, the graphical
            version will automatically be built unless you explicitely
            specify CURSESGRAPHX.
        * You will have to link with -lvgagl -lvga. makefile already contains
            a sample definition of LIB with these libraries.
        * The code assumes that Function keys etc. map to the 'standard'
            escape sequences. pMARS will not recognize these keys otherwise.
        * You need root privileges for the SVGA library, so either run pMARS
            as root, or a better solution is to set the SUID bit of the
            executable (do the following with root privileges).
            # chown root pmars
            # chmod u+s pmars
        * The code assumes your mouse is available via /dev/mouse. This is
            usually a link to the 'real' mouse interface, e.g. on my system:
            # cd /dev
            # ls -l mouse
            lrwxrwxrwx   1 root     root        5 Mar  2 00:22 mouse -> ttyS0
        * The second digit of the the argument to -v indicates (just like in
            the DOS version) the graphics mode:
            1 ... 640x480 with 256 colors
            2 ... 800x600 with 256 colors
            3 ...1024x768 with 256 colors
            6 ... 320x200 with 256 colors
            all other digits will result in the 640x480x256 mode.

        Deficiencies/bugs of the Linux/SVGA version:
        * Currently, the following keys are recognized by pMARS: F1-F10, the
            cursor keys, insert, delete, home, end, page up and page down,
            and Alt-a to Alt-z.
        * The result of a fight is printed to the console after returning
            from the graphical display -- only the last two lines or so are
            not visible until you press <return>.
        * Bug reports are welcome - just drop a note to m.maierhofer@ieee.org

    DOS
        You will need the free 32-bit DOS compiler DJGPP - a DOS port of gcc -
        and ndmake or some other make program. pMARS also compiles with the
        16-bit DOS compilers of the Borland/Turbo C family, but the maximum
        number of warriors is limited to 8 because of memory constraints.

        The GRAPHX compile directive enables the combined VGA/textmode
        display of pmarsv.exe. You can enable the graphics and textmode
        displays selectively with the DOSGRXGRAPHX and DOSTXTGRAPHX
        directives. If you so desire, you can even link in a curses display
        using the PDCurses library by specifying the CURSESGRAPHX directive.

    MAC
        The Mac GUI version of pMARS, MacpMARS, will compile with Think C
        and MPW C, possibly others. Source code for the interface/display of
        MacpMARS as well as instructions on how to make the executable are
        in MacpMARS*s.cpt.hqx. The GUI code has not been updated for v0.7 of
        pMARS yet, so you need the base archive for v0.6 (pmars06s.zip).

    VMS
        pvms*s.zip contains command files to build pMARS for VMS flavors. You
        need DEC C; VAX C will not work. This file also contains a complete VMS
        help system.

    OTHERS
        pMARS should compile with Borland C++ for OS/2, although we haven't
        tried it. The OS/2 version currently doesn't have a core display.
        pMARS has also been reported to compile on Amigas.

____________________
Language support

    All strings are contained in the file str_eng.c for easy translation into
    languages other than english. If you would like to see pMARS speak your
    native tongue, translate the strings and send the file to us. We will then
    include the new str_???.c in the next release and might even release the
    foreign language binaries.

$Id: README,v 1.1.1.1 2000/08/31 10:22:56 iltzu Exp $
