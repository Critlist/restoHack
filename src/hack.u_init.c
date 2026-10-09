/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.u_init.c - version 1.0.3 */
/* $FreeBSD$ */

/*
 * Player initialization for 1984 Hack - character creation and starting
 * equipment Original 1984 source: docs/historical/original-source/hack.u_init.c
 *
 * Key modernizations: ANSI C function signatures, mutable role strings for
 * memory safety
 */

#include "hack.h"

#include <stdio.h>
#include <stdlib.h>
#define Strcpy (void)strcpy
#define Strcat (void)strcat
#define UNDEF_TYP 0
#define UNDEF_SPE '\177'
/* addinv() declared in hack.h */
extern char *eos();
extern char plname[];

struct you zerou;
char pl_character[PL_CSIZ];
/**
 * MODERN ADDITION (2025): Mutable character role strings
 *
 * WHY: Original K&R C code at line 170 attempts to modify roles[i][0] = pc;
 * In K&R C, string literals could be modified, but in ANSI C and modern C,
 * string literals are stored in read-only memory, causing segmentation faults.
 *
 * HOW: Convert string literals to mutable character arrays using static
 * storage. This preserves the exact same memory layout and behavior as original
 * K&R C while being compatible with modern compilers.
 *
 * PRESERVES: Original game logic for experienced player character selection
 * ADDS: ANSI C compatibility without changing game behavior
 *
 * AUTHENTICITY FIX (2025): Added separate Cave-woman array to prevent
 * pointer reassignment to string literal, maintaining consistent mutable
 * behavior.
 */
static char role_tourist[] = "Tourist";
static char role_speleologist[] = "Speleologist";
static char role_fighter[] = "Fighter";
static char role_knight[] = "Knight";
static char role_caveman[] = "Cave-man";
static char role_cavewoman[] = "Cave-woman";
static char role_wizard[] = "Wizard";

char *(roles[]) = {/* must all have distinct first letter */
                   /* roles[4] may be changed to -man */
                   role_tourist, role_speleologist, role_fighter,
                   role_knight,  role_caveman,      role_wizard};
#define NR_OF_ROLES SIZE(roles)
char rolesyms[NR_OF_ROLES + 1]; /* filled by u_init() */

#ifdef WIZARD
struct trobj Extra_objs[] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};
#endif /* WIZARD */

struct trobj Cave_man[] = {{MACE, 1, WEAPON_SYM, 1, 1},
                           {BOW, 1, WEAPON_SYM, 1, 1},
                           {ARROW, 0, WEAPON_SYM, 25, 1}, /* quan is variable */
                           {LEATHER_ARMOR, 0, ARMOR_SYM, 1, 1},
                           {0, 0, 0, 0, 0}};

struct trobj Fighter[] = {{TWO_HANDED_SWORD, 0, WEAPON_SYM, 1, 1},
                          {RING_MAIL, 0, ARMOR_SYM, 1, 1},
                          {0, 0, 0, 0, 0}};

struct trobj Knight[] = {{LONG_SWORD, 0, WEAPON_SYM, 1, 1},
                         {SPEAR, 2, WEAPON_SYM, 1, 1},
                         {RING_MAIL, 1, ARMOR_SYM, 1, 1},
                         {HELMET, 0, ARMOR_SYM, 1, 1},
                         {SHIELD, 0, ARMOR_SYM, 1, 1},
                         {PAIR_OF_GLOVES, 0, ARMOR_SYM, 1, 1},
                         {0, 0, 0, 0, 0}};

struct trobj Speleologist[] = {{STUDDED_LEATHER_ARMOR, 0, ARMOR_SYM, 1, 1},
                               {UNDEF_TYP, 0, POTION_SYM, 2, 0},
                               {FOOD_RATION, 0, FOOD_SYM, 3, 1},
                               {PICK_AXE, UNDEF_SPE, TOOL_SYM, 1, 0},
                               {ICE_BOX, 0, TOOL_SYM, 1, 0},
                               {0, 0, 0, 0, 0}};

struct trobj Tinopener[] = {{CAN_OPENER, 0, TOOL_SYM, 1, 1}, {0, 0, 0, 0, 0}};

struct trobj Tourist[] = {{UNDEF_TYP, 0, FOOD_SYM, 10, 1},
                          {POT_EXTRA_HEALING, 0, POTION_SYM, 2, 0},
                          {EXPENSIVE_CAMERA, 0, TOOL_SYM, 1, 1},
                          {DART, 2, WEAPON_SYM, 25, 1}, /* quan is variable */
                          {0, 0, 0, 0, 0}};

