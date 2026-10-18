/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.makemon.c - version 1.0.2 */
/* $FreeBSD$ */

/*
 * Monster creation system for 1984 Hack - spawning and monster management
 * Original 1984 source: docs/historical/original-source/hack.makemon.c
 *
 * Key modernizations: ANSI C function signatures
 */

#include "hack.h"
extern char fut_geno[];
/* MODERN: removed bogus extern for index() - provided by string.h/compat.h */
extern struct obj *mkobj_at(int let, int x, int y);

/* Function prototypes for missing functions */
extern int d(int n, int x);
extern int newcham(struct monst *mtmp, struct permonst *mdat);
extern int getwn(struct monst *mtmp);
extern void initworm(struct monst *mtmp);

/* Forward declarations for functions defined in this file */
static int goodpos(int x, int y);
void rloc(struct monst *mtmp);
coord enexto(xchar xx, xchar yy);

struct monst zeromonst;

/*
 * called with [x,y] = coordinates;
 *	[0,0] means anyplace
 *	[u.ux,u.uy] means: call mnexto (if !in_mklev)
 *
 *	In case we make an Orc or killer bee, we make an entire horde (swarm);
 *	note that in this case we return only one of them (the one at [x,y]).
 */
struct monst *makemon(struct permonst *ptr, int x, int y) {
  struct monst *mtmp;
  int tmp, ct;
  boolean anything = (!ptr);
  extern boolean in_mklev;

  if (x != 0 || y != 0)
    if (m_at(x, y))
      return ((struct monst *)0);
  if (ptr) {
    if (index(fut_geno, ptr->mlet))
      return ((struct monst *)0);
  } else {
    ct = CMNUM - (int)strlen(fut_geno); /* MODERN: cast strlen to int */
    if (index(fut_geno, 'm'))
      ct++; /* make only 1 minotaur */
    if (index(fut_geno, '@'))
      ct++;
    if (ct <= 0)
      return (0); /* no more monsters! */
    tmp = rn2(ct * dlevel / 24 + 7);
    if (tmp < dlevel - 4)
      tmp = rn2(ct * dlevel / 24 + 12);
    if (tmp >= ct)
      tmp = rn1(ct - ct / 2, ct / 2);
    for (ct = 0; ct < CMNUM; ct++) {
      if (ct >= CMNUM)
        break; /* MODERN: bounds check prevents OOB access to mons[] array */
      ptr = &mons[ct];
      if (index(fut_geno, ptr->mlet))
        continue;
      if (!tmp--)
        goto gotmon;
    }
    panic("makemon?");
  }
gotmon:
  mtmp = newmonst(ptr->pxlth);
  *mtmp = zeromonst; /* clear all entries in structure */
  for (ct = 0; ct < (int)ptr->pxlth;
       ct++) /* MODERN: Cast pxlth to int for loop comparison */
    ((char *)&(mtmp->mextra[0]))[ct] = 0;
  mtmp->nmon = fmon;
  fmon = mtmp;
  mtmp->m_id = flags.ident++;
  mtmp->data = ptr;
  mtmp->mxlth = ptr->pxlth;
  if (ptr->mlet == 'D')
    mtmp->mhpmax = mtmp->mhp = 80;
  else if (!ptr->mlevel)
    mtmp->mhpmax = mtmp->mhp = (schar)rnd(4); /* MODERN: cast to schar */
  else
    mtmp->mhpmax = mtmp->mhp = (schar)d(ptr->mlevel, 8); /* MODERN: cast to schar */
  mtmp->mx = (xchar)x; /* MODERN: cast to xchar */
  mtmp->my = (xchar)y; /* MODERN: cast to xchar */
  mtmp->mcansee = 1;
  if (ptr->mlet == 'M') {
    mtmp->mimic = 1;
    mtmp->mappearance = ']';
  }
  if (!in_mklev) {
    if (x == u.ux && y == u.uy && ptr->mlet != ' ')
      mnexto(mtmp);
    if (x == 0 && y == 0)
      rloc(mtmp);
  }
  if (ptr->mlet == 's' || ptr->mlet == 'S') {
    mtmp->mhide = mtmp->mundetected = 1;
    if (in_mklev)
      if (mtmp->mx && mtmp->my)
        (void)mkobj_at(0, mtmp->mx, mtmp->my);
  }
  if (ptr->mlet == ':') {
    mtmp->cham = 1;
    {
      int idx = dlevel + 14 + rn2(CMNUM - 14 - dlevel);
      if (idx >= 0 && idx < CMNUM) /* MODERN: bounds check prevents OOB access
                                      to mons[] array */
        (void)newcham(mtmp, &mons[idx]);
      else
        (void)newcham(mtmp, &mons[rn2(CMNUM)]); /* safe fallback */
    }
  }
  if (ptr->mlet == 'I' || ptr->mlet == ';')
    mtmp->minvis = 1;
  if (ptr->mlet == 'L' || ptr->mlet == 'N' ||
      (in_mklev && index("&w;", ptr->mlet) && rn2(5)))
    mtmp->msleep = 1;

#ifndef NOWORM
  if (ptr->mlet == 'w' && getwn(mtmp))
    initworm(mtmp);
#endif /* NOWORM */

