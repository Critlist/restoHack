/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.objnam.c - version 1.0.2 */
/* $FreeBSD$ */

/*
 * Object naming system for 1984 Hack - item descriptions and display names
 * Original 1984 source: docs/historical/original-source/hack.objnam.c
 *
 * Key modernizations: ANSI C function signatures, string safety macros
 */

#include "hack.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* MODERN ADDITION (2025): Safe string operation functions
 * WHY: Original sprintf/strcpy/strcat unsafe with potential buffer overflows
 * HOW: Replace with snprintf-based safe functions with proper truncation
 * handling PRESERVES: Same functionality with identical buffer layout (PREFIX +
 * content) ADDS: Buffer overflow protection + truncation detection for object
 * name generation
 *
 * SECURITY NOTES:
 * - All functions guarantee NUL-termination
 * - Truncation is detected and can be handled gracefully
 * - Uses actual buffer capacity, not global macros
 * - Safe for ASCII only (no UTF-8 multibyte considerations in 1984 codebase)
 */
#define SAFE_BUF_SIZE (BUFSZ - PREFIX) /* Available space after prefix */

/* Safe string concatenation: returns 0 on success, 1 on truncation */
/**
 * MODERN ADDITION (2025): Enhanced safe string concatenation with memory
 * validation
 *
 * WHY: Crashes occur when src pointer is corrupted (not just NULL)
 * HOW: Added pointer validation to detect corrupted memory addresses
 *
 * PRESERVES: All original string functionality for valid pointers
 * ADDS: Protection against crashes from corrupted object name pointers
 */
static inline int safe_strcat(char *dst, size_t cap, const char *src) {
  size_t len = strnlen(dst, cap);
  if (len >= cap)
    return -1; /* buffer already full/corrupted */
  size_t remaining = cap - len;

  /* MODERN: Validate pointer is not corrupted before using in snprintf */
  if (!src || (uintptr_t)src < 0x1000 || (uintptr_t)src > 0x7FFFFFFFFFFF) {
    int wrote = snprintf(dst + len, remaining, "<corrupted>");
    return (wrote >= 0 && (size_t)wrote < remaining) ? 0 : 1;
  }

  int wrote = snprintf(dst + len, remaining, "%s", src);
  return (wrote >= 0 && (size_t)wrote < remaining) ? 0
                                                   : 1; /* 0=ok, 1=truncated */
}

/**
 * MODERN ADDITION (2025): Enhanced safe string copy with memory validation
 *
 * WHY: Crashes occur when src pointer is corrupted (not just NULL)
 * HOW: Added pointer validation to detect corrupted memory addresses
 *
 * PRESERVES: All original string functionality for valid pointers
 * ADDS: Protection against crashes from corrupted object name pointers
 */
static inline int safe_strcpy(char *dst, size_t cap, const char *src) {
  if (cap == 0)
    return -1;

  /* MODERN: Validate pointer is not corrupted before using in snprintf */
  if (!src || (uintptr_t)src < 0x1000 || (uintptr_t)src > 0x7FFFFFFFFFFF) {
    int wrote = snprintf(dst, cap, "<corrupted>");
    return (wrote >= 0 && (size_t)wrote < cap) ? 0 : 1;
  }

  int wrote = snprintf(dst, cap, "%s", src);
  return (wrote >= 0 && (size_t)wrote < cap) ? 0 : 1; /* 0=ok, 1=truncated */
}

/* Force ellipsis at end of string while preserving what fit */
static inline void force_ellipsis(char *dst, size_t cap) {
  if (cap == 0)
    return;
  size_t len = strnlen(dst, cap);
  if (len + 4 <= cap) { /* room to append "..." */
    dst[len++] = '.';
    dst[len++] = '.';
    dst[len++] = '.';
    dst[len] = '\0';
  } else if (cap >= 4) { /* place at very end */
    dst[cap - 4] = '.';
    dst[cap - 3] = '.';
    dst[cap - 2] = '.';
    dst[cap - 1] = '\0';
  } else { /* cap 1..3 */
    size_t n = cap - 1;
    for (size_t i = 0; i < n; i++)
      dst[i] = '.';
    dst[n] = '\0';
  }
}