struct trobj Wizard[] = {{ELVEN_CLOAK, 0, ARMOR_SYM, 1, 1},
                         {UNDEF_TYP, UNDEF_SPE, WAND_SYM, 2, 0},
                         {UNDEF_TYP, UNDEF_SPE, RING_SYM, 2, 0},
                         {UNDEF_TYP, UNDEF_SPE, POTION_SYM, 2, 0},
                         {UNDEF_TYP, UNDEF_SPE, SCROLL_SYM, 3, 0},
                         {0, 0, 0, 0, 0}};

int u_init() {
  int i;
  char exper = 'y', pc;
  extern char readchar();
  if (flags.female) /* should have been set in HACKOPTIONS */
    roles[4] = role_cavewoman;
  else
    roles[4] = role_caveman;
  for (i = 0; i < NR_OF_ROLES; i++)
    rolesyms[i] = roles[i][0];
  rolesyms[i] = 0;

  if ((pc = pl_character[0])) {
    if ('a' <= pc && pc <= 'z')
      pc += 'A' - 'a';
    if ((i = role_index(pc)) >= 0)
      goto got_suffix; /* implies experienced */
    printf("\nUnknown role: %c\n", pc);
    pl_character[0] = pc = 0;
  }

  printf("\nAre you an experienced player? [ny] ");

  while (!index("ynYN \n\004", (exper = readchar())))
    bell();
  if (exper == '\004') /* Give him an opportunity to get out */
    end_of_input();
  printf("%c\n", exper); /* echo */
  if (index("Nn \n", exper)) {
    exper = 0;
    goto beginner;
  }

  printf("\nTell me what kind of character you are:\n");
  printf("Are you");
  for (i = 0; i < NR_OF_ROLES; i++) {
    printf(" a %s", roles[i]);
    if (i == 2) /* %% */
      printf(",\n\t");
    else if (i < NR_OF_ROLES - 2)
      printf(",");
    else if (i == NR_OF_ROLES - 2)
      printf(" or");
  }
  printf("? [%s] ", rolesyms);

  while ((pc = readchar())) {
    if ('a' <= pc && pc <= 'z')
      pc += 'A' - 'a';
    if ((i = role_index(pc)) >= 0) {
      printf("%c\n", pc);   /* echo */
      (void)fflush(stdout); /* should be seen */
      break;
    }
    if (pc == '\n')
      break;
    if (pc == '\004') /* Give him the opportunity to get out */
      end_of_input();
    bell();
  }
  if (pc == '\n')
    pc = 0;

beginner:
  if (!pc) {
    printf("\nI'll choose a character for you.\n");
    i = rn2(NR_OF_ROLES);
    pc = rolesyms[i];
    printf("This game you will be a%s %s.\n", exper ? "n experienced" : "",
           roles[i]);
    getret();
    /* give him some feedback in case mklev takes much time */
    (void)putchar('\n');
    (void)fflush(stdout);
  }
  if (exper) {
    /* MODERN: Add bounds checking for array access */
    if (i >= 0 && i < NR_OF_ROLES) {
      roles[i][0] = pc;
    }
  }

got_suffix:

  /* MODERN: Add bounds checking for roles array access */
  if (i >= 0 && i < NR_OF_ROLES) {
    (void)strncpy(pl_character, roles[i], PL_CSIZ - 1);
    pl_character[PL_CSIZ - 1] = 0;
  } else {
    /* Fallback to default character if index invalid */
    (void)strncpy(pl_character, roles[0], PL_CSIZ - 1);
    pl_character[PL_CSIZ - 1] = 0;
  }
  flags.beginner = 1;
  u = zerou;
  u.usym = '@';
  u.ulevel = 1;
  init_uhunger();
#ifdef QUEST
  u.uhorizon = 6;
#endif /* QUEST */
  uarm = uarm2 = uarmh = uarms = uarmg = uwep = uball = uchain = uleft =
      uright = 0;

  switch (pc) {
  case 'c':
  case 'C':
    Cave_man[2].trquan = 12 + rnd(9) * rnd(9);
    u.uhp = u.uhpmax = 16;
    u.ustr = u.ustrmax = 18;
    ini_inv(Cave_man);
    break;
  case 't':
  case 'T':
    Tourist[3].trquan = 20 + rnd(20);
    u.ugold = u.ugold0 = rnd(1000);
    u.uhp = u.uhpmax = 10;
    u.ustr = u.ustrmax = 8;
    ini_inv(Tourist);
    if (!rn2(25))
      ini_inv(Tinopener);
    break;
  case 'w':
  case 'W':
    for (i = 1; i <= 4; i++)
      if (!rn2(5))
        Wizard[i].trquan += rn2(3) - 1;
    u.uhp = u.uhpmax = 15;
    u.ustr = u.ustrmax = 16;
    ini_inv(Wizard);
    break;
  case 's':
  case 'S':
    Fast = INTRINSIC;
    Stealth = INTRINSIC;
    u.uhp = u.uhpmax = 12;
    u.ustr = u.ustrmax = 10;
    ini_inv(Speleologist);
    if (!rn2(10))
      ini_inv(Tinopener);
    break;
  case 'k':
  case 'K':
    u.uhp = u.uhpmax = 12;
    u.ustr = u.ustrmax = 10;
    ini_inv(Knight);
    break;
  case 'f':
  case 'F':
    u.uhp = u.uhpmax = 14;
    u.ustr = u.ustrmax = 17;
    ini_inv(Fighter);
    break;
  default: /* impossible */
    u.uhp = u.uhpmax = 12;
    u.ustr = u.ustrmax = 16;
  }
  find_ac();
  if (!rn2(20)) {
    int d = rn2(7) - 2; /* biased variation */
    u.ustr += d;
    u.ustrmax += d;
  }

#ifdef WIZARD
  if (wizard)
    wiz_inv();
#endif /* WIZARD */

  /* make sure he can carry all he has - especially for T's */
  while (inv_weight() > 0 && u.ustr < 118)
    u.ustr++, u.ustrmax++;
  return 1; /* MODERN: Return success indicator - was missing return value */
}

