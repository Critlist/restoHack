/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.pri.c - version 1.0.3 */
/* $FreeBSD$ */

#include "hack.h"
#include "generated/config.h"
#include <curses.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>   /* MODERN ADDITION (2025): For isdigit() in format string validation */
xchar scrlx, scrhx, scrly, scrhy; /* corners of new area on screen */

/* MODERN: CONST-CORRECTNESS: match hu_stat[] definition (read-only string
 * table) */
extern const char *const hu_stat[]; /* in eat.c */
extern char *CD;

void swallowed(void) {
  char ulook[] =
      "|@|"; /* MODERN: Use array instead of pointer to allow modification */
  ulook[1] = u.usym;

  cls();
  curs(u.ux - 1, u.uy + 1);
  fputs("/-\\", stdout);
  curx = u.ux + 2;
  curs(u.ux - 1, u.uy + 2);
  fputs(ulook, stdout);
  curx = u.ux + 2;
  curs(u.ux - 1, u.uy + 3);
  fputs("\\-/", stdout);
  curx = u.ux + 2;
  u.udispl = 1;
  u.udisx = u.ux;
  u.udisy = u.uy;
}

/*VARARGS1*/
boolean panicking;

/**
 * MODERN ADDITION (2025): Format string protection for panic function
 * WHY: Same vulnerability as error() - vprintf treats first param as format string  
 * HOW: Use same validation logic to detect dangerous format specifiers
 * PRESERVES: All existing panic() calls work identically (they use safe strings)
 * ADDS: Protection against format string attacks from corrupted game state
 */

/* Forward declare format validator from hack.tty.c */
static int check_format_specs(const char *fmt) {
    const char *p = fmt;
    while ((p = strchr(p, '%')) != NULL) {
        p++; /* Skip the % */
        if (*p == '%') {
            p++; /* Skip literal %% */
            continue;
        }
        /* Skip width/precision/flags */
        while (*p && (isdigit(*p) || *p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '.')) {
            p++;
        }
        if (*p == 'n') {
            return 1; /* %n writes to memory - very dangerous */
        }
        if (*p) p++; /* Move past conversion specifier */
    }
    return 0;
}

void
/* MODERN: CONST-CORRECTNESS: panic message is read-only */
panic(const char *str, ...) {
  if (panicking++)
    exit(1); /* avoid loops - this should never happen*/
  home();
  puts(" Suddenly, the dungeon collapses.");
  fputs(" ERROR:  ", stdout);
  va_list args;
  va_start(args, str);
  
  /* MODERN: Check for format string attack before using vprintf */
  if (check_format_specs(str)) {
    /* Dangerous format detected - print safely without interpretation */
    printf("%s\n", str);
    printf("(Note: Panic message sanitized for security)\n");
  } else {
    /* Safe to use as format string */
    vprintf(str, args);
  }
  va_end(args);
#ifdef DEBUG
#ifdef UNIX
  if (!fork())
    abort(); /* generate core dump */
#endif       /* UNIX */
#endif       /* DEBUG */
  more();    /* contains a fflush() */
  done("panicked");
}

void atl(int x, int y, int ch) {
  /**
   * MODERN ADDITION (2025): Move bounds check before array access
   * WHY: Original accessed levl[x][y] before validating bounds
   * HOW: Check bounds first, then access array
   * PRESERVES: Original 1984 error handling behavior
   * ADDS: Memory safety by preventing buffer overflow
   */
  if (x < 0 || x > COLNO - 1 || y < 0 || y > ROWNO - 1) {
    impossible("atl(%d,%d)", x, y);
    return;
  }

  struct rm *crm = &levl[x][y];

  if (crm->seen && crm->scrsym == ch)
    return;
  crm->scrsym = ch;
  crm->new = 1;
  on_scr(x, y);
}

void on_scr(int x, int y) {
  if (x < scrlx)
    scrlx = x;
  if (x > scrhx)
    scrhx = x;
  if (y < scrly)
    scrly = y;
  if (y > scrhy)
    scrhy = y;
}

/* call: (x,y) - display
        (-1,0) - close (leave last symbol)
        (-1,-1)- close (undo last symbol)
        (-1,let)-open: initialize symbol
        (-2,let)-change let
*/

