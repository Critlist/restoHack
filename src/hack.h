/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.h - version 1.0.3 */

#include "config.h"
#include <stdlib.h>
#include <string.h>

/* Modern debugging overlay - non-invasive logging for learning */
#ifdef DEBUG_LEARNING
#define HACK_LOG(...) fprintf(stderr, "[HACK_DEBUG] " __VA_ARGS__)
#define HACK_TRACE(func) fprintf(stderr, "[TRACE] Entering %s\n", func)
#else
#define HACK_LOG(...) ((void)0)
#define HACK_TRACE(func) ((void)0)
#endif

/* Visibility macros for learning module boundaries */
#define EXPORT   /* Function should be visible outside this module */
#define INTERNAL /* Function is internal to this module only */
#define API      /* Part of the game's public API */

/* K&R compatibility and learning warnings */
#ifdef DEBUG_LEARNING
#define CHAR_BEHAVIOR_WARNING()                                                \
  HACK_LOG("WARNING: char signedness may differ from K&R original\n")
#define POINTER_ARITHMETIC_CHECK(ptr)                                          \
  HACK_LOG("Pointer arithmetic at %s:%d - verify bounds\n", __FILE__, __LINE__)
#else
#define CHAR_BEHAVIOR_WARNING() ((void)0)
#define POINTER_ARITHMETIC_CHECK(ptr) ((void)0)
#endif

/* MODERN: index/rindex macros moved to compat.h for cleaner organization */
#include "compat.h"

#define Null(type) ((struct type *)0)

#include "def.objclass.h"

/* Original 1984: typedef struct { xchar x,y; } coord; */
typedef struct {
  unsigned char x, y; /* MODERN: unsigned to prevent buffer underflow */
} coord;

#include "def.flag.h"
#include "def.gold.h"
#include "def.monst.h" /* uses coord */
#include "def.obj.h"
#include "def.trap.h"

#define plur(x) (((x) == 1) ? "" : "s")

#define BUFSZ 256  /* for getlin buffers */
#define PL_NSIZ 32 /* name of player, ghost, shopkeeper */

/**
 * MODERN ADDITION (2025): Safe object array access macros
 *
 * WHY: Original code accesses objects[] array without bounds checking
 * HOW: Inline bounds check with fallback to STRANGE_OBJECT (index 0)
 * PRESERVES: Same functionality when indices are valid
 * ADDS: Protection against array bounds violations and segfaults
 */
#define SAFE_OBJ(otyp)                                                         \
  ((otyp) >= 0 && (otyp) < NROFOBJECTS ? (otyp) : STRANGE_OBJECT)
#define SAFE_OBJECTS(otyp) objects[SAFE_OBJ(otyp)]

/* MODERN: Safe levl[][] access with bounds checking */
#define SAFE_LEVL_TYP(x,y) (isok((x),(y)) ? levl[(x)][(y)].typ : STONE)

#include "def.mkroom.h"
#include "def.permonst.h"
#include "def.rm.h"

extern void *alloc(unsigned lth);
/* MODERN: CONST-CORRECTNESS: panic message is read-only */
/* MODERN: noreturn attribute tells compiler panic() never returns */
extern void panic(const char *str, ...) __attribute__((noreturn));

/* Critical missing function prototypes */
extern int carrying(int type);
/* MODERN: CONST-CORRECTNESS: pline message is read-only */
extern void pline(const char *line, ...);
extern void nomul(int nturns);
extern int doredotopl(void);
extern void redotoplin(void);
extern void remember_topl(void);
/* MODERN: CONST-CORRECTNESS: xmore message is read-only */
extern void xmore(const char *s);
/* MODERN: CONST-CORRECTNESS: cmore message is read-only */
extern void cmore(const char *s);
extern void putsym(char c);
/* MODERN: CONST-CORRECTNESS: putstr message is read-only */
extern void putstr(const char *s);
/* MODERN: CONST-CORRECTNESS: xwaitforspace allowed chars is read-only */
extern void xwaitforspace(const char *s);
extern int getdir(boolean s);
/* MODERN: CONST-CORRECTNESS: getpos goal parameter is read-only */
extern coord getpos(int force, const char *goal);
extern int dist(int x1, int y1);
extern int cansee(int x, int y);
extern void setmangry(struct monst *mtmp);
extern int rn2(int x);
extern int rnd(int x);
extern void setuwep(struct obj *obj);
extern void freeinv(struct obj *obj);
extern void Tmp_at(
    schar x,
    schar y); /* MODERN: Keep original schar to preserve -1,-2 sentinel logic */
