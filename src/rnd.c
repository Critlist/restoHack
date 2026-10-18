/* rnd.c - version 1.0.2 */
/* $FreeBSD$ */

/*
 * Random number utilities for 1984 Hack - dice rolling and random generation
 * Original 1984 source: docs/historical/original-source/rnd.c
 *
 * Key modernizations: ANSI C function signatures
 */

#include <stdlib.h>

/* Original 1984: #define RND(x) (random() % x) */
/**
 * MODERN ADDITION (2026): Guard the RND() modulus
 *
 * WHY: random() % x is a division by zero for x <= 0, which would crash the
 * game. Callers must never pass one, but a bad argument should not be fatal.
 *
 * HOW: RND() routes through rnd_mod(), which returns 0 for x <= 0 and the
 * original random() % x otherwise.
 *
 * PRESERVES: Identical results for every positive x.
 * ADDS: Defined behaviour for x <= 0 (returns 0).
 */
static int rnd_mod(int x) { return (x > 0) ? (random() % x) : 0; }
#define RND(x) rnd_mod(x)

/* MODERN: K&R to ANSI C conversion - function signatures only */
int rn1(int x, int y) { return (RND(x) + y); }

int rn2(int x) { return (RND(x)); }

int rnd(int x) { return (RND(x) + 1); }

int d(int n, int x) {
  int tmp = n;

  while (n--)
    tmp += RND(x);
  return (tmp);
}