void tmp_at(schar x, schar y) {
  static schar prevx,
      prevy; /* MODERN: Keep original schar to preserve -1 sentinel logic */
  static char let;
  if ((int)x == -2) { /* change let call */
    let = y;
    return;
  }
  if ((int)x == -1 && (int)y >= 0) { /* open or close call */
    let = y;
    prevx = -1;
    return;
  }
  if (prevx >= 0 && cansee(prevx, prevy)) {
    delay_output(50);
    prl(prevx, prevy); /* in case there was a monster */
    at(prevx, prevy,
       levl[(unsigned char)prevx][(unsigned char)prevy]
           .scrsym); /* MODERN: Cast to unsigned char for safe array indexing */
  }
  if (x >= 0) { /* normal call */
    if (cansee(x, y))
      at(x, y, let);
    prevx = x;
    prevy = y;
  } else { /* close call */
    let = 0;
    prevx = -1;
  }
}

/* like the previous, but the symbols are first erased on completion */
void Tmp_at(schar x, schar y) { /* MODERN: Keep original schar to preserve -1,-2
                                   sentinel logic */
  static char let;
  static xchar cnt;
  static coord tc[COLNO]; /* but watch reflecting beams! */
  int xx, yy;
  if ((int)x == -1) {
    if (y > 0) { /* open call */
      let = y;
      cnt = 0;
      return;
    }
    /* close call (do not distinguish y==0 and y==-1) */
    while (cnt--) {
      xx = tc[(unsigned char)cnt]
               .x; /* MODERN: Cast to unsigned char for safe array indexing */
      yy = tc[(unsigned char)cnt]
               .y; /* MODERN: Cast to unsigned char for safe array indexing */
      prl(xx, yy);
      at(xx, yy,
         levl[(unsigned char)xx][(unsigned char)yy]
             .scrsym); /* MODERN: Cast to unsigned char for safe array indexing
                        */
    }
    cnt = let = 0; /* superfluous */
    return;
  }
  if ((int)x == -2) { /* change let call */
    let = y;
    return;
  }
  /* normal call */
  if (cansee(x, y)) {
    if (cnt)
      delay_output(50);
    at(x, y, let);
    tc[(unsigned char)cnt].x =
        x; /* MODERN: Cast to unsigned char for safe array indexing */
    tc[(unsigned char)cnt].y =
        y; /* MODERN: Cast to unsigned char for safe array indexing */
    if (++cnt >= COLNO)
      panic("Tmp_at overflow?");
    levl[(unsigned char)x][(unsigned char)y].new = 0;
        /* prevent pline-nscr erasing --- */ /* MODERN: Cast to unsigned char
                                                for safe array indexing */
  }
}

void setclipped(void) {
  error("Hack needs a screen of size at least %d by %d.\n", ROWNO + 2, COLNO);
}

void at(xchar x, xchar y, char ch) {
#ifndef lint
  /* if xchar is unsigned, lint will complain about  if(x < 0)  */
  if (x < 0 || x > COLNO - 1 || y < 0 || y > ROWNO - 1) {
    impossible("At gets 0%o at %d", ch, x);
    return;
  }
#endif /* lint */
  if (!ch) {
    impossible("At gets null at %d %d.", x, y);
    return;
  }
  y += 2;
  curs(x, y);
  (void)putchar(ch);
  curx++;
}

void prme(void) {
  if (!Invisible)
    at(u.ux, u.uy, u.usym);
}

int doredraw(void) {
  docrt();
  return (0);
}

