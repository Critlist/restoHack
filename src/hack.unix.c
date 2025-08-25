/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.unix.c - version 1.0.3 */
/* $FreeBSD$ */

/*
 * Unix system interface for 1984 Hack - file operations and system calls
 * Original 1984 source: docs/historical/original-source/hack.unix.c
 *
 * Key modernizations: ANSI C function signatures, POSIX compatibility
 */

/* This file collects some Unix dependencies; hack.pager.c contains some more */

/*
 * The time is used for:
 *	- seed for random()
 *	- year on tombstone and yymmdd in record file
 *	- phase of the moon (various monsters react to NEW_MOON or FULL_MOON)
 *	- night and midnight (the undead are dangerous at midnight)
 *	- determination of what files are "very old"
 */

#include "generated/config.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(HAVE_GETENTROPY)
#include <sys/random.h>
#endif
#if defined(HAVE_BSD_STDLIB_H)
#include <bsd/stdlib.h>
#endif
#include "hack.h" /* mainly for index() which depends on BSD */

#include <sys/stat.h>
#include <sys/types.h> /* for time_t and stat */
#include <time.h>

#if 0
/* ORIGINAL 1984 CODE - commented out for NetBSD/pkgsrc compatibility */
void
setrandom(void)
{
#ifdef __linux__
	/* Linux doesn't have srandomdev(), use time-based seeding */
	(void) srandom((unsigned int)time(NULL));
#else
	(void) srandomdev();
#endif
}
#endif

/**
 * MODERN ADDITION (2025): Portable RNG seeding with feature detection
 *
 * WHY: NetBSD/pkgsrc build fails with OS-specific #ifdef __linux__ checks.
 * Original code violated portability principles by hardcoding platform names.
 *
 * HOW: Uses CMake feature detection to test for arc4random, srandomdev, and
 * getentropy at build time. Falls back to time+PID seeding if none available.
 *
 * PRESERVES: Original setrandom() function name and behavior - initializes
 * random number generator with high-quality seed when available.
 *
 * ADDS: Cross-platform compatibility using feature detection instead of
 * OS-specific conditionals. Follows project standard of avoiding platform
 * ifdefs.
 */
static uint32_t secure_seed(void) {
#if defined(HAVE_ARC4RANDOM_BUF)
  uint32_t s = 0; /* MODERN: Initialize to prevent MSan warning */
  arc4random_buf(&s, sizeof(s));
  return s;
#elif defined(HAVE_ARC4RANDOM)
  return arc4random();
#elif defined(HAVE_GETENTROPY)
  uint32_t s = 0; /* MODERN: Initialize to prevent MSan warning */
  if (getentropy(&s, sizeof(s)) == 0)
    return s;
  /* fall through on error */
#endif
  /* last-resort fallback */
  return (uint32_t)(time(NULL) ^ (getpid() * 0x9e3779b1u));
}

/**
 * MODERN ADDITION (2025): Cross-platform PRNG seeding wrapper
 *
 * WHY: Original 1984 code used simple time()-based seeding which is
 * predictable. Modern systems need cryptographically secure seeding for game
 * integrity.
 *
 * HOW: Uses feature detection to choose best available seeding method:
 * srandomdev() on BSD systems, secure_seed() fallback on others.
 *
 * PRESERVES: Same srandom() interface as original 1984 implementation
 * ADDS: Secure entropy sources (arc4random, getentropy) for unpredictable seeds
 */
static void seed_prng(void) {
#ifdef HAVE_SRANDOMDEV
  srandomdev();
#else
  uint32_t seed = secure_seed();
  srandom(seed);
#endif
}

void setrandom(void) { seed_prng(); }

struct tm *getlt(void) {
  time_t date;

  (void)time(&date);
  return (localtime(&date));
}

int getyear(void) { return (1900 + getlt()->tm_year); }

char *getdatestr(void) {
  static char datestr[32]; /* MODERN: Buffer increased to handle worst-case
                              snprintf output safely */
  struct tm *lt = getlt();

  (void)snprintf(datestr, sizeof(datestr),
                 "%02d%02d%02d", /* MODERN: Safe sprintf replacement with
                                    adequate buffer */
                 lt->tm_year % 100, lt->tm_mon + 1, lt->tm_mday);
  return (datestr);
}

