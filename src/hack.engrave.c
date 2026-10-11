/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.engrave.c - version 1.0.3 */
/* $FreeBSD$ */

/*
 * Engraving system for 1984 Hack - floor inscriptions and Elbereth
 * Original 1984 source: docs/historical/original-source/hack.engrave.c
 *
 * Key modernizations: ANSI C function signatures
 */

#include "hack.h"
#include <limits.h>

extern const char
    *nomovemsg; /* MODERN: const because assigned string literals */
extern char nul[];
extern struct obj zeroobj;
struct engr {
  struct engr *nxt_engr;
  char *engr_txt;
  xchar engr_x, engr_y;
  unsigned engr_lth; /* for save & restore; not length of text */
  long engr_time;    /* moment engraving was (will be) finished */
  xchar engr_type;
#define DUST 1
#define ENGRAVE 2
#define BURN 3
} *head_engr;

struct engr *engr_at(xchar x, xchar y) {
  struct engr *ep = head_engr;
  while (ep) {
    if (x == ep->engr_x && y == ep->engr_y)
      return (ep);
    ep = ep->nxt_engr;
  }
  return ((struct engr *)0);
}

int sengr_at(const char *s, xchar x, xchar y) {
  struct engr *ep = engr_at(x, y);
  char *t;
  int n;
  if (ep && ep->engr_time <= moves) {
    t = ep->engr_txt;
    /*
                    if(!strcmp(s,t)) return(1);
    */
    n = strlen(s);
    while (*t) {
      if (!strncmp(s, t, n))
        return (1);
      t++;
    }
  }
  return (0);
}

void u_wipe_engr(int cnt) {
  if (!u.uswallow && !Levitation)
    wipe_engr_at(u.ux, u.uy, cnt);
}

void wipe_engr_at(xchar x, xchar y, xchar cnt) {
  struct engr *ep = engr_at(x, y);
  int lth, pos;
  char ch;
  if (ep) {
    if ((ep->engr_type != DUST) || Levitation) {
      cnt = rn2(1 + 50 / (cnt + 1)) ? 0 : 1;
    }
    lth = strlen(ep->engr_txt);
    if (lth && cnt > 0) {
      while (cnt--) {
        pos = rn2(lth);
        if ((ch = ep->engr_txt[pos]) == ' ')
          continue;
        ep->engr_txt[pos] = (ch != '?') ? '?' : ' ';
      }
    }
    while (lth && ep->engr_txt[lth - 1] == ' ')
      ep->engr_txt[--lth] = 0;
    /* MODERN: Fix memory corruption - don't modify ep->engr_txt pointer!
     * Instead, shift the text left to remove leading spaces */
    if (ep->engr_txt[0] == ' ') {
      char *src = ep->engr_txt;
      char *dst = ep->engr_txt;
      while (*src == ' ')
        src++; /* find first non-space */
      if (*src) {
        while ((*dst++ = *src++))
          ; /* shift text left */
      } else {
        *dst = 0; /* empty string */
      }
    }
    if (!ep->engr_txt[0])
      del_engr(ep);
  }
}

void read_engr_at(int x, int y) {
  struct engr *ep = engr_at(x, y);
  if (ep && ep->engr_txt[0]) {
    switch (ep->engr_type) {
    case DUST:
      pline("Something is written here in the dust.");
      break;
    case ENGRAVE:
      pline("Something is engraved here on the floor.");
      break;
    case BURN:
      pline("Some text has been burned here in the floor.");
      break;
    default:
      impossible("Something is written in a very strange way.", 0, 0);
    }
    /* MODERN: Safe format string - prevent format string attacks from save
     * files */
    pline("You read: \"%.*s\".", (int)strlen(ep->engr_txt), ep->engr_txt);
  }
}

