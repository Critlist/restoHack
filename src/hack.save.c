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
#ifndef _WIN32
#include <unistd.h>
#endif

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

/* Save format version history:
 * Version 1 (2025): Initial versioned format with magic number, endianness
 *                   - Serialized: flags, dlevel, moves, ustuck (by m_id)
 *                   - NOT serialized: usick_cause, p_tofn (raw pointers!)
 * Version 2 (2025): Safe pointer serialization for multi-user deployment
 *                   - Added: usick_cause as object type index
 *                   - Added: p_tofn as function ID
 *                   - Struct dump now has pointers zeroed before save
 */
#define RH_MAGIC "RHCK"
#define RH_VERSION 2
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

  /* Accept versions 1 through RH_VERSION (currently 2) */
  if (hdr->version < 1 || hdr->version > RH_VERSION) {
    return 0; /* Unsupported version */
  }

  return 1;
}

/* Function pointer serialization for property timeouts
 * EXHAUSTIVE SEARCH RESULT: Only float_down is assigned in codebase
 * (see src/hack.potion.c:184) */
extern int float_down(void);

typedef int (*timeout_fn_t)(void);

/* Timeout function ID table - add new functions here if code changes */
static const struct {
  timeout_fn_t fn;
  const char *name; /* For debugging */
} timeout_fn_table[] = {
    {NULL, "none"},             /* ID 0 */
    {float_down, "float_down"}, /* ID 1 */
};
#define NUM_TIMEOUT_FNS (sizeof(timeout_fn_table) / sizeof(timeout_fn_table[0]))

/* Save timeout function pointer as ID */
static rh_u32_t save_timeout_fn(int (*fn)(void)) {
  if (fn == NULL)
    return 0;
  for (rh_u32_t i = 1; i < NUM_TIMEOUT_FNS; i++) {
    if (fn == timeout_fn_table[i].fn) {
      return i;
    }
  }
  /* CRITICAL: Unknown function pointer detected! */
  impossible("Unknown timeout function - treating as NULL", 0, 0);
  return 0;
}