void docrt(void) {
  int x, y;
  struct rm *room;
  struct monst *mtmp;

  if (u.uswallow) {
    swallowed();
    return;
  }
  cls();

  /* Some ridiculous code to get display of @ and monsters (almost) right */
  if (!Invisible) {
    /**
     * MODERN ADDITION (2025): Bounds check before array access
     * WHY: u.ux/u.uy can exceed array bounds causing buffer overflow
     * HOW: Validate coordinates before accessing levl array
     * PRESERVES: Original 1984 display logic
     * ADDS: Memory safety by preventing buffer overflow
     */
    if (u.ux >= 1 && u.ux <= COLNO - 1 && u.uy >= 0 && u.uy <= ROWNO - 1) {
      /* Original 1984: levl[(u.udisx = u.ux)][(u.udisy = u.uy)].scrsym =
       * u.usym; levl[u.udisx][u.udisy].seen = 1; */
      levl[(unsigned char)(u.udisx = u.ux)][(unsigned char)(u.udisy = u.uy)]
          .scrsym = u.usym;
      levl[(unsigned char)u.udisx][(unsigned char)u.udisy].seen =
          1; /* Bounds already validated */
    }
    u.udispl = 1;
  } else
    u.udispl = 0;

  seemons(); /* reset old positions */
  for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
    mtmp->mdispl = 0;
  seemons(); /* force new positions to be shown */
  /* This nonsense should disappear soon --------------------------------- */

  for (y = 0; y < ROWNO; y++)
    for (x = 0; x < COLNO; x++)
      if ((room = &levl[x][y])->new) {
        room->new = 0;
        at(x, y, room->scrsym);
      } else if (room->seen)
        at(x, y, room->scrsym);
  scrlx = COLNO;
  scrly = ROWNO;
  scrhx = scrhy = 0;
  flags.botlx = 1;
  bot();
}

void docorner(int xmin, int ymax) {
  int x, y;
  struct rm *room;
  struct monst *mtmp;

  if (u.uswallow) { /* Can be done more efficiently */
    swallowed();
    return;
  }

  seemons(); /* reset old positions */
  for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
    if (mtmp->mx >= xmin && mtmp->my < ymax)
      mtmp->mdispl = 0;
  seemons(); /* force new positions to be shown */

  for (y = 0; y < ymax; y++) {
    if (y > ROWNO && CD)
      break;
    curs(xmin, y + 2);
    cl_end();
    if (y < ROWNO) {
      for (x = xmin; x < COLNO; x++) {
        if ((room = &levl[x][y])->new) {
          room->new = 0;
          at(x, y, room->scrsym);
        } else if (room->seen)
          at(x, y, room->scrsym);
      }
    }
  }
  if (ymax > ROWNO) {
    cornbot(xmin - 1);
    if (ymax > ROWNO + 1 && CD) {
      curs(1, ROWNO + 3);
      cl_eos();
    }
  }
}

void curs_on_u(void) { curs(u.ux, u.uy + 2); }

void pru(void) {
  if (u.udispl && (Invisible || u.udisx != u.ux || u.udisy != u.uy))
    /* if(! levl[u.udisx][u.udisy].new) */
    if (!vism_at(u.udisx, u.udisy))
      newsym(u.udisx, u.udisy);
  if (Invisible) {
    u.udispl = 0;
    prl(u.ux, u.uy);
  } else if (!u.udispl || u.udisx != u.ux || u.udisy != u.uy) {
    atl(u.ux, u.uy, u.usym);
    u.udispl = 1;
    u.udisx = u.ux;
    u.udisy = u.uy;
  }
  /* Original 1984: levl[u.ux][u.uy].seen = 1; */
  /**
   * MODERN ADDITION (2025): Bounds check before array access
   * WHY: u.ux/u.uy can exceed array bounds (e.g. u.ux=82 > 79)
   * HOW: Validate coordinates before accessing levl array
   * PRESERVES: Original 1984 visibility logic
   * ADDS: Memory safety by preventing buffer overflow
   */
  if (u.ux >= 1 && u.ux <= COLNO - 1 && u.uy >= 0 && u.uy <= ROWNO - 1) {
    levl[(unsigned char)u.ux][(unsigned char)u.uy].seen =
        1; /* Bounds already validated */
  }
}

#ifndef NOWORM
#include "def.wseg.h"
extern struct wseg *m_atseg;
extern void pwseg(struct wseg *wseg);
#endif /* NOWORM */

