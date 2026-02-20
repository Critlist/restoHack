/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.main.c - version 1.0.3 */
/* $FreeBSD$ */

/*
 * Main entry point for 1984 Hack - initialization and game loop
 * Original 1984 source: docs/historical/original-source/hack.main.c
 *
 * Key modernizations: ANSI C main() signature, POSIX system includes
 */

#include <signal.h>
#include <stdio.h>
/* MODERN ADDITION (2025): Added for ANSI C standard library compatibility */
#include "hack.h"
#include <fcntl.h> /* for open */
#include <stdlib.h>
#include <sys/stat.h> /* for umask */
#ifndef _WIN32
#include <unistd.h>
#endif

/* MODERN ADDITION (2025): Terminal size globals for resize protection */
extern int CO, LI;         /* Defined in hack.termcap.c */
extern void startup(void); /* Re-initialize termcap - hack.termcap.c */
extern void docrt(void);   /* Complete screen redraw - hack.pri.c */

#ifdef QUEST
#define gamename "quest"
#else
#define gamename "hack"
#endif

extern char plname[PL_NSIZ], pl_character[PL_CSIZ];
extern struct permonst mons[CMNUM + 2];
extern char genocided[60], fut_geno[];

int (*afternmv)();
int (*occupation)();
const char *occtxt; /* MODERN: const because assigned string literals */

void done1(int sig);
void hangup(int sig);

/**
 * MODERN ADDITION (2025): Terminal resize protection handler
 *
 * WHY: 1984 Hack assumes fixed terminal size during gameplay. When terminals
 * resize mid-game (window manager events, Steam closing, etc.), the display
 * coordinate system becomes corrupted, causing map rendering bugs, cursor
 * positioning errors, and potential buffer overruns in screen output routines.
 *
 * HOW: SIGWINCH handler detects terminal resize events and either:
 * 1. Forces a complete screen redraw with updated terminal dimensions, or
 * 2. Pauses the game with a warning until terminal size is restored
 * Uses termcap to re-query terminal size and validates against minimum
 * requirements.
 *
 * PRESERVES: Original 1984 display logic and coordinate system unchanged.
 * All existing screen positioning, cursor movement, and drawing routines work
 * identically when terminal size is stable.
 *
 * ADDS: Modern terminal resize resilience. Prevents display corruption from
 * dynamic terminal sizing in modern window managers. Maintains game state
 * integrity during resize events.
 */
static volatile int resize_pending = 0;
static int original_CO = 0, original_LI = 0;

void handle_resize(int sig);
void check_resize(void);

int hackpid; /* current pid */
int locknum; /* max num of players */
#ifdef DEF_PAGER
char *catmore; /* default pager */
#endif
char SAVEF[PL_NSIZ + 11] = "save/"; /* save/99999player */
char *hname;                        /* name of the game (argv[0] of call) */
char obuf[BUFSIZ];                  /* BUFSIZ is defined in stdio.h */

extern const char
    *nomovemsg; /* MODERN: const because assigned string literals */
extern long wailmsg;

#ifdef CHDIR
static void chdirx(const char *dir,
                   boolean wr); /* MODERN: const because dir is read-only */
#endif

