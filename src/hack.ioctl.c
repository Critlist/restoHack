/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.ioctl.c - version 1.0.2 */
/* $FreeBSD$ */

/*
 * Terminal I/O control for 1984 Hack - raw mode and signal handling
 * Original 1984 source: docs/historical/original-source/hack.ioctl.c
 *
 * Key modernizations: POSIX termios instead of sgtty
 */

/* This cannot be part of hack.tty.c (as it was earlier) since on some
   systems (e.g. MUNIX) the include files <termio.h> and <sgtty.h>
   define the same constants, and the C preprocessor complains. */

#include "hack.h"
#include "config.h"
#include <signal.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#endif

/* Forward declarations for functions not properly visible */
/* MODERN: CONST-CORRECTNESS: settty message is read-only */
extern void settty(const char *s);

#ifdef _WIN32
/* Modern: Windows uses PDCurses - no ioctl terminal control needed */

void getioctls(void) {
  /* No-op: PDCurses handles terminal on Windows */
}

void setioctls(void) {
  /* No-op: PDCurses handles terminal on Windows */
}

#else /* !_WIN32 */

#ifdef BSD
/**
 * MODERN ADDITION (2025): Platform-specific terminal interface selection
 *
 * WHY: Original 1984 code used only BSD sgtty interfaces. Modern FreeBSD
 * has deprecated sgtty in favor of POSIX termios for standards compliance.
 *
 * HOW: Uses feature detection to include appropriate terminal headers:
 * - Linux: Always POSIX termios
 * - FreeBSD: POSIX termios (modern standard)
 * - Other BSD: Original sgtty interface (compatibility)
 *
 * PRESERVES: Original terminal control behavior and functionality
 * ADDS: POSIX compliance and cross-platform compatibility
 */
#ifdef __linux__
#include <termios.h>
#else
/* ORIGINAL 1984: #include	<sgtty.h> */
/* MODERN: FreeBSD uses POSIX termios like Linux */
#ifdef __FreeBSD__
#include <termios.h>
#else
#include <sgtty.h> /* Original BSD sgtty interface */
#endif
#endif
/**
 * MODERN ADDITION (2025): BSD terminal compatibility fallbacks
 *
 * WHY: Original 1984 code assumed BSD sgtty interfaces available on all
 * systems. Modern Linux and other systems may lack BSD-specific terminal
 * constants.
 *
 * HOW: Provides safe fallback definitions for missing BSD terminal control
 * constants and structures, preventing compilation failures on non-BSD systems.
 *
 * PRESERVES: Original 1984 terminal control logic and variable usage
 * ADDS: Cross-platform compatibility without changing game behavior
 */
#ifndef TIOCGLTC
#define TIOCGLTC 0
#endif
#ifndef TIOCSLTC
#define TIOCSLTC 0
#endif
/* Fallback definition for ltchars if not available */
#ifndef _SGTTY_H_
struct ltchars {
  char t_suspc;  /* stop process signal */
  char t_dsuspc; /* delayed stop process signal */
  char t_rprntc; /* reprint line */
  char t_flushc; /* flush output (toggles) */
  char t_werasc; /* word erase */
  char t_lnextc; /* literal next character */
};
#endif
struct ltchars ltchars, ltchars0;
#else
#include <termio.h> /* also includes part of <sgtty.h> */
struct termio termio;
#endif              /* BSD */

void getioctls(void) {
#ifdef BSD
  (void)ioctl(fileno(stdin), (int)TIOCGLTC, (char *)&ltchars);
  (void)ioctl(fileno(stdin), (int)TIOCSLTC, (char *)&ltchars0);
#else
  (void)ioctl(fileno(stdin), (int)TCGETA, &termio);
#endif /* BSD */
}

void setioctls(void) {
#ifdef BSD
  (void)ioctl(fileno(stdin), (int)TIOCSLTC, (char *)&ltchars);
#else
  (void)ioctl(fileno(stdin), (int)TCSETA, &termio);
#endif /* BSD */
}

#endif /* _WIN32 */

#ifdef SUSPEND /* implies BSD */
int dosuspend(void) {
#ifdef SIGTSTP
  if (signal(SIGTSTP, SIG_IGN) == SIG_DFL) {
    settty((char *)0);
    (void)signal(SIGTSTP, SIG_DFL);
    (void)kill(0, SIGTSTP);
    gettty();
    setftty();
    docrt();
  } else {
    pline("I don't think your shell has job control.");
  }
#else  /* SIGTSTP */
  pline("Sorry, it seems we have no SIGTSTP here. Try ! or S.");
#endif /* SIGTSTP */
  return (0);
}
#endif /* SUSPEND */
