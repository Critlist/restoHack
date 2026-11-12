/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.save.c - version 1.0.3 */
/* $FreeBSD$ */

#include "hack.h"
#include "def.edog.h" /* Modern: v3 validation */
#include "def.eshk.h" /* Modern: v3 validation */
#include "def.wseg.h" /* Modern: v3 validation */
#include <errno.h> /* Modern: errno for the outdated-save check */
#include <fcntl.h>
#include <signal.h>
#include <stddef.h> /* Modern: offsetof */
#include <stdint.h>
#include <stdlib.h> /* Modern: malloc */
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

/* Save format version history:
 * Version 1 (2025): Initial versioned format with magic number, endianness
 *                   - Serialized: flags, dlevel, moves, ustuck (by m_id)
 *                   - NOT serialized: usick_cause, p_tofn (raw pointers!)
 * Version 2 (2025): Safe pointer serialization for multi-user deployment
 *                   - Added: usick_cause as object type index
 *                   - Added: p_tofn as function ID
 *                   - Struct dump now has pointers zeroed before save
 *                   - Header written in the middle of the file, after the
 *                     current level; monster types still raw pointers.
 * Version 3 (2026): Header first, build fingerprint, length + CRC32, stable
 *                   monster type IDs; the same header on bones files.
 *                   Version 1 and 2 saves are not migrated automatically
 *                   (see "Saved Games" in README.md); they are kept aside.
 *
 * Bump RH_VERSION for every deliberate change to what is written. The build
 * fingerprint (rh_fingerprint()) catches layout and table changes, but not a
 * field whose meaning changes while its size and place stay the same.
 */
#define RH_MAGIC "RHCK"
#define RH_VERSION 3
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
  bwrite(fd, data, len);
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
  if (f->qwertz)
    packed2 |= 0x40000000; /* Modern: qwertz, spare bit 30; old saves read 0 */
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
  f->qwertz = (packed2 & 0x40000000) ? 1 : 0; /* Modern: qwertz */
  f->no_of_wizards = (packed2 >> 25) & 3;
  f->moonphase = (packed2 >> 27) & 7;

  f->end_top = sr_u32(fd);
  f->end_around = sr_u32(fd);
}

/* Function pointer serialization for property timeouts
 * EXHAUSTIVE SEARCH RESULT: Only float_down is assigned in codebase
 * (see src/hack.potion.c:184) */

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
extern struct obj *restobjchn(int fd);
extern struct monst *restmonchn(int fd);
extern int dosave0(int hu);
extern void savenames(int fd);
extern void restnames(int fd);
extern int descr_perm_count(void);
extern int get_descr_perm(unsigned *perm, int n);
extern int set_descr_perm(const unsigned *perm, int n);

/**
 * MODERN ADDITION (2026): Object description block at the end of the save
 * WHY: The shuffled potion/scroll/wand/ring/gem appearances were lost on
 *      restore (see hack.o_init.c), so restored games showed the defaults.
 * HOW: After the last saved level, dosave0() writes
 *        int  RH_DESCR_TAG        (native int, like the level numbers)
 *        u32  RH_DESCR_VERSION    (big-endian, like the other sw_u32 fields)
 *        u32  count               (must equal SIZE(objects))
 *        u32  perm[count]         (default-description index, or 0xFFFFFFFF)
 *      dorecover()'s level loop reads a level number before each level and
 *      stops at end of file, so the tag sits where the next level number
 *      would be; it can never be a valid level (1..MAXLEVEL).
 * PRESERVES: The existing stream layout. Saves without the block (written
 *      before this change) still end at the last level and load as before,
 *      with default descriptions.
 * ADDS: Restored games keep their own appearances. A bad or truncated block
 *      is ignored (defaults are kept) instead of aborting the restore.
 */
#define RH_DESCR_TAG 0x52484453 /* "RHDS" */
#define RH_DESCR_VERSION 1

/* Modern: Write the description block (nothing if the shuffle is unknown) */
static void save_descr_block(int fd) {
  int n = descr_perm_count();
  int tag = RH_DESCR_TAG;
  unsigned *perm = (unsigned *)alloc((unsigned)n * sizeof(unsigned));

  if (get_descr_perm(perm, n)) {
    bwrite(fd, (char *)&tag, sizeof tag);
    sw_u32(fd, RH_DESCR_VERSION);
    sw_u32(fd, (rh_u32_t)n);
    for (int i = 0; i < n; i++)
      sw_u32(fd, (rh_u32_t)perm[i]);
  }
  free(perm);
}

/* Modern: Like sr_u32() but returns 0 on a short read instead of aborting */
static int sr_u32_try(int fd, rh_u32_t *val) {
  unsigned char buf[4];
  if (read(fd, (char *)buf, 4) != 4)
    return (0);
  *val = ((rh_u32_t)buf[0] << 24) | ((rh_u32_t)buf[1] << 16) |
         ((rh_u32_t)buf[2] << 8) | buf[3];
  return (1);
}

/* Modern: Read the description block (tag already consumed) and apply it if
 * it is valid; otherwise the default descriptions stay in place. */
static void rest_descr_block(int fd) {
  int n = descr_perm_count();
  rh_u32_t version, count, val;
  unsigned *perm;
  int ok = 1;

  if (!sr_u32_try(fd, &version) || version != RH_DESCR_VERSION ||
      !sr_u32_try(fd, &count) || count != (rh_u32_t)n)
    ok = 0;
  if (ok) {
    perm = (unsigned *)alloc((unsigned)n * sizeof(unsigned));
    for (int i = 0; i < n; i++) {
      if (!sr_u32_try(fd, &val)) {
        ok = 0;
        break;
      }
      perm[i] = (unsigned)val;
    }
    if (ok)
      ok = set_descr_perm(perm, n);
    free(perm);
  }
  if (!ok)
    pline("Warning: object descriptions in save file unusable - using "
          "defaults.");
}