int main(int argc, char *argv[]) {
  int fd;
#ifdef CHDIR
  char *dir;
#endif

  hname = argv[0];
  hackpid = getpid();

#ifdef _WIN32
  /* Modern: Initialize Windows console (UTF-8, control handlers) */
  win32_init_console();
#endif

#ifdef CHDIR /* otherwise no chdir() */
  /*
   * See if we must change directory to the playground.
   * (Perhaps hack runs suid and playground is inaccessible
   *  for the player.)
   * The environment variable HACKDIR is overridden by a
   *  -d command line option (must be the first option given)
   */

  dir = getenv("HACKDIR");
  if (argc > 1 && !strncmp(argv[1], "-d", 2)) {
    argc--;
    argv++;
    dir = argv[0] + 2;
    if (*dir == '=' || *dir == ':')
      dir++;
    if (!*dir && argc > 1) {
      argc--;
      argv++;
      dir = argv[0];
    }
    if (!*dir)
      error("Flag -d must be followed by a directory name.");
  }
#endif

  /*
   * Who am i? Algorithm: 1. Use name as specified in HACKOPTIONS
   *			2. Use $USER or $LOGNAME	(if 1. fails)
   *			3. Use getlogin()		(if 2. fails)
   * The resulting name is overridden by command line options.
   * If everything fails, or if the resulting name is some generic
   * account like "games", "play", "player", "hack" then eventually
   * we'll ask him.
   * Note that we trust him here; it is possible to play under
   * somebody else's name.
   */
  {
    char *s;

    initoptions();
    if (!*plname && (s = getenv("USER")))
      (void)strncpy(plname, s, sizeof(plname) - 1);
    if (!*plname && (s = getenv("LOGNAME")))
      (void)strncpy(plname, s, sizeof(plname) - 1);
    if (!*plname && (s = getlogin()))
      (void)strncpy(plname, s, sizeof(plname) - 1);
  }

  /*
   * Now we know the directory containing 'record' and
   * may do a prscore().
   */
  if (argc > 1 && !strncmp(argv[1], "-s", 2)) {
#ifdef CHDIR
    chdirx(dir, 0);
#endif
    prscore(argc, argv);
    /* MODERN ADDITION (2025): Memory cleanup for sanitizers */
    cleanup_all_engravings();
    exit(0);
  }

  /*
   * It seems he really wants to play.
   * Remember tty modes, to be restored on exit.
   */
  gettty();
  setbuf(stdout, obuf);
  umask(007);
  setrandom();
  startup();
  cls();
  u.uhp = 1;  /* prevent RIP on early quits */
  u.ux = FAR; /* prevent nscr() */
  (void)signal(SIGHUP, hangup);
  /* MODERN ADDITION (2025): Handle window manager close events in
   * Hyprland/Wayland */
  (void)signal(SIGTERM, hangup);
  /* MODERN ADDITION (2025): Handle Ctrl+\ quit signal for complete coverage */
  (void)signal(SIGQUIT, hangup);
#ifdef SIGWINCH
  /* MODERN ADDITION (2025): Terminal resize protection - see handle_resize() */
  (void)signal(SIGWINCH, handle_resize);
  original_CO = CO; /* Store initial terminal size for validation */
  original_LI = LI;
#endif

  /*
   * Find the creation date of this game,
   * so as to avoid restoring outdated savefiles.
   */
  gethdate(hname);

  /*
   * We cannot do chdir earlier, otherwise gethdate will fail.
   */
#ifdef CHDIR
  chdirx(dir, 1);
#endif

  /*
   * Process options.
   */
  while (argc > 1 && argv[1][0] == '-') {
    argv++;
    argc--;
    switch (argv[0][1]) {
#ifdef WIZARD
    case 'D':
      /*			if(!strcmp(getlogin(), WIZARD)) */
      wizard = TRUE;
      /*			else
                                      printf("Sorry.\n"); */
      break;
#endif
#ifdef NEWS
    case 'n':
      flags.nonews = TRUE;
      break;
#endif
    case 'u':
      if (argv[0][2])
        (void)strncpy(plname, argv[0] + 2, sizeof(plname) - 1);
      else if (argc > 1) {
        argc--;
        argv++;
        (void)strncpy(plname, argv[0], sizeof(plname) - 1);
      } else
        printf("Player name expected after -u\n");
      break;
    default:
      /* allow -T for Tourist, etc. */
      (void)strncpy(pl_character, argv[0] + 1, sizeof(pl_character) - 1);

      /* printf("Unknown option: %s\n", *argv); */
    }
  }

  if (argc > 1) {
    locknum = atoi(argv[1]);
    if (locknum < 0)
      locknum = 0; /* MODERN: prevent negative values */
  }
#ifdef MAX_NR_OF_PLAYERS
  if (!locknum || locknum > MAX_NR_OF_PLAYERS)
    locknum = MAX_NR_OF_PLAYERS;
#endif
#ifdef DEF_PAGER
  if (!(catmore = getenv("HACKPAGER")) && !(catmore = getenv("PAGER")))
    catmore = DEF_PAGER;
#endif
#ifdef MAIL
  getmailstatus();
#endif
#ifdef WIZARD
  if (wizard)
    (void)strncpy(plname, "wizard", PL_NSIZ - 1), plname[PL_NSIZ - 1] = '\0';
  else
#endif
      if (!*plname || !strncmp(plname, "player", 4) ||
          !strncmp(plname, "games", 4))
    askname();
  plnamesuffix(); /* strip suffix from name; calls askname() */
                  /* again if suffix was whole name */
                  /* accepts any suffix */
#ifdef WIZARD
  if (!wizard) {
#endif
    /*
     * check for multiple games under the same name
     * (if !locknum) or check max nr of players (otherwise)
     */
    (void)signal(SIGQUIT, SIG_IGN);
    (void)signal(SIGINT, SIG_IGN);
    if (!locknum) {
      (void)strncpy(lock, plname, PL_NSIZ + 4 - 1);
      lock[PL_NSIZ + 4 - 1] = '\0'; /* MODERN: Ensure null termination */
    }
#ifdef ENABLE_MODERN_LOCKING
    /* MODERN ADDITION (2025): Clean up any stale locks on startup */
    modern_cleanup_locks();
#endif
    getlock(); /* sets lock if locknum != 0 */
#ifdef WIZARD
  } else {
    char *sfoo;
    (void)strncpy(lock, plname, PL_NSIZ + 4 - 1);
    lock[PL_NSIZ + 4 - 1] = '\0'; /* MODERN: Ensure null termination */
    if ((sfoo = getenv("MAGIC")))
      while (*sfoo) {
        switch (*sfoo++) {
        case 'n':
          (void)srandom(*sfoo++);
          break;
        }
      }
    if ((sfoo = getenv("GENOCIDED"))) {
      if (*sfoo == '!') {
        struct permonst *pm = mons;
        char *gp = genocided;

        while (pm < mons + CMNUM + 2) {
          if (!index(sfoo, pm->mlet))
            *gp++ = pm->mlet;
          pm++;
        }
        *gp = 0;
      } else
        (void)strncpy(genocided, sfoo, sizeof(genocided) - 1);
      (void)strncpy(fut_geno, genocided, 60 - 1);
      fut_geno[60 - 1] = '\0'; /* MODERN: Ensure null termination */
    }
  }
#endif
  setftty();
  (void)snprintf(SAVEF, sizeof(SAVEF), "save/%d%s", getuid(),
                 plname); /* MODERN: Safe sprintf replacement - identical
                             output, prevents overflow */
  regularize(SAVEF + 5);  /* avoid . or / in name */
  if ((fd = open(SAVEF, 0)) >= 0 && (uptodate(fd) || unlink(SAVEF) == 666)) {
    (void)signal(SIGINT, done1);
    pline("Restoring old save file...");
    (void)fflush(stdout);
    if (!dorecover(fd))
      goto not_recovered;
    pline("Hello %s, welcome to %s!", plname, gamename);
    flags.move = 0;
  } else {
  not_recovered:
    fobj = fcobj = invent = 0;
    fmon = fallen_down = 0;
    ftrap = 0;
    fgold = 0;
    flags.ident = 1;
    init_objects();
    u_init();

    (void)signal(SIGINT, done1);
    mklev();
    u.ux = (xchar)xupstair; /* MODERN: Safe cast - map coords are 0-79, within xchar range */
    u.uy = (xchar)yupstair; /* MODERN: Safe cast - map coords are 0-21, within xchar range */
    (void)inshop();
    setsee();
    flags.botlx = 1;
    makedog();
    {
      struct monst *mtmp;
      if ((mtmp = m_at(u.ux, u.uy)) != 0)
        mnexto(mtmp);
    }
    seemons();
#ifdef NEWS
    if (flags.nonews || !readnews())
    /* after reading news we did docrt() already */
#endif
      docrt();

    /* give welcome message before pickup messages */
    pline("Hello %s, welcome to %s!", plname, gamename);

    pickup(1);
    read_engr_at(u.ux, u.uy);
    flags.move = 1;
  }

  flags.moonphase = phase_of_the_moon();
  if (flags.moonphase == FULL_MOON) {
    pline("You are lucky! Full moon tonight.");
    u.uluck++;
  } else if (flags.moonphase == NEW_MOON) {
    pline("Be careful! New moon tonight.");
  }

  initrack();

  for (;;) {
    if (flags.move) { /* actual time passed */

      settrack();

      if (moves % 2 == 0 || (!(Fast & ~INTRINSIC) && (!Fast || rn2(3)))) {
        movemon();
        if (!rn2(70))
          (void)makemon((struct permonst *)0, 0, 0);
      }
      if (Glib)
        glibr();
      hack_timeout();
      ++moves;
      if (flags.time)
        flags.botl = 1;
      if (u.uhp < 1) {
        pline("You die...");
        done("died");
      }
      if (u.uhp * 10 < u.uhpmax && moves - wailmsg > 50) {
        wailmsg = moves;
        if (u.uhp == 1)
          pline("You hear the wailing of the Banshee...");
        else
          pline("You hear the howling of the CwnAnnwn...");
      }
      if (u.uhp < u.uhpmax) {
        if (u.ulevel > 9) {
          if (Regeneration || !(moves % 3)) {
            flags.botl = 1;
            u.uhp += rnd((int)u.ulevel - 9);
            if (u.uhp > u.uhpmax)
              u.uhp = u.uhpmax;
          }
        } else if (Regeneration || (!(moves % (22 - u.ulevel * 2)))) {
          flags.botl = 1;
          u.uhp++;
        }
      }
      if (Teleportation && !rn2(85))
        tele();
      if (Searching && multi >= 0)
        (void)dosearch();
      gethungry();
      invault();
      amulet();
    }
    if (multi < 0) {
      if (!++multi) {
        pline(nomovemsg ? nomovemsg : "You can move again.");
        nomovemsg = 0;
        if (afternmv)
          (*afternmv)();
        afternmv = 0;
      }
    }

    find_ac();
#ifndef QUEST
    if (!flags.mv || Blind)
#endif
    {
      seeobjs();
      seemons();
      nscr();
    }
    if (flags.botl || flags.botlx)
      bot();

#ifdef SIGWINCH
    /* MODERN ADDITION (2025): Check for pending terminal resize */
    check_resize();
#endif

    flags.move = 1;

    if (multi >= 0 && occupation) {
      if (monster_nearby())
        stop_occupation();
      else if ((*occupation)() == 0)
        occupation = 0;
      continue;
    }

    if (multi > 0) {
#ifdef QUEST
      if (flags.run >= 4)
        finddir();
#endif
      lookaround();
      if (!multi) { /* lookaround may clear multi */
        flags.move = 0;
        continue;
      }
      if (flags.mv) {
        if (multi < COLNO && !--multi)
          flags.mv = flags.run = 0;
        domove();
      } else {
        --multi;
        rhack(save_cm);
      }
    } else if (multi == 0) {
#ifdef MAIL
      ckmailstatus();
#endif
      rhack((char *)0);
    }
    if (multi && multi % 7 == 0)
      (void)fflush(stdout);
  }
}