  if (anything)
    if (ptr->mlet == 'O' || ptr->mlet == 'k') {
      coord mm;
      int cnt = rnd(10);
      mm.x = (unsigned char)x; /* MODERN: cast to coord field type */
      mm.y = (unsigned char)y; /* MODERN: cast to coord field type */
      while (cnt--) {
        mm = enexto((xchar)mm.x, (xchar)mm.y); /* MODERN: cast to xchar for function */
        (void)makemon(ptr, mm.x, mm.y);
      }
    }

  return (mtmp);
}

coord enexto(xchar xx, xchar yy) {
  int x, y; /* MODERN: int, not xchar: range grows and xchar can wrap */
  coord foo[15], *tfoo;
  int range;

  tfoo = foo;
  range = 1;
  do { /* full kludge action. */
    for (x = xx - range; x <= xx + range; x++)
      if (goodpos(x, yy - range)) {
        tfoo->x = (unsigned char)x; /* MODERN: cast to coord field type */
        tfoo++->y = (unsigned char)(yy - range); /* MODERN: cast to coord field type */
        if (tfoo == &foo[15])
          goto foofull;
      }
    for (x = xx - range; x <= xx + range; x++)
      if (goodpos(x, yy + range)) {
        tfoo->x = (unsigned char)x; /* MODERN: cast to coord field type */
        tfoo++->y = (unsigned char)(yy + range); /* MODERN: cast to coord field type */
        if (tfoo == &foo[15])
          goto foofull;
      }
    for (y = yy + 1 - range; y < yy + range; y++)
      if (goodpos(xx - range, y)) {
        tfoo->x = (unsigned char)(xx - range); /* MODERN: cast to coord field type */
        tfoo++->y = (unsigned char)y; /* MODERN: cast to coord field type */
        if (tfoo == &foo[15])
          goto foofull;
      }
    for (y = yy + 1 - range; y < yy + range; y++)
      if (goodpos(xx + range, y)) {
        tfoo->x = (unsigned char)(xx + range); /* MODERN: cast to coord field type */
        tfoo++->y = (unsigned char)y; /* MODERN: cast to coord field type */
        if (tfoo == &foo[15])
          goto foofull;
      }
    range++;
  } while (tfoo == foo);
foofull:
  return (foo[rn2((int)(tfoo - foo))]); /* MODERN: cast pointer diff to int */
}

int goodpos(int x, int y) /* used only in mnexto and rloc */
{
  return (!(x < 1 || x > COLNO - 2 || y < 1 || y > ROWNO - 2 || m_at(x, y) ||
            !ACCESSIBLE(levl[x][y].typ) || (x == u.ux && y == u.uy) ||
            sobj_at(ENORMOUS_ROCK, x, y)));
}

void rloc(struct monst *mtmp) {
  int tx, ty;
  char ch = mtmp->data->mlet;

#ifndef NOWORM
  if (ch == 'w' && mtmp->mx)
    return; /* do not relocate worms */
#endif      /* NOWORM */
  do {
    tx = rn1(COLNO - 3, 2);
    ty = rn2(ROWNO);
  } while (!goodpos(tx, ty));
  mtmp->mx = (xchar)tx; /* MODERN: cast to xchar */
  mtmp->my = (xchar)ty; /* MODERN: cast to xchar */
  if (u.ustuck == mtmp) {
    if (u.uswallow) {
      u.ux = (xchar)tx; /* MODERN: cast to xchar */
      u.uy = (xchar)ty; /* MODERN: cast to xchar */
      docrt();
    } else
      u.ustuck = 0;
  }
  pmon(mtmp);
}

struct monst *mkmon_at(char let, int x, int y) {
  int ct;
  struct permonst *ptr;

  for (ct = 0; ct < CMNUM; ct++) {
    if (ct >= CMNUM)
      break; /* MODERN: bounds check prevents OOB access to mons[] array */
    ptr = &mons[ct];
    if (ptr->mlet == let)
      return (makemon(ptr, x, y));
  }
  return (0);
}
