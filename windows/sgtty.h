/* sgtty.h - Windows shim for Unix terminal control
 *
 * MODERN ADDITION (2026): Provides minimal sgtty declarations for Windows builds.
 * sgtty is an obsolete Unix terminal interface - PDCurses handles all terminal I/O.
 */

#ifndef SGTTY_H_SHIM
#define SGTTY_H_SHIM

#ifdef _WIN32

/* sgtty structures - not used on Windows (PDCurses handles terminal) */
struct sgttyb {
  char sg_ispeed;
  char sg_ospeed;
  char sg_erase;
  char sg_kill;
  short sg_flags;
};

/* Terminal mode flags */
#define CBREAK  0x02
#define ECHO    0x08
#define RAW     0x20
#define XTABS   0x1800  /* Expand tabs to spaces */

/* tcflag_t - terminal control flag type */
typedef unsigned int tcflag_t;

/* ioctl commands for terminal - no-ops on Windows */
#define TIOCGETP 0x7408
#define TIOCSETP 0x7409
#define TIOCSETN 0x740a

/* gtty/stty - terminal control functions (no-ops, PDCurses handles this) */
static inline int gtty(int fd, struct sgttyb *buf) {
  (void)fd;
  (void)buf;
  return 0;
}

static inline int stty(int fd, struct sgttyb *buf) {
  (void)fd;
  (void)buf;
  return 0;
}

#endif /* _WIN32 */
#endif /* SGTTY_H_SHIM */
