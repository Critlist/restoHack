/* alloc.c - version 1.0.2 */
/* $FreeBSD$ */

/*
 * Memory allocation utilities for 1984 Hack - safe malloc/free wrappers
 * Original 1984 source: docs/historical/original-source/alloc.c
 *
 * Key modernizations: ANSI C function signatures, enhanced type safety
 */

#include "hack.h"
#include <stdlib.h>
#include <string.h>

#ifdef LINT

/*
   a ridiculous definition, suppressing
        "possible pointer alignment problem" for (long *) malloc()
        "enlarg defined but never used"
        "ftell defined (in <stdio.h>) but never used"
   from lint
*/
#include <stdio.h>
void *alloc(unsigned n) {
  /* Original 1984: return(&dummy); */
  static long dummy_storage =
      0; /* MODERN: static storage prevents dangling pointer */
  long dummy = ftell(stderr);
  if (n)
    dummy = 0; /* make sure arg is used */
  dummy_storage = dummy;
  return (&dummy_storage);
}

#else

#if 0
/* ORIGINAL 1984 CODE - preserved for reference */
long *
alloc(lth)
unsigned lth;
{
	char *ptr;

	if(!(ptr = malloc(lth)))
		panic("Cannot get %d bytes", lth);
	return((long *) ptr);
}

long *
enlarge(ptr,lth)
char *ptr;
unsigned lth;
{
	char *nptr;

	if(!(nptr = realloc(ptr,lth)))
		panic("Cannot reallocate %d bytes", lth);
	return((long *) nptr);
}
#endif

/**
 * MODERN ADDITION (2025): ANSI C memory allocation functions
 *
 * WHY: Original returned long* which required casting at every call site.
 * Modern void* return eliminates casting and follows ANSI C conventions.
 *
 * HOW: Changed return type from long* to void*, updated function signatures
 * to ANSI C style with proper parameter declarations.
 *
 * PRESERVES: Identical allocation behavior and error handling via panic()
 * ADDS: Type safety and ANSI C compliance without casting requirements
 */
void *alloc(unsigned lth) {
  char *ptr;

  if (!(ptr = malloc(lth)))
    panic("Cannot get %d bytes", lth);
  /* Zero the block: the save code writes whole structs to disk, and malloc
   * leaves struct padding uninitialised, which would end up in save files. */
  memset(ptr, 0, lth);
  return (ptr);
}

void *enlarge(char *ptr, unsigned lth);
void *enlarge(char *ptr, unsigned lth) {
  char *nptr;

  if (!(nptr = realloc(ptr, lth)))
    panic("Cannot reallocate %d bytes", lth);
  return (nptr);
}

#endif /* LINT */
