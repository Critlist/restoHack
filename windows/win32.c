/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* win32.c - Windows compatibility layer implementation */

#ifdef _WIN32

#include <windows.h>
#include <stdlib.h>
#include <io.h>

/* Restore PDCurses symbols (renamed/defined by win32.h or windows.h) */
#undef move
#undef delay_output
#undef MOUSE_MOVED  /* Defined by wincontypes.h, redefined by PDCurses */
#include <curses.h>
#include <stdio.h>
#include <string.h>

/* NOTE: We intentionally do NOT include hack.h here to avoid circular
 * dependencies and type conflicts. win32.c is self-contained. */

/* flock() operation constants (must match win32.h) */
#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8

/* fcntl() command constants (must match win32.h) */
#define F_GETFD 1

/* ===========================================================================
 * POSIX COMPATIBILITY SHIMS
 * ===========================================================================
 * These functions provide POSIX-like interfaces for Windows.
 */

int hack_getuid(void) {
  return 0;  /* Single-user game, no UID on Windows */
}

int hack_getgid(void) {
  return 0;  /* Single-user game, no GID on Windows */
}

int hack_getpid(void) {
  return (int)GetCurrentProcessId();  /* Windows API, always available */
}

/* Static buffer for username - Windows usernames max 256 chars */
static char win32_username[257] = {0};

char *hack_getlogin(void) {
  DWORD size = sizeof(win32_username);
  if (win32_username[0] == '\0') {
    if (!GetUserNameA(win32_username, &size)) {
      /* Fallback if GetUserName fails */
      strncpy(win32_username, "Player", sizeof(win32_username) - 1);
    }
  }
  return win32_username;
}

int hack_setgid(int gid) {
  (void)gid;
  return 0;  /* No-op on Windows, always succeeds */
}

int hack_usleep(unsigned int usec) {
  Sleep((usec + 999) / 1000);  /* Convert microseconds to milliseconds */
  return 0;  /* Always succeeds */
}

void hack_srandom(unsigned int seed) {
  srand(seed);  /* Use standard C srand() on Windows */
}

long hack_random(void) {
  return (long)rand();  /* Use standard C rand() on Windows */
}

/* kill() shim - check if process exists (sig=0) or stub for signals */
int hack_kill(int pid, int sig) {
  if (sig == 0) {
    /* Signal 0 = check if process exists */
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid);
    if (h != NULL) {
      CloseHandle(h);
      return 0;  /* Process exists */
    }
    return -1;  /* Process doesn't exist (errno should be ESRCH) */
  }
  /* Other signals not supported on Windows */
  return -1;
}

/* flock() shim - uses Windows LockFileEx/UnlockFileEx */
int hack_flock(int fd, int operation) {
  HANDLE h = (HANDLE)_get_osfhandle(fd);
  OVERLAPPED ovlp = {0};
  DWORD flags = 0;

  if (h == INVALID_HANDLE_VALUE) {
    return -1;
  }

  if (operation & LOCK_UN) {
    /* Unlock */
    if (UnlockFileEx(h, 0, MAXDWORD, MAXDWORD, &ovlp)) {
      return 0;
    }
    return -1;
  }

  /* Lock operation */
  if (operation & LOCK_EX) {
    flags |= LOCKFILE_EXCLUSIVE_LOCK;
  }
  if (operation & LOCK_NB) {
    flags |= LOCKFILE_FAIL_IMMEDIATELY;
  }

  if (LockFileEx(h, flags, 0, MAXDWORD, MAXDWORD, &ovlp)) {
    return 0;
  }
  return -1;
}

/* fcntl() shim - minimal implementation for file descriptor checks */
int hack_fcntl(int fd, int cmd, ...) {
  (void)fd;
  if (cmd == F_GETFD) {
    /* Return 0 if fd is valid, -1 otherwise */
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    return (h != INVALID_HANDLE_VALUE) ? 0 : -1;
  }
  return 0;  /* Other commands succeed silently */
}

