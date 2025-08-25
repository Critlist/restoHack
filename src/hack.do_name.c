/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.do_name.c - version 1.0.3 */
/* $FreeBSD$ */

/*
 * Object naming system for 1984 Hack - item identification and custom names
 * Original 1984 source: docs/historical/original-source/hack.do_name.c
 *
 * Key modernizations: ANSI C function signatures
 */

#include "hack.h"
#include <stdio.h>
extern char plname[];

coord
/**
 * MODERN ADDITION (2025): const-qualified goal parameter
 * 
 * WHY: goal parameter is never modified, only displayed to user
 * HOW: Changed char *goal to const char *goal for const-correctness
 * 
 * PRESERVES: All original positioning functionality
 * ADDS: Type safety for string literals passed as goal
 */
getpos(int force, const char *goal) {
  int cx, cy, i, c;
  extern char sdir[];           /* defined in hack.c */
  extern schar xdir[], ydir[];  /* idem */
  extern char *visctrl(char c); /* see below */
  coord cc;
  pline("(For instructions type a ?)");
  cx = u.ux;
  cy = u.uy;
  curs(cx, cy + 2);
  while ((c = readchar()) != '.') {
    for (i = 0; i < 8; i++)
      if (sdir[i] == c) {
        if (1 <= cx + xdir[i] && cx + xdir[i] <= COLNO)
          cx += xdir[i];
        if (0 <= cy + ydir[i] && cy + ydir[i] <= ROWNO - 1)
          cy += ydir[i];
        goto nxtc;
      }
    if (c == '?') {
      pline("Use [hjkl] to move the cursor to %s.", goal);
      pline("Type a . when you are at the right place.");
    } else {
      pline("Unknown direction: '%s' (%s).", visctrl(c),
            force ? "use hjkl or ." : "aborted");
      if (force)
        goto nxtc;
      cc.x = -1;
      cc.y = 0;
      return (cc);
    }
  nxtc:;
    curs(cx, cy + 2);
  }
  cc.x = cx;
  cc.y = cy;
  return (cc);
}

int do_mname(void) {
  char buf[BUFSZ];
  coord cc;
  int cx, cy, lth;
  unsigned
      i; /* MODERN: Match mxlth unsigned type to avoid sign-compare warning */
  struct monst *mtmp, *mtmp2;
  extern char *lmonnam();
  cc = getpos(0, "the monster you want to name");
  cx = cc.x;
  cy = cc.y;
  if (cx < 0)
    return (0);
  mtmp = m_at(cx, cy);
  if (!mtmp) {
    if (cx == u.ux && cy == u.uy)
      pline("This ugly monster is called %s and cannot be renamed.", plname);
    else
      pline("There is no monster there.");
    return (1);
  }
  if (mtmp->mimic) {
    pline("I see no monster there.");
    return (1);
  }
  if (!cansee(cx, cy)) {
    pline("I cannot see a monster there.");
    return (1);
  }
  pline("What do you want to call %s? ", lmonnam(mtmp));
  getlin(buf);
  clrlin();
  if (!*buf || *buf == '\033')
    return (1);
  lth = strlen(buf) + 1;
  if (lth > 63) {
    buf[62] = 0; /* MODERN: Safe bounds checking for 62-char monster names */
    lth = 63;
  }
  mtmp2 = newmonst(mtmp->mxlth + lth);
  *mtmp2 = *mtmp;
  for (i = 0; i < mtmp->mxlth; i++)
    ((char *)mtmp2->mextra)[i] = ((char *)mtmp->mextra)[i];
  mtmp2->mnamelth = lth;
  /* MODERN: Critical fix - mxlth stays the same so NAME() macro works correctly */

  /**
   * MODERN ADDITION (2025): Bounds checking for NAME() macro access
   *
   * WHY: NAME(mtmp) macro uses pointer arithmetic with mxlth offset - if
   * corrupted could write to arbitrary memory HOW: Validate mxlth is within
   * reasonable bounds before pointer arithmetic
   *
   * PRESERVES: All original monster naming functionality for valid cases
   * ADDS: Protection against memory corruption from invalid mxlth values
   */
  if (mtmp2->mxlth >
      1024) { /* MODERN: Sanity check - mxlth should never be this large */
    impossible("monster mxlth corruption detected: %u", mtmp2->mxlth, 0);
    monfree(mtmp2);
    return (1);
  }
  (void)strcpy(NAME(mtmp2), buf);
  replmon(mtmp, mtmp2);
  return (1);
}

/*
 * This routine changes the address of  obj . Be careful not to call it
 * when there might be pointers around in unknown places. For now: only
 * when  obj  is in the inventory.
 */
void do_oname(struct obj *obj) {
  struct obj *otmp, *otmp2;
  int lth;
  char buf[BUFSZ];
  pline("What do you want to name %s? ", doname(obj));
  getlin(buf);
  clrlin();
  if (!*buf || *buf == '\033')
    return;
  lth = strlen(buf) + 1;
  if (lth > 63) {
    buf[62] = 0; /* MODERN: Safe bounds checking for 62-char object names */
    lth = 63;
  }
  otmp2 = newobj(lth);
  *otmp2 = *obj;
  otmp2->onamelth = lth;
  (void)strcpy(ONAME(otmp2), buf);

  setworn((struct obj *)0, obj->owornmask);
  setworn(otmp2, otmp2->owornmask);

  /* do freeinv(obj); etc. by hand in order to preserve
     the position of this object in the inventory */
  if (obj == invent)
    invent = otmp2;
  else
    for (otmp = invent;; otmp = otmp->nobj) {
      if (!otmp)
        panic("Do_oname: cannot find obj.");
      if (otmp->nobj == obj) {
        otmp->nobj = otmp2;
        break;
      }
    }
  /*obfree(obj, otmp2);*/ /* now unnecessary: no pointers on bill */
  free((char *)obj);      /* let us hope nobody else saved a pointer */
}

