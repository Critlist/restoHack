/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.end.c - version 1.0.3 */
/* $FreeBSD$ */

/*
 * Game ending system for 1984 Hack - death, victory, and score handling
 * Original 1984 source: docs/historical/original-source/hack.end.c
 *
 * Key modernizations: ANSI C function signatures, modern file locking
 */

#include "hack.h"
#include <signal.h>
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif
/* MODERN ADDITION (2025): Added for stale lock detection - stat() and time() */
#include <sys/stat.h>
#include <time.h>
/* MODERN: Safe sprintf replacement - same interface, prevents overflow */
/* MODERN ADDITION (2025): Bounds-safe sprintf replacement for outentry function
 */
#define SafeAppend(linebuf, ...)                                               \
  do {                                                                         \
    char *pos = eos(linebuf);                                                  \
    size_t remaining = BUFSZ - (pos - (linebuf));                              \
    if (remaining > 1) {                                                       \
      snprintf(pos, remaining, __VA_ARGS__);                                   \
    }                                                                          \
  } while (0)

#define Sprintf(buf, ...)                                                      \
  (void)snprintf(buf, 200, __VA_ARGS__) /* MODERN: Increased from 128 to 200   \
                                           to prevent truncation warnings */
extern char plname[], pl_character[];

/* Forward declarations for missing functions */
extern void paybill(void);
extern void savebones(void);
extern void outrip(void);
/* MODERN: CONST-CORRECTNESS: settty message is read-only */
extern void settty(const char *);

/* Forward declarations for functions in this file */
void topten(void);
void outheader(void);
void clearlocks(void);
char *itoa(int a);
const char *ordin(int n); /* MODERN: const because returns string literals */
char *eos(char *s);
void charcat(char *s, char c);

xchar maxdlevel = 1;

int doquit(void) {
  (void)signal(SIGINT, SIG_IGN);
  pline("Really quit?");
  if (readchar() != 'y') {
    (void)signal(SIGINT, done1);
    clrlin();
    (void)fflush(stdout);
    if (multi > 0)
      nomul(0);
    return (0);
  }
  done("quit");
  /* NOTREACHED */
  return 1; /* MODERN: Never reached but required for return-type correctness */
}

void done1(int sig) {
  (void)sig;
  doquit();
}

int done_stopprint;
int done_hup;

void done_intr(int sig) {
  (void)sig;
  done_stopprint++;
  (void)signal(SIGINT, SIG_IGN);
  (void)signal(SIGQUIT, SIG_IGN);
}

void done_hangup(int sig) {
  done_hup++;
  (void)signal(SIGHUP, SIG_IGN);
  done_intr(sig);
}

void done_in_by(struct monst *mtmp) {
  static char buf[BUFSZ];
  pline("You die ...");
  if (mtmp->data->mlet == ' ') {
    Sprintf(buf, "the ghost of %s", (char *)mtmp->mextra);
    killer = buf;
  } else if (mtmp->mnamelth) {
    /**
     * MODERN: Bounds checking for NAME() macro in death*/
    if (mtmp->mxlth > 1024) { /* MODERN: Sanity check for corrupted mxlth */
      Sprintf(buf, "%s called <corrupted name>", mtmp->data->mname);
    } else {
      Sprintf(buf, "%s called %s", mtmp->data->mname, NAME(mtmp));
    }
    killer = buf;
  } else if (mtmp->minvis) {
    Sprintf(buf, "invisible %s", mtmp->data->mname);
    killer = buf;
  } else
    killer = mtmp->data->mname;
  done("died");
}

/* called with arg "died", "drowned", "escaped", "quit", "choked", "panicked",
   "burned", "starved" or "tricked" */