/* ===========================================================================
 * TERMINAL LAYER - PDCurses Termcap Shim
 * ===========================================================================
 *
 * CRITICAL: This shim uses PDCurses API calls, NOT ANSI escape sequences.
 * PDCurses is the execution engine - tputs() calls move(), clrtoeol(), etc.
 * directly rather than outputting ANSI strings.
 *
 * This ensures correct rendering across Windows Terminal, cmd.exe, and
 * various console configurations that may not interpret ANSI escapes.
 */

static int pdcurses_initialized = 0;

/* Initialize PDCurses and set UTF-8 console mode */
int tgetent(char *bp, const char *name) {
  (void)bp;
  (void)name;

  if (!pdcurses_initialized) {
    /* Initialize PDCurses */
    initscr();
    cbreak();      /* Disable line buffering */
    noecho();      /* Don't echo input */
    nonl();        /* Don't translate newline */
    keypad(stdscr, TRUE);  /* Enable keypad for arrow keys */

    /* Set UTF-8 console mode for box-drawing characters */
    SetConsoleCP(CP_UTF8);        /* Input codepage */
    SetConsoleOutputCP(CP_UTF8);  /* Output codepage */

    pdcurses_initialized = 1;
  }

  return 1;  /* Success */
}

/* Return symbolic capability strings (interpreted by tputs()) */
char *tgetstr(const char *id, char **area) {
  (void)area;

  /* Map termcap capability codes to symbolic identifiers
   * tputs() will execute these via PDCurses API calls */
  static char cm[] = "CM";  /* Cursor motion */
  static char ce[] = "CE";  /* Clear to end of line */
  static char cl[] = "CL";  /* Clear screen */
  static char so[] = "SO";  /* Standout mode (inverse video) */
  static char se[] = "SE";  /* End standout mode */
  static char us[] = "US";  /* Underline mode */
  static char ue[] = "UE";  /* End underline mode */
  static char md[] = "MD";  /* Bold mode */
  static char me[] = "ME";  /* End all modes */

  /* Return only capabilities that Hack actually uses */
  if (strcmp(id, "cm") == 0) return cm;
  if (strcmp(id, "ce") == 0) return ce;
  if (strcmp(id, "cl") == 0) return cl;
  if (strcmp(id, "so") == 0) return so;
  if (strcmp(id, "se") == 0) return se;
  if (strcmp(id, "us") == 0) return us;
  if (strcmp(id, "ue") == 0) return ue;
  if (strcmp(id, "md") == 0) return md;
  if (strcmp(id, "me") == 0) return me;

  return NULL;  /* Capability not supported */
}

/* Return terminal dimensions from PDCurses */
int tgetnum(const char *id) {
  if (strcmp(id, "co") == 0) return COLS;  /* Columns */
  if (strcmp(id, "li") == 0) return LINES; /* Lines */
  return -1;  /* Capability not supported */
}

/* Check boolean terminal capability */
int tgetflag(const char *id) {
  /* PDCurses/Windows console supports standard capabilities */
  if (strcmp(id, "bs") == 0) return 1;  /* Backspace works */
  if (strcmp(id, "am") == 0) return 1;  /* Auto margin */
  /* NP, nx, os - not applicable to Windows console */
  return 0;  /* Capability not present */
}

/* Format cursor motion string with coordinates */
char *tgoto(const char *cap, int col, int row) {
  static char buf[32];
  (void)cap;

  /* Encode coordinates in symbolic string for tputs() */
  snprintf(buf, sizeof(buf), "CM:%d,%d", row, col);
  return buf;
}

/* Execute terminal operations via PDCurses API (not putchar) */
void tputs(const char *str, int affcnt, int (*outc)(int)) {
  (void)affcnt;
  (void)outc;

  if (!str) return;

  /* Execute operations via PDCurses API, not character output */
  if (strncmp(str, "CM:", 3) == 0) {
    /* Cursor motion: parse coordinates and call move() */
    int row, col;
    if (sscanf(str + 3, "%d,%d", &row, &col) == 2) {
      move(row, col);
      refresh();
    }
  } else if (strcmp(str, "CE") == 0) {
    /* Clear to end of line */
    clrtoeol();
    refresh();
  } else if (strcmp(str, "CL") == 0) {
    /* Clear screen */
    clear();
    refresh();
  } else if (strcmp(str, "SO") == 0) {
    /* Standout mode (inverse video) */
    attron(A_STANDOUT);
  } else if (strcmp(str, "SE") == 0) {
    /* End standout mode */
    attroff(A_STANDOUT);
  } else if (strcmp(str, "US") == 0) {
    /* Underline mode */
    attron(A_UNDERLINE);
  } else if (strcmp(str, "UE") == 0) {
    /* End underline mode */
    attroff(A_UNDERLINE);
  } else if (strcmp(str, "MD") == 0) {
    /* Bold mode */
    attron(A_BOLD);
  } else if (strcmp(str, "ME") == 0) {
    /* End all modes */
    attrset(A_NORMAL);
  }
}