int ddocall(void) {
  struct obj *obj;

  pline("Do you want to name an individual object? [ny] ");
  switch (readchar()) {
  case '\033':
    break;
  case 'y':
    obj = getobj("#", "name");
    if (obj)
      do_oname(obj);
    break;
  default:
    obj = getobj("?!=/", "call");
    if (obj)
      docall(obj);
  }
  return (0);
}

void docall(struct obj *obj) {
  char buf[BUFSZ];
  struct obj otemp;
  char **str1;
  extern char *xname();
  char *str;

  otemp = *obj;
  otemp.quan = 1;
  otemp.onamelth = 0;
  str = xname(&otemp);
  pline("Call %s %s: ", index(vowels, *str) ? "an" : "a", str);
  getlin(buf);
  clrlin();
  if (!*buf || *buf == '\033')
    return;
  str = newstring(strlen(buf) + 1);
  (void)strcpy(str, buf);
  str1 = &(SAFE_OBJECTS(obj->otyp).oc_uname); /* MODERN: Bounds-checked object access */
  if (*str1)
    free(*str1);
  *str1 = str;
}

const char *const ghostnames[] = {/* these names should have length < PL_NSIZ */
                                  "adri",   "andries", "andreas", "bert",
                                  "david",  "dirk",    "emile",   "frans",
                                  "fred",   "greg",    "hether",  "jay",
                                  "john",   "jon",     "kay",     "kenny",
                                  "maud",   "michiel", "mike",    "peter",
                                  "robert", "ron",     "tom",     "wilmar"};

char *xmonnam(struct monst *mtmp, int vb) {
  static char buf[BUFSZ]; /* MODERN: Static buffer reuse issue - overwrites on each call
                             but needed for 1984 compatibility with callers expecting persistent string */
  extern char *shkname();
  if (mtmp->mnamelth && !vb) {
    (void)strncpy(buf, NAME(mtmp), BUFSZ - 1); /* MODERN: Safe copy */
    buf[BUFSZ - 1] = 0;
    return (buf);
  }
  switch (mtmp->data->mlet) {
  case ' ': {
    const char *gn =
        (char *)
            mtmp->mextra; /* MODERN: const qualifier to match ghostnames[] */
    if (!*gn) {           /* might also look in scorefile */
      gn = ghostnames[rn2(SIZE(ghostnames))];
      if (!rn2(2)) {
        /* MODERN: Safe copy with bounds check for ghost names */
        const char *name = !rn2(5) ? plname : gn;
        (void)strncpy((char *)mtmp->mextra, name, PL_NSIZ - 1);
        ((char *)mtmp->mextra)[PL_NSIZ - 1] = '\0';
      }
    }
    (void)snprintf(buf, BUFSZ, "%s's ghost",
                   gn); /* MODERN: Safe sprintf replacement - identical output,
                           prevents overflow */
  } break;
  case '@':
    if (mtmp->isshk) {
      (void)strcpy(buf, shkname(mtmp));
      break;
    }
    /* FALLTHROUGH */
  default:
    (void)snprintf(buf, BUFSZ,
                   "the %s%s", /* MODERN: Safe sprintf replacement - identical
                                  output, prevents overflow */
                   mtmp->minvis ? "invisible " : "", mtmp->data->mname);
  }
  if (vb && mtmp->mnamelth) {
    /* MODERN: Safe concatenation with bounds checking */
    size_t buflen = strlen(buf);
    if (buflen + 8 < BUFSZ) { /* " called " = 8 chars */
      (void)strcat(buf, " called ");
      buflen += 8;
      if (buflen < BUFSZ - 1) {
        (void)strncat(buf, NAME(mtmp), BUFSZ - 1 - buflen);
        buf[BUFSZ - 1] = '\0';
      }
    }
  }
  return (buf);
}

char *lmonnam(struct monst *mtmp) { return (xmonnam(mtmp, 1)); }
char *monnam(struct monst *mtmp) { return (xmonnam(mtmp, 0)); }

char *Monnam(struct monst *mtmp) {
  char *bp = monnam(mtmp);
  if ('a' <= *bp && *bp <= 'z')
    *bp += ('A' - 'a');
  return (bp);
}

char *amonnam(struct monst *mtmp, const char *adj) {
  char *bp = monnam(mtmp);
  static char buf[BUFSZ + 64]; /* MODERN: Extra space for adj + static buffer reuse issue
                                  but needed for 1984 compatibility with callers expecting persistent string */

  if (!strncmp(bp, "the ", 4))
    bp += 4;
  (void)snprintf(buf, sizeof(buf), "the %s %s", adj,
                 bp); /* MODERN: Safe sprintf with correct buffer size */
  return (buf);
}

char *Amonnam(struct monst *mtmp, const char *adj) {
  char *bp = amonnam(mtmp, adj);

  *bp = 'T';
  return (bp);
}

char *Xmonnam(struct monst *mtmp) {
  char *bp = Monnam(mtmp);
  if (!strncmp(bp, "The ", 4)) {
    bp += 2;
    *bp = 'A';
  }
  return (bp);
}

char *visctrl(char c) {
  static char ccc[3];
  if (c < 040) {
    ccc[0] = '^';
    ccc[1] = c + 0100;
    ccc[2] = 0;
  } else {
    ccc[0] = c;
    ccc[1] = 0;
  }
  return (ccc);
}
