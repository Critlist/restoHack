/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.termcap.c - version 1.0.3 */
/* $FreeBSD$ */

/*
 * Modern: On Windows (PDCURSES), termcap functions (tgetent, tgetstr, etc.)
 * are provided by win32.c PDCurses shim, not Unix termcap library.
 * See win32.c for Windows implementation details.
 */

#include "hack.h" /* for function prototypes - includes def.flag.h */
#include "generated/config.h"
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <termcap.h>
#include <unistd.h>
#endif

static char tbuf[512];
static char *HO, *CL, *CE, *hack_UP, *CM, *ND, *XD, *hack_BC, *SO, *SE, *TI,
    *TE;
static char *VS, *VE;
static int SG;
static char hack_PC = '\0';
char *CD;   /* tested in pri.c: docorner() */
int CO, LI; /* used in pri.c and whatis.c */

void startup(void) {
  char *term;
  char *tptr;
  char *tbufptr, *pc;

  tptr = (char *)alloc(1024);

  tbufptr = tbuf;
  if (!(term = getenv("TERM")))
    error("Can't get TERM.");
  if (tgetent(tptr, term) < 1)
    error("Unknown terminal type: %s.", term);
  if (tgetflag("NP") || tgetflag("nx"))
    flags.nonull = 1;
  if ((pc = tgetstr("pc", &tbufptr)))
    hack_PC = *pc;
  if (!(hack_BC = tgetstr("bc", &tbufptr))) {
    if (!tgetflag("bs"))
      error("Terminal must backspace.");
    hack_BC = tbufptr;
    tbufptr += 2;
    *hack_BC = '\b';
  }
  HO = tgetstr("ho", &tbufptr);
  CO = tgetnum("co");
  LI = tgetnum("li");
  if (CO < COLNO || LI < ROWNO + 2)
    setclipped();
  if (!(CL = tgetstr("cl", &tbufptr)))
    error("Hack needs CL.");
  ND = tgetstr("nd", &tbufptr);
  if (tgetflag("os"))
    error("Hack can't have OS.");
  CE = tgetstr("ce", &tbufptr);
  hack_UP = tgetstr("up", &tbufptr);
  /* It seems that xd is no longer supported, and we should use
     a linefeed instead; unfortunately this requires resetting
     CRMOD, and many output routines will have to be modified
     slightly. Let's leave that till the next release. */
  XD = tgetstr("xd", &tbufptr);
  /* not: 		XD = tgetstr("do", &tbufptr); */
  if (!(CM = tgetstr("cm", &tbufptr))) {
    if (!hack_UP && !HO)
      error("Hack needs CM or hack_UP or HO.", 0, 0);
    printf("Playing hack on terminals without cm is suspect...\n");
    getret();
  }
  SO = tgetstr("so", &tbufptr);
  SE = tgetstr("se", &tbufptr);
  SG = tgetnum("sg"); /* -1: not fnd; else # of spaces left by so */
  if (!SO || !SE || (SG > 0))
    SO = SE = 0;
  CD = tgetstr("cd", &tbufptr);
  set_whole_screen(); /* uses LI and CD */
  if ((size_t)(tbufptr - tbuf) > sizeof(tbuf))
    error("TERMCAP entry too big...\n"); /* MODERN: Cast pointer diff to size_t
                                            for size comparison */
  free(tptr);
}

void start_screen(void) {
  xputs(TI);
  xputs(VS);
}

void end_screen(void) {
  xputs(VE);
  xputs(TE);
}

/* Cursor movements */
extern xchar curx, cury;

void curs(int x, int y) /* not xchar: perhaps xchar is unsigned and
                           curx-x would be unsigned as well */
{

  if (y == cury && x == curx)
    return;
  if (!ND && (curx != x || x <= 3)) { /* Extremely primitive */
    cmov(x, y);                       /* bunker!wtm */
    return;
  }
  if (abs(cury - y) <= 3 && abs(curx - x) <= 3)
    nocmov(x, y);
  else if ((x <= 3 && abs(cury - y) <= 3) || (!CM && x < abs(curx - x))) {
    (void)putchar('\r');
    curx = 1;
    nocmov(x, y);
  } else if (!CM) {
    nocmov(x, y);
  } else
    cmov(x, y);
}