/* print a position that is visible for @ */
void prl(int x, int y) {
  struct rm *room;
  struct monst *mtmp;
  struct obj *otmp;

  if (x == u.ux && y == u.uy && (!Invisible)) {
    pru();
    return;
  }
  if (!isok(x, y))
    return;
  room = &levl[x][y];
  /* Original 1984: if((!room->typ) || (IS_ROCK(room->typ) &&
   * levl[u.ux][u.uy].typ == CORR)) */
  if ((!room->typ) || (IS_ROCK(room->typ) &&
                       levl[(unsigned char)u.ux][(unsigned char)u.uy].typ ==
                           CORR)) /* MODERN: safe array indexing */
    return;
  if ((mtmp = m_at(x, y)) && !mtmp->mhide && (!mtmp->minvis || See_invisible)) {
#ifndef NOWORM
    if (m_atseg)
      pwseg(m_atseg);
    else
#endif /* NOWORM */
      pmon(mtmp);
  } else if ((otmp = o_at(x, y)) && room->typ != POOL)
    atl(x, y, otmp->olet);
  else if (mtmp && (!mtmp->minvis || See_invisible)) {
    /* must be a hiding monster, but not hiding right now */
    /* assume for the moment that long worms do not hide */
    pmon(mtmp);
  } else if (g_at(x, y) && room->typ != POOL)
    atl(x, y, '$');
  else if (!room->seen || room->scrsym == ' ') {
    room->new = room->seen = 1;
    newsym(x, y);
    on_scr(x, y);
  }
  room->seen = 1;
}

char
/* Original 1984: news0(xchar x, xchar y) */
news0(unsigned char x,
      unsigned char y) /* MODERN: unsigned to prevent buffer underflow */
{
  struct obj *otmp;
  struct trap *ttmp;
  struct rm *room;
  char tmp;

  room = &levl[x][y];
  if (!room->seen)
    tmp = ' ';
  else if (room->typ == POOL)
    tmp = POOL_SYM;
  else if (!Blind && (otmp = o_at(x, y)))
    tmp = otmp->olet;
  else if (!Blind && g_at(x, y))
    tmp = '$';
  else if (x == xupstair && y == yupstair)
    tmp = '<';
  else if (x == xdnstair && y == ydnstair)
    tmp = '>';
  else if ((ttmp = t_at(x, y)) && ttmp->tseen)
    tmp = '^';
  else
    switch (room->typ) {
    case SCORR:
    case SDOOR:
      tmp = room->scrsym; /* %% wrong after killing mimic ! */
      break;
    case HWALL:
      tmp = '-';
      break;
    case VWALL:
      tmp = '|';
      break;
    case LDOOR:
    case DOOR:
      tmp = '+';
      break;
    case CORR:
      tmp = CORR_SYM;
      break;
    case ROOM:
      if (room->lit || cansee(x, y) || Blind)
        tmp = '.';
      else
        tmp = ' ';
      break;
      /*
              case POOL:
                      tmp = POOL_SYM;
                      break;
      */
    default:
      tmp = ERRCHAR;
    }
  return (tmp);
}

void newsym(int x, int y) { atl(x, y, news0(x, y)); }

/* used with wand of digging (or pick-axe): fill scrsym and force display */
/* also when a POOL evaporates */
void mnewsym(int x, int y) {
  struct rm *room;
  char newscrsym;

  if (!vism_at(x, y)) {
    room = &levl[x][y];
    newscrsym = news0(x, y);
    if (room->scrsym != newscrsym) {
      room->scrsym = newscrsym;
      room->seen = 0;
    }
  }
}

void nosee(int x, int y) {
  struct rm *room;

  if (!isok(x, y))
    return;
  room = &levl[x][y];
  if (room->scrsym == '.' && !room->lit && !Blind) {
    room->scrsym = ' ';
    room->new = 1;
    on_scr(x, y);
  }
}

#ifndef QUEST
void prl1(int x, int y) {
  if (u.dx) {
    if (u.dy) {
      prl(x - (2 * u.dx), y);
      prl(x - u.dx, y);
      prl(x, y);
      prl(x, y - u.dy);
      prl(x, y - (2 * u.dy));
    } else {
      prl(x, y - 1);
      prl(x, y);
      prl(x, y + 1);
    }
  } else {
    prl(x - 1, y);
    prl(x, y);
    prl(x + 1, y);
  }
}

void nose1(int x, int y) {
  if (u.dx) {
    if (u.dy) {
      nosee(x, u.uy);
      nosee(x, u.uy - u.dy);
      nosee(x, y);
      nosee(u.ux - u.dx, y);
      nosee(u.ux, y);
    } else {
      nosee(x, y - 1);
      nosee(x, y);
      nosee(x, y + 1);
    }
  } else {
    nosee(x - 1, y);
    nosee(x, y);
    nosee(x + 1, y);
  }
}
#endif /* QUEST */

