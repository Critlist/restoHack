#ifndef DEF_PERMONST_H
#define DEF_PERMONST_H

/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* def.permonst.h - version 1.0.2 */

/**
 * MODERN ADDITION (2025): const-qualified monster name string
 *
 * WHY: String literals are const char*, causing 251 discarded-qualifier
 * warnings HOW: Changed mname from char* to const char* for type safety
 *
 * PRESERVES: All original monster data and functionality
 * ADDS: Type safety and eliminates const qualifier warnings
 */
struct permonst {
  const char *mname; /* MODERN: const-qualified for string literal safety */
  char mlet;
  schar mlevel, mmove, ac, damn, damd;
  unsigned pxlth;
};

extern struct permonst mons[];
#define PM_ACID_BLOB &mons[7]
#define PM_ZOMBIE &mons[13]
#define PM_PIERCER &mons[17]
#define PM_KILLER_BEE &mons[26]
#define PM_WRAITH &mons[33]
#define PM_MIMIC &mons[37]
#define PM_VAMPIRE &mons[43]
#define PM_CHAMELEON &mons[47]
#define PM_DEMON &mons[54]
#define PM_MINOTAUR &mons[55] /* last in mons array */
#define PM_SHK &mons[56]      /* very last */
#define PM_GHOST &pm_ghost
#define PM_EEL &pm_eel
#define PM_WIZARD &pm_wizard
#define CMNUM 55 /* number of common monsters */

#endif /* DEF_PERMONST_H */