extern void mnexto(struct monst *mtmp);
extern int abon(void);
extern void dighole(void);
extern int doapply(void);
extern int holetime(void);
extern int rn1(int x, int y);
extern void fracture_rock(struct obj *obj);

/* PLAYER SYSTEM PROTOTYPES - from studying hack.u_init.c */
/* K&R OBJECT CREATION SYSTEM - Forward declarations for authentic 1984
 * functions */
extern struct obj *mkobj(int let);   /* Create object by symbol */
extern struct obj *mksobj(int otyp); /* Create object by type */
extern struct obj *mkobj_at(int let, int x,
                            int y); /* Create object at position */
extern struct obj *mksobj_at(int otyp, int x,
                             int y); /* Create specific object at position */
extern int letter(int c);            /* Check if character is a letter */
extern int probtype(int let);        /* Get probable object type from symbol */
/* addinv() - K&R function, implicitly declared */
extern int weight(struct obj *obj);
extern void setworn(struct obj *obj, long mask);
extern void init_objects(void);
extern char readchar(void);
extern void end_of_input(void);
extern void bell(void);

/* CHARACTER INITIALIZATION STRUCTURES AND FUNCTIONS */
struct trobj {
  uchar trotyp;
  schar trspe;
  char trolet;
  Bitfield(trquan, 6);
  Bitfield(trknown, 1);
};

extern int role_index(char pc);     /* Find role index by character */
extern int ini_inv(struct trobj *); /* Initialize starting inventory */
extern int wiz_inv(void);           /* Wizard starting items */

/* TERMINAL FUNCTION PROTOTYPES - Forward declarations for hack.termcap.c */
extern void xputs(char *s);
extern int xputc(int c);
extern void cmov(int x, int y);
extern void nocmov(int x, int y);
extern void home(void);

/* MISSING FUNCTION PROTOTYPES - Essential for linking (non-conflicting only) */
extern int cansee(int x, int y);
extern int canseemon(struct monst *mtmp);
extern int isok(int x, int y);
extern char *eos(char *s);
extern int letindex(char let);
extern char *doname(struct obj *obj);
extern void getlin(char *buf);
/* MODERN: CONST-CORRECTNESS: xwaitforspace allowed chars is read-only */
extern void xwaitforspace(const char *s);
extern struct obj *splitobj(struct obj *obj, int cnt);
extern void obfree(struct obj *obj, struct obj *merge);
extern void oinit(void);
extern struct trap *maketrap(int x, int y, int typ);
extern void vtele(void);
extern void teleds(int nux, int nuy);
extern int teleok(int x, int y);
extern int dotele(void);
extern int float_down(void);
extern void morehungry(int num);
/* MODERN: CONST-CORRECTNESS: poisoned message parameters are read-only */
extern void poisoned(const char *string1, const char *string2);
extern void fall_down(struct monst *mtmp);
extern int doeat(void);
extern int doread(void);
extern int dodrink(void);
extern int dozap(void);
/* MODERN: CONST-CORRECTNESS: make_engr_at text is read-only */
extern void make_engr_at(int x, int y, const char *s);
extern coord mazexy(void);
extern void mkshop(void);
extern void mkzoo(int type);
extern void mkswamp(void);

/* COMBAT SYSTEM PROTOTYPES - hack.fight.c and related */
extern void seemimic(struct monst *mtmp);
extern void mondead(struct monst *mtmp);
extern void mondied(struct monst *mdef);
extern void monstone(struct monst *mdef);
extern void killed(struct monst *mtmp);
extern void done_in_by(struct monst *mtmp);
extern int d(int n, int x); /* Dice roll function */
extern void setan(const char *str,
                  char *buf); /* MODERN: const because str is read-only */
extern int dbon(void);        /* Damage bonus */
/* MODERN: CONST-CORRECTNESS: hit message parameters are read-only */
extern void hit(const char *str, struct monst *mtmp, const char *force);
extern void u_wipe_engr(int cnt);
extern int m_move(struct monst *mtmp, int after);
extern void wakeup(struct monst *mtmp);
extern void cutworm(struct monst *mtmp, xchar x, xchar y, uchar weptyp);
extern void losehp_m(int n, struct monst *mtmp);
extern void corrode_armor(void);
extern void corrode_weapon(void);
extern int attack(struct monst *mtmp);

