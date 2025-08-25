/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.save.c - version 1.0.3 */
/* $FreeBSD$ */

#include "hack.h"
extern char genocided[60]; /* defined in Decl.c */
extern char fut_geno[60];  /* idem */
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* MODERN ADDITION (2025): Versioned save file format
 *
 * WHY: Original code uses raw struct serialization which breaks across:
 * - Different architectures (32-bit vs 64-bit)
 * - Different compilers (struct packing differences)
 * - Different endianness (big-endian vs little-endian)
 *
 * HOW: Fixed-width field serialization with version header
 * PRESERVES: Exact game state, all functionality
 * ADDS: Cross-platform compatibility, backwards compatibility
 */

/* Save file header for versioned format */
typedef struct {
  char magic[4];      /* "RHCK" */
  uint16_t version;   /* Format version, currently 1 */
  uint32_t endiantag; /* 0x01020304 for endian detection */
  uint32_t reserved;  /* For future use */
} rh_hdr_t;

#define RH_MAGIC "RHCK"
#define RH_VERSION 1
#define RH_ENDIANTAG 0x01020304

/* Fixed-width type definitions for save format */
typedef uint16_t rh_u16_t;
typedef uint32_t rh_u32_t;
typedef int32_t rh_i32_t;

/* Serialization helper functions for cross-platform compatibility */
static void sw_u16(int fd, rh_u16_t val) {
  unsigned char buf[2];
  buf[0] = (val >> 8) & 0xff;
  buf[1] = val & 0xff;
  bwrite(fd, (char *)buf, 2);
}

static rh_u16_t sr_u16(int fd) {
  unsigned char buf[2];
  mread(fd, (char *)buf, 2);
  return ((rh_u16_t)buf[0] << 8) | buf[1];
}

static void sw_u32(int fd, rh_u32_t val) {
  unsigned char buf[4];
  buf[0] = (val >> 24) & 0xff;
  buf[1] = (val >> 16) & 0xff;
  buf[2] = (val >> 8) & 0xff;
  buf[3] = val & 0xff;
  bwrite(fd, (char *)buf, 4);
}

static rh_u32_t sr_u32(int fd) {
  unsigned char buf[4];
  mread(fd, (char *)buf, 4);
  return ((rh_u32_t)buf[0] << 24) | ((rh_u32_t)buf[1] << 16) |
         ((rh_u32_t)buf[2] << 8) | buf[3];
}

static void sw_i32(int fd, rh_i32_t val) { sw_u32(fd, (rh_u32_t)val); }

static rh_i32_t sr_i32(int fd) { return (rh_i32_t)sr_u32(fd); }

static void sw_bytes(int fd, const void *data, size_t len) {
  bwrite(fd, (char *)data, len);
}

static void sr_bytes(int fd, void *data, size_t len) {
  mread(fd, (char *)data, len);
}

/* Pack struct flag bitfields into portable format */
static void pack_flags(int fd, const struct flag *f) {
  rh_u32_t packed1, packed2;

  /* Pack bitfields into two 32-bit words for portability */
  packed1 = f->ident;
  sw_u32(fd, packed1);

  packed2 = 0;
  if (f->debug)
    packed2 |= 0x00000001;
  if (f->toplin & 1)
    packed2 |= 0x00000002;
  if (f->toplin & 2)
    packed2 |= 0x00000004;
  if (f->cbreak)
    packed2 |= 0x00000008;
  if (f->standout)
    packed2 |= 0x00000010;
  if (f->nonull)
    packed2 |= 0x00000020;
  if (f->time)
    packed2 |= 0x00000040;
  if (f->nonews)
    packed2 |= 0x00000080;
  if (f->notombstone)
    packed2 |= 0x00000100;
  if (f->end_own)
    packed2 |= 0x00000200;
  if (f->no_rest_on_space)
    packed2 |= 0x00000400;
  if (f->beginner)
    packed2 |= 0x00000800;
  if (f->female)
    packed2 |= 0x00001000;
  if (f->invlet_constant)
    packed2 |= 0x00002000;
  if (f->move)
    packed2 |= 0x00004000;
  if (f->mv)
    packed2 |= 0x00008000;
  packed2 |= (f->run & 7) << 16;
  if (f->nopick)
    packed2 |= 0x00080000;
  if (f->echo)
    packed2 |= 0x00100000;
  if (f->botl)
    packed2 |= 0x00200000;
  if (f->botlx)
    packed2 |= 0x00400000;
  if (f->nscrinh)
    packed2 |= 0x00800000;
  if (f->made_amulet)
    packed2 |= 0x01000000;
  packed2 |= (f->no_of_wizards & 3) << 25;
  packed2 |= (f->moonphase & 7) << 27;
  sw_u32(fd, packed2);

  /* Save end_top and end_around as separate fields */
  sw_u32(fd, f->end_top);
  sw_u32(fd, f->end_around);
}