/* ===========================================================================
 * FILE LOCKING - Windows LockFileEx Replacement for flock()
 * ===========================================================================
 *
 * CRITICAL: Handle must be stored and properly cleaned up on exit.
 * "Process exits anyway" is NOT an acceptable assumption - handle leaks
 * can break save file semantics and cause corruption.
 */

static HANDLE game_lock_handle = INVALID_HANDLE_VALUE;

/* Lock game for single-player access */
int win32_lock_game(void) {
  OVERLAPPED ovlp = {0};

  /* Open/create lock file with shared access for reading
   * This allows us to lock it exclusively while still permitting reads */
  game_lock_handle = CreateFileA(
    "game.lock",  /* Lock file name */
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    OPEN_ALWAYS,  /* Create if doesn't exist */
    FILE_ATTRIBUTE_NORMAL,
    NULL
  );

  if (game_lock_handle == INVALID_HANDLE_VALUE) {
    return 0;  /* Failed to open/create lock file */
  }

  /* Attempt exclusive lock (non-blocking) */
  if (!LockFileEx(
        game_lock_handle,
        LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
        0,
        MAXDWORD,
        MAXDWORD,
        &ovlp)) {
    /* Lock failed - another instance running */
    CloseHandle(game_lock_handle);
    game_lock_handle = INVALID_HANDLE_VALUE;
    return 0;
  }

  return 1;  /* Lock acquired successfully */
}

/* Release game lock and cleanup handle */
void win32_unlock_game(void) {
  if (game_lock_handle != INVALID_HANDLE_VALUE) {
    OVERLAPPED ovlp = {0};

    /* Unlock the file */
    UnlockFileEx(game_lock_handle, 0, MAXDWORD, MAXDWORD, &ovlp);

    /* Close handle */
    CloseHandle(game_lock_handle);
    game_lock_handle = INVALID_HANDLE_VALUE;
  }
}

/* ===========================================================================
 * CONSOLE CONTROL - Signal Handling for Windows
 * ===========================================================================
 *
 * Windows doesn't have POSIX signals. We use SetConsoleCtrlHandler to map:
 * - CTRL_C_EVENT       -> SIGINT (quit prompt)
 * - CTRL_CLOSE_EVENT   -> SIGHUP (save and exit)
 * - CTRL_BREAK_EVENT   -> SIGHUP (save and exit)
 *
 * CRITICAL: Must call win32_unlock_game() before exit to prevent handle leaks.
 */

/* Forward declarations for Hack functions we'll call */
extern void done(const char *);
extern int dosave(void);

/* Windows console control handler */
static BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType) {
  switch (dwCtrlType) {
    case CTRL_C_EVENT:
      /* Ctrl+C: Same behavior as Unix SIGINT - prompt to quit */
      done("quit");
      return TRUE;

    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
      /* Window close or Ctrl+Break: Save and exit like Unix SIGHUP */
      win32_unlock_game();  /* CRITICAL: Clean up lock before exit */
      dosave();
      return TRUE;

    default:
      return FALSE;  /* Let default handler process it */
  }
}

/* Initialize Windows console */
void win32_init_console(void) {
  /* Install our console control handler */
  SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
}

/* Cleanup Windows console */
void win32_cleanup_console(void) {
  /* Release game lock if still held */
  win32_unlock_game();

  /* Remove console control handler */
  SetConsoleCtrlHandler(console_ctrl_handler, FALSE);

  /* Cleanup PDCurses if initialized */
  if (pdcurses_initialized) {
    endwin();
    pdcurses_initialized = 0;
  }
}

#endif /* _WIN32 */