/**
 * MODERN ADDITION (2026): Save format version 3
 * WHY: A version 2 save could not survive a game update. Its header sat
 *      after the first level, so a level was read before anything was
 *      checked; monster types were raw pointers, which are only relocated
 *      correctly for monsters inside mons[] (pets, ghosts, eels, guards...
 *      broke with another binary); bones files had no header at all. The
 *      original code coped by deleting every save older than the binary.
 * HOW: Every save and bones file starts with a 32-byte big-endian header:
 *         0 "RHCK"                 4 u16 RH_VERSION   6 u16 kind
 *         8 u32 RH_ENDIANTAG      12 u32 build fingerprint, high half
 *        16 u32 fingerprint, low  20 u32 payload length
 *        24 u32 payload CRC-32    28 u32 reserved (0)
 *      rh_check_file() checks the header, the length and the CRC and then
 *      walks the whole payload with the v_*() validators, which bound every
 *      length, count, index and map position, all before the real loader
 *      runs. Monster types are written as stable IDs (rh_mon_id()).
 * PRESERVES: After the header the stream is the version 2 stream without
 *      the old mid-file header and without the monbegin pointers; levels,
 *      objects and struct you are still raw dumps, as in 1985.
 * ADDS: Saves and bones load in any build with the same fingerprint, and
 *      anything else is refused before game state is touched. Refused saves
 *      are kept (move_outdated_save()), never deleted.
 */
/* RH_KIND_SAVE, RH_KIND_BONES and RH_HDR_LEN are in hack.h */
#define RH_HDR_LEN_OFF 20 /* offset of the payload length field */
#define RH_MAX_PAYLOAD (16UL * 1024 * 1024) /* far above any real save */
#define RH_MAX_CHAIN 10000L /* entries in any one saved chain */
#define RH_MAX_MXL 8192     /* monster extra data + name */
#define RH_MAX_ENGR 32767U  /* engraving text, as in rest_engravings() */

/* Modern: monster types that live outside mons[] */
extern struct permonst li_dog, dog, la_dog, hell_hound, pm_wizard;
extern long wgrowtime[32];
void clearlocks(void); /* Modern: hack.end.c */

/**
 * MODERN ADDITION (2026): Stable monster type IDs
 * WHY: monst->data points either into mons[] or at one of the separate
 *      permonst objects below; the version 2 relocation by &mons[0] only
 *      worked for the former.
 * HOW: ID 0 .. CMNUM+1 is the mons[] index; RH_SPECIAL_BASE + n is entry n
 *      of rh_special_pm(). That list is APPEND ONLY: never reorder or reuse
 *      an entry. These are every permonst object outside mons[] (all
 *      assignments to ->data were checked: makemon(), newcham(), the dog
 *      growth in hack.fight.c and PM_CHAMELEON in hack.mon.c).
 * PRESERVES: The 1985 tables themselves are unchanged.
 * ADDS: Unknown IDs are refused instead of producing wild pointers.
 */
#define RH_SPECIAL_BASE 1000U
#define RH_NSPECIAL 9U
#define RH_BAD_ID 0xFFFFFFFFU

static struct permonst *rh_special_pm(unsigned n) {
  switch (n) {
  case 0:
    return (&pm_ghost);
  case 1:
    return (&pm_wizard);
  case 2:
    return (&pm_eel);
#ifdef MAIL
  case 3:
    return (&pm_mail_daemon);
#endif /* MAIL */
  case 4:
    return (&li_dog);
  case 5:
    return (&dog);
  case 6:
    return (&la_dog);
  case 7:
    return (&hell_hound);
  case 8:
    return (vault_guard_pm());
  default:
    return (NULL); /* 3 without MAIL: reserved */
  }
}

/* Modern: stable ID of a monster type, RH_BAD_ID if it is not known */
unsigned rh_mon_id(const struct permonst *pm) {
  unsigned i;

  for (i = 0; i < CMNUM + 2; i++)
    if (pm == &mons[i])
      return (i);
  for (i = 0; i < RH_NSPECIAL; i++)
    if (pm && pm == rh_special_pm(i))
      return (RH_SPECIAL_BASE + i);
  return (RH_BAD_ID);
}

/* Modern: monster type for a stable ID, NULL if the ID is unknown */
struct permonst *rh_mon_pm(unsigned id) {
  if (id < CMNUM + 2)
    return (&mons[id]);
  if (id >= RH_SPECIAL_BASE && id - RH_SPECIAL_BASE < RH_NSPECIAL)
    return (rh_special_pm(id - RH_SPECIAL_BASE));
  return (NULL);
}

/* Modern: payload length and CRC-32, summed by bwrite() while rh_sum_on */
int rh_sum_on = 0;
static rh_u32_t rh_sum_len, rh_sum_crc;

static rh_u32_t rh_crc32(rh_u32_t crc, const unsigned char *p,
                         unsigned long n) {
  int k;

  while (n--) {
    crc ^= *p++;
    for (k = 0; k < 8; k++)
      crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
  }
  return (crc);
}

void rh_sum_add(const void *loc, unsigned num) {
  rh_sum_crc = rh_crc32(rh_sum_crc, (const unsigned char *)loc, num);
  rh_sum_len += num;
}

/**
 * MODERN ADDITION (2026): Build fingerprint
 * WHY: A raw struct dump is only meaningful to a build with the same layout
 *      and the same tables; a size check alone misses reordered or renamed
 *      table entries, other ABIs and changed constants.
 * HOW: FNV-1a 64 over the ABI (byte order, type sizes, bit-field layout
 *      probe), the size of every raw structure in the stream, the offsets
 *      of the fields the loader relies on, the constants that size saved
 *      arrays, the identity of every mons[], special monster and objects[]
 *      entry (name, letter/class, extra size) in order, and the timeout
 *      function table. The 1985 mons[] and objects[] tables must not be
 *      edited, not even to fix a typo: that changes the fingerprint and
 *      every existing save is then refused (kept aside).
 * PRESERVES: Nothing in the game uses it except save and bones checks.
 * ADDS: Catches layout, ABI and table changes. It cannot catch a field
 *      whose meaning changes while its size and place stay the same: bump
 *      RH_VERSION for those.
 */
