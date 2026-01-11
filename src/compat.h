/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* compat.h - MODERN compatibility layer for portability */

/**
 * MODERN ADDITION (2025): Compatibility header for portability
 * WHY: Centralize BSD function aliases and platform-specific code
 * HOW: Use macros to map BSD names to standard C library functions
 * PRESERVES: Original 1984 BSD function calls in code
 * ADDS: Windows/MSVC compatibility and cleaner organization
 */

#ifndef RESTOHACK_COMPAT_H
#define RESTOHACK_COMPAT_H

#include <string.h>

/* Historic BSD names used in 1984 code */
#ifndef index
#define index  strchr
#endif
#ifndef rindex
#define rindex strrchr
#endif

/* Windows/MSVC shims (still 1980s semantics) */
#if defined(_MSC_VER)
#include <stdlib.h>
#define srandom srand
#define random  rand
#endif

/* Modern: Windows compatibility layer injection */
#ifdef _WIN32
#include "win32.h"
#endif

#endif /* RESTOHACK_COMPAT_H */