void nocmov(int x, int y) {
  if (cury > y) {
    if (hack_UP) {
      while (cury > y) { /* Go up. */
        xputs(hack_UP);
        cury--;
      }
    } else if (CM) {
      cmov(x, y);
    } else if (HO) {
      home();
      curs(x, y);
    } /* else impossible("..."); */
  } else if (cury < y) {
    if (XD) {
      while (cury < y) {
        xputs(XD);
        cury++;
      }
    } else if (CM) {
      cmov(x, y);
    } else {
      while (cury < y) {
        xputc('\n');
        curx = 1;
        cury++;
      }
    }
  }
  if (curx < x) { /* Go to the right. */
    if (!ND)
      cmov(x, y);
    else /* bah */
         /* should instead print what is there already */
      while (curx < x) {
        xputs(ND);
        curx++;
      }
  } else if (curx > x) {
    while (curx > x) { /* Go to the left. */
      xputs(hack_BC);
      curx--;
    }
  }
}

void cmov(int x, int y) {
  xputs(tgoto(CM, x - 1, y - 1));
  cury = (xchar)y; /* MODERN: cast int to xchar */
  curx = (xchar)x; /* MODERN: cast int to xchar */
}

int xputc(int c) { return fputc(c, stdout); }

void xputs(char *s) { tputs(s, 1, xputc); }

void cl_end() {
  if (CE)
    xputs(CE);
  else { /* no-CE fix - free after Harold Rynes */
    /* this looks terrible, especially on a slow terminal
       but is better than nothing */
    int cx = curx, cy = cury;

    while (curx < COLNO) {
      xputc(' ');
      curx++;
    }
    curs(cx, cy);
  }
}

void clear_screen() {
  xputs(CL);
  curx = cury = 1;
}

void home(void) {
  if (HO)
    xputs(HO);
  else if (CM)
    xputs(tgoto(CM, 0, 0));
  else
    curs(1, 1); /* using UP ... */
  curx = cury = 1;
}

void standoutbeg() {
  if (SO)
    xputs(SO);
}

void standoutend() {
  if (SE)
    xputs(SE);
}

void backsp() {
  xputs(hack_BC);
  curx--;
}

void bell(void) {
  (void)putchar('\007'); /* curx does not change */
  (void)fflush(stdout);
}

static short tmspc10[] __attribute__((unused)) = {/* from termcap */
                          0,  2000, 1333, 909, 743, 666, 500, 333, 166,
                          83, 55,   41,   20,  10,  5,   3,   2,   1}; /* Original 1984: used in delay timing, currently disabled code */

#if !HAVE_DELAY_OUTPUT && !defined(_STATIC_BUILD)
/**
 * MODERN ADDITION (2025): Fallback delay_output for systems without curses
 * version
 *
 * WHY: NetBSD provides delay_output in libcurses, but other systems may not
 * have it. Original implementation was disabled with #if 0, causing link
 * failures.
 *
 * HOW: Uses feature detection to provide fallback only when curses doesn't have
 * it. Simplified implementation using usleep() for portability across Unix
 * systems.
 *
 * PRESERVES: Original delay_output timing behavior (50ms default).
 * Maintains authentic 1984 gameplay timing for visual effects.
 *
 * ADDS: Conditional compilation based on curses feature detection rather than
 * hardcoded disabling. Matches curses library signature (returns int).
 */
int delay_output(int ms) {
  if (ms <= 0)
    return 0;
  (void)fflush(stdout);
  usleep((useconds_t)ms * 1000u);
  return 0; /* success, matching curses library behavior */
}
#endif /* !HAVE_DELAY_OUTPUT */

#if 0
/* ORIGINAL 1984 CODE - preserved for reference */
delay_output() {
	/* delay 50 ms - could also use a 'nap'-system call */
	/* BUG: if the padding character is visible, as it is on the 5620
	   then this looks terrible. */
	if(!flags.nonull)
		tputs("50", 1, xputc);

		/* cbosgd!cbcephus!pds for SYS V R2 */
		/* is this terminfo, or what? */
		/* tputs("$<50>", 1, xputc); */
	else {
		(void) fflush(stdout);
		usleep(50*1000);
	}
	else if(ospeed > 0 || ospeed < SIZE(tmspc10)) if(CM) {
		/* delay by sending cm(here) an appropriate number of times */
		int cmlen = strlen(tgoto(CM, curx-1, cury-1));
		int i = 500 + tmspc10[ospeed]/2;

		while(i > 0) {
			cmov(curx, cury);
			i -= cmlen*tmspc10[ospeed];
		}
	}
}
#endif /* 0 */

void cl_eos(void) /* free after Robert Viduya */
{                 /* must only be called with curx = 1 */

  if (CD)
    xputs(CD);
  else {
    int cx = curx, cy = cury;
    while (cury <= LI - 2) {
      cl_end();
      xputc('\n');
      curx = 1;
      cury++;
    }
    cl_end();
    curs(cx, cy);
  }
}