void glo(int foo) {
  /* construct the string  xlock.n  */
  char *tf;

  tf = lock;
  while (*tf && *tf != '.')
    tf++;
  (void)snprintf(tf, (PL_NSIZ + 4) - (tf - lock), ".%d",
                 foo); /* MODERN: Safe sprintf replacement - identical output,
                          prevents overflow */
}

/*
 * plname is filled either by an option (-u Player  or  -uPlayer) or
 * explicitly (-w implies wizard) or by askname.
 * It may still contain a suffix denoting pl_character.
 */
void askname(void) {
  register int c, ct;
  printf("\nWho are you? ");
  (void)fflush(stdout);
  ct = 0;
  while ((c = getchar()) != '\n') {
    if (c == EOF)
      error("End of input\n");
    /* some people get confused when their erase char is not ^H */
    if (c == '\010') {
      if (ct > 0)
        ct--;
      continue;
    }
    if (c != '-')
      if (c < 'A' || (c > 'Z' && c < 'a') || c > 'z')
        c = '_';
    if (ct >= 0 &&
        ct < (int)sizeof(plname) - 1) /* MODERN: prevent integer overflow */
      plname[ct++] = c;
  }
  plname[ct] = 0;
  if (ct == 0)
    askname();
}

/*VARARGS1*/
/* MODERN: CONST-CORRECTNESS: impossible message is read-only */
void impossible(const char *s, int x1, int x2) {
  pline(s, x1, x2);
  pline("Program in disorder - perhaps you'd better Quit.");
}