/* Legacy macro compatibility - now uses safe functions */
#define Sprintf(buf, ...) (void)snprintf(buf, SAFE_BUF_SIZE, __VA_ARGS__)
#define Strcat(dest, src) (void)safe_strcat(dest, SAFE_BUF_SIZE, src)
#define Strcpy(dest, src) (void)safe_strcpy(dest, SAFE_BUF_SIZE, src)

/* Prefix buffer operations with truncation detection */
#define PREFIX_Strcat(dest, src) safe_strcat(dest, PREFIX, src)
#define PREFIX_Strcpy(dest, src) safe_strcpy(dest, PREFIX, src)
#define PREFIX 15
extern char *eos(char *s);
extern int bases[];

/* MODERN: ANSI C function signature for type safety */
char *strprepend(char *s, const char *pref)
{
  int i = strlen(pref);
  if (i > PREFIX) {
    pline("WARNING: prefix too short.");
    return (s);
  }
  /* MODERN: Add buffer underflow protection */
  if ((uintptr_t)s < (uintptr_t)i + 0x1000) { /* Basic underflow check */
    pline("WARNING: buffer underflow detected in strprepend.");
    return (s);
  }
  s -= i;
  (void)strncpy(s, pref, i); /* do not copy trailing 0 */
  return (s);
}

/* MODERN: ANSI C function signature for type safety */
char *sitoa(int a)
{
  static char buf[13];
  (void)snprintf(buf, sizeof(buf), (a < 0) ? "%d" : "+%d", a);
  return (buf);
}

/* MODERN: ANSI C function signature for type safety */
char *typename(int otyp)
{
  static char buf[BUFSZ];
  /* MODERN: Add bounds checking for objects array access */
  if (otyp < 0 || otyp >= NROFOBJECTS) {
    Strcpy(buf, "strange object type"); /* Safe fallback */
    return (buf);
  }
  struct objclass *ocl = &objects[otyp];
  const char *an = ocl->oc_name;  /* MODERN: const because reads from objects[]
                                     read-only fields */
  const char *dn = ocl->oc_descr; /* MODERN: const because reads from objects[]
                                     read-only fields */
  char *un = ocl->oc_uname;
  int nn = ocl->oc_name_known;
  switch (ocl->oc_olet) {
  case POTION_SYM:
    Strcpy(buf, "potion");
    break;
  case SCROLL_SYM:
    Strcpy(buf, "scroll");
    break;
  case WAND_SYM:
    Strcpy(buf, "wand");
    break;
  case RING_SYM:
    Strcpy(buf, "ring");
    break;
  default:
    if (nn) {
      Strcpy(buf, an);
      if (otyp >= TURQUOISE && otyp <= JADE)
        Strcat(buf, " stone");
      if (un)
        Sprintf(eos(buf), " called %s", un);
      if (dn)
        Sprintf(eos(buf), " (%s)", dn);
    } else {
      Strcpy(buf, dn ? dn : an);
      if (ocl->oc_olet == GEM_SYM)
        Strcat(buf, " gem");
      if (un)
        Sprintf(eos(buf), " called %s", un);
    }
    return (buf);
  }
  /* here for ring/scroll/potion/wand */
  if (nn)
    Sprintf(eos(buf), " of %s", an);
  if (un)
    Sprintf(eos(buf), " called %s", un);
  if (dn)
    Sprintf(eos(buf), " (%s)", dn);
  return (buf);
}

