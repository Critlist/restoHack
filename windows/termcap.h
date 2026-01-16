/* termcap.h - Windows termcap shim header
 *
 * MODERN ADDITION (2026): Provides termcap declarations for Windows builds.
 * The actual implementations are in win32.c using PDCurses.
 */

#ifndef TERMCAP_H_SHIM
#define TERMCAP_H_SHIM

#ifdef _WIN32

/* Termcap functions - implemented in win32.c via PDCurses */
int tgetent(char *bp, const char *name);
char *tgetstr(const char *id, char **area);
int tgetnum(const char *id);
int tgetflag(const char *id);
char *tgoto(const char *cap, int col, int row);
void tputs(const char *str, int affcnt, int (*outc)(int));

#endif /* _WIN32 */
#endif /* TERMCAP_H_SHIM */
