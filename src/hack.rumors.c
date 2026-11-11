/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.rumors.c - version 1.0.3 */
/* $FreeBSD$ */

#include "hack.h" /* for RUMORFILE and BSD (index) */
#include <stdio.h>
#define CHARSZ 8 /* number of bits in a char */
/* MODERN: removed bogus extern for index() - provided by string.h/compat.h */

/* Forward declarations */
int skipline(FILE *rumf);
int used(int i);

int n_rumors = 0;
int n_used_rumors = -1;
char *usedbits;

static void init_rumors(FILE *rumf) {
  int i;
  n_used_rumors = 0;
  while (skipline(rumf))
    n_rumors++;
  rewind(rumf);
  /* MODERN: Prevent division by zero and integer overflow */
  if (CHARSZ == 0 || n_rumors < 0) {
    impossible("Invalid rumor system parameters: rumors=%d charsz=%d", n_rumors,
               CHARSZ);
    return;
  }
  i = n_rumors / CHARSZ;
  usedbits = (char *)alloc((unsigned)(i + 1));
  for (; i >= 0; i--)
    usedbits[i] = 0;
}

int skipline(FILE *rumf) {
  char line[COLNO];
  while (1) {
    if (!fgets(line, sizeof(line), rumf))
      return (0);
    /* MODERN: Ensure null termination */
    line[COLNO - 1] = '\0';
    if (index(line, '\n'))
      return (1);
  }
}

static void outline(FILE *rumf) {
  char line[COLNO];
  char *ep;
  if (!fgets(line, sizeof(line), rumf))
    return;
  /* MODERN: Ensure null termination and validate line length */
  line[COLNO - 1] = '\0';
  if ((ep = index(line, '\n')) != 0)
    *ep = 0;
  pline("This cookie has a scrap of paper inside! It reads: ");
  pline("%s",
        line); /* MODERN: Safe format string - prevent format string attacks */
}

void outrumor(void) {
  int rn, i;
  int byte_idx, bit_idx;
  FILE *rumf;
  if (n_rumors <= n_used_rumors || (rumf = fopen(RUMORFILE, "r")) == (FILE *)0)
    return;
  if (n_used_rumors < 0)
    init_rumors(rumf);
  if (!n_rumors)
    goto none;
  rn = rn2(n_rumors - n_used_rumors);
  i = 0;
  while (rn || used(i)) {
    (void)skipline(rumf);
    if (!used(i))
      rn--;
    i++;
  }
  /* MODERN: Bounds check before array access */
  byte_idx = i / CHARSZ;
  bit_idx = i % CHARSZ;
  if (byte_idx >= 0 && byte_idx < (n_rumors / CHARSZ) + 1 && bit_idx >= 0 &&
      bit_idx < CHARSZ) {
    usedbits[byte_idx] |= (1 << bit_idx);
    n_used_rumors++;
  } else {
    impossible("Rumor index out of bounds: i=%d byte_idx=%d", i, byte_idx);
  }
  outline(rumf);
none:
  (void)fclose(rumf);
}

int used(int i) {
  /* MODERN: Bounds check before array access */
  int byte_idx = i / CHARSZ;
  int bit_idx = i % CHARSZ;
  if (byte_idx >= 0 && byte_idx < (n_rumors / CHARSZ) + 1 && bit_idx >= 0 &&
      bit_idx < CHARSZ) {
    return (usedbits[byte_idx] & (1 << bit_idx));
  }
  return 0; /* Safe default for out-of-bounds access */
}