int ini_inv(trop)
struct trobj *trop;
{
  struct obj *obj;
  /* mkobj() declared in hack.h */
  while (trop->trolet) {
    obj = mkobj(trop->trolet);
    obj->known = trop->trknown;
    /* not obj->dknown = 1; - let him look at it at least once */
    obj->cursed = 0;
    if (obj->olet == WEAPON_SYM) {
      obj->quan = trop->trquan;
      trop->trquan = 1;
    }
    if (trop->trspe != UNDEF_SPE)
      obj->spe = trop->trspe;
    if (trop->trotyp != UNDEF_TYP)
      obj->otyp = trop->trotyp;
    else if (obj->otyp == WAN_WISHING) /* gitpyr!robert */
      obj->otyp = WAN_DEATH;
    obj->owt = weight(obj); /* defined after setting otyp+quan */
    obj = addinv(obj);
    if (obj->olet == ARMOR_SYM) {
      switch (obj->otyp) {
      case SHIELD:
        if (!uarms)
          setworn(obj, W_ARMS);
        break;
      case HELMET:
        if (!uarmh)
          setworn(obj, W_ARMH);
        break;
      case PAIR_OF_GLOVES:
        if (!uarmg)
          setworn(obj, W_ARMG);
        break;
      case ELVEN_CLOAK:
        if (!uarm2)
          setworn(obj, W_ARM);
        break;
      default:
        if (!uarm)
          setworn(obj, W_ARM);
      }
    }
    if (obj->olet == WEAPON_SYM)
      if (!uwep)
        setuwep(obj);
#ifndef PYRAMID_BUG
    if (--trop->trquan)
      continue; /* make a similar object */
#else
    if (trop->trquan) { /* check if zero first */
      --trop->trquan;
      if (trop->trquan)
        continue; /* make a similar object */
    }
#endif /* PYRAMID_BUG */
    trop++;
  }
  return 1; /* MODERN: Return success indicator - was missing return value */
}

#ifdef WIZARD
int wiz_inv() {
  struct trobj *trop = &Extra_objs[0];
  char *ep = getenv("INVENT");
  int type;
  while (ep && *ep) {
    type = atoi(ep);
    ep = index(ep, ',');
    if (ep)
      while (*ep == ',' || *ep == ' ')
        ep++;
    if (type <= 0 || type > NROFOBJECTS)
      continue;
    trop->trotyp = type;
    trop->trolet = objects[type].oc_olet;
    trop->trspe = 4;
    trop->trknown = 1;
    trop->trquan = 1;
    ini_inv(trop);
  }
  /* give him a wand of wishing by default */
  trop->trotyp = WAN_WISHING;
  trop->trolet = WAND_SYM;
  trop->trspe = 20;
  trop->trknown = 1;
  trop->trquan = 1;
  ini_inv(trop);
  return 1; /* MODERN: Return success indicator - was missing return value */
}
#endif /* WIZARD */

int plnamesuffix() {
  char *p;
  if ((p = rindex(plname, '-'))) {
    *p = 0;
    pl_character[0] = p[1];
    pl_character[1] = 0;
    if (!plname[0]) {
      askname();
      plnamesuffix();
    }
  }
  return 0; /* MODERN: Return 0 - no suffix found or processed - was missing
               return value */
}

int role_index(char pc) /* ANSI C conversion from K&R style */
{                       /* must be called only from u_init() */
  /* so that rolesyms[] is defined */
  char *cp;

  if ((cp = index(rolesyms, pc)))
    return (cp - rolesyms);
  return (-1);
}