static uint64_t rh_fp;

static void fp_bytes(const void *p, size_t n) {
  const unsigned char *b = (const unsigned char *)p;

  while (n--) {
    rh_fp ^= *b++;
    rh_fp *= 0x100000001b3ULL;
  }
}

static void fp_num(unsigned long v) {
  unsigned char b[4];

  b[0] = (unsigned char)(v >> 24);
  b[1] = (unsigned char)(v >> 16);
  b[2] = (unsigned char)(v >> 8);
  b[3] = (unsigned char)v;
  fp_bytes(b, 4);
}

static void fp_str(const char *s) {
  if (s)
    fp_bytes(s, strlen(s) + 1);
  else
    fp_num(0xFFFFFFFFUL);
}

static void fp_pm(const struct permonst *pm) {
  if (!pm) {
    fp_str(NULL);
    return;
  }
  fp_str(pm->mname);
  fp_num((unsigned char)pm->mlet);
  fp_num(pm->pxlth);
}

struct rh_abi_probe {
  unsigned a : 1; /* SysV packs c after this bit-field, MS layout does not */
  unsigned char c;
};

static void rh_fingerprint(rh_u32_t *hi, rh_u32_t *lo) {
  static boolean done = FALSE;
  static uint64_t fp;
  unsigned i;
  int n;
  uint32_t endian = RH_ENDIANTAG;

  if (!done) {
    rh_fp = 0xcbf29ce484222325ULL;
    fp_str("restoHack save");
    fp_num(RH_VERSION);
    fp_bytes(&endian, sizeof endian); /* native byte order */
    fp_num(sizeof(short));
    fp_num(sizeof(int));
    fp_num(sizeof(long));
    fp_num(sizeof(void *));
    fp_num(offsetof(struct rh_abi_probe, c));
    fp_num(sizeof(struct rh_abi_probe));
    /* raw structures and arrays in the stream */
    fp_num(sizeof(struct you));
    fp_num(sizeof(struct obj));
    fp_num(sizeof(struct monst));
    fp_num(sizeof(struct gold));
    fp_num(sizeof(struct trap));
    fp_num(sizeof(struct rm));
    fp_num(sizeof(levl));
    fp_num(sizeof(rooms));
    fp_num(sizeof(doors));
    fp_num(sizeof(struct permonst));
    fp_num(sizeof(struct objclass));
    fp_num(sizeof(struct eshk));
    fp_num(sizeof(struct edog));
    fp_num(engr_rec_size());
    fp_num(names_fixed_size());
    fp_num(sizeof(u.uprops));
    fp_num(sizeof(pl_character));
    fp_num(sizeof(genocided));
    fp_num(sizeof(fut_geno));
    fp_num(sizeof(xupstair));
#ifndef NOWORM
    fp_num(sizeof(struct wseg));
    fp_num(sizeof(wsegs));
    fp_num(offsetof(struct wseg, nseg));
    fp_num(offsetof(struct monst, mtrapseen)); /* after wormno */
#endif /* NOWORM */
    /* fields the loader relies on */
    fp_num(offsetof(struct monst, nmon));
    fp_num(offsetof(struct monst, data));
    fp_num(offsetof(struct monst, m_id));
    fp_num(offsetof(struct monst, mx));
    fp_num(offsetof(struct monst, minvent));
    fp_num(offsetof(struct monst, mxlth));
    fp_num(offsetof(struct monst, mextra));
    fp_num(offsetof(struct obj, nobj));
    fp_num(offsetof(struct obj, o_id));
    fp_num(offsetof(struct obj, ox));
    fp_num(offsetof(struct obj, otyp));
    fp_num(offsetof(struct obj, olet));
    fp_num(offsetof(struct obj, owornmask));
    fp_num(offsetof(struct obj, oextra));
    fp_num(offsetof(struct gold, gx));
    fp_num(offsetof(struct trap, tx));
    fp_num(offsetof(struct you, ux));
    fp_num(offsetof(struct you, usick_cause));
    fp_num(offsetof(struct you, ustuck));
    fp_num(offsetof(struct you, uprops));
    fp_num(offsetof(struct eshk, billct));
    /* constants that size or index saved data */
    fp_num(CMNUM);
    fp_num(NROFOBJECTS);
    fp_num(LAST_RING);
    fp_num(MAXLEVEL);
    fp_num(COLNO);
    fp_num(ROWNO);
    fp_num(PL_NSIZ);
    fp_num(MAXNROFROOMS);
    fp_num(DOORMAX);
    fp_num(BILLSZ);
    fp_num(TRAPNUM);
    /* table identity, in order */
    for (i = 0; i < CMNUM + 2; i++)
      fp_pm(&mons[i]);
    fp_num(RH_NSPECIAL);
    for (i = 0; i < RH_NSPECIAL; i++)
      fp_pm(rh_special_pm(i));
    n = descr_perm_count();
    fp_num((unsigned long)n);
    for (i = 0; i < (unsigned)n; i++) {
      fp_str(objects[i].oc_name);
      fp_num((unsigned char)objects[i].oc_olet);
    }
    fp_num(NUM_TIMEOUT_FNS);
    for (i = 0; i < NUM_TIMEOUT_FNS; i++)
      fp_str(timeout_fn_table[i].name);
#ifdef RH_TEST_FP_SALT
    fp_str(RH_TEST_FP_SALT); /* Modern: test builds only, never defined */
#endif
    fp = rh_fp;
    done = TRUE;
  }
  *hi = (rh_u32_t)(fp >> 32);
  *lo = (rh_u32_t)(fp & 0xFFFFFFFFU);
}