/* MODERN: CONST-CORRECTNESS: make_engr_at text is read-only */
void make_engr_at(int x, int y, const char *s) {
  struct engr *ep;

  if ((ep = engr_at(x, y)))
    del_engr(ep);
  ep = (struct engr *)alloc((unsigned)(sizeof(struct engr) + strlen(s) + 1));
  ep->nxt_engr = head_engr;
  head_engr = ep;
  ep->engr_x = x;
  ep->engr_y = y;
  ep->engr_txt = (char *)(ep + 1);
  (void)strcpy(ep->engr_txt, s);
  ep->engr_time = 0;
  ep->engr_type = DUST;
  ep->engr_lth = strlen(s) + 1;
}

int doengrave(void) {
  int len;
  char *sp;
  struct engr *ep, *oep = engr_at(u.ux, u.uy);
  char buf[BUFSZ];
  xchar type;
  int spct; /* number of leading spaces */
  struct obj *otmp;
  multi = 0;

  if (u.uswallow) {
    pline("You're joking. Hahaha!"); /* riv05!a3 */
    return (0);
  }

  /* one may write with finger, weapon or wand */
  otmp = getobj("#-)/", "write with");
  if (!otmp)
    return (0);

  if (otmp == &zeroobj)
    otmp = 0;
  if (otmp && otmp->otyp == WAN_FIRE && otmp->spe) {
    type = BURN;
    otmp->spe--;
  } else {
    /* first wield otmp */
    if (otmp != uwep) {
      if (uwep && uwep->cursed) {
        /* Andreas Bormann */
        pline("Since your weapon is welded to your hand,");
        pline("you use the %s.", aobjnam(uwep, (char *)0));
        otmp = uwep;
      } else {
        if (!otmp)
          pline("You are now empty-handed.");
        else if (otmp->cursed)
          pline("The %s %s to your hand!", aobjnam(otmp, "weld"),
                (otmp->quan == 1) ? "itself" : "themselves");
        else
          pline("You now wield %s.", doname(otmp));
        setuwep(otmp);
      }
    }

    if (!otmp)
      type = DUST;
    else if (otmp->otyp == DAGGER || otmp->otyp == TWO_HANDED_SWORD ||
             otmp->otyp == CRYSKNIFE || otmp->otyp == LONG_SWORD ||
             otmp->otyp == AXE) {
      type = ENGRAVE;
      if ((int)otmp->spe <= -3) {
        type = DUST;
        pline("Your %s too dull for engraving.", aobjnam(otmp, "are"));
        if (oep && oep->engr_type != DUST)
          return (1);
      }
    } else
      type = DUST;
  }
  if (Levitation && type != BURN) { /* riv05!a3 */
    pline("You can't reach the floor!");
    return (1);
  }
  if (oep && oep->engr_type == DUST) {
    pline("You wipe out the message that was written here.");
    del_engr(oep);
    oep = 0;
  }
  if (type == DUST && oep) {
    pline("You cannot wipe out the message that is %s in the rock.",
          (oep->engr_type == BURN) ? "burned" : "engraved");
    return (1);
  }

  pline("What do you want to %s on the floor here? ",
        (type == ENGRAVE) ? "engrave"
        : (type == BURN)  ? "burn"
                          : "write");
  getlin(buf);
  clrlin();
  spct = 0;
  sp = buf;
  while (*sp == ' ')
    spct++, sp++;
  len = strlen(sp);
  if (!len || *buf == '\033') {
    if (type == BURN)
      otmp->spe++;
    return (0);
  }

  switch (type) {
  case DUST:
  case BURN:
    if (len > 15) {
      multi = -(len / 10);
      nomovemsg = "You finished writing.";
    }
    break;
  case ENGRAVE: /* here otmp != 0 */
  {
    int len2 = (otmp->spe + 3) * 2 + 1;

    pline("Your %s dull.", aobjnam(otmp, "get"));
    if (len2 < len) {
      len = len2;
      sp[len] = 0;
      otmp->spe = -3;
      nomovemsg = "You cannot engrave more.";
    } else {
      otmp->spe -= (schar)(len / 2); /* MODERN: Safe cast - engraving length bounded */
      nomovemsg = "You finished engraving.";
    }
    multi = -len;
  } break;
  }
  if (oep) {
    /* MODERN: Check for integer overflow in allocation size calculation */
    size_t existing_len = strlen(oep->engr_txt);
    size_t total_needed = (size_t)len + existing_len + spct;
    if (total_needed > 32767) { /* Reasonable limit for engraving length */
      pline("The combined engraving would be too long.");
      return (0);
    }
    len += existing_len + spct;
  }
  ep = (struct engr *)alloc((unsigned)(sizeof(struct engr) + len + 1));
  ep->nxt_engr = head_engr;
  head_engr = ep;
  ep->engr_x = u.ux;
  ep->engr_y = u.uy;
  sp = (char *)(ep + 1); /* (char *)ep + sizeof(struct engr) */
  ep->engr_txt = sp;
  if (oep) {
    /* MODERN: Safe string concatenation with bounds checking */
    int oep_len = strlen(oep->engr_txt);
    int buf_len = strlen(buf);
    if (oep_len + buf_len + 1 <= len + 1) { /* +1 for null terminator */
      (void)strcpy(sp, oep->engr_txt);
      (void)strcat(sp, buf);
    } else {
      /* Truncate if combined length exceeds allocated space */
      (void)strncpy(sp, oep->engr_txt, len);
      sp[len] = 0; /* Ensure null termination */
    }
    del_engr(oep);
  } else {
    /* MODERN: Safe copy with bounds checking */
    (void)strncpy(sp, buf, len);
    sp[len] = 0; /* Ensure null termination */
  }
  ep->engr_lth = len + 1;
  ep->engr_type = type;
  ep->engr_time = moves - multi;

  /* kludge to protect pline against excessively long texts */
  if (len > BUFSZ - 20)
    sp[BUFSZ - 20] = 0;

  return (1);
}

