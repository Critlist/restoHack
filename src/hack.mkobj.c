/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.mkobj.c - version 1.0.3 */
/* $FreeBSD$ */

/*
 * Object creation system for 1984 Hack - item spawning and generation
 * Original 1984 source: docs/historical/original-source/hack.mkobj.c
 *
 * Key modernizations: ANSI C function signatures
 */

#include "hack.h"

char mkobjstr[] = "))[[!!!!????%%%%/=**))[[!!!!????%%%%/=**(%";

struct obj *mkobj_at(int let, int x, int y) {
  struct obj *otmp = mkobj(let);
  otmp->ox = (xchar)x; /* MODERN: explicit cast to xchar for coordinate */
  otmp->oy = (xchar)y; /* MODERN: explicit cast to xchar for coordinate */
  otmp->nobj = fobj;
  fobj = otmp;
  return (otmp);
}

struct obj *mksobj_at(int otyp, int x, int y) {
  struct obj *otmp = mksobj(otyp);
  otmp->ox = (xchar)x; /* MODERN: explicit cast to xchar for coordinate */
  otmp->oy = (xchar)y; /* MODERN: explicit cast to xchar for coordinate */
  otmp->nobj = fobj;
  fobj = otmp;
  return (otmp);
}

struct obj *mkobj(int let) {
  if (!let)
    let = mkobjstr[rn2(sizeof(mkobjstr) - 1)];
  return (mksobj(
      letter(let)
          ? CORPSE + ((let > 'Z') ? (let - 'a' + 'Z' - '@' + 1) : (let - '@'))
          : probtype(let)));
}

struct obj zeroobj;

struct obj *mksobj(int otyp) {
  struct obj *otmp;
  /* MODERN: Add bounds checking for objects array access */
  if (otyp < 0 || otyp >= NROFOBJECTS) {
    impossible("mksobj called with invalid otyp %d", otyp, 0);
    otyp = STRANGE_OBJECT; /* Safe fallback */
  }
  unsigned char let = (unsigned char)
      objects[otyp].oc_olet; /* MODERN: Use unsigned char to prevent negative
                                values in switch */

  otmp = newobj(0);
  *otmp = zeroobj;
  otmp->age = moves;
  otmp->o_id = flags.ident++;
  otmp->quan = 1;
  otmp->olet = (char)let; /* MODERN: explicit cast from unsigned char to char */
  otmp->otyp = (uchar)otyp; /* MODERN: explicit cast to uchar */
  otmp->dknown = index("/=!?*", let) ? 0 : 1;
  switch (let) {
  case WEAPON_SYM:
    otmp->quan = (uchar)((otmp->otyp <= ROCK) ? rn1(6, 6) : 1); /* MODERN: cast to uchar */
    if (!rn2(11))
      otmp->spe = (schar)rnd(3); /* MODERN: cast to schar */
    else if (!rn2(10)) {
      otmp->cursed = 1;
      otmp->spe = (schar)(-rnd(3)); /* MODERN: cast to schar */
    }
    break;
  case FOOD_SYM:
    if (otmp->otyp >= CORPSE)
      break;
#ifdef NOT_YET_IMPLEMENTED
    /* if tins are to be identified, need to adapt doname() etc */
    if (otmp->otyp == TIN)
      otmp->spe = rnd(...);
#endif /* NOT_YET_IMPLEMENTED */
       /* FALLTHROUGH */
  case GEM_SYM:
    otmp->quan = rn2(6) ? 1 : 2;
    /* FALLTHROUGH */
  case TOOL_SYM:
  case CHAIN_SYM:
  case BALL_SYM:
  case ROCK_SYM:
  case POTION_SYM:
  case SCROLL_SYM:
  case AMULET_SYM:
    break;
  case ARMOR_SYM:
    if (!rn2(8))
      otmp->cursed = 1;
    if (!rn2(10))
      otmp->spe = (schar)rnd(3); /* MODERN: cast to schar */
    else if (!rn2(9)) {
      otmp->spe = (schar)(-rnd(3)); /* MODERN: cast to schar */
      otmp->cursed = 1;
    }
    break;
  case WAND_SYM:
    if (otmp->otyp == WAN_WISHING)
      otmp->spe = 3;
    else
      otmp->spe = (schar)rn1(5, (objects[otmp->otyp].bits & NODIR) ? 11 : 4); /* MODERN: cast to schar */
    break;
  case RING_SYM:
    if (objects[otmp->otyp].bits & SPEC) {
      if (!rn2(3)) {
        otmp->cursed = 1;
        otmp->spe = (schar)(-rnd(2)); /* MODERN: cast to schar */
      } else
        otmp->spe = (schar)rnd(2); /* MODERN: cast to schar */
    } else if (otmp->otyp == RIN_TELEPORTATION ||
               otmp->otyp == RIN_AGGRAVATE_MONSTER ||
               otmp->otyp == RIN_HUNGER || !rn2(9))
      otmp->cursed = 1;
    break;
  default:
    panic("impossible mkobj: unknown object class '%c' (0x%02x) for otyp %d",
          (let >= 32 && let < 127) ? let : '?', let,
          otyp); /* MODERN: Better error reporting */
  }
  otmp->owt = (uchar)weight(otmp); /* MODERN: cast weight() result to uchar */
  return (otmp);
}

int letter(int c) {
  return (('@' <= c && c <= 'Z') || ('a' <= c && c <= 'z'));
}

int weight(struct obj *obj) {
  /* MODERN: Add bounds checking for objects array access */
  if (obj->otyp >= NROFOBJECTS) {
    return (obj->quan + 1) / 2; /* Safe fallback weight calculation */
  }
  int wt = objects[obj->otyp].oc_weight;
  return (wt ? wt * obj->quan : (obj->quan + 1) / 2);
}

struct gold *mkgold(long num, int x, int y) {
  struct gold *gold;
  long amount = (num ? num : 1 + (rnd(dlevel + 2) * rnd(30)));

  if ((gold = g_at(x, y)))
    gold->amount += amount;
  else {
    gold = newgold();
    gold->ngold = fgold;
    gold->gx = (xchar)x; /* MODERN: explicit cast to xchar for coordinate */
    gold->gy = (xchar)y; /* MODERN: explicit cast to xchar for coordinate */
    gold->amount = amount;
    fgold = gold;
    /* do sth with display? */
  }
  return gold; /* MODERN: Return gold pointer - was missing return value */
}