#ifdef CHDIR
static void chdirx(const char *dir,
                   boolean wr) /* MODERN: const because dir is read-only */
{

#ifdef SECURE
  if (dir /* User specified directory? */
#ifdef HACKDIR
      && strcmp(dir, HACKDIR) /* and not the default? */
#endif
  ) {
#if 0
		/* ORIGINAL 1984 CODE - preserved for reference */
		/* revoke */
		setgid(getgid());
#endif
    /**
     * MODERN: Privilege dropping with error checking*/
    /* revoke */
    if (setgid(getgid()) != 0) {
      /* Privilege dropping failed, warn but continue */
      perror("setgid warning");
    }
  }
#endif

#ifdef HACKDIR
  if (dir == NULL)
    dir = HACKDIR;
#endif

  if (dir && chdir(dir) < 0) {
    perror(dir);
    error("Cannot chdir to game directory."); /* MODERN: safe message prevents
                                                 format string attack */
  }

  /* warn the player if he cannot write the record file */
  /* perhaps we should also test whether . is writable */
  /* unfortunately the access systemcall is worthless */
  if (wr) {
    int fd;

    if (dir == NULL)
      dir = ".";
    if ((fd = open(RECORD, 2)) < 0) {
      printf("Warning: cannot write %s/%s", dir, RECORD);
      getret();
    } else
      (void)close(fd);
  }
}
#endif

