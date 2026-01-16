/* sys/ioctl.h - Windows shim for Unix ioctl
 *
 * MODERN ADDITION (2026): Provides minimal ioctl declarations for Windows builds.
 * Terminal ioctl operations are handled by PDCurses, this is just for compatibility.
 */

#ifndef SYS_IOCTL_H_SHIM
#define SYS_IOCTL_H_SHIM

#ifdef _WIN32

/* ioctl request codes - terminal related (no-ops on Windows) */
#define TIOCGWINSZ  0x5413  /* Get window size */
#define TIOCSWINSZ  0x5414  /* Set window size */
#define FIONREAD    0x541B  /* Bytes available to read */

/* Window size structure */
struct winsize {
  unsigned short ws_row;
  unsigned short ws_col;
  unsigned short ws_xpixel;
  unsigned short ws_ypixel;
};

/* ioctl - stub that returns success for known operations */
static inline int ioctl(int fd, unsigned long request, ...) {
  (void)fd;
  (void)request;
  return 0;  /* Pretend success - PDCurses handles terminal */
}

#endif /* _WIN32 */
#endif /* SYS_IOCTL_H_SHIM */