/* Be careful not to call panic from here! */
/* MODERN: CONST-CORRECTNESS: death reason string is read-only */
void done(const char *st1) {

#ifdef WIZARD
  if (wizard && *st1 == 'd') {
    u.uswldtim = 0;
    if (u.uhpmax < 0)
      u.uhpmax = 100; /* arbitrary */
    u.uhp = u.uhpmax;
    pline("For some reason you are still alive.");
    flags.move = 0;
    if (multi > 0)
      multi = 0;
    else
      multi = -1;
    flags.botl = 1;
    return;
  }
#endif /* WIZARD */
  (void)signal(SIGINT, done_intr);
  (void)signal(SIGQUIT, done_intr);
  (void)signal(SIGHUP, done_hangup);
  if (*st1 == 'q' && u.uhp < 1) {
    st1 = "died";
    killer = "quit while already on Charon's boat";
  }
  if (*st1 == 's')
    killer = "starvation";
  else if (*st1 == 'd' && st1[1] == 'r')
    killer = "drowning";
  else if (*st1 == 'p')
    killer = "panic";
  else if (*st1 == 't')
    killer = "trickery";
  else if (!index("bcd", *st1))
    killer = st1;
  paybill();
  clearlocks();
  if (flags.toplin == 1)
    more();
  if (index("bcds", *st1)) {
#ifdef WIZARD
    if (!wizard)
#endif /* WIZARD */
      savebones();
    if (!flags.notombstone)
      outrip();
  }
  if (*st1 == 'c')
    killer = st1;    /* after outrip() */
  settty((char *)0); /* does a clear_screen() */
  if (!done_stopprint)
    printf("Goodbye %s %s...\n\n", pl_character, plname);
  {
    long int tmp;
    tmp = u.ugold - u.ugold0;
    if (tmp < 0)
      tmp = 0;
    if (*st1 == 'd' || *st1 == 'b')
      tmp -= tmp / 10;
    u.urexp += tmp;
    u.urexp += 50 * maxdlevel;
    if (maxdlevel > 20)
      u.urexp += 1000 * ((maxdlevel > 30) ? 10 : maxdlevel - 20);
  }
  if (*st1 == 'e') {
    extern struct monst *mydogs;
    struct monst *mtmp;
    struct obj *otmp;
    int i;
    unsigned worthlessct = 0;
    boolean has_amulet = FALSE;

    killer = st1;
    keepdogs();
    mtmp = mydogs;
    if (mtmp) {
      if (!done_stopprint)
        printf("You");
      while (mtmp) {
        if (!done_stopprint)
          printf(" and %s", monnam(mtmp));
        if (mtmp->mtame)
          u.urexp += mtmp->mhp;
        mtmp = mtmp->nmon;
      }
      if (!done_stopprint)
        printf("\nescaped from the dungeon with %ld points,\n", u.urexp);
    } else if (!done_stopprint)
      printf("You escaped from the dungeon with %ld points,\n", u.urexp);
    for (otmp = invent; otmp; otmp = otmp->nobj) {
      if (otmp->olet == GEM_SYM) {
        objects[otmp->otyp].oc_name_known = 1;
        i = otmp->quan * objects[otmp->otyp].g_val;
        if (i == 0) {
          worthlessct += otmp->quan;
          continue;
        }
        u.urexp += i;
        if (!done_stopprint)
          printf("\t%s (worth %d Zorkmids),\n", doname(otmp), i);
      } else if (otmp->olet == AMULET_SYM) {
        otmp->known = 1;
        i = (otmp->spe < 0) ? 2 : 5000;
        u.urexp += i;
        if (!done_stopprint)
          printf("\t%s (worth %d Zorkmids),\n", doname(otmp), i);
        if (otmp->spe >= 0) {
          has_amulet = TRUE;
          killer = "escaped (with amulet)";
        }
      }
    }
    if (worthlessct)
      if (!done_stopprint)
        printf("\t%u worthless piece%s of coloured glass,\n", worthlessct,
               plur(worthlessct));
    if (has_amulet)
      u.urexp *= 2;
  } else if (!done_stopprint)
    printf("You %s on dungeon level %d with %ld points,\n", st1, dlevel,
           u.urexp);
  if (!done_stopprint)
    printf("and %ld piece%s of gold, after %ld move%s.\n", u.ugold,
           plur(u.ugold), moves, plur(moves));
  if (!done_stopprint)
    printf("You were level %u with a maximum of %d hit points when you %s.\n",
           u.ulevel, u.uhpmax, st1);
  if (*st1 == 'e' && !done_stopprint) {
    getret(); /* all those pieces of coloured glass ... */
    cls();
  }
#ifdef WIZARD
  if (!wizard)
#endif /* WIZARD */
    topten();
  if (done_stopprint)
    printf("\n\n");

  /* MODERN: Memory cleanup for sanitizers */
  cleanup_all_engravings();

  exit(0);
}