/* Modern: start a version 3 file; bwrite() sums everything after this */
void rh_begin_file(int fd, int kind) {
  rh_u32_t hi, lo;

  rh_fingerprint(&hi, &lo);
  rh_sum_on = 0;
  sw_bytes(fd, RH_MAGIC, 4);
  sw_u16(fd, RH_VERSION);
  sw_u16(fd, (rh_u16_t)kind);
  sw_u32(fd, RH_ENDIANTAG);
  sw_u32(fd, hi);
  sw_u32(fd, lo);
  sw_u32(fd, 0); /* length, patched by rh_end_file() */
  sw_u32(fd, 0); /* CRC-32, patched by rh_end_file() */
  sw_u32(fd, 0); /* reserved */
  rh_sum_len = 0;
  rh_sum_crc = 0xFFFFFFFFU;
  rh_sum_on = 1;
}

/* Modern: finish a version 3 file: patch length and CRC into the header */
int rh_end_file(int fd) {
  unsigned char t[8];
  rh_u32_t crc = ~rh_sum_crc;

  rh_sum_on = 0;
  t[0] = (unsigned char)(rh_sum_len >> 24);
  t[1] = (unsigned char)(rh_sum_len >> 16);
  t[2] = (unsigned char)(rh_sum_len >> 8);
  t[3] = (unsigned char)rh_sum_len;
  t[4] = (unsigned char)(crc >> 24);
  t[5] = (unsigned char)(crc >> 16);
  t[6] = (unsigned char)(crc >> 8);
  t[7] = (unsigned char)crc;
  if (lseek(fd, (off_t)RH_HDR_LEN_OFF, SEEK_SET) != (off_t)RH_HDR_LEN_OFF)
    return (0);
  return (write(fd, t, sizeof t) == (ssize_t)sizeof t);
}

static rh_u32_t get_u32(const unsigned char *b) {
  return (((rh_u32_t)b[0] << 24) | ((rh_u32_t)b[1] << 16) |
          ((rh_u32_t)b[2] << 8) | b[3]);
}

/**
 * MODERN ADDITION (2026): Structural validation of a whole payload
 * WHY: The loader allocates and indexes with lengths, IDs and positions
 *      taken from the file. The CRC only detects accidental damage; a file
 *      with a valid CRC can still hold any value.
 * HOW: Walk a copy of the payload in memory, in exactly the order savelev()
 *      and dosave0() write it, checking every length against sane limits
 *      and against the bytes that remain, bounding every chain, and
 *      checking every type ID, level number and map position the loader
 *      will use as an index. Nothing is allocated or changed in the game.
 * PRESERVES: The loader itself is unchanged apart from its last-resort
 *      checks, which now keep the save instead of deleting it.
 * ADDS: A file that would make the loader misbehave is refused while the
 *      game is still in a known state.
 */
struct rh_cur {
  const unsigned char *p;
  unsigned long len, off;
  const char *err;
};

static int v_bad(struct rh_cur *c, const char *why) {
  if (!c->err)
    c->err = why;
  return (0);
}

static const unsigned char *v_take(struct rh_cur *c, unsigned long n) {
  const unsigned char *r;

  if (c->err)
    return (NULL);
  if (n > c->len - c->off) {
    (void)v_bad(c, "it is truncated");
    return (NULL);
  }
  r = c->p + c->off;
  c->off += n;
  return (r);
}

static int v_int(struct rh_cur *c, int *v) {
  const unsigned char *q = v_take(c, sizeof *v);

  if (!q)
    return (0);
  memcpy(v, q, sizeof *v);
  return (1);
}

static int v_u32(struct rh_cur *c, rh_u32_t *v) {
  const unsigned char *q = v_take(c, 4);

  if (!q)
    return (0);
  *v = get_u32(q);
  return (1);
}

static int v_onmap(int x, int y) {
  return (x >= 0 && x < COLNO && y >= 0 && y < ROWNO);
}

/* Modern: a NUL inside the first n bytes */
static int v_strz(const void *s, size_t n) { return (memchr(s, 0, n) != NULL); }

static int v_objchn(struct rh_cur *c, int onmap) {
  const unsigned char *q;
  struct obj o;
  int xl;
  long n;

  for (n = 0;; n++) {
    if (n > RH_MAX_CHAIN)
      return (v_bad(c, "an object list is too long"));
    if (!v_int(c, &xl))
      return (0);
    if (xl == -1)
      return (1);
    if (xl < 0 || xl > 63)
      return (v_bad(c, "an object name length is invalid"));
    if (!(q = v_take(c, sizeof(struct obj) + (unsigned)xl)))
      return (0);
    memcpy(&o, q, sizeof o);
    if ((int)o.otyp >= descr_perm_count())
      return (v_bad(c, "an object type is invalid"));
    if (onmap && !v_onmap(o.ox, o.oy))
      return (v_bad(c, "an object is off the map"));
  }
}

static int v_eshk(const unsigned char *x) {
  struct eshk e;

  memcpy(&e, x, sizeof e);
  return (e.billct >= 0 && e.billct <= BILLSZ && e.shoproom >= 0 &&
          e.shoproom < MAXNROFROOMS && v_onmap(e.shk.x, e.shk.y) &&
          v_onmap(e.shd.x, e.shd.y) && v_strz(e.customer, PL_NSIZ) &&
          v_strz(e.shknam, PL_NSIZ));
}