/* MONSTER AND NAMING SYSTEM PROTOTYPES */
extern void replmon(struct monst *mtmp, struct monst *mtmp2);
extern void docall(struct obj *obj);
extern char *visctrl(char c);
extern char *Monnam(struct monst *mtmp);
extern char *monnam(struct monst *mtmp);
/* MODERN: CONST-CORRECTNESS: amonnam adjective parameter is read-only */
extern char *amonnam(struct monst *mtmp, const char *adj);
/* MODERN: CONST-CORRECTNESS: Amonnam adjective parameter is read-only */
extern char *Amonnam(struct monst *mtmp, const char *adj);

/* GLOBAL VARIABLES */
extern char morc;
extern struct wseg *wsegs[32];

/* Function prototypes */
/* MODERN: CONST-CORRECTNESS: error message is read-only */
extern void error(const char *s, ...);
extern void initoptions(void);
extern void prscore(int argc, char **argv);
extern void gettty(void);
/* MODERN: CONST-CORRECTNESS: settty message is read-only */
extern void settty(const char *s);
extern void setrandom(void);
extern void startup(void);
extern void term_getsize(void);
extern void cls(void);
extern void gethdate(char *name);
/* askname() defined in hack.main.c */
extern int plnamesuffix();
extern void getlock(void);
extern void setftty(void);
extern int dorecover(int fd);
extern int u_init();
/* role_index() - K&R function, implicitly declared */
/* ini_inv() - K&R function, implicitly declared */
/* wiz_inv() - K&R function, implicitly declared */
extern int dlevelp(int level);
extern void mklev(void);
extern void seeobjs(void);
extern void seemons(void);
extern void docrt(void);
extern void read_engr_at(int x, int y);
/* MODERN: CONST-CORRECTNESS: losehp death message is read-only */
extern void losehp(int n, const char *how);
extern void losexp(void);
extern int dosearch(void);
extern void gethungry(void);

/* Monster interaction functions */
extern void
youswld(struct monst *mtmp, int dam, int die,
        const char *name); /* MODERN: const because name is read-only */
extern int wiz_hit(struct monst *mtmp);
extern void
justswld(struct monst *mtmp,
         const char *name); /* MODERN: const because name is read-only */
/* MODERN: CONST-CORRECTNESS: kludge parameters are read-only */
extern void kludge(const char *str, const char *arg);
extern void stealgold(struct monst *mtmp);
extern int stealamulet(struct monst *mtmp);
extern struct obj *steal(struct monst *mtmp);
extern void ringoff(struct obj *obj);
extern int armoroff(struct obj *otmp);
extern int sgn(int x);
extern void aggravate(void);
extern void clonewiz(struct monst *mtmp);
extern int dorr(struct obj *otmp);
extern int cursed(struct obj *otmp);
extern void rescham(void);
extern int has_dnstairs(struct mkroom *sroom);
extern int has_upstairs(struct mkroom *sroom);
extern int isbig(struct mkroom *sroom);
extern void findname(char *nampt, char let);
extern int nexttodoor(int sx, int sy);
extern int dist2(int x0, int y0, int x1, int y1);
extern int sq(int a);
extern void invault(void);
extern void amulet(void);
extern void find_ac(void);
/* MODERN: CONST-CORRECTNESS: strange_feeling message is read-only */
extern void strange_feeling(struct obj *obj, const char *txt);
extern void ghost_from_bottle(void);
extern void lesshungry(int num);
extern void losestr(int num);
extern void pluslvl(void);
extern void float_up(void);
extern void litroom(boolean on);
extern void level_tele(void);
extern int identify(struct obj *otmp);
extern int monstersym(char ch);
extern int chwepon(struct obj *otmp, int amount);
extern void charcat(char *s, char c);
extern struct monst *bhit(int ddx, int ddy, int range, char sym, int (*fhitm)(),
                          int (*fhito)(), struct obj *obj);
