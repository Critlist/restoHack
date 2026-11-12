/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.o_init.c - version 1.0.3 */
/* $FreeBSD$ */

/*
 * Object initialization for 1984 Hack - item properties and game setup
 * Original 1984 source: docs/historical/original-source/hack.o_init.c
 *
 * Key modernizations: ANSI C function signatures
 */

#include "config.h" /* for typedefs */
#include "hack.h"
#include "def.objects.h"
#include "hack.onames.h" /* for LAST_GEM */
#include <stdio.h>
#include <string.h>
/* Function prototypes (bwrite/mread/error/panic are in hack.h) */
/* MODERN: CONST-CORRECTNESS: cornline text is read-only */
/* MODERN: CONST-CORRECTNESS: pline message is read-only */

/* Forward declarations for functions defined in this file */
void setgemprobs(void);
void savenames(int fd);
void restnames(int fd);
int interesting_to_discover(int i);
void snapshot_default_descr(void);
int descr_perm_count(void);
int get_descr_perm(unsigned *perm, int n);
int set_descr_perm(const unsigned *perm, int n);

/**
 * MODERN ADDITION (2026): Save/restore of the shuffled object descriptions
 * WHY: The original savenames() wrote objects[] (including the raw oc_descr
 *      pointers that init_objects() shuffles) and restnames() read them back,
 *      which only worked because every run loaded at the same addresses.
 *      restnames() now keeps the running binary's pointers instead, and on
 *      the restore path init_objects() is never called, so every restored
 *      game showed the unshuffled default appearances (e.g. "sky blue
 *      potion") and items changed their names after a restore.
 * HOW: Remember the default (unshuffled) oc_descr pointers before anything
 *      shuffles or overwrites them. When saving, describe the shuffle as an
 *      index per object: "object i shows the default description of object
 *      j". On restore, validate that list and rebuild oc_descr from this
 *      binary's own default strings. The I/O lives in hack.save.c.
 * PRESERVES: init_objects() shuffle, the original savenames()/restnames()
 *      stream layout, and the loading of saves that lack the new block.
 * ADDS: Restored games keep the appearances they were saved with.
 */
#define DESCR_NONE 0xFFFFFFFFu /* Modern: object has no description */
static const char *default_descr[SIZE(objects)]; /* Modern: unshuffled */
static boolean have_default_descr = FALSE;

/* Modern: Record the unshuffled descriptions once. The first call happens
 * either in init_objects() before the shuffle (new game) or in restnames()
 * before the saved objects[] is read (restore); both see the defaults. */
void snapshot_default_descr(void) {
  int i;
  if (have_default_descr)
    return;
  for (i = 0; i < SIZE(objects); i++)
    default_descr[i] = objects[i].oc_descr;
  have_default_descr = TRUE;
}

/* Modern: Number of entries in a description permutation (SIZE(objects) is
 * only known in this file, which includes def.objects.h). */
int descr_perm_count(void) { return (SIZE(objects)); }

/* Modern: Fill perm[i] with the index j whose default description object i
 * currently shows (DESCR_NONE if it has none). Returns 1 on success, 0 if
 * the mapping cannot be expressed (then nothing should be saved). */
int get_descr_perm(unsigned *perm, int n) {
  int i, j;
  boolean used[SIZE(objects)];

  if (!have_default_descr || n != SIZE(objects))
    return (0);
  for (j = 0; j < n; j++)
    used[j] = FALSE;
  for (i = 0; i < n; i++) {
    perm[i] = DESCR_NONE;
    if (objects[i].oc_descr == NULL)
      continue;
    /* Same class only; identical strings (e.g. gem colours) may share a
     * pointer, so take the first unused match to keep it a permutation. */
    for (j = 0; j < n; j++)
      if (!used[j] && default_descr[j] == objects[i].oc_descr &&
          objects[j].oc_olet == objects[i].oc_olet)
        break;
    if (j == n)
      return (0);
    used[j] = TRUE;
    perm[i] = (unsigned)j;
  }
  return (1);
}

/* Modern: Apply a saved permutation after validating all of it. Returns 1
 * if applied, 0 if rejected (oc_descr is then left untouched). */