static int v_monchn(struct rh_cur *c, int onmap, unsigned long *worms) {
  const unsigned char *q, *x;
  struct permonst *pm;
  struct monst m;
  rh_u32_t id;
  int xl;
  long n;

  for (n = 0;; n++) {
    if (n > RH_MAX_CHAIN)
      return (v_bad(c, "a monster list is too long"));
    if (!v_int(c, &xl))
      return (0);
    if (xl == -1)
      return (1);
    if (xl < 0 || xl > RH_MAX_MXL)
      return (v_bad(c, "a monster record length is invalid"));
    if (!v_u32(c, &id))
      return (0);
    if (!(pm = rh_mon_pm(id)))
      return (v_bad(c, "it contains an unknown monster type"));
    if (!(q = v_take(c, sizeof(struct monst) + (unsigned)xl)))
      return (0);
    memcpy(&m, q, sizeof m);
    x = q + offsetof(struct monst, mextra);
    /* Modern: the record length first, in a type that cannot wrap, so
     * that every check below reads x only inside the xl bytes taken */
    if (m.mxlth > RH_MAX_MXL ||
        (unsigned long)m.mxlth + m.mnamelth != (unsigned long)xl)
      return (v_bad(c, "a monster record length is invalid"));
    if (m.mxlth < pm->pxlth ||
        (m.mtame && m.mxlth < sizeof(struct edog)) ||
        (m.isshk && (m.mxlth < sizeof(struct eshk) || !v_eshk(x))) ||
        (m.isgd && (m.mxlth < vault_guard_pm()->pxlth || !egd_ok(x))) ||
        (pm == &pm_ghost && !v_strz(x, PL_NSIZ)) ||
        (m.mnamelth && x[m.mxlth + m.mnamelth - 1] != 0))
      return (v_bad(c, "a monster record is invalid"));
    if (onmap) {
      if (!v_onmap(m.mx, m.my) || (m.mdispl && !v_onmap(m.mdx, m.mdy)))
        return (v_bad(c, "a monster is off the map"));
#ifndef NOWORM
      if (m.wormno)
        *worms |= 1UL << m.wormno;
#endif /* NOWORM */
    }
    if (m.minvent && !v_objchn(c, 0))
      return (0);
  }
}

/* Modern: one level as written by savelev(); returns its number or 0 */
static int v_level(struct rh_cur *c, int lev) {
  const unsigned char *q;
  struct mkroom r[MAXNROFROOMS + 1];
  coord d[DOORMAX];
  struct gold g;
  struct trap t;
  unsigned char st[4];
  unsigned lth;
  unsigned long worms = 0;
  int hpid, i, thislev;
  long n;

  if (!v_int(c, &hpid) || !(q = v_take(c, 1)))
    return (0);
  thislev = q[0];
  if (thislev < 1 || thislev > MAXLEVEL || (lev && thislev != lev))
    return (v_bad(c, "a level number is invalid"));
  if (!v_take(c, sizeof(levl)) || !v_take(c, sizeof(long)))
    return (0);
  for (i = 0; i < 4; i++) {
    if (!(q = v_take(c, sizeof(xupstair))))
      return (0);
    memcpy(&st[i], q, 1);
  }
  if (!v_onmap(st[0], st[1]) || !v_onmap(st[2], st[3]))
    return (v_bad(c, "a staircase is off the map"));
  if (!v_monchn(c, 1, &worms))
    return (0);
  for (n = 0;; n++) {
    if (n > RH_MAX_CHAIN || !(q = v_take(c, sizeof g)))
      return (v_bad(c, "the gold list is invalid"));
    memcpy(&g, q, sizeof g);
    if (!g.gx)
      break;
    if (!v_onmap(g.gx, g.gy))
      return (v_bad(c, "gold is off the map"));
  }
  for (n = 0;; n++) {
    if (n > RH_MAX_CHAIN || !(q = v_take(c, sizeof t)))
      return (v_bad(c, "the trap list is invalid"));
    memcpy(&t, q, sizeof t);
    if (!t.tx)
      break;
    if (!v_onmap(t.tx, t.ty) || t.ttyp >= TRAPNUM)
      return (v_bad(c, "a trap is invalid"));
  }
  if (!v_objchn(c, 1) || !v_objchn(c, 0))
    return (0);
  for (n = 0;; n++) {
    if (n > RH_MAX_CHAIN || !(q = v_take(c, sizeof lth)))
      return (v_bad(c, "the engraving list is invalid"));
    memcpy(&lth, q, sizeof lth);
    if (!lth)
      break;
    if (lth > RH_MAX_ENGR)
      return (v_bad(c, "an engraving is too long"));
    if (!(q = v_take(c, engr_rec_size() + lth)))
      return (0);
    if (!engr_rec_ok(q, lth))
      return (v_bad(c, "an engraving is invalid"));
  }
#ifndef QUEST
  if (!(q = v_take(c, sizeof(rooms))))
    return (0);
  memcpy(r, q, sizeof r);
  for (i = 0; i <= MAXNROFROOMS && r[i].hx >= 0; i++)
    if (r[i].lx < 0 || r[i].lx > r[i].hx || r[i].hx >= COLNO ||
        r[i].ly < 0 || r[i].ly > r[i].hy || r[i].hy >= ROWNO ||
        r[i].fdoor < 0 || r[i].doorct < 0 ||
        r[i].fdoor + r[i].doorct > DOORMAX)
      return (v_bad(c, "a room is invalid"));
  if (i > MAXNROFROOMS) /* the game scans rooms[] until hx < 0 */
    return (v_bad(c, "the room list has no end"));
  if (!(q = v_take(c, sizeof(doors))))
    return (0);
  memcpy(d, q, sizeof d);
  for (i = 0; i < DOORMAX; i++)
    if (!v_onmap(d[i].x, d[i].y))
      return (v_bad(c, "a door is off the map"));
#endif /* QUEST */
#ifndef NOWORM
  {
    struct wseg *ws[32];
    struct wseg s;
    unsigned long present = 0;

    if (!(q = v_take(c, sizeof(wsegs))))
      return (0);
    memcpy(ws, q, sizeof ws);
    for (i = 1; i < 32; i++) {
      if (!ws[i])
        continue;
      present |= 1UL << i;
      for (n = 0;; n++) {
        if (n > RH_MAX_CHAIN || !(q = v_take(c, sizeof s)))
          return (v_bad(c, "a worm is invalid"));
        memcpy(&s, q, sizeof s);
        if (!v_onmap(s.wx, s.wy))
          return (v_bad(c, "a worm is off the map"));
        if (!s.nseg)
          break;
      }
    }
    if (worms & ~present)
      return (v_bad(c, "a worm has no tail"));
    if (!v_take(c, sizeof(wgrowtime)))
      return (0);
  }
#endif /* NOWORM */
  return (thislev);
}