/**
 * MODERN: Expanded buffer sizes for modern systems */
#define NAMSZ 64 /* MODERN: Expanded from 8 to handle modern usernames */
#define DTHSZ                                                                  \
  128 /* MODERN: Expanded from 40 to prevent death message truncation */
#define PERSMAX 1
#define POINTSMIN 1  /* must be > 0 */
#define ENTRYMAX 100 /* must be >= 10 */
#define PERS_IS_UID  /* delete for PERSMAX per name; now per uid */
struct toptenentry {
  struct toptenentry *tt_next;
  long int points;
  int level, maxlvl, hp, maxhp;
  int uid;
  char plchar;
  char sex;
  char name[NAMSZ + 1];
  char death[DTHSZ + 1];
  char date[7]; /* yymmdd */
} *tt_head;

/* MODERN: Static arena allocator to prevent memory leaks */
static struct toptenentry arena[ENTRYMAX + 2]; /* +2 for t0 and safety margin */
static int arena_used = 0;

static void reset_topten_arena(void) {
  arena_used = 0;
  memset(arena, 0, sizeof(arena));
}

struct toptenentry *newttentry(void) {
  if (arena_used >= ENTRYMAX + 2) {
    panic("topten arena exhausted");
  }
  return &arena[arena_used++];
}

/* Forward declaration after struct definition */
int outentry(int rank, struct toptenentry *t1, int so);