extern struct monst *boomhit(int dx, int dy);
/* MODERN: CONST-CORRECTNESS: exclam returns read-only string literals */
extern const char *exclam(int force);
extern int newcham(struct monst *mtmp, struct permonst *mdat);
extern void rloco(struct obj *obj);
extern int revive(struct obj *obj);
extern int findit(void);
extern void unstuck(struct monst *mtmp);
extern void buzz(int type, xchar sx, xchar sy, int dx, int dy);
/* MODERN: CONST-CORRECTNESS: thitu damage description is read-only */
extern int thitu(int tlev, int dam, const char *name);
extern int zhit(struct monst *mon, int type);
extern void burn_scrolls(void);
extern void nscr(void);
extern void bot(void);
extern void cornbot(int lth);
extern void unpmon(struct monst *mon);
extern void swallowed(void);
extern void more(void);
extern void pmon(struct monst *mon);
extern int getwn(struct monst *mtmp);
extern void initworm(struct monst *mtmp);
extern void worm_move(struct monst *mtmp);
extern void worm_nomove(struct monst *mtmp);
extern void wormsee(int wormno);
extern void remseg(struct wseg *wtmp);
extern void atl(int x, int y, int ch);
extern void on_scr(int x, int y);
extern void tmp_at(schar x, schar y);
extern void setclipped(void);
extern void at(xchar x, xchar y, char ch);
extern void prme(void);
extern int doredraw(void);
extern void docorner(int xmin, int ymax);
extern void curs_on_u(void);
extern void pru(void);
/* delay_output provided by curses library or fallback implementation */
extern void prl(int x, int y);
extern void newsym(int x, int y);
extern void mnewsym(int x, int y);
extern void nosee(int x, int y);
extern void prl1(int x, int y);
extern void nose1(int x, int y);
extern int vism_at(int x, int y);
extern int isok(int x, int y);
extern int canseemon(struct monst *mtmp);
extern int monster_nearby(void);
extern void stop_occupation(void);
extern void lookaround(void);
extern void domove(void);
extern void rhack(char *save_cm);
extern int movecmd(char sym);
extern void confdir(void);
extern int doextcmd(void);
extern char lowc(char sym);
extern char hack_unctrl(char sym);
extern void glo(int foo);
/* MODERN: CONST-CORRECTNESS: impossible message is read-only */
extern void impossible(const char *s, int x1, int x2);
extern void getret(void);

/* BUILD BLOCKER PROTOTYPES - Critical missing functions */
extern void askname(void);
extern void regularize(char *s);
extern void center(int line, char *text);
extern int getyear(void);
extern int uptodate(int fd);
extern int inshop(void);
extern void setsee(void);
extern void makedog(void);
/* extern struct monst *m_at(int x, int y); -- declared in def.monst.h */
extern int readnews(void);
extern int pickup(int all);
extern int phase_of_the_moon(void);
extern void initrack(void);
extern void settrack(void);
extern coord *gettrack(int x, int y);
extern void movemon(void);
extern struct monst *makemon(struct permonst *ptr, int x, int y);
extern void monfree(struct monst *mtmp);
extern void rloc(struct monst *mtmp);
extern coord enexto(xchar xx, xchar yy);
extern void glibr(void);
extern void hack_timeout(void);
extern void stoned_dialogue(void);
/* MODERN: CONST-CORRECTNESS: death reason string is read-only */
extern void done(const char *how);

/* MODERN LOCKING SYSTEM - hack.lock.c */
extern int modern_lock_game(void);
extern void modern_unlock_game(void);
extern int modern_lock_record(void);
extern void modern_unlock_record(void);
extern void modern_cleanup_locks(void);
extern int modern_locking_available(void);
extern void tele(void);
extern int init_uhunger(void);
extern int inv_weight(void);
extern int inv_cnt(void);
extern void movobj(struct obj *obj, int ox, int oy);
extern void drown(void);
extern void dotrap(struct trap *trap);
extern void addtobill(struct obj *obj);
extern void done1(int sig);
extern int doquit(void);
extern void hangup(int sig);
/* MODERN ADDITION (2025): Enhanced signal handlers for window manager
 * compatibility */
extern void modern_cleanup_handler(int sig);
extern void modern_save_handler(int sig);