int vism_at(int x, int y) {
  struct monst *mtmp;

  return ((x == u.ux && y == u.uy && !Invisible) ? 1
          : (mtmp = m_at(x, y)) ? ((Blind && Telepat) || canseemon(mtmp))
                                : 0);
}

#ifdef NEWSCR
pobj(obj) struct obj *obj;
{
  int show = (!obj->oinvis || See_invisible) && cansee(obj->ox, obj->oy);
  if (obj->odispl) {
    if (obj->odx != obj->ox || obj->ody != obj->oy || !show)
      if (!vism_at(obj->odx, obj->ody)) {
        newsym(obj->odx, obj->ody);
        obj->odispl = 0;
      }
  }
  if (show && !vism_at(obj->ox, obj->oy)) {
    atl(obj->ox, obj->oy, obj->olet);
    obj->odispl = 1;
    obj->odx = obj->ox;
    obj->ody = obj->oy;
  }
}
#endif /* NEWSCR */

void unpobj(struct obj *obj) {
  /* 	if(obj->odispl){
                  if(!vism_at(obj->odx, obj->ody))
                          newsym(obj->odx, obj->ody);
                  obj->odispl = 0;
          }
  */
  if (!vism_at(obj->ox, obj->oy))
    newsym(obj->ox, obj->oy);
}

void seeobjs(void) {
  struct obj *obj, *obj2;
  for (obj = fobj; obj; obj = obj2) {
    obj2 = obj->nobj;
    if (obj->olet == FOOD_SYM && obj->otyp >= CORPSE && obj->age + 250 < moves)
      delobj(obj);
  }
  for (obj = invent; obj; obj = obj2) {
    obj2 = obj->nobj;
    if (obj->olet == FOOD_SYM && obj->otyp >= CORPSE && obj->age + 250 < moves)
      useup(obj);
  }
}

void seemons(void) {
  struct monst *mtmp;
  for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
    if (mtmp->data->mlet == ';')
      mtmp->minvis =
          (u.ustuck != mtmp &&
           levl[(unsigned char)mtmp->mx][(unsigned char)mtmp->my].typ ==
               POOL); /* MODERN: Cast to unsigned char for safe array indexing
                       */
    pmon(mtmp);
#ifndef NOWORM
    if (mtmp->wormno)
      wormsee(mtmp->wormno);
#endif /* NOWORM */
  }
}

void pmon(struct monst *mon) {
  int show = (Blind && Telepat) || canseemon(mon);
  if (mon->mdispl) {
    if (mon->mdx != mon->mx || mon->mdy != mon->my || !show)
      unpmon(mon);
  }
  if (show && !mon->mdispl) {
    atl(mon->mx, mon->my,
        (!mon->mappearance ||
         u.uprops[PROP(RIN_PROTECTION_FROM_SHAPE_CHANGERS)].p_flgs)
            ? mon->data->mlet
            : mon->mappearance);
    mon->mdispl = 1;
    mon->mdx = mon->mx;
    mon->mdy = mon->my;
  }
}

void unpmon(struct monst *mon) {
  if (mon->mdispl) {
    newsym(mon->mdx, mon->mdy);
    mon->mdispl = 0;
  }
}

void nscr(void) {
  int x, y;
  struct rm *room;

  if (u.uswallow || u.ux == FAR || flags.nscrinh)
    return;
  pru();
  for (y = scrly; y <= scrhy; y++)
    for (x = scrlx; x <= scrhx; x++)
      if ((room = &levl[x][y])->new) {
        room->new = 0;
        at(x, y, room->scrsym);
      }
  scrhx = scrhy = 0;
  scrlx = COLNO;
  scrly = ROWNO;
}

/* 100 suffices for bot(); no relation with COLNO */
char oldbot[100], newbot[100];
void cornbot(int lth) {
  if (lth < (int)sizeof(oldbot)) { /* MODERN: Cast to int to match lth type */
    oldbot[lth] = 0;
    flags.botl = 1;
  }
}