void save_engravings(int fd) {
  struct engr *ep = head_engr, *next;
  while (ep) {
    /* MODERN: free each engraving once written, like saveobjchn().  The
     * original dropped the list (head_engr = 0) without freeing it, leaking
     * every engraving each time a level was saved. */
    next = ep->nxt_engr;
    if (ep->engr_lth && ep->engr_txt[0]) {
      /* MODERN: Prevent integer overflow in write size calculation */
      if (ep->engr_lth > UINT_MAX - sizeof(struct engr)) {
        impossible("Engraving length overflow during save: %u", ep->engr_lth,
                   0);
      } else {
        bwrite(fd, (char *)&(ep->engr_lth), sizeof(ep->engr_lth));
        bwrite(fd, (char *)ep, sizeof(struct engr) + ep->engr_lth);
      }
    }
    free((char *)ep);
    ep = next;
  }
  bwrite(fd, (char *)nul, sizeof(unsigned));
  head_engr = 0;
}

void rest_engravings(int fd) {
  struct engr *ep;
  unsigned lth;
  head_engr = 0;
  while (1) {
    mread(fd, (char *)&lth, sizeof(unsigned));
    if (lth == 0)
      return;
    /* MODERN: Prevent integer overflow and limit engraving size */
    if (lth > 32767 || lth > UINT_MAX - sizeof(struct engr)) {
      error("Save file corrupted: invalid engraving size");
    }
    ep = (struct engr *)alloc(sizeof(struct engr) + lth);
    mread(fd, (char *)ep, sizeof(struct engr) + lth);
    ep->nxt_engr = head_engr;
    ep->engr_txt = (char *)(ep + 1); /* Andreas Bormann */
    head_engr = ep;
  }
}

void del_engr(struct engr *ep) {
  struct engr *ept;
  if (ep == head_engr)
    head_engr = ep->nxt_engr;
  else {
    for (ept = head_engr; ept; ept = ept->nxt_engr) {
      if (ept->nxt_engr == ep) {
        ept->nxt_engr = ep->nxt_engr;
        goto fnd;
      }
    }
    impossible("Error in del_engr?", 0, 0);
    return;
  fnd:;
  }
  free((char *)ep);
}

/**
 * MODERN: Memory cleanup for sanitizers */
void cleanup_all_engravings(void) {
  struct engr *ep, *next;

  for (ep = head_engr; ep; ep = next) {
    next = ep->nxt_engr;
    free((char *)ep);
  }
  head_engr = NULL;
}