/* LEVEL GENERATION PROTOTYPES - from hack.mklev.c */
extern void oinit(void);
extern void makemaz(void);
extern int makerooms(void);
extern void mktrap(int num, int mazeflag, struct mkroom *croom);
extern struct gold *mkgold(long amount, int x, int y);
extern void makecorridors(void);
extern void make_niches(void);
extern void makevtele(void);
extern void mkshop(void);
extern void mkzoo(int type);
extern void mkswamp(void);
extern int maker(schar lowx, schar dx, schar lowy, schar dy);
extern int addrs(int lowx, int lowy, int hix, int hiy);
extern int addrsx(int lx, int ly, int hx, int hy, boolean discarded);
extern int okdoor(int x, int y);
extern void dosdoor(int x, int y, struct mkroom *aroom, int type);
extern void join(int a, int b);
extern void makeniche(boolean with_trap);
/* MODERN: CONST-CORRECTNESS: make_engr_at text is read-only */
extern void make_engr_at(int x, int y, const char *s);

/* TERMCAP FUNCTION PROTOTYPES - from hack.termcap.c */
extern void startup(void);
extern void term_getsize(void);
extern void start_screen(void);
extern void end_screen(void);
extern void curs(int x, int y);
extern void nocmov(int x, int y);
extern void cmov(int x, int y);
extern int xputc(int c);
extern void xputs(char *s);
extern void cl_end(void);
extern void clear_screen(void);
extern void home(void);
extern void standoutbeg(void);
extern void standoutend(void);
extern void backsp(void);
extern void bell(void);
extern void cl_eos(void);
extern void setclipped(void);
extern void getret(void);
extern void set_whole_screen(void);

/* Original 1984: extern xchar xdnstair, ydnstair, xupstair, yupstair; */
extern unsigned char xdnstair, ydnstair, xupstair,
    yupstair; /* stairs up and down - MODERN: unsigned to prevent buffer
                 underflow */

extern xchar dlevel;

/* FUNCTIONS NEEDED BY hack.do.c */
extern void getlev(int fd, int pid, xchar lev);
extern void placebc(int u_in);
/* MODERN: CONST-CORRECTNESS: selftouch message is read-only */
extern void selftouch(const char *arg);
extern void losedogs(void);
extern void potionhit(struct monst *mon, struct obj *obj);
extern void potionbreathe(struct obj *obj);
extern int shkcatch(struct obj *obj);
/* MODERN: CONST-CORRECTNESS: miss message is read-only */
extern void miss(const char *s, struct monst *mtmp);
extern int tamedog(struct monst *mtmp, struct obj *obj);
extern void mpickobj(struct monst *mtmp, struct obj *otmp);
extern void unsee(void);
extern void splitbill(struct obj *obj, struct obj *otmp);
extern void subfrombill(struct obj *obj);
extern void unplacebc(void);
extern void keepdogs(void);
extern void seeoff(int mode);
/* Original 1984: extern void savelev(int fd, xchar lev); */
extern void
savelev(int fd,
        unsigned char lev); /* MODERN: unsigned to prevent buffer underflow */

/* FORWARD DECLARATIONS FOR hack.mon.c */
struct engr;
extern void wipe_engr_at(xchar x, xchar y, xchar cnt);
/* MODERN: CONST-CORRECTNESS: sengr_at search string is read-only */
extern int sengr_at(const char *s, xchar x, xchar y);
extern void del_engr(struct engr *ep);
/* MODERN ADDITION (2025): Memory cleanup for sanitizers */
extern void cleanup_all_engravings(void);
extern void bwrite(int fd, char *loc, unsigned num);
extern void mread(int fd, char *buf, unsigned len);
extern int mhitu(struct monst *mtmp);
extern int hitu(struct monst *mtmp, int dam);

/* FUNCTIONS NEEDED BY hack.lev.c */
extern void savemonchn(int fd, struct monst *mtmp);
extern void savegoldchn(int fd, struct gold *gold);
extern void savetrapchn(int fd, struct trap *trap);
extern void saveobjchn(int fd, struct obj *otmp);
extern void save_engravings(int fd);
extern void rest_engravings(int fd);
extern int getbones(void);
extern void makelevel(void);
extern void setgd(void);
/* System functions declared in unistd.h - removed to avoid conflicts */
extern int mintrap(struct monst *mtmp);
extern int dog_move(struct monst *mtmp, int after);
extern int shk_move(struct monst *mtmp);
extern int gd_move(void);
extern void gddead(void);
extern void replgd(struct monst *mtmp, struct monst *mtmp2);
extern void inrange(struct monst *mtmp);
extern int hitmm(struct monst *magr, struct monst *mdef);
extern int night(void);
extern int midnight(void);
extern int inroom(int x, int y);
extern void shopdig(int fall);
extern void initedog(struct monst *mtmp);
extern int follower(struct monst *mtmp);
extern void relmon(struct monst *mtmp);
extern int poisonous(struct obj *obj);
extern void relobj(struct monst *mtmp, int show);
extern int mfndpos(struct monst *mon, coord poss[9], int info[9], int flag);
extern int online(int x, int y);
extern int dochug(struct monst *mtmp);
extern void set_pager(int mode);
/* MODERN: CONST-CORRECTNESS: page_line text parameter is read-only */
extern int page_line(const char *line);
extern void wormdead(struct monst *mtmp);
extern void wormhit(struct monst *mtmp);
extern void cutworm(struct monst *mtmp, xchar x, xchar y, uchar weptyp);