int phase_of_the_moon(void) /* 0-7, with 0: new, 4: full */
{                           /* moon period: 29.5306 days */
                            /* year: 365.2422 days */
  struct tm *lt = getlt();
  int epact, diy, golden;

  diy = lt->tm_yday;
  golden = (lt->tm_year % 19) + 1;
  epact = (11 * golden + 18) % 30;
  if ((epact == 25 && golden > 11) || epact == 24)
    epact++;

  return ((((((diy + epact) * 6) + 11) % 177) / 22) & 7);
}

int night(void) {
  int hour = getlt()->tm_hour;

  return (hour < 6 || hour > 21);
}

int midnight(void) { return (getlt()->tm_hour == 0); }

struct stat buf, hbuf;

void gethdate(char *name) {
  /* old version - for people short of space */
  /* char *np; */ /* MODERN ADDITION (2025): Commented out to avoid
                     redeclaration with PATH version */

  /* MODERN: Validate argv[0] length to prevent buffer overflow */
  if (!name || strlen(name) > 256) {
    error("Invalid executable name provided.");
    return;
  }

  /* MODERN: Extract basename only - no directory traversal allowed */
  const char *basename = strrchr(name, '/');
  if (basename) {
    name = (char *)(basename + 1); /* Use only the executable name */
  }

#if 0
	name = "/usr/games/hide/hack";
	if(stat(name, &hbuf))
		error("Cannot get status of %s.",
			(np = rindex(name, '/')) ? np+1 : name);
#else
/* version using PATH from: seismo!gregc@ucsf-cgl.ARPA (Greg Couch) */

/*
 * The problem with   #include	<sys/param.h>   is that this include file
 * does not exist on all systems, and moreover, that it sometimes includes
 * <sys/types.h> again, so that the compiler sees these typedefs twice.
 */
#define MAXPATHLEN 1024

  /* MODERN: Additional name validation after basename extraction */
  if (strlen(name) > 64) { /* Reasonable executable name limit */
    error("Executable name too long.");
    return;
  }

  char *np;
  const char *path; /* MODERN: const because can point to getenv() result or
                       string literal */
  char filename[MAXPATHLEN + 1];
  if (index(name, '/') != NULL || (path = getenv("PATH")) == NULL)
    path = "";

  for (;;) {
    if ((np = (char *)index(path, ':')) ==
        NULL) /* MODERN: Cast needed due to const path */
      np = (char *)(path + strlen(path)); /* point to end str - MODERN: Cast
                                             needed due to const path */
    if (np - path <= 1)                   /* %% */
      (void)strcpy(filename, name);
    else {
      /* MODERN: Safe path construction with bounds checking */
      size_t path_len = np - path;
      size_t name_len = strlen(name);

      /* Ensure we have space for path + '/' + name + '\0' */
      if (path_len + 1 + name_len + 1 > MAXPATHLEN) {
        continue; /* Skip this path entry if it would overflow */
      }

      (void)strncpy(filename, path, path_len);
      filename[path_len] = '/';
      filename[path_len + 1] = '\0'; /* MODERN: Ensure null termination */

      /* MODERN: Use snprintf for final copy with bounds checking */
      int ret = snprintf(filename + path_len + 1, MAXPATHLEN - path_len - 1,
                         "%s", name);
      if (ret < 0 || ret >= (int)(MAXPATHLEN - path_len - 1)) {
        continue; /* Skip this path entry on truncation */
      }
    }
    if (stat(filename, &hbuf) == 0)
      return;
    if (*np == '\0')
      break;
    path = np + 1;
  }
  error("Cannot get file status."); /* MODERN: Safe message without format
                                       string vulnerability */
#endif
}

int uptodate(int fd) {
  if (fstat(fd, &buf)) {
    pline("Cannot get status of saved level? ");
    return (0);
  }
  if (buf.st_mtime < hbuf.st_mtime) {
    pline("Saved level is out of date. ");
    return (0);
  }
  return (1);
}

/* see whether we should throw away this xlock file */
int veryold(int fd) {
  int i;
  time_t date;

  if (fstat(fd, &buf))
    return (0); /* cannot get status */
  if (buf.st_size != sizeof(int))
    return (0); /* not an xlock file */
  (void)time(&date);
  if (date - buf.st_mtime < 3L * 24L * 60L * 60L) { /* recent */
    extern int errno;
    int lockedpid; /* should be the same size as hackpid */

    if (read(fd, (char *)&lockedpid, sizeof(lockedpid)) != sizeof(lockedpid))
      /* strange ... */
      return (0);

    /* From: Rick Adams <seismo!rick> */
    /* This will work on 4.1cbsd, 4.2bsd and system 3? & 5. */
    /* It will do nothing on V7 or 4.1bsd. */
    if (!(kill(lockedpid, 0) == -1 && errno == ESRCH))
      return (0);
  }
  (void)close(fd);
  for (i = 1; i <= MAXLEVEL; i++) { /* try to remove all */
    glo(i);
    (void)unlink(lock);
  }
  glo(0);
  if (unlink(lock))
    return (0); /* cannot remove it */
  return (1);   /* success! */
}