/* Modern: a whole save as written by dosave0() */
static int v_save(struct rh_cur *c) {
  const unsigned char *q;
  struct you yu;
  rh_u32_t v, dl, mdl, i, nprops;
  unsigned long dummy = 0, used = 0;
  int lev0, t, prev = 0, n = descr_perm_count();

  if (!(lev0 = v_level(c, 0)) || !v_objchn(c, 0) || !v_objchn(c, 0) ||
      !v_monchn(c, 0, &dummy))
    return (0);
  if (!v_u32(c, &v))
    return (0);
  if (v != (rh_u32_t)getuid())
    return (v_bad(c, "it belongs to another user"));
  if (!v_take(c, 16) || !v_u32(c, &dl) || !v_u32(c, &mdl) || !v_u32(c, &v))
    return (0);
  if (dl < 1 || dl > MAXLEVEL || mdl < dl || mdl > MAXLEVEL ||
      dl != (rh_u32_t)lev0)
    return (v_bad(c, "the dungeon level is invalid"));
  if (!v_u32(c, &v))
    return (0);
  if (v != sizeof(struct you))
    return (v_bad(c, "the player record has the wrong size"));
  if (!(q = v_take(c, sizeof(struct you))))
    return (0);
  memcpy(&yu, q, sizeof yu);
  if (!v_onmap(yu.ux, yu.uy) || (yu.udispl && !v_onmap(yu.udisx, yu.udisy)) ||
      yu.ulevel < 1 || yu.ulevel > 14)
    return (v_bad(c, "the player record is invalid"));
  if (!v_u32(c, &v) || v > 1 || (v && !v_u32(c, &v)))
    return (v_bad(c, "the engulfing monster is invalid"));
  if (!v_u32(c, &v) ||
      (v != 0xFFFFFFFFU && v != 0xFFFFFFFEU && v >= NROFOBJECTS))
    return (v_bad(c, "the cause of sickness is invalid"));
  nprops = sizeof(u.uprops) / sizeof(u.uprops[0]);
  for (i = 0; i < nprops; i++)
    if (!v_u32(c, &v) || v >= NUM_TIMEOUT_FNS)
      return (v_bad(c, "a timeout is invalid"));
  if (!(q = v_take(c, sizeof pl_character)) || !v_strz(q, sizeof pl_character) ||
      !(q = v_take(c, sizeof genocided)) || !v_strz(q, sizeof genocided) ||
      !(q = v_take(c, sizeof fut_geno)) || !v_strz(q, sizeof fut_geno))
    return (v_bad(c, "the character data is invalid"));
  if (c->err || !names_check(c->p + c->off, c->len - c->off, &used))
    return (v_bad(c, "the object names are invalid"));
  c->off += used;
  while (c->off < c->len) {
    if (!v_int(c, &t))
      return (0);
    if (t == RH_DESCR_TAG) {
      if (!v_u32(c, &v) || v != RH_DESCR_VERSION || !v_u32(c, &v) ||
          v != (rh_u32_t)n || !v_take(c, 4UL * (unsigned long)n))
        return (v_bad(c, "the object descriptions are invalid"));
      break; /* must be last */
    }
    if (t < 1 || (rh_u32_t)t > mdl || (rh_u32_t)t == dl || t <= prev)
      return (v_bad(c, "a level number is invalid"));
    prev = t;
    if (!v_level(c, t))
      return (0);
  }
  if (c->off != c->len)
    return (v_bad(c, "it has data after the end"));
  return (1);
}

static int read_full(int fd, unsigned char *buf, unsigned long len) {
  ssize_t r;

  while (len) {
    r = read(fd, buf, len);
    if (r <= 0)
      return (0);
    buf += r;
    len -= (unsigned long)r;
  }
  return (1);
}

static int write_full(int fd, const unsigned char *buf, unsigned long len) {
  ssize_t r;

  while (len) {
    r = write(fd, buf, len);
    if (r <= 0)
      return (-1);
    buf += r;
    len -= (unsigned long)r;
  }
  return (1);
}

/* Modern: a private, already unlinked copy of the checked bytes, positioned
 * at the payload; -1 on failure. See rh_check_file(). */
static int rh_private_copy(const unsigned char *h, const unsigned char *buf,
                           unsigned long len) {
  FILE *f = tmpfile();
  int nfd;

  if (!f)
    return (-1);
  nfd = dup(fileno(f)); /* the unlinked file lives on through nfd */
  (void)fclose(f);
  if (nfd < 0)
    return (-1);
  if (!write_full(nfd, h, RH_HDR_LEN) || !write_full(nfd, buf, len) ||
      lseek(nfd, (off_t)RH_HDR_LEN, SEEK_SET) != (off_t)RH_HDR_LEN) {
    (void)close(nfd);
    return (-1);
  }
  return (nfd);
}

/**
 * MODERN ADDITION (2026): Check a save or bones file before loading it
 * Returns a new file descriptor, positioned at the payload, for a private
 * copy of exactly the bytes that were checked (an unlinked temporary
 * file), or -1 with *why describing the problem. The loader reads only
 * that copy, so a file changed between check and load cannot reach it.
 * lev is the level a bones file must hold (0 for saves). fd itself is only
 * read: the file is never changed or removed here, and the caller still
 * owns (and closes) fd.
 */