void stop_occupation(void) {
  if (occupation) {
    pline("You stop %s.", occtxt);
    occupation = 0;
  }
}

/**
 * MODERN ADDITION (2025): Terminal resize signal handler implementation
 *
 * Handles SIGWINCH events to prevent display corruption when terminal
 * is resized during gameplay. See full documentation at function declaration.
 */
void handle_resize(int sig) {
  /* Set volatile flag - main loop will handle actual resize */
  resize_pending = 1;

  /* Re-register handler (some systems need this) */
#ifdef SIGWINCH
  signal(SIGWINCH, handle_resize);
#endif

  /* Signal handler should be minimal - just set flag */
  (void)sig; /* Suppress unused parameter warning */
}

/**
 * MODERN: Check and handle pending terminal resize*/
void check_resize(void) {
  if (!resize_pending)
    return;

  /* Query current terminal size */
  int new_CO, new_LI;
  startup(); /* Re-read termcap values */
  new_CO = CO;
  new_LI = LI;

  /* Check if size actually changed */
  if (new_CO == original_CO && new_LI == original_LI) {
    /* False alarm or size restored - continue normally */
    resize_pending = 0;
    return;
  }

  /* Terminal size changed - handle it */
  if (new_CO < COLNO || new_LI < ROWNO + 2) {
    /* Terminal too small - pause game */
    cls();
    printf("\n\nTERMINAL TOO SMALL!\n");
    printf("Current: %dx%d, Required: %dx%d\n", new_CO, new_LI, COLNO,
           ROWNO + 2);
    printf("Please resize terminal and press any key...");
    getchar();
    /* Re-check after user input */
    startup();
    if (CO >= COLNO && LI >= ROWNO + 2) {
      /* Size is now adequate */
      docrt(); /* Full screen redraw */
      resize_pending = 0;
    }
    /* If still too small, will re-prompt on next check */
  } else {
    /* Terminal size acceptable but changed - redraw everything */
    docrt();
    pline("[Terminal resized to %dx%d - display refreshed]", new_CO, new_LI);
    resize_pending = 0;

    /* Update stored size for next comparison */
    original_CO = new_CO;
    original_LI = new_LI;
  }
}
