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
#include "def.objects.h"
#include "hack.onames.h" /* for LAST_GEM */
#include <stdio.h>
#include <string.h>
/* Function prototypes */
/* MODERN: Signatures must match hack.lev.c/hack.h exactly (void return,
 * unsigned length). Conflicting declarations are UB. */
extern void bwrite(int fd, char *buf, unsigned len);
extern void mread(int fd, char *buf, unsigned len);
/* MODERN: CONST-CORRECTNESS: error message is read-only */
extern void error(const char *s, ...);
/* MODERN: CONST-CORRECTNESS: panic message is read-only */
extern void panic(const char *str, ...);
extern int rn2(int x);
extern void *alloc(unsigned lth);
/* MODERN: CONST-CORRECTNESS: cornline text is read-only */
extern void cornline(int mode, const char *text);
/* MODERN: CONST-CORRECTNESS: pline message is read-only */
extern void pline(const char *line, ...);
extern char *typename(int otyp);

/* Forward declarations for functions defined in this file */
int letindex(char let);
void init_objects(void);
int probtype(int let);
void setgemprobs(void);
void oinit(void);
void savenames(int fd);
void restnames(int fd);
int dodiscovered(void);
int interesting_to_discover(int i);

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
  extern xchar dlevel;

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
      objects[i].oc_uname = (char *)alloc(len);
      mread(fd, objects[i].oc_uname,
            (int)len); /* MODERN: cast to int for mread */
    }
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