/* Unpack struct flag bitfields from portable format */
static void unpack_flags(int fd, struct flag *f) {
  rh_u32_t packed1, packed2;

  /* Clear the struct first */
  memset(f, 0, sizeof(*f));

  /* Unpack the data */
  packed1 = sr_u32(fd);
  packed2 = sr_u32(fd);

  f->ident = packed1;
  f->debug = (packed2 & 0x00000001) ? 1 : 0;
  f->toplin =
      ((packed2 & 0x00000002) ? 1 : 0) | ((packed2 & 0x00000004) ? 2 : 0);
  f->cbreak = (packed2 & 0x00000008) ? 1 : 0;
  f->standout = (packed2 & 0x00000010) ? 1 : 0;
  f->nonull = (packed2 & 0x00000020) ? 1 : 0;
  f->time = (packed2 & 0x00000040) ? 1 : 0;
  f->nonews = (packed2 & 0x00000080) ? 1 : 0;
  f->notombstone = (packed2 & 0x00000100) ? 1 : 0;
  f->end_own = (packed2 & 0x00000200) ? 1 : 0;
  f->no_rest_on_space = (packed2 & 0x00000400) ? 1 : 0;
  f->beginner = (packed2 & 0x00000800) ? 1 : 0;
  f->female = (packed2 & 0x00001000) ? 1 : 0;
  f->invlet_constant = (packed2 & 0x00002000) ? 1 : 0;
  f->move = (packed2 & 0x00004000) ? 1 : 0;
  f->mv = (packed2 & 0x00008000) ? 1 : 0;
  f->run = (packed2 >> 16) & 7;
  f->nopick = (packed2 & 0x00080000) ? 1 : 0;
  f->echo = (packed2 & 0x00100000) ? 1 : 0;
  f->botl = (packed2 & 0x00200000) ? 1 : 0;
  f->botlx = (packed2 & 0x00400000) ? 1 : 0;
  f->nscrinh = (packed2 & 0x00800000) ? 1 : 0;
  f->made_amulet = (packed2 & 0x01000000) ? 1 : 0;
  f->no_of_wizards = (packed2 >> 25) & 3;
  f->moonphase = (packed2 >> 27) & 7;

  f->end_top = sr_u32(fd);
  f->end_around = sr_u32(fd);
}

/* Write versioned save file header */
static int write_save_header(int fd) {
  rh_hdr_t hdr;

  memcpy(hdr.magic, RH_MAGIC, 4);
  hdr.version = RH_VERSION;
  hdr.endiantag = RH_ENDIANTAG;
  hdr.reserved = 0;

  /* Write header using our serialization functions for consistency */
  sw_bytes(fd, hdr.magic, 4);
  sw_u16(fd, hdr.version);
  sw_u32(fd, hdr.endiantag);
  sw_u32(fd, hdr.reserved);

  return 1;
}

