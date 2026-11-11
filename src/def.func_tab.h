#ifndef DEF_FUNC_TAB_H
#define DEF_FUNC_TAB_H

/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* def.func_tab.h - version 1.0.2 */

struct func_tab {
  char f_char;
  int (*f_funct)(void);
};

extern struct func_tab cmdlist[];

struct ext_func_tab {
  const char *ef_txt; /* MODERN: const because points to string literals */
  int (*ef_funct)(void);
};

extern struct ext_func_tab extcmdlist[];

#endif /* DEF_FUNC_TAB_H */