void getlock(void) {
  extern int errno, hackpid, locknum;
  int i = 0, fd;

  (void)fflush(stdout);

  /* we ignore QUIT and INT at this point */

#ifdef ENABLE_MODERN_LOCKING
  /* MODERN ADDITION (2025): Use flock()-based locking for reliability */
  if (!modern_lock_game()) {
    getret();
    error("");
  }
#else
  /* Original 1984 link()-based locking preserved as fallback */
  if (link(HLOCK, LLOCK) == -1) {
    int errnosv = errno;

    perror(HLOCK);
    printf("Cannot link %s to %s\n", LLOCK, HLOCK);
    switch (errnosv) {
    case ENOENT:
      printf("Perhaps there is no (empty) file %s ?\n", HLOCK);
      break;
    case EACCES:
      printf("It seems you don't have write permission here.\n");
      break;
    case EEXIST:
      printf("(Try again or rm %s.)\n", LLOCK);
      break;
    default:
      printf("I don't know what is wrong.");
    }
    getret();
    error("");
  }
#endif

  regularize(lock);
  glo(0);
  if (locknum > 25)
    locknum = 25;

  do {
    if (locknum)
      lock[0] = 'a' + i++;

    if ((fd = open(lock, 0)) == -1) {
      if (errno == ENOENT)
        goto gotlock; /* no such file */
      perror(lock);
#ifdef ENABLE_MODERN_LOCKING
      modern_unlock_game();
#else
      (void)unlink(LLOCK);
#endif
      error("Cannot open lock file."); /* MODERN: Safe message without format
                                          string vulnerability */
    }

    if (veryold(fd)) /* if true, this closes fd and unlinks lock */
      goto gotlock;
    (void)close(fd);
  } while (i < locknum);

#ifdef ENABLE_MODERN_LOCKING
  modern_unlock_game();
#else
  (void)unlink(LLOCK);
#endif
  error(locknum ? "Too many hacks running now."
                : "There is a game in progress under your name.");
gotlock:
  fd = creat(lock, FMASK);
#ifdef ENABLE_MODERN_LOCKING
  /* Modern locking doesn't need LLOCK cleanup */
#else
  if (unlink(LLOCK) == -1)
    error("Cannot unlink lock file."); /* MODERN: Safe message without format
                                          string vulnerability */
#endif
  if (fd == -1) {
    error("cannot creat lock file.");
  } else {
    if (write(fd, (char *)&hackpid, sizeof(hackpid)) != sizeof(hackpid)) {
      error("cannot write lock");
    }
    if (close(fd) == -1) {
      error("cannot close lock");
    }
  }
}

#ifdef MAIL

/*
 * Notify user when new mail has arrived. [Idea from Merlyn Leroy, but
 * I don't know the details of his implementation.]
 * { Later note: he disliked my calling a general mailreader and felt that
 *   hack should do the paging itself. But when I get mail, I want to put it
 *   in some folder, reply, etc. - it would be unreasonable to put all these
 *   functions in hack. }
 * The mail daemon '2' is at present not a real monster, but only a visual
 * effect. Thus, makemon() is superfluous. This might become otherwise,
 * however. The motion of '2' is less restrained than usual: diagonal moves
 * from a DOOR are possible. He might also use SDOOR's. Also, '2' is visible
 * in a ROOM, even when you are Blind.
 * Its path should be longer when you are Telepat-hic and Blind.
 *
 * Interesting side effects:
 *	- You can get rich by sending yourself a lot of mail and selling
 *	  it to the shopkeeper. Unfortunately mail isn't very valuable.
 *	- You might die in case '2' comes along at a critical moment during
 *	  a fight and delivers a scroll the weight of which causes you to
 *	  collapse.
 *
 * Possible extensions:
 *	- Open the file MAIL and do fstat instead of stat for efficiency.
 *	  (But sh uses stat, so this cannot be too bad.)
 *	- Examine the mail and produce a scroll of mail called "From somebody".
 *	- Invoke MAILREADER in such a way that only this single letter is read.
 *
 *	- Make him lose his mail when a Nymph steals the letter.
 *	- Do something to the text when the scroll is enchanted or cancelled.
 */