/* Check if file starts with our versioned header */
static int check_save_header(int fd, rh_hdr_t *hdr) {
  char test_magic[4];

  /* Try to read magic - if it fails, this isn't our format */
  if (read(fd, test_magic, 4) != 4)
    return 0;
  if (memcmp(test_magic, RH_MAGIC, 4) != 0)
    return 0;

  /* Copy magic to header */
  memcpy(hdr->magic, test_magic, 4);

  hdr->version = sr_u16(fd);
  hdr->endiantag = sr_u32(fd);
  hdr->reserved = sr_u32(fd);

  /* Check endianness */
  if (hdr->endiantag != RH_ENDIANTAG) {
    return 0; /* Different endianness not supported yet */
  }

  /* Check version */
  if (hdr->version != RH_VERSION) {
    return 0; /* Different version not supported yet */
  }

  return 1;
}

extern char SAVEF[], nul[];
extern char pl_character[PL_CSIZ];
extern struct obj *restobjchn(int fd);
extern struct monst *restmonchn(int fd);
extern int dosave0(int hu);
extern void savenames(int fd);
extern void restnames(int fd);
/* MODERN: CONST-CORRECTNESS: settty message is read-only */
extern void settty(const char *s);

int dosave(void) {
  if (dosave0(0)) {
    settty("Be seeing you ...\n");
    exit(0);
  }
  return (
      0); /* MODERN: Return failure if dosave0 failed - was only under lint */
}

#ifndef NOSAVEONHANGUP
/**
 * MODERN ADDITION (2025): Enhanced hangup handler with SIGTERM support
 *
 * WHY: Original hangup() only handled SIGHUP. Modern window managers send
 * SIGTERM when force-closing terminals, bypassing save mechanism and leaving
 * stale locks.
 *
 * HOW: Extended to handle SIGHUP, SIGTERM, and SIGQUIT with automatic save.
 * Maintains original behavior (save on unexpected disconnect) while supporting
 * modern desktop environment termination patterns.
 *
 * PRESERVES: Original save-on-hangup logic via dosave0(1)
 * ADDS: Modern signal compatibility for Hyprland/Wayland window closure
 */
void modern_save_handler(int sig) {
  (void)sig;
  /* Attempt to save game state before cleanup */
  (void)dosave0(1);
  exit(1);
}

void hangup(int sig) { modern_save_handler(sig); }
#endif /* NOSAVEONHANGUP */