int set_descr_perm(const unsigned *perm, int n) {
  int i;
  boolean used[SIZE(objects)];

  if (!have_default_descr || n != SIZE(objects))
    return (0);
  for (i = 0; i < n; i++)
    used[i] = FALSE;
  for (i = 0; i < n; i++) {
    if (perm[i] == DESCR_NONE) {
      if (default_descr[i] != NULL)
        return (0);
      continue;
    }
    if (perm[i] >= (unsigned)n || default_descr[perm[i]] == NULL ||
        default_descr[i] == NULL || used[perm[i]] ||
        objects[perm[i]].oc_olet != objects[i].oc_olet)
      return (0);
    used[perm[i]] = TRUE;
  }
  for (i = 0; i < n; i++)
    objects[i].oc_descr =
        (perm[i] == DESCR_NONE) ? NULL : default_descr[perm[i]];
  return (1);
}

int letindex(char let) {
  int i = 0;
  char ch;
  while ((ch = obj_symbols[i++]) != 0)
    if (ch == let)
      return (i);
  return (0);
}

void init_objects(void) {
  int i, j, first, last, sum, end;
  char let;
  const char *tmp; /* MODERN: const to match oc_descr field type */
  /* init base; if probs given check that they add up to 100,
     otherwise compute probs; shuffle descriptions */
  snapshot_default_descr(); /* Modern: remember them before the shuffle */
  end = SIZE(objects);
  first = 0;
  while (first < end) {
    let = objects[first].oc_olet;
    last = first + 1;
    while (last < end && objects[last].oc_olet == let &&
           objects[last].oc_name != NULL)
      last++;
    i = letindex(let);
    if ((!i && let != ILLOBJ_SYM) || bases[i] != 0)
      error("initialization error");
    bases[i] = first;

    if (let == GEM_SYM)
      setgemprobs();
  check:
    sum = 0;
    for (j = first; j < last; j++)
      sum += objects[j].oc_prob;
    if (sum == 0) {
      for (j = first; j < last; j++)
        objects[j].oc_prob =
            (schar)((100 + j - first) /
                    (last - first)); /* MODERN: cast to schar */
      goto check;
    }
    if (sum != 100)
      error("init-prob error for %c", let);

    if (objects[first].oc_descr != NULL && let != TOOL_SYM) {
      /* shuffle, also some additional descriptions */
      while (last < end && objects[last].oc_olet == let)
        last++;
      j = last;
      while (--j > first) {
        i = first + rn2(j + 1 - first);
        tmp = objects[j].oc_descr;
        objects[j].oc_descr = objects[i].oc_descr;
        objects[i].oc_descr = tmp;
      }
    }
    first = last;
  }
}

int probtype(int let) {
  int i =
      bases[letindex((char)let)]; /* MODERN: cast int to char for letindex */
  int prob = rn2(100);
  int category_start = i;
  (void)category_start; /* Original 1984: intended for bounds tracking,
                           currently unused */
  /* MODERN: Safe probability calculation with bounds checking*/

  /* Find the end of this object category */
  int category_end = i;
  while (category_end < NROFOBJECTS && objects[category_end].oc_olet == let &&
         objects[category_end].oc_name != NULL) {
    category_end++;
  }

  /* Original 1984: while((prob -= objects[i].oc_prob) >= 0) i++; */
  while ((prob -= objects[i].oc_prob) >= 0) {
    i++;
    /* MODERN: Bounds check - stay within this object category */
    if (i >= category_end || i >= NROFOBJECTS) {
      /* If we've exhausted all objects in category, return the last valid one
       */
      i = category_end - 1;
      break;
    }
  }
  if (objects[i].oc_olet != let || !objects[i].oc_name)
    panic("probtype(%c) error, i=%d", let, i);
  return (i);
}

void setgemprobs(void) {
  int j, first;

  first = bases[letindex(GEM_SYM)];

  for (j = 0; j < 9 - dlevel / 3; j++)
    objects[first + j].oc_prob = 0;
  first += j;
  if (first >= LAST_GEM || first >= SIZE(objects) ||
      objects[first].oc_olet != GEM_SYM || objects[first].oc_name == NULL)
    printf("Not enough gems? - first=%d j=%d LAST_GEM=%d\n", first, j,
           LAST_GEM);
  for (j = first; j < LAST_GEM; j++)
    objects[j].oc_prob =
        (schar)((20 + j - first) /
                (LAST_GEM - first)); /* MODERN: cast to schar */
}

void oinit(void) /* level dependent initialization */
{
  setgemprobs();
}