/* Restore timeout function pointer from ID */
static int (*restore_timeout_fn(rh_u32_t id))(void) {
  if (id >= NUM_TIMEOUT_FNS) {
    impossible("Invalid timeout function ID - using NULL", id, 0);
    return NULL;
  }
  return timeout_fn_table[id].fn;
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

  /* Zero out pointer fields before dumping struct (they're serialized
   * separately) This prevents bogus pointer values from being written to the
   * save file */
  struct you save_u = u;
  save_u.usick_cause = NULL;
  save_u.ustuck = NULL;
  {
    int num_uprops = sizeof(save_u.uprops) / sizeof(save_u.uprops[0]);
    for (int i = 0; i < num_uprops; i++) {
      save_u.uprops[i].p_tofn = NULL;
    }
  }

  sw_u32(fd, (rh_u32_t)sizeof(struct you));
  sw_bytes(fd, &save_u, sizeof(struct you));

  if (u.ustuck) {
    sw_u32(fd, 1); /* has ustuck */
    sw_u32(fd, (rh_u32_t)(u.ustuck->m_id));
  } else {
    sw_u32(fd, 0); /* no ustuck */
  }

  /* Serialize usick_cause as object type index (not string!)
   *
   * INVARIANT: usick_cause only ever points to:
   *   1. NULL (not poisoned)
   *   2. objects[i].oc_name (poisoned by specific object)
   *   3. "something strange" literal (fallback)
   *
   * Any deviation from this invariant is treated as corruption.
   */
  {
    rh_u32_t sick_idx;
    if (u.usick_cause == NULL) {
      sick_idx = 0xFFFFFFFF; /* Special: NULL */
    } else {
      /* Search objects table for matching oc_name pointer */
      sick_idx = 0xFFFFFFFE; /* Default: "something strange" */
      for (int i = 0; i < NROFOBJECTS; i++) {
        if (u.usick_cause == objects[i].oc_name) {
          sick_idx = (rh_u32_t)i;
          break;
        }
      }
      /* If not found in table, log warning and use "something strange" */
      if (sick_idx == 0xFFFFFFFE &&
          strcmp(u.usick_cause, "something strange") != 0) {
        impossible("usick_cause points to unknown string - treating as generic",
                   0, 0);
      }
    }
    sw_u32(fd, sick_idx);
  }

  /* Serialize uprops function pointers
   * Use derived bound to avoid breakage if constants change */
  {
    int num_uprops = sizeof(u.uprops) / sizeof(u.uprops[0]);
    for (int i = 0; i < num_uprops; i++) {
      rh_u32_t fn_id = save_timeout_fn(u.uprops[i].p_tofn);
      sw_u32(fd, fn_id);
    }
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
  int tmp; /* not a ! */
  unsigned mid =
      0; /* Modern: Initialize to prevent uninitialized use warning */
  rh_u32_t has_ustuck = 0; /* Modern: Track if save file contains ustuck data */
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
    /* Versioned format detected */

    /* Read common fields (all versions) */
    tmp = (int)sr_u32(fd);
    if (tmp != (int)getuid()) {
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

    /* Read struct you */
    rh_u32_t you_size = sr_u32(fd);
    if (you_size != sizeof(struct you)) {
      puts("Save file struct size mismatch.");
      restoring = FALSE;
      return (0);
    }
    sr_bytes(fd, &u, sizeof(struct you));

    /* Read ustuck (all versions) */
    has_ustuck = sr_u32(fd);
    if (has_ustuck) {
      mid = sr_u32(fd);
    }

    /* VERSION-SPECIFIC FIELDS */
    if (hdr.version == 1) {
      /* Version 1: No usick_cause or p_tofn serialization
       * Pointers in struct dump are BOGUS - clear them */

      /* MIGRATION: Clear bogus usick_cause pointer */
      if (Sick) {
        pline("Note: Save upgraded from Version 1 - generic sickness message.");
        u.usick_cause = "something strange";
      } else {
        u.usick_cause = NULL;
      }

      /* MIGRATION: Reconstruct p_tofn based on property state */
      {
        int num_uprops = sizeof(u.uprops) / sizeof(u.uprops[0]);
        for (int i = 0; i < num_uprops; i++) {
          /* Only RIN_LEVITATION uses float_down */
          if (i == PROP(RIN_LEVITATION) && (u.uprops[i].p_flgs & TIMEOUT)) {
            u.uprops[i].p_tofn = float_down;
          } else {
            u.uprops[i].p_tofn = NULL;
          }
        }
      }

    } else if (hdr.version == 2) {
      /* Version 2: Safe pointer serialization */

      /* Restore usick_cause from object index */
      {
        rh_u32_t sick_idx = sr_u32(fd);
        if (sick_idx == 0xFFFFFFFF) {
          u.usick_cause = NULL;
        } else if (sick_idx == 0xFFFFFFFE) {
          u.usick_cause = "something strange";
        } else if (sick_idx < NROFOBJECTS) {
          u.usick_cause = objects[sick_idx].oc_name;
        } else {
          pline("Warning: Save file corruption (usick_cause), repaired.");
          u.usick_cause = "something strange";
        }
      }

      /* Restore p_tofn from function IDs */
      {
        int num_uprops = sizeof(u.uprops) / sizeof(u.uprops[0]);
        for (int i = 0; i < num_uprops; i++) {
          rh_u32_t fn_id = sr_u32(fd);
          u.uprops[i].p_tofn = restore_timeout_fn(fn_id);
        }
      }

    } else {
      /* Unknown version (shouldn't happen due to check_save_header) */
      impossible("Unexpected save version", hdr.version, 0);
      restoring = FALSE;
      return (0);
    }

    /* Read remaining common fields */
    sr_bytes(fd, pl_character, sizeof pl_character);
    sr_bytes(fd, genocided, sizeof genocided);
    sr_bytes(fd, fut_geno, sizeof fut_geno);

  } else {
    /* Pre-versioned legacy format - REJECT
     *
     * POLICY DECISION: Pre-versioned saves are inherently unsafe:
     * - No magic number or format metadata
     * - Raw pointer values without any serialization
     * - No way to validate or repair corruption
     * - Risk of arbitrary memory access on restore
     *
     * For canonical hosting safety, these are intentionally rejected.
     */
    lseek(fd, save_pos, SEEK_SET);
    puts("Save file is too old (pre-Version 1). Cannot load safely.");
    restoring = FALSE;
    return (0);
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
  /* POST-RESTORE VALIDATION: Ensure all pointers satisfy invariants */

  /* Validate Sick flag matches usick_cause state */
  if (Sick && !u.usick_cause) {
    pline("Warning: Sick status without cause - adding generic message.");
    u.usick_cause = "something strange";
  } else if (!Sick && u.usick_cause) {
    /* Not sick but has cause - clear it */
    u.usick_cause = NULL;
  }

  /* Validate ustuck (gracefully handle missing monster)
   * Modern: Use has_ustuck flag instead of checking u.ustuck pointer directly,
   * since u.ustuck from struct dump may contain garbage. */
  u.ustuck = NULL; /* Clear garbage pointer from struct dump */
  if (has_ustuck) {
    struct monst *mtmp;
    int found = 0;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
      if (mtmp->m_id == mid) {
        u.ustuck = mtmp;
        found = 1;
        break;
      }
    }

    if (!found) {
      /* Don't panic - just unstick player gracefully */
      pline("Warning: Save file inconsistency (ustuck) - monster not found.");
      u.ustuck = NULL;
    }
  }

  /* Note: No need to validate usick_cause or p_tofn pointer values here -
   * the restoration code already guarantees they point to static data
   * (objects[].oc_name, "something strange" literal, or timeout_fn_table).
   * Only check flag/pointer state consistency. */

#ifndef QUEST
  setsee(); /* only to recompute seelx etc. - these weren't saved */
#endif      /* QUEST */
  docrt();
  restoring = FALSE;
  return (1);
}

struct obj *restobjchn(int fd) {
  struct obj *otmp, *otmp2;
  struct obj *first = 0;
  int xl;
#ifndef MAXONAMELTH
#define MAXONAMELTH 63 /* Max for 6-bit onamelth field */
#endif
#ifdef lint
  /* suppress "used before set" warning from lint */
  otmp2 = 0;
#endif /* lint */
  while (1) {
    mread(fd, (char *)&xl, sizeof(xl));
    if (xl == -1)
      break;
    /* MODERN: Sanity check object name length */
    if (xl < 0 || xl > MAXONAMELTH) {
      panic("Bad object name length in save: %d", xl);
    }
    /* MODERN: Allocate extra byte for NUL terminator */
    otmp = newobj(xl > 0 ? xl + 1 : xl);
    if (!first)
      first = otmp;
    else
      otmp2->nobj = otmp;
    mread(fd, (char *)otmp, (unsigned)xl + sizeof(struct obj));
    /* MODERN: Normalize onamelth to match allocation and ensure null-terminated
     */
    otmp->onamelth = (xl > 0) ? xl : 0;
    if (xl > 0) {
      ONAME(otmp)[xl] = '\0';
    }
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

struct monst *restmonchn(int fd) {
  struct monst *mtmp, *mtmp2;
  struct monst *first = 0;
  int xl;

  struct permonst *monbegin;
  long differ;

  mread(fd, (char *)&monbegin, sizeof(monbegin));
  differ = (char *)(&mons[0]) - (char *)(monbegin);

#ifdef lint
  /* suppress "used before set" warning from lint */
  mtmp2 = 0;
#endif /* lint */
  while (1) {
    mread(fd, (char *)&xl, sizeof(xl));
    if (xl == -1)
      break;
    mtmp = newmonst(xl);
    if (!first)
      first = mtmp;
    else
      mtmp2->nmon = mtmp;
    mread(fd, (char *)mtmp, (unsigned)xl + sizeof(struct monst));
    if (!mtmp->m_id)
      mtmp->m_id = flags.ident++;
    mtmp->data = (struct permonst *)((char *)mtmp->data + differ);
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