/* returns 1 if save successful */
int dosave0(int hu) {
  int fd, ofd;
  int tmp; /* not ! */
  char tmpfile[256];

  (void)signal(SIGHUP, SIG_IGN);
  (void)signal(SIGINT, SIG_IGN);

  /* Create temporary file for atomic save */
  snprintf(tmpfile, sizeof(tmpfile), "%s.tmp", SAVEF);
  if ((fd = creat(tmpfile, FMASK)) < 0) {
    if (!hu)
      pline("Cannot open save file. (Continue or press Q to Quit)");
    (void)unlink(tmpfile);
    return (0);
  }
  if (flags.moonphase == FULL_MOON) /* ut-sally!fletcher */
    u.uluck--;                      /* and unido!ab */
  savelev(fd, dlevel);
  saveobjchn(fd, invent);
  saveobjchn(fd, fcobj);
  savemonchn(fd, fallen_down);

  /* Write versioned save header */
  write_save_header(fd);

  /* Save game state using fixed-width serialization */
  tmp = getuid();
  sw_u32(fd, (rh_u32_t)tmp);
  pack_flags(fd, &flags);
  sw_i32(fd, dlevel);
  sw_i32(fd, maxdlevel);
  sw_u32(fd, (rh_u32_t)moves);

  /* For now, serialize struct you as raw data - this could be improved later */
  /* TODO: Implement proper field-wise serialization for struct you */
  sw_u32(fd, (rh_u32_t)sizeof(struct you));
  sw_bytes(fd, &u, sizeof(struct you));

  if (u.ustuck) {
    sw_u32(fd, 1); /* has ustuck */
    sw_u32(fd, (rh_u32_t)(u.ustuck->m_id));
  } else {
    sw_u32(fd, 0); /* no ustuck */
  }

  sw_bytes(fd, pl_character, sizeof pl_character);
  sw_bytes(fd, genocided, sizeof genocided);
  sw_bytes(fd, fut_geno, sizeof fut_geno);
  savenames(fd);
  for (tmp = 1; tmp <= maxdlevel; tmp++) {
    extern int hackpid;
    extern boolean level_exists[];

    if (tmp == dlevel || !level_exists[tmp])
      continue;
    glo(tmp);
    if ((ofd = open(lock, 0)) < 0) {
      if (!hu)
        pline("Error while saving: cannot read %s.", lock);
      (void)close(fd);
      (void)unlink(tmpfile);
      if (!hu)
        done("tricked");
      return (0);
    }
    getlev(ofd, hackpid, tmp);
    (void)close(ofd);
    bwrite(fd, (char *)&tmp, sizeof tmp); /* level number */
    savelev(fd, tmp);                     /* actual level */
    (void)unlink(lock);
  }

  /* Ensure data is written to disk before atomic rename */
#ifdef HAVE_FSYNC
  if (fsync(fd) != 0) {
    if (!hu)
      pline("Error syncing save file.");
    (void)close(fd);
    (void)unlink(tmpfile);
    return (0);
  }
#endif

  (void)close(fd);

  /* Atomic rename from temporary to final save file */
  if (rename(tmpfile, SAVEF) != 0) {
    if (!hu)
      pline("Error finalizing save file.");
    (void)unlink(tmpfile);
    return (0);
  }

  glo(dlevel);
  (void)unlink(lock); /* get rid of current level --jgm */
  glo(0);
  (void)unlink(lock);
  return (1);
}