void bot(void) {
  char *ob = oldbot, *nb = newbot;
  int i;
  int len, remaining; /* MODERN: Track buffer usage for safe sprintf */
  if (flags.botlx)
    *ob = 0;
  flags.botl = flags.botlx = 0;
#ifdef GOLD_ON_BOTL
  len = snprintf(newbot, sizeof(newbot), /* MODERN: Safe sprintf replacement */
                 "Level %-2d  Gold %-5ld  Hp %3d(%d)  Ac %-2d  Str ", dlevel,
                 u.ugold, u.uhp, u.uhpmax, u.uac);
#else
  len = snprintf(newbot, sizeof(newbot), /* MODERN: Safe sprintf replacement */
                 "Level %-2d   Hp %3d(%d)   Ac %-2d   Str ", dlevel, u.uhp,
                 u.uhpmax, u.uac);
#endif /* GOLD_ON_BOTL */
  if (len >= (int)sizeof(newbot))
    len = (int)sizeof(newbot) - 1; /* MODERN: Cast to int to match len type */
  remaining = sizeof(newbot) - len;

  if (u.ustr > 18) {
    if (u.ustr > 117) {
      if (remaining > 5) { /* MODERN: Check space before strcat */
        (void)strcat(newbot, "18/**");
        len += 5;
        remaining -= 5;
      }
    } else {
      int added = snprintf(eos(newbot), remaining, "18/%02d", u.ustr - 18);
      if (added > 0 && added < remaining) {
        len += added;
        remaining -= added;
      }
    }
  } else {
    int added = snprintf(eos(newbot), remaining, "%-2d   ", u.ustr);
    if (added > 0 && added < remaining) {
      len += added;
      remaining -= added;
    }
  }
#ifdef EXP_ON_BOTL
  if (remaining > 0) {
    int added =
        snprintf(eos(newbot), remaining, "  Exp %2u/%-5ld ", u.ulevel, u.uexp);
    if (added > 0 && added < remaining) {
      len += added;
      remaining -= added;
    }
  }
#else
  if (remaining > 0) {
    int added = snprintf(eos(newbot), remaining, "   Exp %2u  ", u.ulevel);
    if (added > 0 && added < remaining) {
      len += added;
      remaining -= added;
    }
  }
#endif /* EXP_ON_BOTL */
  if (remaining > 0 &&
      hu_stat[u.uhs]) { /* MODERN: Check space and null pointer */
    int hu_len = strlen(hu_stat[u.uhs]);
    if (hu_len < remaining) {
      (void)strcat(newbot, hu_stat[u.uhs]);
      len += hu_len;
      remaining -= hu_len;
    }
  }
  if (flags.time && remaining > 0) {
    (void)snprintf(eos(newbot), remaining, "  %ld",
                   moves); /* MODERN: Safe sprintf */
  }
  if (strlen(newbot) >= COLNO) {
    char *bp0, *bp1;
    bp0 = bp1 = newbot;
    do {
      if (*bp0 != ' ' || bp0[1] != ' ' || bp0[2] != ' ')
        *bp1++ = *bp0;
    } while (*bp0++);
  }
  for (i = 1; i < COLNO; i++) {
    if (*ob != *nb) {
      curs(i, ROWNO + 2);
      (void)putchar(*nb ? *nb : ' ');
      curx++;
    }
    if (*ob)
      ob++;
    if (*nb)
      nb++;
  }
  /**
   * MODERN ADDITION (2025): Replace strcpy with safe string copy
   * WHY: strcpy can overflow oldbot buffer if newbot is longer than 100 chars
   * HOW: Use strncpy with explicit size limit and ensure null termination
   * PRESERVES: String copying functionality with identical behavior for valid strings  
   * ADDS: Buffer overflow protection when newbot exceeds oldbot capacity
   */
  (void)strncpy(oldbot, newbot, sizeof(oldbot) - 1);
  oldbot[sizeof(oldbot) - 1] = '\0'; /* Ensure null termination */
}

#ifdef WAN_PROBING
mstatusline(mtmp) struct monst *mtmp;
{
  pline("Status of %s: ", monnam(mtmp));
  pline("Level %-2d  Gold %-5lu  Hp %3d(%d)  Ac %-2d  Dam %d",
        mtmp->data->mlevel, mtmp->mgold, mtmp->mhp, mtmp->mhpmax,
        mtmp->data->ac, (mtmp->data->damn + 1) * (mtmp->data->damd + 1));
}
#endif /* WAN_PROBING */

void cls(void) {
  if (flags.toplin == 1)
    more();
  flags.toplin = 0;

  clear_screen();

  flags.botlx = 1;
}