void topten(void) {
  reset_topten_arena(); /* MODERN: Reset arena for fresh allocation */
  int uid = getuid();
  int rank, rank0 = -1, rank1 = 0;
  int occ_cnt = PERSMAX;
  struct toptenentry *t0, *t1, *tprev;
  const char *recfile =
      RECORD; /* MODERN: const because points to string literal */
  const char *reclock =
      "record_lock"; /* MODERN: const because points to string literal */
  (void)reclock;     /* Original 1984: used in link() locking, conditionally
                        compiled */
  int sleepct = 300;
  (void)sleepct; /* Original 1984: used in link() retry loop, conditionally
                    compiled */
  FILE *rfile;
  int flg = 0;
  extern char *getdatestr();
#define HUP if (!done_hup)

  /**
   * MODERN ADDITION (2025): Stale lock detection and cleanup
   *
   * WHY: Original 1984 code could leave stale record_lock files if the game
   * crashed or was killed before proper cleanup, causing infinite loops in
   * subsequent runs waiting for lock release.
   *
   * HOW: Check if record_lock exists and is older than 5 minutes (300 seconds).
   * If so, assume it's stale from a crashed process and remove it
   * automatically. Uses stat() to get file modification time and compare with
   * current time.
   *
   * PRESERVES: Original Unix file locking semantics using link() for atomicity
   * ADDS: Defensive programming against stale resources
   */
  struct stat lockstat;
  (void)lockstat; /* Original 1984: used for stale lock detection, conditionally
                     compiled */
#ifdef ENABLE_MODERN_LOCKING
  /* Modern locking: flock() automatically releases on process death */
  /* No stale locks to clean up */
#else
  /* Original 1984 system: Clean up stale record locks */
  if (stat(reclock, &lockstat) == 0) {
    time_t now = time(NULL);
    if (now - lockstat.st_mtime > 300) { /* 5 minutes */
      HUP printf("Removing stale record lock (age: %ld seconds)\n",
                 (long)(now - lockstat.st_mtime));
      (void)unlink(reclock);
    }
  }
#endif

#ifdef ENABLE_MODERN_LOCKING
  /* MODERN ADDITION (2025): Use flock()-based record locking */
  if (!modern_lock_record()) {
    HUP puts("Cannot access record file!");
    return;
  }
#else
  /* Original 1984 link()-based record locking */
  while (link(recfile, reclock) == -1) {
    HUP perror(reclock);
    if (!sleepct--) {
      HUP puts("I give up. Sorry.");
      HUP puts("Perhaps there is an old record_lock around?");
      return;
    }
    HUP printf("Waiting for access to record file. (%d)\n", sleepct);
    HUP(void) fflush(stdout);
    sleep(1);
  }
#endif
  if (!(rfile = fopen(recfile, "r"))) {
    HUP puts("Cannot open record file!");
    goto unlock;
  }
  HUP(void) putchar('\n');

  /* create a new 'topten' entry */
  t0 = newttentry();
  t0->level = dlevel;
  t0->maxlvl = maxdlevel;
  t0->hp = u.uhp;
  t0->maxhp = u.uhpmax;
  t0->points = u.urexp;
  t0->plchar = pl_character[0];
  t0->sex = (flags.female ? 'F' : 'M');
  t0->uid = uid;
  (void)strncpy(t0->name, plname, NAMSZ);
  (t0->name)[NAMSZ] = 0;
  (void)strncpy(t0->death, killer, DTHSZ);
  (t0->death)[DTHSZ] = 0;
  (void)strncpy(t0->date, getdatestr(),
                sizeof(t0->date) - 1);   /* MODERN: Safe bounded copy */
  t0->date[sizeof(t0->date) - 1] = '\0'; /* MODERN: Ensure null termination */

  /* assure minimum number of points */
  if (t0->points < POINTSMIN)
    t0->points = 0;

  t1 = tt_head = newttentry();
  tprev = 0;
  /* rank0: -1 undefined, 0 not_on_list, n n_th on list */
  for (rank = 1;;) {
    if (fscanf(rfile, "%6s %d %d %d %d %d %ld %c%c %64[^,],%128[^\n]", t1->date,
               &t1->uid, &t1->level, &t1->maxlvl, &t1->hp, &t1->maxhp,
               &t1->points, &t1->plchar, &t1->sex, t1->name, t1->death) != 11 ||
        t1->points < POINTSMIN)
      t1->points = 0;
    if (rank0 < 0 && t1->points < t0->points) {
      rank0 = rank++;
      if (tprev == 0)
        tt_head = t0;
      else
        tprev->tt_next = t0;
      t0->tt_next = t1;
      occ_cnt--;
      flg++; /* ask for a rewrite */
    } else
      tprev = t1;
    if (t1->points == 0)
      break;
    if (
#ifdef PERS_IS_UID
        t1->uid == t0->uid &&
#else
        strncmp(t1->name, t0->name, NAMSZ) == 0 &&
#endif /* PERS_IS_UID */
        t1->plchar == t0->plchar && --occ_cnt <= 0) {
      if (rank0 < 0) {
        rank0 = 0;
        rank1 = rank;
        HUP printf("You didn't beat your previous score of %ld points.\n\n",
                   t1->points);
      }
      if (occ_cnt < 0) {
        flg++;
        continue;
      }
    }
    if (rank <= ENTRYMAX) {
      t1 = t1->tt_next = newttentry();
      rank++;
    }
    if (rank > ENTRYMAX) {
      t1->points = 0;
      break;
    }
  }
  if (flg) { /* rewrite record file */
    (void)fclose(rfile);
    if (!(rfile = fopen(recfile, "w"))) {
      HUP puts("Cannot write record file\n");
      goto unlock;
    }

    if (!done_stopprint)
      if (rank0 > 0) {
        if (rank0 <= 10)
          puts("You made the top ten list!\n");
        else
          printf("You reached the %d%s place on the top %d list.\n\n", rank0,
                 ordin(rank0), ENTRYMAX);
      }
  }
  if (rank0 == 0)
    rank0 = rank1;
  if (rank0 <= 0)
    rank0 = rank;
  if (!done_stopprint)
    outheader();
  t1 = tt_head;
  for (rank = 1; t1->points != 0; rank++, t1 = t1->tt_next) {
    if (flg)
      fprintf(rfile, "%6s %d %d %d %d %d %ld %c%c %s,%s\n", t1->date, t1->uid,
              t1->level, t1->maxlvl, t1->hp, t1->maxhp, t1->points, t1->plchar,
              t1->sex, t1->name, t1->death);
    if (done_stopprint)
      continue;
    if (rank >
            (int)flags.end_top && /* MODERN: Cast to int to match rank type */
        (rank < rank0 - (int)flags.end_around ||
         rank > rank0 + (int)flags.end_around) /* MODERN: Cast to int to match
                                                  rank type */
        && (!flags.end_own ||
#ifdef PERS_IS_UID
            t1->uid != t0->uid))
#else
            strncmp(t1->name, t0->name, NAMSZ)))