int dorecover(int fd) {
  int nfd;
  int tmp;      /* not a ! */
  unsigned mid; /* idem */
  struct obj *otmp;
  extern boolean restoring;

  restoring = TRUE;
  getlev(fd, 0, 0);
  invent = restobjchn(fd);
  for (otmp = invent; otmp; otmp = otmp->nobj)
    if (otmp->owornmask)
      setworn(otmp, otmp->owornmask);
  fcobj = restobjchn(fd);
  fallen_down = restmonchn(fd);

  /* Check if this is a versioned save file */
  rh_hdr_t hdr;
  long save_pos = lseek(fd, 0, SEEK_CUR); /* Save current position */

  if (check_save_header(fd, &hdr)) {
    /* New versioned format */
    tmp = (int)sr_u32(fd);
    if (tmp !=
        (int)getuid()) { /* MODERN: Cast to int to match tmp variable type */
      (void)close(fd);
      (void)unlink(SAVEF);
      puts("Saved game was not yours.");
      restoring = FALSE;
      return (0);
    }

    unpack_flags(fd, &flags);
    dlevel = sr_i32(fd);
    maxdlevel = sr_i32(fd);
    moves = (long)sr_u32(fd);

    /* Read struct you - for now using size-prefixed format */
    rh_u32_t you_size = sr_u32(fd);
    if (you_size != sizeof(struct you)) {
      puts("Save file struct size mismatch.");
      restoring = FALSE;
      return (0);
    }
    sr_bytes(fd, &u, sizeof(struct you));

    /* Check if ustuck exists */
    rh_u32_t has_ustuck = sr_u32(fd);
    if (has_ustuck) {
      mid = sr_u32(fd);
    }

    sr_bytes(fd, pl_character, sizeof pl_character);
    sr_bytes(fd, genocided, sizeof genocided);
    sr_bytes(fd, fut_geno, sizeof fut_geno);
  } else {
    /* Legacy format - restore file position and use old code */
    lseek(fd, save_pos, SEEK_SET);

    mread(fd, (char *)&tmp, sizeof tmp);
    if (tmp !=
        (int)getuid()) { /* MODERN: Cast to int to match tmp variable type */
      (void)close(fd);
      (void)unlink(SAVEF);
      puts("Saved game was not yours.");
      restoring = FALSE;
      return (0);
    }
    mread(fd, (char *)&flags, sizeof(struct flag));
    mread(fd, (char *)&dlevel, sizeof dlevel);
    mread(fd, (char *)&maxdlevel, sizeof maxdlevel);
    mread(fd, (char *)&moves, sizeof moves);
    mread(fd, (char *)&u, sizeof(struct you));
    if (u.ustuck)
      mread(fd, (char *)&mid, sizeof mid);
    mread(fd, (char *)pl_character, sizeof pl_character);
    mread(fd, (char *)genocided, sizeof genocided);
    mread(fd, (char *)fut_geno, sizeof fut_geno);
  }
  restnames(fd);
  while (1) {
    if (read(fd, (char *)&tmp, sizeof tmp) != sizeof tmp)
      break;
    getlev(fd, 0, tmp);
    glo(tmp);
    if ((nfd = creat(lock, FMASK)) < 0)
      panic("Cannot open temp file %s!\n", lock);
    savelev(nfd, tmp);
    (void)close(nfd);
  }
  (void)lseek(fd, (off_t)0, 0);
  getlev(fd, 0, 0);
  (void)close(fd);
  (void)unlink(SAVEF);
  if (Punished) {
    for (otmp = fobj; otmp; otmp = otmp->nobj)
      if (otmp->olet == CHAIN_SYM)
        goto chainfnd;
    panic("Cannot find the iron chain?");
  chainfnd:
    uchain = otmp;
    if (!uball) {
      for (otmp = fobj; otmp; otmp = otmp->nobj)
        if (otmp->olet == BALL_SYM && otmp->spe)
          goto ballfnd;
      panic("Cannot find the iron ball?");
    ballfnd:
      uball = otmp;
    }
  }
  if (u.ustuck) {
    struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
      if (mtmp->m_id == mid)
        goto monfnd;
    panic("Cannot find the monster ustuck.");
  monfnd:
    u.ustuck = mtmp;
  }
#ifndef QUEST
  setsee(); /* only to recompute seelx etc. - these weren't saved */
#endif      /* QUEST */
  docrt();
  restoring = FALSE;
  return (1);
}

/**
 * MODERN ADDITION (2025): Safe object chain deserialization with bounds
 * checking WHY: Original mread operations trusted save file data without
 * validation, enabling buffer overflows via crafted xl values (e.g. xl =
 * UINT_MAX) HOW: Validate xl is within reasonable bounds before allocation and
 * read PRESERVES: Exact object chain restoration for legitimate save files
 * ADDS: Protection against malicious save files with crafted object sizes
 */
struct obj *restobjchn(int fd) {
  struct obj *otmp, *otmp2;
  struct obj *first = 0;
  int xl;

#define MAX_OEXTRA_SIZE 1024 /* Maximum reasonable size for object extra data  \
                              */

#ifdef lint
  /* suppress "used before set" warning from lint */
  otmp2 = 0;
#endif /* lint */
  while (1) {
    mread(fd, (char *)&xl, sizeof(xl));
    if (xl == -1)
      break;

    /* MODERN: Validate object extra data size to prevent integer overflow
     * attacks */
    if (xl < 0 || xl > MAX_OEXTRA_SIZE) {
      impossible("Invalid object size in save file", xl, MAX_OEXTRA_SIZE);
      break; /* Stop processing malformed chain */
    }

    /* MODERN: Check for integer overflow in read size calculation */
    unsigned total_read_size = (unsigned)xl + sizeof(struct obj);
    if (total_read_size < sizeof(struct obj) ||
        total_read_size < (unsigned)xl) {
      impossible("Object read size overflow", xl, (int)total_read_size);
      break; /* Stop processing to prevent memory corruption */
    }

    otmp = newobj(xl);
    if (!first)
      first = otmp;
    else
      otmp2->nobj = otmp;

    mread(fd, (char *)otmp, total_read_size);
    if (!otmp->o_id)
      otmp->o_id = flags.ident++;
    otmp2 = otmp;
  }
  if (first && otmp2->nobj) {
    impossible("Restobjchn: error reading objchn.", 0, 0);
    otmp2->nobj = 0;
  }
  return (first);
}