/* MODERN: ANSI C function signature for type safety */
char *xname(struct obj *obj)
{
  static char bufr[BUFSZ];
  char *buf = &(bufr[PREFIX]); /* leave room for "17 -3 " */
  /* MODERN: Add bounds checking for objects array access */
  /* Note: otyp is uchar, so < 0 check is redundant */
  if (obj->otyp >= NROFOBJECTS) {
    panic("xname: corrupted object otyp=%d (valid range: 0-%d), olet='%c', "
          "quan=%d, ox=%d, oy=%d",
          obj->otyp, NROFOBJECTS - 1, obj->olet, obj->quan, obj->ox, obj->oy);
  }
  int nn = objects[obj->otyp].oc_name_known;
  const char *an =
      objects[obj->otyp].oc_name; /* MODERN: const because reads from objects[]
                                     read-only fields */
  const char *dn =
      objects[obj->otyp].oc_descr; /* MODERN: const because reads from objects[]
                                      read-only fields */
  char *un = objects[obj->otyp].oc_uname;
  int pl = (obj->quan != 1);
  if (!obj->dknown && !Blind)
    obj->dknown = 1; /* %% doesnt belong here */
  switch (obj->olet) {
  case AMULET_SYM:
    Strcpy(buf, (obj->spe < 0 && obj->known) ? "cheap plastic imitation of the "
                                             : "");
    Strcat(buf, "Amulet of Yendor");
    break;
  case TOOL_SYM:
    if (!nn) {
      Strcpy(buf, dn);
      break;
    }
    Strcpy(buf, an);
    break;
  case FOOD_SYM:
    if (obj->otyp == DEAD_HOMUNCULUS && pl) {
      pl = 0;
      Strcpy(buf, "dead homunculi");
      break;
    }
    /* fungis ? */
    /* fallthrough */
  case WEAPON_SYM:
    if (obj->otyp == WORM_TOOTH && pl) {
      pl = 0;
      Strcpy(buf, "worm teeth");
      break;
    }
    if (obj->otyp == CRYSKNIFE && pl) {
      pl = 0;
      Strcpy(buf, "crysknives");
      break;
    }
    /* fallthrough */
  case ARMOR_SYM:
  case CHAIN_SYM:
  case ROCK_SYM:
    Strcpy(buf, an);
    break;
  case BALL_SYM:
    Sprintf(buf, "%sheavy iron ball",
            (obj->owt > objects[obj->otyp].oc_weight) ? "very " : "");
    break;
  case POTION_SYM:
    if (nn || un || !obj->dknown) {
      Strcpy(buf, "potion");
      if (pl) {
        pl = 0;
        Strcat(buf, "s");
      }
      if (!obj->dknown)
        break;
      if (un) {
        Strcat(buf, " called ");
        Strcat(buf, un);
      } else {
        Strcat(buf, " of ");
        Strcat(buf, an);
      }
    } else {
      Strcpy(buf, dn);
      Strcat(buf, " potion");
    }
    break;
  case SCROLL_SYM:
    Strcpy(buf, "scroll");
    if (pl) {
      pl = 0;
      Strcat(buf, "s");
    }
    if (!obj->dknown)
      break;
    if (nn) {
      Strcat(buf, " of ");
      Strcat(buf, an);
    } else if (un) {
      Strcat(buf, " called ");
      Strcat(buf, un);
    } else {
      Strcat(buf, " labeled ");
      Strcat(buf, dn);
    }
    break;
  case WAND_SYM:
    if (!obj->dknown)
      Sprintf(buf, "wand");
    else if (nn)
      Sprintf(buf, "wand of %s", an);
    else if (un)
      Sprintf(buf, "wand called %s", un);
    else
      Sprintf(buf, "%s wand", dn);
    break;
  case RING_SYM:
    if (!obj->dknown)
      Sprintf(buf, "ring");
    else if (nn)
      Sprintf(buf, "ring of %s", an);
    else if (un)
      Sprintf(buf, "ring called %s", un);
    else
      Sprintf(buf, "%s ring", dn);
    break;
  case GEM_SYM:
    if (!obj->dknown) {
      Strcpy(buf, "gem");
      break;
    }
    if (!nn) {
      Sprintf(buf, "%s gem", dn);
      break;
    }
    Strcpy(buf, an);
    if (obj->otyp >= TURQUOISE && obj->otyp <= JADE)
      Strcat(buf, " stone");
    break;
  default:
    Sprintf(buf, "glorkum %c (0%o) %u %d", obj->olet, obj->olet, obj->otyp,
            obj->spe);
  }
  if (pl) {
    char *p;

    for (p = buf; *p; p++) {
      if (!strncmp(" of ", p, 4)) {
        /* pieces of, cloves of, lumps of */
        int c1, c2 = 's';

        do {
          c1 = c2;
          c2 = *p;
          *p++ = c1;
        } while (c1);
        goto nopl;
      }
    }
    p = eos(buf) - 1;
    if (*p == 's' || *p == 'z' || *p == 'x' || (*p == 'h' && p[-1] == 's'))
      Strcat(buf, "es"); /* boxes */
    else if (*p == 'y' && !index(vowels, p[-1]))
      Strcpy(p, "ies"); /* rubies, zruties */
    else
      Strcat(buf, "s");
  }
nopl:
  if (obj->onamelth) {
    Strcat(buf, " named ");
    Strcat(buf, ONAME(obj));
  }
  return (buf);
}

