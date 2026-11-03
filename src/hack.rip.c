/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.rip.c - version 1.0.2 */
/* $FreeBSD$ */

#include "hack.h"
#include <stdio.h>


/* MODERN ADDITION (2025): Made tombstone strings writable to prevent segfaults
 */
/* Original 1984 code used read-only string literals which crash on modern
 * systems */
static char rip[][60] = {
    "                       ----------",
    "                      /          \\",
    "                     /    REST    \\",
    "                    /      IN      \\",
    "                   /     PEACE      \\",
    "                  /                  \\",
    "                  |                  |",
    "                  |                  |",
    "                  |                  |",
    "                  |                  |",
    "                  |                  |",
    "                  |       1001       |",
    "                 *|     *  *  *      | *",
    "        _________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______\n",
    ""};

void outrip(void) {
  int dp = 0; /* Index into rip array */
  char *dpx;
  char buf[BUFSZ];
  int x, y;

  cls();
  (void)strncpy(buf, plname, BUFSZ - 1);
  buf[BUFSZ - 1] = '\0'; /* MODERN: Ensure null termination */
  buf[16] = 0;
  center(6, buf);
  (void)snprintf(buf, BUFSZ, "%ld AU",
                 u.ugold); /* MODERN: Safe sprintf replacement - identical
                              output, prevents overflow */
  center(7, buf);
  (void)snprintf(buf, BUFSZ,
                 "killed by%s", /* MODERN: Safe sprintf replacement - identical
                                   output, prevents overflow */
                 !strncmp(killer, "the ", 4)     ? ""
                 : !strcmp(killer, "starvation") ? ""
                 : index(vowels, *killer)        ? " an"
                                                 : " a");
  center(8, buf);
  (void)strncpy(buf, killer, BUFSZ - 1);
  buf[BUFSZ - 1] = '\0'; /* MODERN: Ensure null termination */
  if (strlen(buf) > 16) {
    int i, i0, i1;
    i0 = i1 = 0;
    for (i = 0; i <= 16; i++)
      if (buf[i] == ' ')
        i0 = i, i1 = i + 1;
    if (!i0)
      i0 = i1 = 16;
    if (i1 + 16 < BUFSZ) /* MODERN: bounds check prevents buffer overflow */
      buf[i1 + 16] = 0;
    center(10, buf + i1);
    buf[i0] = 0;
  }
  center(9, buf);
  (void)snprintf(buf, BUFSZ, "%4d",
                 getyear()); /* MODERN: Safe sprintf replacement - identical
                                output, prevents overflow */
  center(11, buf);
  for (y = 8; dp >= 0 && dp < (int)(sizeof(rip)/sizeof(rip[0])) && rip[dp][0]; y++, dp++) { /* MODERN: bounds check prevents array overflow */
    x = 0;
    dpx = rip[dp];
    while (dpx[x]) {
      while (dpx[x] == ' ')
        x++;
      curs(x, y);
      while (dpx[x] && dpx[x] != ' ') {
        if (done_stopprint)
          return;
        curx++;
        (void)putchar(dpx[x++]);
      }
    }
  }
  getret();
}

void center(int line, char *text) {
  char *ip, *op;
  int offset, max_len, text_len;
  ip = text;
  text_len = strlen(text);

  /* MODERN ADDITION (2025): Bounds checking to prevent buffer overflow */
  /* Now safe with writable 2D array */
  offset = 28 - ((text_len + 1) / 2);
  if (offset < 0)
    offset = 0;
  if (offset + text_len >= 60) { /* Array width is 60 */
    max_len = 60 - offset - 1;
    if (max_len <= 0)
      return;
  } else {
    max_len = text_len;
  }

  if (line < 0 || line >= (int)(sizeof(rip)/sizeof(rip[0]))) return; /* MODERN: bounds check prevents line overflow */
  op = &rip[line][offset];
  while (*ip && max_len-- > 0)
    *op++ = *ip++;
}
