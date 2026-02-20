/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.pager.c - version 1.0.3 */
/* $FreeBSD$ */

/* This file contains the command routine dowhatis() and a pager. */
/* Also readmail() and doshell(), and generally the things that
   contact the outside world. */

#include <sys/types.h>
/**
 * MODERN ADDITION (2025): Fix deprecated sys/signal.h warning
 *
 * WHY: sys/signal.h is deprecated on modern systems and generates warnings.
 *      Most systems now use signal.h directly.
 *
 * HOW: Include signal.h directly instead of sys/signal.h to avoid warnings
 *      while maintaining all signal functionality.
 *
 * PRESERVES: All original signal functionality and behavior
 * ADDS: Cross-platform compatibility without build warnings
 */
#if 0
/* ORIGINAL 1984 CODE - preserved for reference */
#include <sys/signal.h>
#endif
#include "hack.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
extern int CO, LI; /* usually COLNO and ROWNO+2 */
extern char *CD;
extern char quitchars[];

/* Function prototypes for functions defined in this file */
int dowhatis(void);
void page_more(FILE *fp, int strip);
void set_pager(int mode);
int page_line(const char *s);
int dohelp(void);
/* MODERN: CONST-CORRECTNESS: page_file filename is read-only */
int page_file(const char *fnam, boolean silent);
void intruph(int sig);
#ifdef UNIX
#ifdef SHELL
int dosh(void);
#endif
int child(int wt);
#endif

/* External function prototypes for functions called but not declared in hack.h
 */
/* MODERN: CONST-CORRECTNESS: cgetret text is read-only */
extern void cgetret(const char *text);
/* MODERN: CONST-CORRECTNESS: settty message is read-only */
extern void settty(const char *arg);

int dowhatis(void) {
  FILE *fp;
  char bufr[BUFSZ + 6];
  char *buf = &bufr[6], *ep, q;
  extern char readchar();

  if (!(fp = fopen(DATAFILE, "r")))
    pline("Cannot open data file!");
  else {
    pline("Specify what? ");
    q = readchar();
    if (q != '\t')
      while (fgets(buf, BUFSZ, fp))
        if (*buf == q) {
          ep = index(buf, '\n');
          if (ep)
            *ep = 0;
          /* else: bad data file */
          /* Expand tab 'by hand' */
          if (buf[1] == '\t') {
            buf = bufr;
            buf[0] = q;
            (void)strncpy(buf + 1, "       ", 7);
          }
          pline("%s", buf); /* MODERN: Fix format string vulnerability */
          if (ep[-1] == ';') {
            pline("More info? ");
            if (readchar() == 'y') {
              page_more(fp, 1); /* does fclose() */
              return (0);
            }
          }
          (void)fclose(fp); /* kopper@psuvax1 */
          return (0);
        }
    pline("I've never heard of such things.");
    (void)fclose(fp);
  }
  return (0);
}

/* make the paging of a file interruptible */
static int got_intrup;

void intruph(int sig) {
  (void)sig;
  got_intrup++;
}

/* simple pager, also used from dohelp() */
void page_more(
    FILE *fp,
    int strip) /* nr of chars to be stripped from each line (0 or 1) */
{
  char *bufr, *ep;
  sig_t prevsig = signal(SIGINT, intruph);

  set_pager(0);
  bufr = (char *)alloc((unsigned)CO);
  bufr[CO - 1] = 0;
  while (fgets(bufr, CO - 1, fp) && (!strip || *bufr == '\t') && !got_intrup) {
    ep = index(bufr, '\n');
    if (ep)
      *ep = 0;
    if (page_line(bufr + strip)) {
      set_pager(2);
      goto ret;
    }
  }
  set_pager(1);
ret:
  free(bufr);
  (void)fclose(fp);
  (void)signal(SIGINT, prevsig);
  got_intrup = 0;
}

static boolean whole_screen = TRUE;
#define PAGMIN 12 /* minimum # of lines for page below level map */

void set_whole_screen(void) { /* called in termcap as soon as LI is known */
  whole_screen = (LI - ROWNO - 2 <= PAGMIN || !CD);
}

#ifdef NEWS
int readnews(void) {
  int ret;

  whole_screen = TRUE; /* force a docrt(), our first */
  ret = page_file(NEWS, TRUE);
  set_whole_screen();
  return (ret); /* report whether we did docrt() */
}
#endif /* NEWS */