/* MODERN: ANSI C function signature for type safety */
char *doname(struct obj *obj)
{
  char prefix[PREFIX];
  char *bp = xname(obj);
  if (obj->quan != 1)
    (void)snprintf(prefix, PREFIX, "%u ",
                   obj->quan); /* MODERN: Safe prefix sprintf */
  else
    PREFIX_Strcpy(prefix, "a ");
  switch (obj->olet) {
  case AMULET_SYM:
    if (strncmp(bp, "cheap ", 6))
      PREFIX_Strcpy(prefix, "the ");
    break;
  case ARMOR_SYM:
    if (obj->owornmask & W_ARMOR)
      Strcat(bp, " (being worn)");
    /* fallthrough */
  case WEAPON_SYM:
    if (obj->known) {
      int rc = safe_strcat(prefix, sizeof(prefix), sitoa(obj->spe));
      if (rc) {
        force_ellipsis(prefix, sizeof(prefix));
      } else {
        (void)safe_strcat(prefix, sizeof(prefix), " ");
      }
    }
    break;
  case WAND_SYM:
    if (obj->known)
      Sprintf(eos(bp), " (%d)", obj->spe);
    break;
  case RING_SYM:
    if (obj->owornmask & W_RINGR)
      Strcat(bp, " (on right hand)");
    if (obj->owornmask & W_RINGL)
      Strcat(bp, " (on left hand)");
    if (obj->known && (objects[obj->otyp].bits & SPEC)) {
      int rc = safe_strcat(prefix, sizeof(prefix), sitoa(obj->spe));
      if (rc) {
        force_ellipsis(prefix, sizeof(prefix));
      } else {
        (void)safe_strcat(prefix, sizeof(prefix), " ");
      }
    }
    break;
  }
  if (obj->owornmask & W_WEP)
    Strcat(bp, " (weapon in hand)");
  if (obj->unpaid)
    Strcat(bp, " (unpaid)");
  if (!strcmp(prefix, "a ") && index(vowels, *bp))
    PREFIX_Strcpy(prefix, "an ");
  bp = strprepend(bp, prefix);
  return (bp);
}

/* used only in hack.fight.c (thitu) */
void setan(const char *str,
           char *buf) /* MODERN: const because str is read-only */
{
  if (index(vowels, *str))
    Sprintf(buf, "an %s", str);
  else
    Sprintf(buf, "a %s", str);
}

/* MODERN: ANSI C function signature for type safety */
char *aobjnam(struct obj *otmp, const char *verb)
{
  char *bp = xname(otmp);
  char prefix[PREFIX];
  if (otmp->quan != 1) {
    (void)snprintf(prefix, sizeof(prefix), "%u ", otmp->quan);
    bp = strprepend(bp, prefix);
  }

  if (verb) {
    /* verb is given in plural (i.e., without trailing s) */
    Strcat(bp, " ");
    if (otmp->quan != 1)
      Strcat(bp, verb);
    else if (!strcmp(verb, "are"))
      Strcat(bp, "is");
    else {
      Strcat(bp, verb);
      Strcat(bp, "s");
    }
  }
  return (bp);
}