#endif /* PERS_IS_UID */
      continue;
    if (rank == rank0 - (int)flags.end_around && /* MODERN: Cast to int to match
                                                    rank type */
        rank0 > (int)flags.end_top + (int)flags.end_around +
                    1 && /* MODERN: Cast to int to match rank type */
        !flags.end_own)
      (void)putchar('\n');
    if (rank != rank0)
      (void)outentry(rank, t1, 0);
    else if (!rank1)
      (void)outentry(rank, t1, 1);
    else {
      int t0lth = outentry(0, t0, -1);
      int t1lth = outentry(rank, t1, t0lth);
      if (t1lth > t0lth)
        t0lth = t1lth;
      (void)outentry(0, t0, t0lth);
    }
  }
  if (rank0 >= rank)
    if (!done_stopprint)
      (void)outentry(0, t0, 1);
  (void)fclose(rfile);
unlock:
  /* Original 1984 code had no cleanup as program exits immediately after
   * topten() */
#ifdef ENABLE_MODERN_LOCKING
  modern_unlock_record();
#else
  (void)unlink(reclock);
#endif
}

void outheader(void) {
  char linebuf[BUFSZ];
  char *bp;
  (void)strcpy(linebuf, "Number Points  Name");
  bp = eos(linebuf);
  while (bp < linebuf + COLNO - 9)
    *bp++ = ' ';
  (void)strcpy(bp, "Hp [max]");
  puts(linebuf);
}

/* so>0: standout line; so=0: ordinary line; so<0: no output, return lth */
int outentry(int rank, struct toptenentry *t1, int so) {
  boolean quit = FALSE, killed = FALSE, starv = FALSE;
  char linebuf[BUFSZ];
  linebuf[0] = 0;
  if (rank)
    SafeAppend(linebuf, "%3d", rank);
  else
    SafeAppend(linebuf, "   ");
  SafeAppend(linebuf, " %6ld %8s", t1->points, t1->name);
  if (t1->plchar == 'X')
    SafeAppend(linebuf, " ");
  else
    SafeAppend(linebuf, "-%c ", t1->plchar);
  if (!strncmp("escaped", t1->death, 7)) {
    if (!strcmp(" (with amulet)", t1->death + 7))
      SafeAppend(linebuf, "escaped the dungeon with amulet");
    else
      SafeAppend(linebuf, "escaped the dungeon [max level %d]", t1->maxlvl);
  } else {
    if (!strncmp(t1->death, "quit", 4)) {
      quit = TRUE;
      if (t1->maxhp < 3 * t1->hp && t1->maxlvl < 4)
        SafeAppend(linebuf, "cravenly gave up");
      else
        SafeAppend(linebuf, "quit");
    } else if (!strcmp(t1->death, "choked"))
      SafeAppend(linebuf, "choked on %s food",
                 (t1->sex == 'F') ? "her" : "his");
    else if (!strncmp(t1->death, "starv", 5)) {
      SafeAppend(linebuf, "starved to death");
      starv = TRUE;
    } else {
      SafeAppend(linebuf, "was killed");
      killed = TRUE;
    }
    SafeAppend(linebuf, " on%s level %d", (killed || starv) ? "" : " dungeon",
               t1->level);
    if (t1->maxlvl != t1->level)
      SafeAppend(linebuf, " [max %d]", t1->maxlvl);
    if (quit && t1->death[4])
      SafeAppend(linebuf, "%s", t1->death + 4);
  }
  if (killed)
    SafeAppend(
        linebuf, " by %s%s",
        (!strncmp(t1->death, "trick", 5) || !strncmp(t1->death, "the ", 4)) ? ""
        : index(vowels, *t1->death) ? "an "
                                    : "a ",
        t1->death);
  SafeAppend(linebuf, ".");
  if (t1->maxhp) {
    char *bp = eos(linebuf);
    char hpbuf[12]; /* MODERN: Buffer size increased from 10 to 12 to prevent
                       overflow from INT_MIN (-2147483648) */
    int hppos;
    snprintf(hpbuf, sizeof(hpbuf), "%s",
             (t1->hp > 0) ? itoa(t1->hp)
                          : "-"); /* MODERN: Safe sprintf replacement to prevent
                                     buffer overflow */
    hppos = COLNO - 7 - strlen(hpbuf);
    if (bp <= linebuf + hppos) {
      while (bp < linebuf + hppos)
        *bp++ = ' ';
      (void)strcpy(bp, hpbuf);
      /* MODERN: Bounds-checked append to prevent buffer*/
      char *append_pos = eos(bp);
      size_t remaining = BUFSZ - (append_pos - linebuf);
      if (remaining > 1) {
        snprintf(append_pos, remaining, " [%d]", t1->maxhp);
      }
    }
  }
  if (so == 0)
    puts(linebuf);
  else if (so > 0) {
    char *bp = eos(linebuf);
    if (so >= COLNO)
      so = COLNO - 1;
    while (bp < linebuf + so)
      *bp++ = ' ';
    *bp = 0;
    standoutbeg();
    fputs(linebuf, stdout);
    standoutend();
    (void)putchar('\n');
  }
  return (strlen(linebuf));
}