#define newstring(x) (char *)alloc((unsigned)(x))
#include "hack.onames.h"

#define ON 1
#define OFF 0

extern struct obj *invent, *uwep, *uarm, *uarm2, *uarmh, *uarms, *uarmg, *uleft,
    *uright, *fcobj;
extern struct obj *uchain; /* defined iff PUNISHED */
extern struct obj *uball;  /* defined if PUNISHED */
extern struct obj *o_at(int x, int y), *sobj_at(int n, int x, int y);

struct prop {
#define TIMEOUT 007777     /* mask */
#define LEFT_RING W_RINGL  /* 010000L */
#define RIGHT_RING W_RINGR /* 020000L */
#define INTRINSIC 040000L
#define LEFT_SIDE LEFT_RING
#define RIGHT_SIDE RIGHT_RING
#define BOTH_SIDES (LEFT_SIDE | RIGHT_SIDE)
  long p_flgs;
  int (*p_tofn)(); /* called after timeout */
};

struct you {
  xchar ux, uy;
  schar dx, dy, dz; /* direction of move (or zap or ... ) */
#ifdef QUEST
  schar di;           /* direction of FF */
  xchar ux0, uy0;     /* initial position FF */
#endif                /* QUEST */
  xchar udisx, udisy; /* last display pos */
  char usym;          /* usually '@' */
  schar uluck;
#define LUCKMAX 10 /* on moonlit nights 11 */
#define LUCKMIN (-10)
  int last_str_turn : 3; /* 0: none, 1: half turn, 2: full turn */
                         /* +: turn right, -: turn left */
  unsigned udispl : 1;   /* @ on display */
  unsigned ulevel : 4;   /* 1 - 14 */
#ifdef QUEST
  unsigned uhorizon : 7;
#endif                    /* QUEST */
  unsigned utrap : 3;     /* trap timeout */
  unsigned utraptype : 1; /* defined if utrap nonzero */
#define TT_BEARTRAP 0
#define TT_PIT 1
  unsigned uinshop : 6; /* used only in shk.c - (roomno+1) of shop */

/* perhaps these #define's should also be generated by makedefs */
#define TELEPAT LAST_RING /* not a ring */
#define Telepat u.uprops[TELEPAT].p_flgs
#define FAST (LAST_RING + 1) /* not a ring */
#define Fast u.uprops[FAST].p_flgs
#define CONFUSION (LAST_RING + 2) /* not a ring */
#define Confusion u.uprops[CONFUSION].p_flgs
#define INVIS (LAST_RING + 3) /* not a ring */
#define Invis u.uprops[INVIS].p_flgs
#define Invisible (Invis && !See_invisible)
#define GLIB (LAST_RING + 4) /* not a ring */
#define Glib u.uprops[GLIB].p_flgs
#define PUNISHED (LAST_RING + 5) /* not a ring */
#define Punished u.uprops[PUNISHED].p_flgs
#define SICK (LAST_RING + 6) /* not a ring */
#define Sick u.uprops[SICK].p_flgs
#define BLIND (LAST_RING + 7) /* not a ring */
#define Blind u.uprops[BLIND].p_flgs
#define WOUNDED_LEGS (LAST_RING + 8) /* not a ring */
#define Wounded_legs u.uprops[WOUNDED_LEGS].p_flgs
#define STONED (LAST_RING + 9) /* not a ring */
#define Stoned u.uprops[STONED].p_flgs
#define PROP(x) (x - RIN_ADORNMENT) /* convert ring to index in uprops */
  unsigned umconf : 1;
  const char *usick_cause; /* MODERN: const because points to object names or
                              string literals */
  struct prop uprops[LAST_RING + 10];