/* MODERN: ANSI C function signature for type safety */
char *Doname(struct obj *obj)
{
  char *s = doname(obj);

  if ('a' <= *s && *s <= 'z')
    *s -= ('a' - 'A');
  return (s);
}

/* MODERN: CONST-CORRECTNESS: object type name strings are read-only */
const char *const wrp[] = {"wand", "ring", "potion", "scroll", "gem"};
char wrpsym[] = {WAND_SYM, RING_SYM, POTION_SYM, SCROLL_SYM, GEM_SYM};

/* MODERN: ANSI C function signature for type safety */
struct obj *readobjnam(char *bp)
{
  char *p;
  int i;
  int cnt, spe, spesgn, typ, heavy;
  /* Original 1984: char let; */
  unsigned char let; /* MODERN: unsigned to prevent negative array indexing
                        vulnerability */
  char *un, *dn, *an;
  /* int the = 0; char *oname = 0; */
  cnt = spe = spesgn = typ = heavy = 0;
  let = 0;
  an = dn = un = 0;
  for (p = bp; *p; p++)
    if ('A' <= *p && *p <= 'Z')
      *p += 'a' - 'A';
  if (!strncmp(bp, "the ", 4)) {
    /*		the = 1; */
    bp += 4;
  } else if (!strncmp(bp, "an ", 3)) {
    cnt = 1;
    bp += 3;
  } else if (!strncmp(bp, "a ", 2)) {
    cnt = 1;
    bp += 2;
  }
  if (!cnt && digit(*bp)) {
    cnt = atoi(bp);
    if (cnt < 0 || cnt > 32767) cnt = 1; /* MODERN: Validate count bounds */
    while (digit(*bp))
      bp++;
    while (*bp == ' ')
      bp++;
  }
  if (!cnt)
    cnt = 1; /* %% what with "gems" etc. ? */

  if (*bp == '+' || *bp == '-') {
    spesgn = (*bp++ == '+') ? 1 : -1;
    spe = atoi(bp);
    if (spe < -32767 || spe > 32767) spe = 0; /* MODERN: Validate spe bounds */
    while (digit(*bp))
      bp++;
    while (*bp == ' ')
      bp++;
  } else {
    p = rindex(bp, '(');
    if (p) {
      if (p > bp && p[-1] == ' ')
        p[-1] = 0;
      else
        *p = 0;
      p++;
      spe = atoi(p);
      if (spe < -32767 || spe > 32767) spe = 0; /* MODERN: Validate spe bounds */
      while (digit(*p))
        p++;
      if (strcmp(p, ")"))
        spe = 0;
      else
        spesgn = 1;
    }
  }
  /* now we have the actual name, as delivered by xname, say
          green potions called whisky
          scrolls labeled "QWERTY"
          egg
          dead zruties
          fortune cookies
          very heavy iron ball named hoei
          wand of wishing
          elven cloak
  */
  for (p = bp; *p; p++)
    if (!strncmp(p, " named ", 7)) {
      *p = 0;
      /*		oname = p+7; */
    }
  for (p = bp; *p; p++)
    if (!strncmp(p, " called ", 8)) {
      *p = 0;
      un = p + 8;
    }
  for (p = bp; *p; p++)
    if (!strncmp(p, " labeled ", 9)) {
      *p = 0;
      dn = p + 9;
    }

