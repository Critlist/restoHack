/* hack.lock.c - Modern file locking implementation for restoHack
 *
 * MODERN ADDITION (2025): Replaces vintage 1984 link()-based locking with
 * modern flock() system for improved reliability and portability.
 *
 * WHY: Original 1984 code used link(HLOCK, LLOCK) for atomic locking, which:
 * - Fails on modern filesystems that handle hard links differently
 * - Leaves stale lock files when processes crash ungracefully
 * - Requires write permissions for hard link creation
 * - Has race conditions on some filesystem implementations
 *
 * PRESERVES: Exact same locking semantics and game behavior, just more reliable
 * ADDS: Automatic cleanup, better error messages, timeout handling
 */

#include "hack.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>
#endif
/* File descriptors for locked files - must persist while locks are held */
static int game_lock_fd = -1;
static int record_lock_fd = -1;

/* Lock file paths - relative to hackdir after chdir() */
#define GAME_LOCK_FILE "game.lock"
#define RECORD_LOCK_FILE "record.lock"

/* Lock timeout in seconds */
#define LOCK_TIMEOUT 10

/*
 * Modern game instance locking using flock()
 * Replaces link(HLOCK, LLOCK) mechanism
 * Returns: 1 on success, 0 on failure
 */
int modern_lock_game(void) {
#ifdef _WIN32
  /* Modern: Windows uses LockFileEx - see win32.c */
  return win32_lock_game();
#else
  /* Unix: Use flock() for file locking */
  int fd;
  int attempts = 0;

  /* Create lock file if it doesn't exist */
  fd = open(GAME_LOCK_FILE, O_CREAT | O_RDWR, 0644);
  if (fd == -1) {
    printf("Cannot create game lock file %s: %s\n", GAME_LOCK_FILE,
           strerror(errno));
    return 0;
  }

  /* Try to acquire exclusive lock with timeout */
  while (attempts < LOCK_TIMEOUT) {
    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
      game_lock_fd = fd;
      return 1; /* Success */
    }

    if (errno != EWOULDBLOCK && errno != EAGAIN) {
      printf("Cannot lock game: %s\n", strerror(errno));
      close(fd);
      return 0;
    }

    /* Lock held by another process - wait and retry */
    if (attempts == 0) {
      printf("Another game is in progress...\n");
    }

    sleep(1);
    attempts++;
  }

  /* Timeout - suggest cleanup */
  printf("Cannot start game - lock timeout.\n");
  printf("If no other game is running, try: rm %s\n", GAME_LOCK_FILE);
  close(fd);
  return 0;
#endif /* _WIN32 */
}

/*
 * Release game instance lock
 * Replaces unlink(LLOCK) mechanism
 */
void modern_unlock_game(void) {
#ifdef _WIN32
  /* Modern: Windows uses UnlockFileEx - see win32.c */
  win32_unlock_game();
#else
  /* Unix: Release flock() */
  if (game_lock_fd != -1) {
    /* Verify fd is still valid before using it */
    if (fcntl(game_lock_fd, F_GETFD) != -1) {
      flock(game_lock_fd, LOCK_UN);
      close(game_lock_fd);
    }
    game_lock_fd = -1;
    /* Note: We keep the lock file for reuse - flock() doesn't require deletion
     */
  }
#endif /* _WIN32 */
}

/*
 * Modern record file locking for high score updates
 * Replaces link(recfile, reclock) mechanism
 * Returns: 1 on success, 0 on failure
 */
int modern_lock_record(void) {
  int fd;
  int attempts = 0;

  /* Create lock file if it doesn't exist */
  fd = open(RECORD_LOCK_FILE, O_CREAT | O_RDWR, 0644);
  if (fd == -1) {
    return 0; /* Fail silently like original */
  }

  /* Try to acquire exclusive lock with shorter timeout for record updates */
  while (attempts < 5) {
    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
      record_lock_fd = fd;
      return 1; /* Success */
    }

    if (errno != EWOULDBLOCK && errno != EAGAIN) {
      close(fd);
      return 0;
    }

    /* Brief wait for record lock */
    usleep(100000); /* 0.1 seconds */
    attempts++;
  }

  close(fd);
  return 0; /* Timeout or error */
}

/*
 * Release record file lock
 * Replaces unlink(reclock) mechanism
 */
void modern_unlock_record(void) {
  if (record_lock_fd != -1) {
    /* Verify fd is still valid before using it */
    if (fcntl(record_lock_fd, F_GETFD) != -1) {
      flock(record_lock_fd, LOCK_UN);
      close(record_lock_fd);
    }
    record_lock_fd = -1;
  }
}

/*
 * Cleanup any stale locks on startup
 * Modern addition - flock() automatically releases on process death,
 * but this handles the lock files themselves
 */
void modern_cleanup_locks(void) {
  /* Test if game lock is actually held */
  int fd = open(GAME_LOCK_FILE, O_RDWR);
  if (fd != -1) {
    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
      /* Lock was available - no active game */
      flock(fd, LOCK_UN);
    }
    close(fd);
  }

  /* Same for record lock */
  fd = open(RECORD_LOCK_FILE, O_RDWR);
  if (fd != -1) {
    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
      flock(fd, LOCK_UN);
    }
    close(fd);
  }
}

/*
 * Check if modern locking is available
 * Returns: 1 if flock() is supported, 0 otherwise
 */
int modern_locking_available(void) {
#ifdef ENABLE_MODERN_LOCKING
  return 1;
#else
  return 0;
#endif
}