char *itoa(int a) {
  static char buf[12];
  snprintf(buf, sizeof(buf), "%d", a); /* MODERN: Safe sprintf replacement -
                                          identical output, prevents overflow */
  return (buf);
}

const char * /* MODERN: const because returns string literals */
ordin(int n) {
  int d = n % 10;
  return ((d == 0 || d > 3 || n / 10 == 1) ? "th"
          : (d == 1)                       ? "st"
          : (d == 2)                       ? "nd"
                                           : "rd");
}

void clearlocks(void) {
  int x;
  (void)signal(SIGHUP, SIG_IGN);

  /* MODERN ADDITION (2025): Clean up memory for sanitizers */
  cleanup_all_engravings();

#ifdef ENABLE_MODERN_LOCKING
  /* MODERN ADDITION (2025): Clean up flock()-based locks */
  modern_unlock_game();
  modern_unlock_record();
#endif

  for (x = maxdlevel; x >= 0; x--) {
    glo(x);
    (void)unlink(lock); /* not all levels need be present */
  }
}

/**
 * MODERN ADDITION (2025): Enhanced signal handling for window manager
 * compatibility
 *
 * WHY: Original 1984 code only handled SIGHUP (terminal disconnect).
 * Modern window managers like Hyprland/Wayland send SIGTERM when forcibly
 * closing terminal windows, leaving stale lock files that cause infinite waits
 * on next game start.
 *
 * HOW: Unified cleanup handler for SIGHUP, SIGTERM, and SIGQUIT ensures proper
 * lock file cleanup regardless of how the process terminates. Uses existing
 * clearlocks() mechanism to maintain compatibility with original game logic.
 *
 * PRESERVES: Original 1984 save-on-hangup vs no-save behavior via
 * NOSAVEONHANGUP ADDS: Modern signal compatibility while maintaining authentic
 * game experience
 */
void modern_cleanup_handler(int sig) {
  (void)sig;
  /* Ignore further interrupts during cleanup */
  (void)signal(SIGINT, SIG_IGN);
  (void)signal(SIGTERM, SIG_IGN);
  (void)signal(SIGQUIT, SIG_IGN);

  /* Use existing 1984 cleanup mechanism */
  clearlocks();

  /* Exit with appropriate code based on signal */
  exit(1);
}

#ifdef NOSAVEONHANGUP
void hangup(int sig) { modern_cleanup_handler(sig); }
#endif /* NOSAVEONHANGUP */

char *eos(char *s) {
  while (*s)
    s++;
  return (s);
}

/* it is the callers responsibility to check that there is room for c */
void charcat(char *s, char c) {
  while (*s)
    s++;
  *s++ = c;
  *s = 0;
}

/*
 * Called with args from main if argc >= 0. In this case, list scores as
 * requested. Otherwise, find scores for the current player (and list them
 * if argc == -1).
 */