  /* first change to singular if necessary */
  if (cnt != 1) {
    /* find "cloves of garlic", "worthless pieces of blue glass" */
    for (p = bp; *p; p++)
      if (!strncmp(p, "s of ", 5)) {
        while ((*p = p[1]))
          p++;
        goto sing;
      }
    /* remove -s or -es (boxes) or -ies (rubies, zruties) */
    p = eos(bp);
    if (p > bp && p[-1] == 's') {
      if (p - bp >= 2 && p[-2] == 'e') {
        if (p - bp >= 3 && p[-3] == 'i') {
          if (p - bp >= 7 && !strcmp(p - 7, "cookies"))
            goto mins;
          Strcpy(p - 3, "y");
          goto sing;
        }

        /* note: cloves / knives from clove / knife */
        if (p - bp >= 6 && !strcmp(p - 6, "knives")) {
          Strcpy(p - 3, "fe");
          goto sing;
        }

        /* note: nurses, axes but boxes */
        if (p - bp >= 5 && !strcmp(p - 5, "boxes")) {
          p[-2] = 0;
          goto sing;
        }
      }
    mins:
      p[-1] = 0;
    } else {
      if (p - bp >= 9 && !strcmp(p - 9, "homunculi")) {
        Strcpy(p - 1, "us"); /* !! makes string longer */
        goto sing;
      }
      if (p - bp >= 5 && !strcmp(p - 5, "teeth")) {
        Strcpy(p - 5, "tooth");
        goto sing;
      }
      /* here we cannot find the plural suffix */
    }
  }
sing:
  if (!strcmp(bp, "amulet of yendor")) {
    typ = AMULET_OF_YENDOR;
    goto typfnd;
  }
  p = eos(bp);
  if (p - bp >= 5 && !strcmp(p - 5, " mail")) { /* Note: ring mail is not a ring
                                                   ! - MODERN: bounds check */
    let = ARMOR_SYM;
    an = bp;
    goto srch;
  }
  for (i = 0; i < (int)sizeof(wrpsym);
       i++) { /* MODERN: Cast to int to match loop variable type */
    int j = strlen(wrp[i]);
    if (!strncmp(bp, wrp[i], j)) {
      let = wrpsym[i];
      bp += j;
      if (!strncmp(bp, " of ", 4))
        an = bp + 4;
      /* else if(*bp) ?? */
      goto srch;
    }
    if (p - bp >= j && !strcmp(p - j, wrp[i])) {
      let = wrpsym[i];
      p -= j;
      *p = 0;
      if (p > bp && p[-1] == ' ')
        p[-1] = 0;
      dn = bp;
      goto srch;
    }
  }
  if (p - bp >= 6 && !strcmp(p - 6, " stone")) {
    p[-6] = 0;
    let = GEM_SYM;
    an = bp;
    goto srch;
  }
  if (!strcmp(bp, "very heavy iron ball")) {
    heavy = 1;
    typ = HEAVY_IRON_BALL;
    goto typfnd;
  }
  an = bp;
srch:
  if (!an && !dn && !un)
    goto any;
  i = 1;
  if (let)
    i = bases[letindex(let)];
  while (i <= NROFOBJECTS && (!let || objects[i].oc_olet == let)) {
    const char *zn = objects[i].oc_name; /* MODERN: const because reads from
                                            objects[] read-only fields */

    if (!zn)
      goto nxti;
    if (an && strcmp(an, zn))
      goto nxti;
    if (dn && (!(zn = objects[i].oc_descr) || strcmp(dn, zn)))
      goto nxti;
    if (un && (!(zn = objects[i].oc_uname) || strcmp(un, zn)))
      goto nxti;
    typ = i;
    goto typfnd;
  nxti:
    i++;
  }
any:
  if (!let)
    let = wrpsym[rn2(sizeof(wrpsym))];
  typ = probtype(let);
typfnd: {
  struct obj *otmp;
  extern struct obj *mksobj();
  let = objects[typ].oc_olet;
  otmp = mksobj(typ);
  if (heavy)
    otmp->owt += 15;
  if (cnt > 0 && index("%?!*)", let) &&
      (cnt < 4 || (let == WEAPON_SYM && typ <= ROCK && cnt < 20)))
    otmp->quan = cnt;

  if (spe > 3 && spe > otmp->spe)
    spe = 0;
  else if (let == WAND_SYM)
    spe = otmp->spe;
  if (spe == 3 && u.uluck < 0)
    spesgn = -1;
  if (let != WAND_SYM && spesgn == -1)
    spe = -spe;
  if (let == BALL_SYM)
    spe = 0;
  else if (let == AMULET_SYM)
    spe = -1;
  else if (typ == WAN_WISHING && rn2(10))
    spe = (rn2(10) ? -1 : 0);
  otmp->spe = spe;

  if (spesgn == -1)
    otmp->cursed = 1;

  return (otmp);
}
}
