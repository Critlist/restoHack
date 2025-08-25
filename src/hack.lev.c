/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.lev.c - version 1.0.3 */
/* $FreeBSD$ */

/*
 * Level save/load system for 1984 Hack - dungeon persistence
 * Original 1984 source: docs/historical/original-source/hack.lev.c
 *
 * Key modernizations: ANSI C function signatures
 */

#include "hack.h"
/* def.mkroom.h already included via hack.h */

#include <limits.h>
#include <unistd.h>
extern struct monst *restmonchn();
extern struct obj *restobjchn();
extern struct obj *billobjs;
extern char *itoa();
extern char SAVEF[];
extern int hackpid;
extern xchar dlevel;
extern char nul[];

#ifndef NOWORM
#include "def.wseg.h"
extern struct wseg *wsegs[32], *wheads[32];
extern long wgrowtime[32];
#endif /* NOWORM */

boolean level_exists[MAXLEVEL + 1];

/* Original 1984: void savelev(int fd, xchar lev) */
void savelev(
    int fd,
    unsigned char lev) /* MODERN: unsigned to prevent buffer underflow */
{
#ifndef NOWORM
  struct wseg *wtmp, *wtmp2;
  int tmp;
#endif /* NOWORM */

  if (fd < 0)
    panic("Save on bad file!"); /* impossible */
  /* Note: lev is unsigned char, so >= 0 check is redundant */
  if (lev <= MAXLEVEL)
    level_exists[lev] = TRUE;

  bwrite(fd, (char *)&hackpid, sizeof(hackpid));
  bwrite(fd, (char *)&lev, sizeof(lev));
  bwrite(fd, (char *)levl, sizeof(levl));
  bwrite(fd, (char *)&moves, sizeof(long));
  bwrite(fd, (char *)&xupstair, sizeof(xupstair));
  bwrite(fd, (char *)&yupstair, sizeof(yupstair));
  bwrite(fd, (char *)&xdnstair, sizeof(xdnstair));
  bwrite(fd, (char *)&ydnstair, sizeof(ydnstair));
  savemonchn(fd, fmon);
  savegoldchn(fd, fgold);
  savetrapchn(fd, ftrap);
  saveobjchn(fd, fobj);
  saveobjchn(fd, billobjs);
  billobjs = 0;
  save_engravings(fd);
#ifndef QUEST
  bwrite(fd, (char *)rooms, sizeof(rooms));
  bwrite(fd, (char *)doors, sizeof(doors));
#endif /* QUEST */
  fgold = 0;
  ftrap = 0;
  fmon = 0;
  fobj = 0;
#ifndef NOWORM
  bwrite(fd, (char *)wsegs, sizeof(wsegs));
  for (tmp = 1; tmp < 32; tmp++) {
    for (wtmp = wsegs[tmp]; wtmp; wtmp = wtmp2) {
      wtmp2 = wtmp->nseg;
      bwrite(fd, (char *)wtmp, sizeof(struct wseg));
    }
    wsegs[tmp] = 0;
  }
  bwrite(fd, (char *)wgrowtime, sizeof(wgrowtime));
#endif /* NOWORM */
}

void bwrite(int fd, char *loc, unsigned num) {
  /* lint wants the 3rd arg of write to be an int; lint -p an unsigned */
  if (write(fd, loc, (int)num) != num)
    panic("cannot write %u bytes to file #%d", num, fd);
}

void saveobjchn(int fd, struct obj *otmp) {
  struct obj *otmp2;
  unsigned xl;
  int minusone = -1;

  while (otmp) {
    otmp2 = otmp->nobj;
    xl = otmp->onamelth;
    /* MODERN: Check for integer overflow in write size calculation */
    if (xl > UINT_MAX - sizeof(struct obj)) {
      error("Save file corrupted: object name length overflow");
    }
    bwrite(fd, (char *)&xl, sizeof(int));
    bwrite(fd, (char *)otmp, xl + sizeof(struct obj));
    free((char *)otmp);
    otmp = otmp2;
  }
  bwrite(fd, (char *)&minusone, sizeof(int));
}

void savemonchn(int fd, struct monst *mtmp) {
  struct monst *mtmp2;
  unsigned xl;
  int minusone = -1;
  struct permonst *monbegin = &mons[0];

  bwrite(fd, (char *)&monbegin, sizeof(monbegin));

  while (mtmp) {
    mtmp2 = mtmp->nmon;
    /* MODERN: Check for integer overflow in length calculation */
    if (mtmp->mxlth > UINT_MAX - mtmp->mnamelth ||
        mtmp->mxlth + mtmp->mnamelth > UINT_MAX - sizeof(struct monst)) {
      error("Save file corrupted: monster data length overflow");
    }
    xl = mtmp->mxlth + mtmp->mnamelth;
    bwrite(fd, (char *)&xl, sizeof(int));
    bwrite(fd, (char *)mtmp, xl + sizeof(struct monst));
    if (mtmp->minvent)
      saveobjchn(fd, mtmp->minvent);
    free((char *)mtmp);
    mtmp = mtmp2;
  }
  bwrite(fd, (char *)&minusone, sizeof(int));
}

void savegoldchn(int fd, struct gold *gold) {
  struct gold *gold2;
  while (gold) {
    gold2 = gold->ngold;
    bwrite(fd, (char *)gold, sizeof(struct gold));
    free((char *)gold);
    gold = gold2;
  }
  bwrite(fd, nul, sizeof(struct gold));
}