void prscore(int argc, char **argv) {
  reset_topten_arena(); /* MODERN: Reset arena for fresh allocation */
  extern char *hname;
  char **players;
  int playerct;
  int rank;
  struct toptenentry *t1, *t2;
  const char *recfile =
      RECORD; /* MODERN: const because points to string literal */
  FILE *rfile;
  int flg = 0;
  int i;
#ifdef nonsense
  long total_score = 0L;
  char totchars[10];
  int totcharct = 0;
#endif /* nonsense */
  int outflg = (argc >= -1);
#ifdef PERS_IS_UID
  int uid = -1;
#else
  char *player0;
#endif /* PERS_IS_UID */

  if (!(rfile = fopen(recfile, "r"))) {
    puts("Cannot open record file!");
    return;
  }

  if (argc > 1 && !strncmp(argv[1], "-s", 2)) {
    if (!argv[1][2]) {
      argc--;
      argv++;
    } else if (!argv[1][3] && index("CFKSTWX", argv[1][2])) {
      argv[1]++;
      argv[1][0] = '-';
    } else
      argv[1] += 2;
  }
  if (argc <= 1) {
#ifdef PERS_IS_UID
    uid = getuid();
    playerct = 0;
#else
    player0 = plname;
    if (!*player0)
      player0 = "hackplayer";
    playerct = 1;
    players = &player0;
#endif /* PERS_IS_UID */
  } else {
    playerct = --argc;
    players = ++argv;
  }
  if (outflg)
    putchar('\n');

  t1 = tt_head = newttentry();
  for (rank = 1;; rank++) {
    if (fscanf(rfile, "%6s %d %d %d %d %d %ld %c%c %64[^,],%128[^\n]", t1->date,
               &t1->uid, &t1->level, &t1->maxlvl, &t1->hp, &t1->maxhp,
               &t1->points, &t1->plchar, &t1->sex, t1->name, t1->death) != 11)
      t1->points = 0;
    if (t1->points == 0)
      break;
#ifdef PERS_IS_UID
    if (!playerct && t1->uid == uid)
      flg++;
    else
#endif /* PERS_IS_UID */
      for (i = 0; i < playerct; i++) {
        if (strcmp(players[i], "all") == 0 ||
            strncmp(t1->name, players[i], NAMSZ) == 0 ||
            (players[i][0] == '-' && players[i][1] == t1->plchar &&
             players[i][2] == 0) ||
            (digit(players[i][0]) && rank <= atoi(players[i])))
          flg++;
      }
    t1 = t1->tt_next = newttentry();
  }
  (void)fclose(rfile);
  if (!flg) {
    if (outflg) {
      printf("Cannot find any entries for ");
      if (playerct < 1)
        printf("you.\n");
      else {
        if (playerct > 1)
          printf("any of ");
        for (i = 0; i < playerct; i++)
          printf("%s%s", players[i], (i < playerct - 1) ? ", " : ".\n");
        printf("Call is: %s -s [playernames]\n", hname);
      }
    }
    return;
  }

  if (outflg)
    outheader();
  t1 = tt_head;
  for (rank = 1; t1->points != 0; rank++, t1 = t2) {
    t2 = t1->tt_next;
#ifdef PERS_IS_UID
    if (!playerct && t1->uid == uid)
      goto outwithit;
    else
#endif /* PERS_IS_UID */
      for (i = 0; i < playerct; i++) {
        if (strcmp(players[i], "all") == 0 ||
            strncmp(t1->name, players[i], NAMSZ) == 0 ||
            (players[i][0] == '-' && players[i][1] == t1->plchar &&
             players[i][2] == 0) ||
            (digit(players[i][0]) && rank <= atoi(players[i]))) {
        outwithit:
          if (outflg)
            (void)outentry(rank, t1, 0);
#ifdef nonsense
          total_score += t1->points;
          if (totcharct < sizeof(totchars) - 1)
            totchars[totcharct++] = t1->plchar;
#endif /* nonsense */
          break;
        }
      }
    free((char *)t1);
  }
#ifdef nonsense
  totchars[totcharct] = 0;

  /* We would like to determine whether he is experienced. However,
     the information collected here only tells about the scores/roles
     that got into the topten (top 100?). We should maintain a
     .hacklog or something in his home directory. */
  flags.beginner = (total_score < 6000);
  for (i = 0; i < 6; i++)
    if (!index(totchars, "CFKSTWX"[i])) {
      flags.beginner = 1;
      if (!pl_character[0])
        pl_character[0] = "CFKSTWX"[i];
      break;
    }
#endif /* nonsense */
}
