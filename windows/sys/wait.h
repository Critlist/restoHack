/* sys/wait.h - Windows shim for Unix process wait functions
 *
 * MODERN ADDITION (2026): Provides minimal wait declarations for Windows builds.
 * Windows doesn't have fork()/wait() - these are stubs for compatibility.
 */

#ifndef SYS_WAIT_H_SHIM
#define SYS_WAIT_H_SHIM

#ifdef _WIN32

#include <sys/types.h>  /* For pid_t from MinGW */

/* Wait status macros - simplified for Windows (no fork/exec model) */
#define WIFEXITED(status)   1
#define WEXITSTATUS(status) (status)
#define WIFSIGNALED(status) 0
#define WTERMSIG(status)    0
#define WIFSTOPPED(status)  0
#define WSTOPSIG(status)    0

/* wait() family - stubs that return immediately (no child processes on Windows) */
static inline pid_t wait(int *status) {
  (void)status;
  return -1;  /* No child processes */
}

static inline pid_t waitpid(pid_t pid, int *status, int options) {
  (void)pid;
  (void)status;
  (void)options;
  return -1;  /* No child processes */
}

/* Options for waitpid */
#define WNOHANG   1
#define WUNTRACED 2

/* fork() - doesn't exist on Windows, return error */
static inline pid_t fork(void) {
  return -1;  /* Always fails - no fork on Windows */
}

#endif /* _WIN32 */
#endif /* SYS_WAIT_H_SHIM */
