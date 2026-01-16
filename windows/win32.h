/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* win32.h - Windows compatibility layer for 1984 Hack */

/* Prevent MinGW from declaring itoa() which conflicts with Hack's itoa(int)
 * _POSIX_ guard in MinGW's stdlib.h skips itoa/ltoa/etc declarations
 * MUST be defined before ANY headers (force-include ensures this) */
#define _POSIX_ 1

#ifndef WIN32_SHIM_H
#define WIN32_SHIM_H

#ifdef _WIN32

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

/* POSIX compatibility shims - declared here, defined in win32.c
 * Avoids including windows.h in header (prevents boolean/FAR conflicts) */

/* User/process identification - Windows is single-user */
int hack_getuid(void);
int hack_getgid(void);
int hack_getpid(void);
char *hack_getlogin(void);

/* Group functions - no-op on Windows (single-user) */
int hack_setgid(int gid);

/* Time functions */
int hack_usleep(unsigned int usec);  /* useconds_t is unsigned int, returns 0 */

/* Random number functions */
void hack_srandom(unsigned int seed);
long hack_random(void);

/* Map POSIX names to Windows equivalents */
#define getuid hack_getuid
#define getgid hack_getgid
#define getpid hack_getpid
#define getlogin hack_getlogin
#define setgid hack_setgid
#define usleep hack_usleep
#define srandom hack_srandom
#define random hack_random

/* Note: itoa collision avoided via _POSIX_ at top of file */

/* Symbol collision fixes - Hack functions that conflict with PDCurses */
#define move hack_maze_move         /* Hack's move(int*, int*, int) vs PDCurses move(int, int) */
#define delay_output hack_delay_output  /* Both define delay_output() */

/* Unix signals that don't exist on Windows - define as unused values */
#ifndef SIGHUP
#define SIGHUP 1   /* Hangup - handled via console control handler */
#endif
#ifndef SIGQUIT
#define SIGQUIT 3  /* Quit - handled via console control handler */
#endif

/* sig_t - signal handler function pointer type (BSD/Unix) */
#ifndef sig_t
typedef void (*sig_t)(int);
#endif

/* kill() - send signal to process (stub for Windows) */
int hack_kill(int pid, int sig);
#define kill hack_kill

/* ESRCH - No such process error code */
#ifndef ESRCH
#define ESRCH 3
#endif

/* File locking - flock() shim for Windows */
#define LOCK_SH 1  /* Shared lock */
#define LOCK_EX 2  /* Exclusive lock */
#define LOCK_NB 4  /* Non-blocking */
#define LOCK_UN 8  /* Unlock */
int hack_flock(int fd, int operation);
#define flock hack_flock

/* fcntl() shim - minimal implementation for lock validation */
#define F_GETFD 1  /* Get file descriptor flags */
#define F_SETFD 2  /* Set file descriptor flags */
int hack_fcntl(int fd, int cmd, ...);
#define fcntl hack_fcntl

#endif /* _WIN32 */
#endif /* WIN32_SHIM_H */