void savetrapchn(int fd, struct trap *trap) {
  struct trap *trap2;
  while (trap) {
    trap2 = trap->ntrap;
    bwrite(fd, (char *)trap, sizeof(struct trap));
    free((char *)trap);
    trap = trap2;
  }
  bwrite(fd, nul, sizeof(struct trap));
}

void getlev(int fd, int pid, xchar lev) {
  struct gold *gold;
  struct trap *trap;
#ifndef NOWORM
  struct wseg *wtmp;
#endif /* NOWORM */
  int tmp;
  long omoves;
  int hpid;
  xchar dlvl;

  /* First some sanity checks */
  mread(fd, (char *)&hpid, sizeof(hpid));
  mread(fd, (char *)&dlvl, sizeof(dlvl));
  if ((pid && pid != hpid) || (lev && dlvl != lev)) {
    pline("Strange, this map is not as I remember it.");
    pline("Somebody is trying some trickery here ...");
    pline("This game is void ...");
    done("tricked");
  }

  fgold = 0;
  ftrap = 0;
  mread(fd, (char *)levl, sizeof(levl));
  mread(fd, (char *)&omoves, sizeof(omoves));
  mread(fd, (char *)&xupstair, sizeof(xupstair));
  mread(fd, (char *)&yupstair, sizeof(yupstair));
  mread(fd, (char *)&xdnstair, sizeof(xdnstair));
  mread(fd, (char *)&ydnstair, sizeof(ydnstair));

  fmon = restmonchn(fd);

  /* regenerate animals while on another level */
  {
    long tmoves = (moves > omoves) ? moves - omoves : 0;
    struct monst *mtmp, *mtmp2;
    extern char genocided[];

    for (mtmp = fmon; mtmp; mtmp = mtmp2) {
      long newhp; /* tmoves may be very large */

      mtmp2 = mtmp->nmon;
      if (index(genocided, mtmp->data->mlet)) {
        mondead(mtmp);
        continue;
      }

      if (mtmp->mtame && tmoves > 250) {
        mtmp->mtame = 0;
        mtmp->mpeaceful = 0;
      }

      newhp =
          mtmp->mhp + (index(MREGEN, mtmp->data->mlet) ? tmoves : tmoves / 20);
      if (newhp > mtmp->mhpmax)
        mtmp->mhp = mtmp->mhpmax;
      else
        mtmp->mhp = newhp;
    }
  }

  setgd();
  gold = newgold();
  mread(fd, (char *)gold, sizeof(struct gold));
  int gold_count = 0; /* MODERN: Prevent infinite loop attack */
  while (gold->gx && gold_count < 1000) { /* Reasonable limit */
    gold_count++;
    gold->ngold = fgold;
    fgold = gold;
    gold = newgold();
    mread(fd, (char *)gold, sizeof(struct gold));
  }
  if (gold_count >= 1000) {
    error("Save file corrupted: too many gold objects");
  }
  free((char *)gold);
  trap = newtrap();
  mread(fd, (char *)trap, sizeof(struct trap));
  int trap_count = 0; /* MODERN: Prevent infinite loop attack */
  while (trap->tx && trap_count < 1000) { /* Reasonable limit */
    trap_count++;
    trap->ntrap = ftrap;
    ftrap = trap;
    trap = newtrap();
    mread(fd, (char *)trap, sizeof(struct trap));
  }
  if (trap_count >= 1000) {
    error("Save file corrupted: too many traps");
  }
  free((char *)trap);
  fobj = restobjchn(fd);
  billobjs = restobjchn(fd);
  rest_engravings(fd);
#ifndef QUEST
  mread(fd, (char *)rooms, sizeof(rooms));
  mread(fd, (char *)doors, sizeof(doors));
#endif /* QUEST */
#ifndef NOWORM
  mread(fd, (char *)wsegs, sizeof(wsegs));
  for (tmp = 1; tmp < 32; tmp++)
    if (wsegs[tmp]) {
      wheads[tmp] = wsegs[tmp] = wtmp = newseg();
      int seg_count = 0; /* MODERN: Prevent infinite loop attack */
      while (seg_count < 500) { /* Reasonable segment limit */
        mread(fd, (char *)wtmp, sizeof(struct wseg));
        if (!wtmp->nseg)
          break;
        seg_count++;
        wheads[tmp]->nseg = wtmp = newseg();
        wheads[tmp] = wtmp;
      }
      if (seg_count >= 500) {
        error("Save file corrupted: too many worm segments");
      }
    }
  mread(fd, (char *)wgrowtime, sizeof(wgrowtime));
#endif /* NOWORM */
}

void mread(int fd, char *buf, unsigned len) {
  int rlen;
  extern boolean restoring;

  rlen = read(fd, buf, (int)len);
  if (rlen !=
      (int)len) { /* MODERN: Cast to int to match rlen type from read() */
    pline("Read error: expected %u bytes, got %d bytes.", len, rlen); /* MODERN: Safe message format */
    if (restoring) {
      (void)unlink(SAVEF);
      error("Error restoring old game.");
    }
    panic("Error reading level file.");
  }
}

void mklev(void) {
  extern boolean in_mklev;

  if (getbones())
    return;

  in_mklev = TRUE;
  makelevel();
  in_mklev = FALSE;
}