  unsigned uswallow : 1; /* set if swallowed by a monster */
  unsigned uswldtim : 4; /* time you have been swallowed */
  unsigned uhs : 3;      /* hunger state - see hack.eat.c */
  schar ustr, ustrmax;
  schar udaminc;
  schar uac;
  int uhp, uhpmax;
  long int ugold, ugold0, uexp, urexp;
  int uhunger; /* refd only in eat.c and shk.c */
  int uinvault;
  struct monst *ustuck;
  int nr_killed[CMNUM + 2]; /* used for experience bookkeeping */
};

extern struct you u;

/* MODERN: CONST-CORRECTNESS: match traps[] definition (read-only string table)
 */
extern const char *const traps[];
extern char *aobjnam();
extern char readchar();

/* inventory system functions */
extern struct obj *addinv(struct obj *obj),
    *splitobj(struct obj *otmp, int cnt);
extern int assigninvlet(struct obj *otmp), carried(struct obj *obj),
    carrying(int type);
extern char obj_to_let(struct obj *obj);
extern void useup(struct obj *obj), freeinv(struct obj *obj),
    delobj(struct obj *obj), freeobj(struct obj *obj);
extern void freegold(struct gold *gold), deltrap(struct trap *trap),
    stackobj(struct obj *obj);
extern void prinv(struct obj *obj), doinv(char *lets);
extern int ddoinv(), dotypeinv(), dolook();
extern int merged(struct obj *otmp, struct obj *obj, int lose), countgold(),
    doprgold(), doprwep(), doprarm(), doprring();
/* MODERN: CONST-CORRECTNESS: ggetobj word parameter is read-only */
extern int digit(char c), ckunpaid(struct obj *otmp),
    ggetobj(const char *word, int (*fn)(struct obj *), int max);
extern int askchain(struct obj *objchn, char *olets, int allflag,
                    int (*fn)(struct obj *), int (*ckfn)(struct obj *),
                    int max);
/* MODERN: CONST-CORRECTNESS: getobj parameters are read-only */
extern struct obj *getobj(const char *let, const char *word),
    *o_on(unsigned int id, struct obj *objchn), *mkgoldobj(long q);
extern struct monst *m_at(int x, int y);
extern struct trap *t_at(int x, int y);
extern struct gold *g_at(int x, int y);
extern struct wseg *m_atseg;
extern void setnotworn(struct obj *obj),
    obfree(struct obj *obj, struct obj *other), unpobj(struct obj *obj);
/* MODERN: CONST-CORRECTNESS: cornline text is read-only */
extern void cornline(int mode, const char *text);
extern int doinvbill(int mode);
/* MODERN: CONST-CORRECTNESS: addtopl message is read-only */
extern void getlin(char *buf), clrlin(void), addtopl(const char *s);
extern char *doname(struct obj *obj);
extern char vowels[];

/* hack.do.c function prototypes */
extern int dodown(void), doup(void), donull(void), dopray(void), dothrow(void),
    doddrop(void);
extern void goto_level(int newlevel, boolean at_stairs);
extern void dropx(struct obj *obj), dropy(struct obj *obj);
extern void more_experienced(int exp, int rexp);
extern void set_wounded_legs(long side, int timex), heal_legs(void);

extern xchar curx, cury; /* cursor location on screen */

extern coord bhitpos; /* place where thrown weapon falls to the ground */

/* Original 1984: extern xchar seehx,seelx,seehy,seely; */
extern unsigned char seehx, seelx, seehy,
    seely; /* where to see - MODERN: unsigned to prevent buffer underflow
              vulnerability */
/* MODERN: CONST-CORRECTNESS: killer points to read-only death reasons */
extern char *save_cm;
extern const char *killer;

extern xchar dlevel, maxdlevel; /* dungeon level */

extern long moves;

extern int multi;

extern char lock[];

#define DIST(x1, y1, x2, y2)                                                   \
  (((x1) - (x2)) * ((x1) - (x2)) + ((y1) - (y2)) * ((y1) - (y2)))

#define PL_CSIZ 20       /* sizeof pl_character */
#define MAX_CARR_CAP 120 /* so that boulders can be heavier */
#define MAXLEVEL 40
#define FAR (COLNO + 2) /* position outside screen */