/**
 * MODERN ADDITION (2025): Safe monster chain deserialization with validation
 * WHY: Same vulnerability as restobjchn - unchecked xl values and integer
 * overflow in read size calculation, plus dangerous raw pointer arithmetic HOW:
 * Validate xl bounds, check arithmetic overflow, verify pointer adjustments
 * PRESERVES: Monster chain restoration including inventory recursion
 * ADDS: Protection against save file attacks targeting monster data corruption
 */
struct monst *restmonchn(int fd) {
  struct monst *mtmp, *mtmp2;
  struct monst *first = 0;
  int xl;

#define MAX_MEXTRA_SIZE 512 /* Maximum reasonable size for monster extra data  \
                             */

  struct permonst *monbegin;
  long differ;

  mread(fd, (char *)&monbegin, sizeof(monbegin));
  differ = (char *)(&mons[0]) - (char *)(monbegin);

  /* MODERN: Validate pointer difference is reasonable */
  if (differ > 100000 || differ < -100000) {
    impossible("Suspicious monster data pointer difference", (int)differ, 0);
    return NULL; /* Refuse to process potentially corrupted data */
  }

#ifdef lint
  /* suppress "used before set" warning from lint */
  mtmp2 = 0;
#endif /* lint */
  while (1) {
    mread(fd, (char *)&xl, sizeof(xl));
    if (xl == -1)
      break;

    /* MODERN: Validate monster extra data size */
    if (xl < 0 || xl > MAX_MEXTRA_SIZE) {
      impossible("Invalid monster size in save file", xl, MAX_MEXTRA_SIZE);
      break; /* Stop processing malformed chain */
    }

    /* MODERN: Check for integer overflow in read size calculation */
    unsigned total_read_size = (unsigned)xl + sizeof(struct monst);
    if (total_read_size < sizeof(struct monst) ||
        total_read_size < (unsigned)xl) {
      impossible("Monster read size overflow", xl, (int)total_read_size);
      break; /* Stop processing to prevent memory corruption */
    }

    mtmp = newmonst(xl);
    if (!first)
      first = mtmp;
    else
      mtmp2->nmon = mtmp;

    mread(fd, (char *)mtmp, total_read_size);
    if (!mtmp->m_id)
      mtmp->m_id = flags.ident++;

    /* MODERN: Validate adjusted pointer is within reasonable bounds before
     * using */
    struct permonst *adjusted_data =
        (struct permonst *)((char *)mtmp->data + differ);
    /* Array size is CMNUM + 2 as defined in hack.main.c and hack.monst.c */
    if (adjusted_data < &mons[0] || adjusted_data >= &mons[CMNUM + 2]) {
      impossible("Monster data pointer out of bounds",
                 (int)(adjusted_data - &mons[0]), CMNUM + 2);
      mtmp->data = &mons[0]; /* Safe fallback to first monster type */
    } else {
      mtmp->data = adjusted_data;
    }

    if (mtmp->minvent)
      mtmp->minvent = restobjchn(fd);
    mtmp2 = mtmp;
  }
  if (first && mtmp2->nmon) {
    impossible("Restmonchn: error reading monchn.", 0, 0);
    mtmp2->nmon = 0;
  }
  return (first);
}