void savenames(int fd) {
  int i;
  unsigned len;
  bwrite(fd, (char *)bases, sizeof bases);
  bwrite(fd, (char *)objects, sizeof objects);
  /* as long as we use only one version of Hack/Quest we
     need not save oc_name and oc_descr, but we must save
     oc_uname for all objects */
  for (i = 0; i < SIZE(objects); i++) {
    if (objects[i].oc_uname) {
      len = (unsigned int)(strlen(objects[i].oc_uname) +
                           1); /* MODERN: cast to unsigned int */
      bwrite(fd, (char *)&len, sizeof len);
      bwrite(fd, objects[i].oc_uname,
             (int)len); /* MODERN: cast to int for bwrite */
    }
  }
}

void restnames(int fd) {
  int i;
  unsigned len;
  
  /* MODERN: Save static pointers before overwriting objects array
     Assumes objects[] static pointers are valid at this point.
     If init_objects() ever moves, ensure snapshot happens after it. */
  const char *saved_names[SIZE(objects)];
  const char *saved_descr[SIZE(objects)];
  /* Modern: init_objects() is not called when restoring, so these are still
     the unshuffled defaults; the saved shuffle is applied later from the
     block at the end of the save file (see dorecover()). */
  snapshot_default_descr();
  for (i = 0; i < SIZE(objects); i++) {
    saved_names[i] = objects[i].oc_name;
    saved_descr[i] = objects[i].oc_descr;
  }
  
  mread(fd, (char *)bases, sizeof bases);
  mread(fd, (char *)objects, sizeof objects);
  
  /* MODERN: Restore static pointers (point to static strings in binary) */
  for (i = 0; i < SIZE(objects); i++) {
    objects[i].oc_name = saved_names[i];
    objects[i].oc_descr = saved_descr[i];
  }
  
  for (i = 0; i < SIZE(objects); i++)
    if (objects[i].oc_uname) {
      mread(fd, (char *)&len, sizeof len);
      if (len == 0 || len > BUFSZ) /* Modern: checked before alloc() */
        rh_load_failed("an object name is invalid");
      objects[i].oc_uname = (char *)alloc(len);
      mread(fd, objects[i].oc_uname,
            (int)len); /* MODERN: cast to int for mread */
      objects[i].oc_uname[len - 1] = '\0'; /* Modern: always terminated */
    }
}

/* Modern: bytes savenames() writes before the oc_uname strings (save v3) */
unsigned long names_fixed_size(void) {
  return ((unsigned long)(sizeof bases + sizeof objects));
}

/* Modern: check what savenames() wrote, at p (at most len bytes), before
 * restnames() trusts it; *used is set to its length. See the validators in
 * hack.save.c. */
int names_check(const unsigned char *p, unsigned long len,
                unsigned long *used) {
  unsigned long off = names_fixed_size();
  struct objclass oc;
  unsigned ulen;
  int i, b;

  if (len < off)
    return (0);
  for (i = 0; i < SIZE(bases); i++) {
    memcpy(&b, p + i * sizeof(int), sizeof b);
    if (b < 0 || b > SIZE(objects))
      return (0);
  }
  for (i = 0; i < SIZE(objects); i++) {
    memcpy(&oc, p + sizeof bases + i * sizeof(struct objclass), sizeof oc);
    if (!oc.oc_uname)
      continue;
    if (len - off < sizeof ulen)
      return (0);
    memcpy(&ulen, p + off, sizeof ulen);
    off += sizeof ulen;
    if (ulen == 0 || ulen > BUFSZ || ulen > len - off ||
        memchr(p + off, 0, ulen) == NULL)
      return (0);
    off += ulen;
  }
  *used = off;
  return (1);
}

int dodiscovered(void) /* free after Robert Viduya */
{
  int i, end;
  int ct = 0;

  cornline(0, "Discoveries");

  end = SIZE(objects);
  for (i = 0; i < end; i++) {
    if (interesting_to_discover(i)) {
      ct++;
      cornline(1, typename(i));
    }
  }
  if (ct == 0) {
    pline("You haven't discovered anything yet...");
    cornline(3, (char *)0);
  } else
    cornline(2, (char *)0);

  return (0);
}

int interesting_to_discover(int i) {
  return (objects[i].oc_uname != NULL ||
          (objects[i].oc_name_known && objects[i].oc_descr != NULL));
}