static struct stat omstat, nmstat;
static char *mailbox;
static long laststattime;

void getmailstatus(void) {
  if (!(mailbox = getenv("MAIL")))
    return;
  if (stat(mailbox, &omstat)) {
#ifdef PERMANENT_MAILBOX
    pline("Cannot get status of MAIL=%s .", mailbox);
    mailbox = 0;
#else
    omstat.st_mtime = 0;
#endif /* PERMANENT_MAILBOX */
  }
}

void ckmailstatus(void) {
  if (!mailbox
#ifdef MAILCKFREQ
      || moves < laststattime + MAILCKFREQ
#endif /* MAILCKFREQ */
  )
    return;
  laststattime = moves;
  if (stat(mailbox, &nmstat)) {
#ifdef PERMANENT_MAILBOX
    pline("Cannot get status of MAIL=%s anymore.", mailbox);
    mailbox = 0;
#else
    nmstat.st_mtime = 0;
#endif /* PERMANENT_MAILBOX */
  } else if (nmstat.st_mtime > omstat.st_mtime) {
    if (nmstat.st_size)
      newmail();
    getmailstatus(); /* might be too late ... */
  }
}

void newmail(void) {
  /* produce a scroll of mail */
  struct obj *obj;
  struct monst *md;
  extern char plname[];
  extern struct obj *mksobj(), *addinv();
  extern struct monst *makemon();
  extern struct permonst pm_mail_daemon;

  obj = mksobj(SCR_MAIL);
  if (md = makemon(&pm_mail_daemon, u.ux, u.uy)) /* always succeeds */
    mdrush(md, 0);

  pline("\"Hello, %s! I have some mail for you.\"", plname);
  if (md) {
    if (dist(md->mx, md->my) > 2)
      pline("\"Catch!\"");
    more();

    /* let him disappear again */
    mdrush(md, 1);
    mondead(md);
  }

  obj = addinv(obj);
  (void)identify(obj); /* set known and do prinv() */
}

/* make md run through the cave */
void mdrush(struct monst *md, boolean away) {
  int uroom = inroom(u.ux, u.uy);
  if (uroom >= 0) {
    int tmp = rooms[uroom].fdoor;
    int cnt = rooms[uroom].doorct;
    int fx = u.ux, fy = u.uy;
    while (cnt--) {
      if (dist(fx, fy) < dist(doors[tmp].x, doors[tmp].y)) {
        fx = doors[tmp].x;
        fy = doors[tmp].y;
      }
      tmp++;
    }
    tmp_at(-1, md->data->mlet); /* open call */
    if (away) {                 /* interchange origin and destination */
      unpmon(md);
      tmp = fx;
      fx = md->mx;
      md->mx = tmp;
      tmp = fy;
      fy = md->my;
      md->my = tmp;
    }
    while (fx != md->mx || fy != md->my) {
      int dx, dy, nfx = fx, nfy = fy, d1, d2;

      tmp_at(fx, fy);
      d1 = DIST(fx, fy, md->mx, md->my);
      for (dx = -1; dx <= 1; dx++)
        for (dy = -1; dy <= 1; dy++)
          if (dx || dy) {
            d2 = DIST(fx + dx, fy + dy, md->mx, md->my);
            if (d2 < d1) {
              d1 = d2;
              nfx = fx + dx;
              nfy = fy + dy;
            }
          }
      if (nfx != fx || nfy != fy) {
        fx = nfx;
        fy = nfy;
      } else {
        if (!away) {
          md->mx = fx;
          md->my = fy;
        }
        break;
      }
    }
    tmp_at(-1, -1); /* close call */
  }
  if (!away)
    pmon(md);
}

void readmail(void) {
#ifdef DEF_MAILREADER /* This implies that UNIX is defined */
  char *mr = 0;
  more();
  if (!(mr = getenv("MAILREADER")))
    mr = DEF_MAILREADER;
  if (child(1)) {
    execl(mr, mr, (char *)0);
    exit(1);
  }
#else  /* DEF_MAILREADER */
  (void)page_file(mailbox, FALSE);
#endif /* DEF_MAILREADER */
  /* get new stat; not entirely correct: there is a small time
     window where we do not see new mail */
  getmailstatus();
}
#endif /* MAIL */

void regularize(char *s) /* normalize file name - we don't like ..'s or /'s */
{
  char *lp;

  while ((lp = index(s, '.')) || (lp = index(s, '/')))
    *lp = '_';
}