int rh_check_file(int fd, int kind, int lev, const char **why) {
  unsigned char h[RH_HDR_LEN], *buf;
  struct stat st;
  rh_u32_t hi, lo, len;
  int ok, nfd;

  *why = "it cannot be read";
  if (fstat(fd, &st) != 0 || lseek(fd, (off_t)0, SEEK_SET) != (off_t)0)
    return (-1);
  if (!read_full(fd, h, RH_HDR_LEN)) {
    *why = "it is truncated";
    return (-1);
  }
  if (memcmp(h, RH_MAGIC, 4) != 0) {
    *why = "it is in an older save format";
    return (-1);
  }
  if ((((unsigned)h[4] << 8) | h[5]) != RH_VERSION) {
    *why = "it is in another save format version";
    return (-1);
  }
  if ((((unsigned)h[6] << 8) | h[7]) != (unsigned)kind) {
    *why = "it is not the right kind of file";
    return (-1);
  }
  rh_fingerprint(&hi, &lo);
  if (get_u32(h + 12) != hi || get_u32(h + 16) != lo) {
    *why = "it was made by an incompatible build of the game";
    return (-1);
  }
  len = get_u32(h + RH_HDR_LEN_OFF);
  if (len > RH_MAX_PAYLOAD || st.st_size < (off_t)RH_HDR_LEN ||
      (unsigned long)st.st_size - RH_HDR_LEN != len) {
    *why = "it is truncated or damaged";
    return (-1);
  }
  if (!(buf = (unsigned char *)malloc(len ? len : 1)))
    return (-1);
  ok = read_full(fd, buf, len);
  if (!ok)
    *why = "it is truncated";
  else if ((~rh_crc32(0xFFFFFFFFU, buf, len)) != get_u32(h + 24)) {
    ok = 0;
    *why = "it is damaged (checksum mismatch)";
  }
#ifndef RH_TEST_SKIP_VALIDATE /* Modern: test builds only, never defined */
  if (ok) {
    struct rh_cur c;

    c.p = buf;
    c.len = len;
    c.off = 0;
    c.err = NULL;
    if (kind == RH_KIND_SAVE)
      ok = v_save(&c);
    else
      ok = v_level(&c, lev) && c.off == c.len;
    if (!ok)
      *why = c.err ? c.err : "it contains invalid data";
  }
#else
  (void)lev;
#endif
  nfd = -1;
  if (ok && (nfd = rh_private_copy(h, buf, len)) < 0)
    *why = "no temporary file could be made to load it";
  free(buf);
  return (nfd);
}

/**
 * MODERN ADDITION (2026): Abandon a restore without losing the save
 * WHY: A failure part-way through dorecover() (truncation, a bad field, an
 *      internal error) used to unlink the save (mread() and the uid check,
 *      both 1985 code) or left half-loaded state behind.
 * HOW: Remove the level files made so far, move the save aside with
 *      move_outdated_save() and exit cleanly. Game state is half loaded, so
 *      going on with a new game in this process would not be safe.
 * PRESERVES: Successful restores are unchanged.
 * ADDS: The player keeps the file and is told where it is.
 */
void restore_failed(const char *why) {
  static int failing = 0;

  if (failing++)
    exit(1);
  rh_sum_on = 0;
  clearlocks();
  if (move_outdated_save())
    error("Your saved game could not be restored (%s).\n"
          "It was not deleted; it was kept as %s.",
          why, outdated_save_name);
  error("Your saved game could not be restored (%s).\n"
        "It was not deleted; %s was left as it is.",
        why, SAVEF);
}

/* Modern: a loader check failed. When restoring a save, keep the save and
 * exit; otherwise (bones, which rh_check_file() has already validated, or
 * our own level files) this cannot happen short of a bug. */
