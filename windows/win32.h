/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* win32.h - Windows compatibility layer for 1984 Hack */

/**
 * MODERN ADDITION (2026): Windows platform support for Steam/itch.io
 *
 * WHY: Enable native Windows builds without polluting original 1984 code.
 *      Steam and itch.io distribution requires Windows console executable.
 *
 * HOW: Dedicated Windows compatibility layer injected via conditional compilation.
 *      All Windows-specific code isolated in win32.c, original files get minimal hooks.
 *
 * PRESERVES: Original 1984 Unix code structure remains pristine (~95% untouched)
 *
 * ADDS: - PDCurses termcap shim for Windows Console API
 *       - File locking via LockFileEx (replaces flock)
 *       - Console control handler for clean shutdown
 *       - UTF-8 console setup for box-drawing characters
 *       - POSIX compatibility shims (getuid, getpid, usleep)
 */

#ifndef WIN32_SHIM_H
#define WIN32_SHIM_H

#ifdef _WIN32

#include <windows.h>
#include <process.h>

/* Terminal layer - PDCurses termcap compatibility shim
 * These replace Unix termcap calls with PDCurses API equivalents */
int tgetent(char *bp, const char *name);
char *tgetstr(const char *id, char **area);
int tgetnum(const char *id);
char *tgoto(const char *cap, int col, int row);
void tputs(const char *str, int affcnt, int (*outc)(int));

/* File locking - Windows replacement for flock()
 * Uses LockFileEx with proper handle cleanup */
int win32_lock_game(void);
void win32_unlock_game(void);

/* Console initialization and cleanup
 * Sets up UTF-8 console mode and control handlers */
void win32_init_console(void);
void win32_cleanup_console(void);

/* POSIX compatibility shims
 * Modern: Inline functions for type safety, avoiding macro collisions */
static inline int hack_getuid(void) {
  return 0;  /* Single-user game, no UID on Windows */
}

static inline int hack_getpid(void) {
  return _getpid();
}

static inline void hack_usleep(unsigned long usec) {
  Sleep((usec + 999) / 1000);  /* Convert microseconds to milliseconds */
}

/* Map POSIX names to Windows equivalents */
#define getuid hack_getuid
#define getpid hack_getpid
#define usleep hack_usleep

#endif /* _WIN32 */
#endif /* WIN32_SHIM_H */