void set_pager(int mode) /* 0: open  1: wait+close  2: close */
{
  static boolean so;
  if (mode == 0) {
    if (!whole_screen) {
      /* clear topline */
      clrlin();
      /* use part of screen below level map */
      curs(1, ROWNO + 4);
    } else {
      cls();
    }
    so = flags.standout;
    flags.standout = 1;
  } else {
    if (mode == 1) {
      curs(1, LI);
      more();
    }
    flags.standout = so ? 1 : 0; /* MODERN: explicit bitfield assignment */
    if (whole_screen)
      docrt();
    else {
      curs(1, ROWNO + 4);
      cl_eos();
    }
  }
}

int page_line(const char *s) /* returns 1 if we should quit */
{
  extern char morc;

  if (cury == LI - 1) {
    if (!*s)
      return (0); /* suppress blank lines at top */
    putchar('\n');
    cury++;
    cmore("q\033");
    if (morc) {
      morc = 0;
      return (1);
    }
    if (whole_screen)
      cls();
    else {
      curs(1, ROWNO + 4);
      cl_eos();
    }
  }
  puts(s);
  cury++;
  return (0);
}

/*
 * Flexible pager: feed it with a number of lines and it will decide
 * whether these should be fed to the pager above, or displayed in a
 * corner.
 * Call:
 *	cornline(0, title or 0)	: initialize
 *	cornline(1, text)	: add text to the chain of texts
 *	cornline(2, morcs)	: output everything and cleanup
 *	cornline(3, 0)		: cleanup
 */

/* MODERN: CONST-CORRECTNESS: cornline text is read-only */
void cornline(int mode, const char *text) {
  static struct line {
    struct line *next_line;
    char *line_text;
  } *texthead, *texttail;
  static int maxlen;
  static int linect;
  struct line *tl;

  if (mode == 0) {
    texthead = 0;
    maxlen = 0;
    linect = 0;
    if (text) {
      cornline(1, text); /* title */
      cornline(1, "");   /* blank line */
    }
    return;
  }

  if (mode == 1) {
    int len;

    if (!text)
      return; /* superfluous, just to be sure */
    linect++;
    len = (int)strlen(text); /* MODERN: cast strlen to int */
    if (len > maxlen)
      maxlen = len;
    tl = (struct line *)alloc(
        (unsigned)((size_t)len + sizeof(struct line) +
                   1)); /* MODERN: cast to size_t for calculation */
    tl->next_line = 0;
    tl->line_text = (char *)(tl + 1);
    (void)strcpy(tl->line_text, text);
    if (!texthead)
      texthead = tl;
    else
      texttail->next_line = tl;
    texttail = tl;
    return;
  }

  /* --- now we really do it --- */
  if (mode == 2 && linect == 1) /* topline only */
    pline("%s",
          texthead->line_text); /* MODERN: Fix format string vulnerability */
  else if (mode == 2) {
    int curline, lth;

    if (flags.toplin == 1)
      more(); /* ab@unido */
    remember_topl();

    lth = CO - maxlen - 2;          /* Use full screen width */
    if (linect < LI && lth >= 10) { /* in a corner */
      home();
      cl_end();
      flags.toplin = 0;
      curline = 1;
      for (tl = texthead; tl; tl = tl->next_line) {
        curs(lth, curline);
        if (curline > 1)
          cl_end();
        putsym(' ');
        putstr(tl->line_text);
        curline++;
      }
      curs(lth, curline);
      cl_end();
      cmore(text);
      home();
      cl_end();
      docorner(lth, curline - 1);
    } else { /* feed to pager */
      set_pager(0);
      for (tl = texthead; tl; tl = tl->next_line) {
        if (page_line(tl->line_text)) {
          set_pager(2);
          goto cleanup;
        }
      }
      if (text) {
        cgetret(text);
        set_pager(2);
      } else
        set_pager(1);
    }
  }

cleanup:
  while ((tl = texthead)) {
    texthead = tl->next_line;
    free((char *)tl);
  }
}

int dohelp(void) {
  char c;

  pline("Long or short help? ");
  while (((c = readchar()) != 'l') && (c != 's') && !index(quitchars, c))
    bell();
  if (!index(quitchars, c))
    (void)page_file((c == 'l') ? HELP : SHELP, FALSE);
  return (0);
}