void rh_load_failed(const char *why) {
  if (restoring)
    restore_failed(why);
  panic("Bad level data: %s.", why);
}
/* MODERN: CONST-CORRECTNESS: settty message is read-only */

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
  unsigned ustuck_id; /* Modern: see below */
  int fd, ofd;
  int tmp; /* not ! */
  char tmpfile[256];

  /**
   * MODERN ADDITION (2026): Never save over a kept out-of-date save
   * WHY: If the out-of-date save found at start-up could not be moved aside
   *      (see move_outdated_save() in hack.main.c), it is still at SAVEF and
   *      the rename() below would replace it with this new game.
   * HOW: Retry the move; if SAVEF has meanwhile gone (moved by the player)
   *      there is nothing to protect. Otherwise refuse to save and let the
   *      game continue. This runs before the signals are ignored below, so a
   *      refused save leaves the hangup/interrupt handlers in place.
   * PRESERVES: Normal saves are unaffected (outdated_save_stuck is 0).
   * ADDS: The player's old save can never be overwritten by a new game.
   */
  /* Modern: a hangup during dorecover() must not write the half-restored
   * game over the save being restored; leave the save as it is. */
  if (restoring) {
    if (hu)
      clearlocks();
    return (0);
  }

  if (outdated_save_stuck) {
    struct stat st;

    if (stat(SAVEF, &st) != 0 && errno == ENOENT)
      outdated_save_stuck = 0; /* the player moved it away */
    else if (move_outdated_save()) {
      outdated_save_stuck = 0;
      if (!hu)
        pline("Old save kept as %s.", outdated_save_name);
    } else {
      if (!hu)
        pline("Not saved; it would overwrite %s.", SAVEF);
      return (0);
    }
  }

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
  /* Modern: savelev() frees fmon, which holds u.ustuck; the 1985 code read
   * u.ustuck->m_id after it (use after free whenever you were held, e.g. by
   * an eel), so take the id first */
  ustuck_id = u.ustuck ? u.ustuck->m_id : 0;
  rh_begin_file(fd, RH_KIND_SAVE); /* Modern: v3 header comes first */
  savelev(fd, dlevel);
  saveobjchn(fd, invent);
  saveobjchn(fd, fcobj);
  savemonchn(fd, fallen_down);

  /* Modern: the version 2 header was written here; see rh_begin_file() */

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
    sw_u32(fd, (rh_u32_t)ustuck_id); /* Modern: was u.ustuck->m_id */
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

    if (tmp == dlevel || !level_exists[tmp])
      continue;
    glo(tmp);
    if ((ofd = open(lock, 0)) < 0) {
      if (!hu)
        pline("Error while saving: cannot read %s.", lock);
      rh_sum_on = 0; /* Modern: abandon the v3 length/CRC sum */
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
  save_descr_block(fd); /* Modern: must stay last, see RH_DESCR_TAG */
  if (!rh_end_file(fd)) { /* Modern: patch length and CRC into the header */
    if (!hu)
      pline("Error writing save file.");
    (void)close(fd);
    (void)unlink(tmpfile);
    return (0);
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

  /* Modern: the caller has checked the whole file with rh_check_file()
   * (header, fingerprint, length, CRC, validators); fd is at the payload.
   * Any failure from here on goes through restore_failed(), which keeps
   * the save and exits. */
  restoring = TRUE;
  getlev(fd, 0, 0);
  invent = restobjchn(fd);
  for (otmp = invent; otmp; otmp = otmp->nobj)
    if (otmp->owornmask)
      setworn(otmp, otmp->owornmask);
  fcobj = restobjchn(fd);
  fallen_down = restmonchn(fd);

  /* Modern: the version 1/2 header check and the version 1 migration that
   * were here are gone; such files are refused by rh_check_file(). */
  tmp = (int)sr_u32(fd);
  if (tmp != (int)getuid()) {
    /* Original 1984:
     *   (void) close(fd);
     *   (void) unlink(SAVEF);
     *   puts("Saved game was not yours.");
     *   restoring = FALSE;
     *   return(0);
     */
    restore_failed("it was not yours"); /* Modern: keep it */
  }

  unpack_flags(fd, &flags);
  dlevel = sr_i32(fd);
  maxdlevel = sr_i32(fd);
  moves = (long)sr_u32(fd);
  /* Modern: last-resort check; rh_check_file() has already checked these */
  if (dlevel < 1 || dlevel > MAXLEVEL || maxdlevel < dlevel ||
      maxdlevel > MAXLEVEL)
    restore_failed("the dungeon level is invalid");

  /* Read struct you */
  rh_u32_t you_size = sr_u32(fd);
  if (you_size != sizeof(struct you))
    restore_failed("the player record has the wrong size");
  sr_bytes(fd, &u, sizeof(struct you));

  /* Read ustuck */
  has_ustuck = sr_u32(fd);
  if (has_ustuck) {
    mid = sr_u32(fd);
  }

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

  sr_bytes(fd, pl_character, sizeof pl_character);
  sr_bytes(fd, genocided, sizeof genocided);
  sr_bytes(fd, fut_geno, sizeof fut_geno);
  restnames(fd);
  /* MODERN: the current level read at the top was only needed to reach the
   * data after it; it is reread below, so free this copy (it leaked). */
  freelev();
  while (1) {
    if (read(fd, (char *)&tmp, sizeof tmp) != sizeof tmp)
      break;
    /* Modern: trailing object description block, written after all levels */
    if (tmp == RH_DESCR_TAG) {
      rest_descr_block(fd);
      break;
    }
    /* Modern: last-resort check; glo() and level_exists[] index by it */
    if (tmp < 1 || tmp > maxdlevel || tmp == dlevel)
      restore_failed("a level number is invalid");
    getlev(fd, 0, tmp);
    glo(tmp);
    if ((nfd = creat(lock, FMASK)) < 0)
      panic("Cannot open temp file %s!\n", lock);
    savelev(nfd, tmp);
    (void)close(nfd);
  }
  /* Original 1984: (void) lseek(fd, (off_t)0, 0); */
  (void)lseek(fd, (off_t)RH_HDR_LEN, 0); /* Modern: skip the v3 header */
  getlev(fd, 0, 0);
  (void)close(fd);
  /* Original 1984: (void) unlink(SAVEF);
   * Modern: moved to the end, so that a failure in the checks below (which
   * can panic) still finds the save in place; see restore_failed(). */
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
  (void)unlink(SAVEF); /* Modern: moved here, see above */
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
      rh_load_failed("an object name length is invalid"); /* Modern */
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

  rh_u32_t id;

  /* Original 1984: the raw &mons[0] of the saving process, used to
   * relocate every monst->data (wrong for types outside mons[]):
   *   struct permonst *monbegin;
   *   long differ;
   *
   *   mread(fd, (char *)&monbegin, sizeof(monbegin));
   *   differ = (char *)(&mons[0]) - (char *)(monbegin);
   */

#ifdef lint
  /* suppress "used before set" warning from lint */
  mtmp2 = 0;
#endif /* lint */
  while (1) {
    mread(fd, (char *)&xl, sizeof(xl));
    if (xl == -1)
      break;
    /* Modern: stable type ID (see rh_mon_id()); checked before allocating */
    id = sr_u32(fd);
    if (xl < 0 || xl > RH_MAX_MXL || !rh_mon_pm(id))
      rh_load_failed("a monster record is invalid");
    mtmp = newmonst(xl);
    if (!first)
      first = mtmp;
    else
      mtmp2->nmon = mtmp;
    mread(fd, (char *)mtmp, (unsigned)xl + sizeof(struct monst));
    /* Modern: the stored lengths must describe exactly what was read */
    if ((unsigned long)mtmp->mxlth + mtmp->mnamelth != (unsigned long)xl)
      rh_load_failed("a monster record length is invalid");
    if (!mtmp->m_id)
      mtmp->m_id = flags.ident++;
    /* Original 1984: mtmp->data = (struct permonst *)
     *                    ((char *) mtmp->data + differ); */
    mtmp->data = rh_mon_pm(id);
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