/* MODERN: CONST-CORRECTNESS: page_file filename is read-only */
int page_file(const char *fnam,
              boolean silent) /* return: 0 - cannot open fnam; 1 - otherwise */
{
#ifdef DEF_PAGER /* this implies that UNIX is defined */
  {
    /* use external pager; this may give security problems */

    int fd = open(fnam, 0);

    if (fd < 0) {
      if (!silent)
        pline("Cannot open data file."); /* MODERN: Safe message without format
                                            string */
      return (0);
    }
    if (child(1)) {
      extern char *catmore;

      /* Now that child() does a setuid(getuid()) and a chdir(),
         we may not be able to open file fnam anymore, so make
         it stdin. */
      (void)close(0);
      if (dup(fd)) {
        if (!silent)
          printf("Cannot open file as stdin.\n"); /* MODERN: Safe message
                                                     without format string */
      } else {
        /* MODERN: Command injection protection - validate pager path */
        if (!catmore || catmore[0] != '/' || strstr(catmore, "..")) {
          if (!silent)
            printf("Invalid pager configuration.\n");
        } else {
          execl(catmore, "page", (char *)0);
          if (!silent)
            printf("Cannot exec pager.\n"); /* MODERN: Safe message without
                                               format string */
        }
      }
      exit(1);
    }
    (void)close(fd);
  }
#else  /* DEF_PAGER */
  {
    FILE *f; /* free after Robert Viduya */

    if ((f = fopen(fnam, "r")) == (FILE *)0) {
      if (!silent) {
        home();
        perror(fnam);
        flags.toplin = 1;
        pline("Cannot open file."); /* MODERN: Safe message without format
                                       string */
      }
      return (0);
    }
    page_more(f, 0);
  }
#endif /* DEF_PAGER */

  return (1);
}

#ifdef UNIX
#ifdef SHELL
int dosh(void) {
  char *str;
  if (child(0)) {
    /* MODERN: Command injection protection - validate SHELL path */
    str = getenv("SHELL");
    if (str && (str[0] != '/' || strstr(str, ".."))) {
      str = NULL; /* Reject relative or suspicious paths */
    }
    if (str)
      execl(str, str, (char *)0);
    else
      execl("/bin/sh", "sh", (char *)0);
    pline("sh: cannot execute.");
    exit(1);
  }
  return (0);
}
#endif /* SHELL */

#ifdef NOWAITINCLUDE
union wait { /* used only for the cast  (union wait *) 0  */
  int w_status;
  struct {
    unsigned short w_Termsig : 7;
    unsigned short w_Coredump : 1;
    unsigned short w_Retcode : 8;
  } w_T;
};

#else

#ifndef _WIN32
#ifdef BSD
#include <sys/wait.h>
#else
#include <wait.h>
#endif /* BSD */
#endif /* _WIN32 */
#endif /* NOWAITINCLUDE */

int child(int wt) {
  int status;
  int f;

  f = fork();
  if (f == 0) {        /* child */
    settty((char *)0); /* also calls end_screen() */

#if 0
		/* ORIGINAL 1984 CODE - preserved for reference */
		/* revoke */
		setgid(getgid());
#endif
    /**
     * MODERN ADDITION (2025): Privilege dropping with error checking */
    if (setgid(getgid()) != 0) {
      /* Privilege dropping failed, but continue - it's advisory */
      perror("setgid warning");
    }

#ifdef CHDIR
#if 0
		/* ORIGINAL 1984 CODE - preserved for reference */
		(void) chdir(getenv("HOME"));
#endif
    /**
     * MODERN ADDITION (2025): Improved chdir error handling*/
    if (chdir(getenv("HOME")) != 0) {
      /* Failed to change to HOME directory, warn but continue */
      perror("chdir to HOME warning");
    }
#endif /* CHDIR */
    return (1);
  }
  if (f == -1) { /* cannot fork */
    pline("Fork failed. Try again.");
    return (0);
  }
  /* fork succeeded; wait for child to exit */
  (void)signal(SIGINT, SIG_IGN);
  (void)signal(SIGQUIT, SIG_IGN);
  (void)wait(&status);
  gettty();
  setftty();
  (void)signal(SIGINT, done1);
#ifdef WIZARD
  if (wizard)
    (void)signal(SIGQUIT, SIG_DFL);
#endif /* WIZARD */
  if (wt)
    getret();
  docrt();
  return (0);
}
#endif /* UNIX */
