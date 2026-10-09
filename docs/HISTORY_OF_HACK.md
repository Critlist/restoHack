# The Complete History of Hack

*From 1981 to 2026: how Hack was written, rewritten, nearly lost, and restored*

*This document reflects the current best historical reconstruction based on primary sources recovered as of 2025-2026. Where earlier secondary accounts conflicted with newly recovered source code and contemporaneous documentation, those discrepancies have been corrected and cited. For the underlying evidence, see [TIMELINE.md](research/TIMELINE.md) and [COMPARISON.md](research/COMPARISON.md).*

## Timeline at a Glance

```text
1980  ████ ROGUE
      │    ∘ Created by Toy, Wichman (UCSC) and Arnold (UC Berkeley)
      │    ∘ Establishes procedural generation + permadeath
      │    ∘ ASCII graphics, turn-based gameplay
      │
~1981   ░░░ HACK PROTOTYPE (Logo on Apple II)
      │    ∘ Summer 1981: Fenlason plays Rogue at UC Berkeley, decides to build his own
      │    ∘ First version written in Logo (Turtle Graphics) on Apple II
      │
1981-82 ████ HACK DEVELOPMENT (Fenlason Era, Lincoln-Sudbury)
      │    ∘ Fenlason ports from Logo to C on PDP-11/70 running V7 Unix
      │    ∘ 57 monsters, 15 potions, 15 scrolls, 16 wands, 17 rings
      │    ∘ Collaborators: Woodland (mazes), Thome (chameleon), Payne (CURS/locks)
      │    ∘ Lincoln-Sudbury's submission goes on the USENIX 82-1 distribution tape
      │
1982  ████ USENIX DISTRIBUTION
      │    ∘ ;login: (June 1982) confirms tape completed before Boston conference
      │    ∘ Fenlason later recalled Hack and JOVE as the tape's most popular items
      │
~1983-84 ░░░ VU AMSTERDAM BRANCH
      │    ∘ Huisjes & de Wilde produce PDP-11 Hack at Vrije Universiteit
      │    ∘ Add zoos, graveyards, swamps, vaults
      │
1984  ████ HACK 1.0 (Brouwer Takes Control)
      │    ∘ Andries Brouwer distributes Hack 1.0 (Dec 17, 1984)
      │    ∘ Near-complete rewrite: adds shops, pets, long worms
      │    ∘ Renames Amulet of Frobozz to Amulet of Yendor
      │    ∘ 61 monster types, expanded food/armor/item systems
      │
1985  ████ HACK 1.0.2 / 1.0.3
      │    ∘ 1.0.2 (April): female adventurer, moon phases, pools
      │    ∘ 1.0.3 (July): Brouwer's final version, 19,239 lines
      │    ∘ Direct template for NetHack development
      │
1987  ████ NETHACK SPLITS OFF
      │    ∘ Stephenson, Miller, and Walz fork Hack 1.0.3
      │    ∘ Independent development begins on Usenet
      │    ∘ Hack development effectively stops
      │
2025  ████ RESTOHACK (Hack 1.0.3)
      │    ∘ K&R → ANSI C conversion (250+ functions)
      │    ∘ Memory safety fixes without gameplay changes
      │    ∘ CMake build system, cross-platform support
      │    ∘ Hack 1.0.3 (1985) gameplay preserved
      │
2025-26 ████ FENLASON'S SOURCE RESURFACES
      │    ∘ Brian Harvey provides preserved LSRHS PDP-11 backups to Dan Stormont (2025)
      │    ∘ Dan publishes them as fenlason-hack; protoHack (2026) restores the code to run on modern Linux
      │    ∘ The source settles long-standing questions about what Fenlason's original contained

Key: ████ = Active Development    ░░░ = Dormancy/Preservation
```

**[protoHack](https://github.com/Critlist/protoHack)** = restoration of Fenlason's original source + research. **restoHack** = stabilized restoration of Hack 1.0.3 + curated history. **[docs/research/](research/)** = shared evidence base.

**Project Status:** **Complete** - Hack 1.0.3 (1985) running on modern systems
**Code Provenance:** [FreeBSD 4.10 Games Collection](historical/original-source/download_hack.sh)
**Documentation:** [CODING_STANDARDS.md](CODING_STANDARDS.md) | [CHANGELOG.md](CHANGELOG.md)

---

## Table of Contents

1. [The Genesis: Rogue and the Birth of a Genre (1980)](#the-genesis-rogue-and-the-birth-of-a-genre-1980)
2. [Enter the Hack: Fenlason's Revolution (1981-1982)](#enter-the-hack-fenlasons-revolution-1981-1982)
3. [The VU Amsterdam Branch (1983-1985)](#the-vu-amsterdam-branch-1983-1985)
4. [The Brouwer Era: Hack's Golden Age (1984-1985)](#the-brouwer-era-hacks-golden-age-1984-1985)
5. [NetHack Splits Off (1987)](#nethack-splits-off-1987)
6. [The Lost Source: Rediscovery (2025)](#the-lost-source-rediscovery-2025)
7. [The Resurrection: restoHack (2025)](#the-resurrection-restohack-2025)

---

## The Genesis: Rogue and the Birth of a Genre (1980)

### The Original Vision

In 1980, **Michael Toy and Glenn Wichman (UC Santa Cruz) created Rogue. Ken Arnold (UC Berkeley) later joined, contributing curses support and interface improvements.** Together, they created something that would fundamentally change computer gaming. *Rogue* wasn't just a game; it was a revolutionary concept that combined:

* **Procedural generation**: Every dungeon was unique, generated algorithmically
* **Permadeath**: No save-scumming, no respawns—death was final
* **ASCII graphics**: Pure text-based visualization that left everything to imagination
* **Turn-based tactical combat**: Every move mattered in a chess-like battle system

---

## Enter the Hack: Fenlason's Revolution (1981-1982)

### The Visionary High School Student

The timeline was previously uncertain; this has now been resolved through the recovery of original 1981-82 source code and contemporaneous documentation (see [TIMELINE.md](research/TIMELINE.md)).

In the summer of 1981 — the summer between Fenlason's sophomore and junior years — **Brian Harvey**, Computer Director at Lincoln-Sudbury Regional High School in Sudbury, Massachusetts, invited students to California to help with a summer class he was teaching at San Francisco State University. During the trip, **Jay Fenlason** visited UC Berkeley and played *Rogue* for the first time: *"I think I got a little bit obsessed then, and like any obsessed person who'd been cut off, I decided to build my own."* (Craddock, p. 92)

Fenlason first created Hack in **Logo** (Turtle Graphics) on an **Apple II**. Once satisfied with the prototype, he migrated the game to C on the PDP-11/70 at Lincoln-Sudbury (Craddock, p. 94). As he recounts in a 2000 Linux.com interview with Julie Bresnick: *"I was curious about some of the game play issues involved in designing it, things like how the rooms and corridors were generated, so I started hacking up some random level generators and stuff to try things out."*

When someone looked over his shoulder and asked what he was working on, they said *"oh, that's cool; when do we get to play it?"* — and Hack was born.

### Collaborators

Fenlason developed Hack as his "first semester programming project" (READ_ME) on a PDP-11/70 running V7 Unix (alpha test site for 2.9BSD), administered by a student-run Computer Center Users Society with ~50 members who had keys and unsupervised access. Harvey, whose background was in the MIT and Stanford AI labs, had built the school's computing environment to resemble those labs: *"a powerful computer system, with lots of software tools, an informal community spirit, and not much formal curriculum."*

Working with three collaborators:

* **Kenny Woodland** (KW) — Maze-generating code for the bottom level, the original BUZZ() function, and "general random things" (READ_ME). Also contributed code for a game called *Bombs*. (Craddock, p. 94)
* **Mike Thome** (MT) — Invented the chameleon, a monster that *"could take on the appearance and abilities of other types of monsters"* (Craddock, p. 94). Credited in READ_ME as "The original chamelian" [sic].
* **Jonathan Payne** (JP) — The lock file system and "the massive CURS()" (READ_ME). Also the author of JOVE (Jonathan's Own Version of Emacs). Payne and Fenlason had *"a friendly rivalry going on as to whose program, JOVE or Hack, could update the screen most efficiently"* (Craddock, p. 94). Both programs shipped on the same USENIX 82-1 tape.

### What Fenlason Actually Built

The exact content of Fenlason's original was long unknown; a detailed comparison is now possible thanks to the 2025 source recovery (see [COMPARISON.md](research/COMPARISON.md)). The recovered code reveals a leaner, more focused game than later versions:

#### **Monster System**

* **57 monsters** (56 in an 8x7 array + vampire bat) — vs. Rogue's 26
* Unique creatures found in no other variant: **displacer beast**, **wumpus**, **ugod**, **xerp**, **zelomp**
* Polymorphic **chameleon** (Thome's contribution)
* The **Amulet of Frobozz** (Brouwer later renamed it to the Amulet of Yendor)

#### **Item System**

* **15 potions** (including "alcohol" — later variants changed this to "booze")
* **15 scrolls** (including unique "rust armor" scroll and original incantation names: "Andova Begarin", "Velox Neb", "Foobie Bletch", "Elbib Yloh")
* **16 wands**, **17 ring types** (including unique "increase hit points" ring)
* **6 armor types** (suits only: leather through plate)
* **2 food types** (food ration and fruit)

#### **What Was NOT in Fenlason's Original**

Several features commonly attributed to early Hack were actually added by later developers:

* **No shops or shopkeepers** — Brouwer's own README confirms these were his addition
* **No pet/dog system** — the `struct monst` has no `mtame` field; Brouwer added pets
* **No long worms** — Hack 1.0's `#ifdef NOWORM` conditional preserves the transition
* **No special room types** (zoos, vaults, graveyards) — added by Huisjes & de Wilde
* **No pools or water** — location types were limited to wall, door, corridor, room

### Technical Architecture

Fenlason's `struct monst` was notably simpler than later versions, packing its state into bit-fields — including a combined 2-bit `mstat` field for flee/sleep/frozen states and `sinv` (see invisible) and `mstuck` fields not present in any other variant:

```c
/* Fenlason's original struct monst (original/hack.h:70-82), verbatim */
struct monst {
	struct monst *nmon;
	char mx,my;
	int sinv:1;	/* special invisible */
	int invis:1;	/* invisible */
	int cham:1;	/* shape-changer */
	int mspeed:2;
	int mstat:2;
	int mcan:1;	/* has been canceled */
	int mstuck:1;	/* you are stuck to this */
	struct permonst *data;
	char mhp,orig_hp;
};
```

### Fenlason on Games

Even decades later, Fenlason remained a purist about gaming and technology. In the 2000 Linux.com interview, he expressed concerns about modern graphical gaming: *"No special effects in the movies will ever live up to those in your head."* He kept his children away from computers, believing they needed to learn *"more about how the world works, like climbing and building towers out of blocks and all the basic physics things that we take for granted."*

### USENIX Distribution

In the first half of 1982, Lincoln-Sudbury's software — including Hack and Jonathan Payne's editor — was submitted for the USENIX 82-1 distribution tape. *;login:* Vol. 7, No. 3 (June 1982) reports the tape complete and describes the Lincoln-Sudbury submission ("quite a few games") without naming the submitter or listing Hack by name. The tapes were distributed at the Boston USENIX conference in July 1982 — the same conference where Toy and Arnold presented their Rogue talk.

Fenlason later recalled: *"Usenix had biannual meetings, Unix users would get together and swap war stories. For each meeting they'd put together a tape of some of the contributed software. I put [Hack] on the tape and forgot about it until someone I know mentioned that the two most popular pieces of software on that particular tape were my silly game and my friend Jonathon's text editor."*

From there, Hack spread through USENIX tapes, university computer labs, and USENET. Fenlason himself, as Julie Bresnick wrote in 2000, "voluntarily avoided participation pretty much since spawning the original Hack."

---

## The VU Amsterdam Branch (1983-1985)

Around 1983-84, **Michiel Huisjes** and **Fred de Wilde** at Vrije Universiteit (VU), Amsterdam, produced their own PDP-11 version based on pre-1.0 Hack. Andries Brouwer later described it (in an April 1985 Usenet reply) as copied from his directory without permission while Hack 1.0 was still in development; he noted it was not in shape for distribution and lacked many features present in 1.0.

The VU Amsterdam developers made substantial additions of their own, documented in their README:

> *"Some additions we (Fred de Wilde and Michiel Huisjes) made are — The Vault / The Swamp / The Zoo / The Graveyard"*

They also replaced Fenlason's scroll of confuse monster with a "losing" scroll (drops all inventory), converted simple teleportation into a level-jumping elevator system, replaced the wand of striking with "exhaustion", and added wands of confusion and door closing. Their version inflated hit dice for the dragon (10 → 20hd), purple worm (15 → 20hd), and demon (10 → 14hd).

Huisjes published the PDP-11 Hack to Usenet (`net.sources`) in February 1985 as five shar parts. In May 1985, a **PC/IX port** adapted it for IBM PC UNIX.

---

## The Brouwer Era: Hack's Golden Age (1984-1985)

### The Near-Complete Rewrite

On **December 17, 1984**, **Andries Brouwer** at Stichting Mathematisch Centrum (CWI), Amsterdam, distributed **Hack 1.0** to `net.sources` in 15 shar parts (sender: `play@mcvax.UUCP`). The announcement promised 10 parts but there were actually 15, all sent on the same day. The volume of Hack traffic on Usenet prompted Gene Spafford to create the `net.games.hack` newsgroup.

Brouwer later wrote that Hack 1.0.3 *"contains very little if anything from the original sources."* This was a near-complete rewrite, not merely an expansion.

### What Brouwer Added

The [source comparison](research/COMPARISON.md) makes clear the scale of Brouwer's transformation:

#### **New Game Systems**

* **Shops and shopkeepers** — an entire economic layer with pricing, theft mechanics, and consequences
* **Pet/dog system** — three dog types (li_dog, dog, la_dog) with full movement AI, loyalty, and training (`hack.dog.c`, 400+ lines)
* **Long worm segments** — multi-tile creatures with segment tracking
* **Amulet of Yendor** — renamed from Fenlason's Amulet of Frobozz

#### **Content Expansion**

* **61 monster types** (up from Fenlason's 57), including shopkeeper, minotaur, nurse, unicorn
* **16 food types** (up from 2): tripe ration, pancake, dead lizard, fortune cookie, fruits, egg, carrot, tin, garlic, corpse
* **12 armor types** (up from 6): added banded mail, studded leather, elven cloak, helmet, shield, gloves
* **Wand of wishing** and **wand of make invisible** — powerful new magic items

### Quest (Undistributed)

Brouwer also wrote **Quest**, a game sharing most of Hack's source but using its own level generator (`quest.mklev.c`) that produced more interesting cave shapes instead of "boring rectangles." Quest was never officially distributed but leaked — a copy appeared at Vrije Universiteit, and it was listed among evidence seized in the 1990 Secret Service raids documented in Bruce Sterling's *The Hacker Crackdown*. Brouwer's own copy was lost when an email transfer from Amsterdam to Denmark was silently discarded by a gateway for exceeding 100 KB.

### Release History

* **Hack 1.0** (Dec 17, 1984) — Initial distribution, 15 shar parts
* **Hack 1.0.1** (Jan 1985) — Patch adding a few features (sender: `play@turing.UUCP`)
* **Hack 1.0.2** (Apr 1-14, 1985) — Full source redistribution over two weeks. The 1.0 single-day dump had overwhelmed many sites, and 1.0.1 required the `patch` utility which was not universally available. Female adventurer option, moon phase effects, pools, swamps.
* **Hack 1.0.3** (Jul 23, 1985) — Brouwer's final version, distributed as an ed script against 1.0.2. **19,239 lines** of meticulously crafted C code. Direct template for NetHack.

---

## NetHack Splits Off (1987)

On **July 28, 1987**, **Mike Stephenson**, **Izchak Miller**, and **Janet Walz** forked Hack 1.0.3 and released **NetHack 1.3d** — beginning independent development by a distributed group of programmers who wanted to modernize and expand Hack for a broader audience.

**Hack development effectively ceased** after Brouwer's 1.0.3 release. Fenlason had long since stepped away; Bresnick's 2000 profile notes that he still played his original version. In his own words, the later versions *"got so busy adding cool features and didn't spend enough time thinking about how it would effect playing the game."*

---

## The Lost Source: Rediscovery (2025)

### Fenlason's Code Resurfaces After 40 Years

Jay Fenlason's original source code was widely considered lost. By the time Julie Bresnick [profiled him in 2000](https://www.linux.com/news/train-life-nethacks-papa/), he had long since moved on: he still played his original version at home, but had, in her words, "voluntarily avoided participation pretty much since spawning the original Hack almost 20 years ago." Brouwer's near-total rewrite as Hack 1.0 became the version the world knew, and Fenlason's original largely dropped out of the historical record.

It resurfaced in 2025 when **Brian Harvey**, who had been Computer Director at Lincoln-Sudbury during Hack's development, provided his preserved copy of the school's PDP-11 backups to **Dan Stormont** for the [Snap!Hack](https://github.com/Sustainable-Games/snaphack) educational project. Dan published the complete original working tree at [Sustainable-Games/fenlason-hack](https://github.com/Sustainable-Games/fenlason-hack).

**Chain of custody:**

1. **Jay Fenlason** — original author, 1981-82 (with Woodland, Thome, and Payne)
2. **Brian Harvey** — preserved from LSRHS PDP-11 backups, 1982-2025
3. **Dan Stormont** — received in 2025 for the Snap!Hack project; published as fenlason-hack

### What the Recovery Revealed

The [protoHack](https://github.com/Critlist/protoHack) project restored Fenlason's original code to run on modern Linux and conducted a [detailed source comparison](research/COMPARISON.md) against all known Hack derivatives. Key findings:

* **The timeline is clearer**: *;login:* (June 1982) shows the Lincoln-Sudbury submission was on the 82-1 distribution tape completed before the Boston conference that July, and Fenlason recalled putting Hack on that tape.
* **Attribution corrections**: Shops, pets, long worms, and special rooms were NOT in Fenlason's original — they were added by Brouwer and the VU Amsterdam developers. Earlier secondary accounts that attributed these features to Fenlason have been corrected throughout this document.
* **Unique artifacts**: The recovered source contains items found in no other variant — the displacer beast, "alcohol" potion, "rust armor" scroll, and original incantation names. It also preserves personal development files (.cshrc, .login), Zork reference material, and experimental branches showing code evolution.
* **Structural evidence**: Fenlason's `struct monst` is simpler than all derivatives, with unique fields (`sinv`, `mstuck`, combined `mstat`) and missing fields (`mtame`, `wormno`, `mxlth`) that prove the pet and worm systems were later additions.

### Lineage

```
Rogue (1980, Toy/Wichman/Arnold)
  |
  v
Hack in Logo (~1981, Apple II, Fenlason)
  |
  v
Fenlason Hack in C (1981-82, PDP-11/70, Lincoln-Sudbury)
  |
  +---> PDP-11 Hack (~1983, Huisjes & de Wilde, VU Amsterdam)
  |       |
  |       +---> PC/IX Hack (1985, IBM PC UNIX port)
  |
  +---> Hack 1.0 (Dec 1984, Brouwer, CWI Amsterdam)
  |       |
  |       +---> Hack 1.0.1 (Jan 1985)
  |       +---> Hack 1.0.2 (Apr 1985)
  |       +---> Hack 1.0.3 (Jul 1985)  <-- restoHack source
  |               |
  |               +---> NetHack 1.3d (Jul 1987, Stephenson/Miller/Walz)
  |
  +---> Quest (Brouwer, undistributed — lost)
```

---

## The Resurrection: restoHack (2025)

### Modern Preservation

**restoHack** brought Hack 1.0.3 (1985) to modern systems:

* **K&R to ANSI C**: 250+ functions converted
* **Memory safety**: Buffer overflow protection without gameplay changes
* **Build system**: CMake replacing fragile Makefiles
* **Cross-platform**: Linux, BSD, macOS support verified

Code changes follow the documentation rules in [CODING_STANDARDS.md](CODING_STANDARDS.md). Changes that do affect behavior — the versioned save format, `flock()` locking, terminal-resize handling, and defensive overflow guards — are recorded in [CHANGELOG.md](CHANGELOG.md).

---

## References and Further Reading

### **Primary Sources**

* **David L. Craddock**, *Dungeon Hacks: How NetHack, Angband, and Other Roguelikes Changed the Course of Video Games* (Press Start Press, 2015) — Contains interview quotes from Jay Fenlason conducted via email in 2014. Pages 92 and 94 detail the origin of Hack.
* ***;login:* Vol. 7, No. 3 (June 1982)** — USENIX distribution tape announcement — [Archive.org scan](https://archive.org/details/login_june-1982/page/n13/mode/2up) | [Full text](https://archive.org/stream/login_june-1982/login_june-1982_djvu.txt)
* **[Jay Fenlason Interview (2000)](https://www.linux.com/news/train-life-nethacks-papa/)** — Julie Bresnick, Linux.com — profile with direct quotes from Fenlason ([NewsForge archive](https://web.archive.org/web/20030512112023/http://newsforge.com/article.pl?sid=00/12/03/2052246))
* **Brian Harvey**, ["Computer Hacking and Ethics" — A Case Study: The Lincoln-Sudbury Regional High School](https://people.eecs.berkeley.edu/~bh/lsrhs.html) (appendix to ACM Select Panel on Hacking position paper, 1985)
* **Andries Brouwer's** [Hack history page](https://homepages.cwi.nl/~aeb/games/hack/hack.html)
* **[Original 1985 Source Code](historical/original-source/)** — Hack 1.0.3 preserved codebase (restoHack base)
* **[FreeBSD Games Collection](https://cgit.freebsd.org/src/tree/games/hack?h=releng/4.10)** — Canonical preservation

### **Research Documents**

* **[TIMELINE.md](research/TIMELINE.md)** — Corrected chronology with primary source citations
* **[COMPARISON.md](research/COMPARISON.md)** — Evidentiary comparison of all known Hack source variants
* **[protoHack](https://github.com/Critlist/protoHack)** — Restoration of Fenlason's original 1981-82 source code

### **restoHack Project Documentation**

* **[README.md](../README.md)** — Quick start and installation guide
* **[CODING_STANDARDS.md](CODING_STANDARDS.md)** — Complete modernization methodology
* **[CHANGELOG.md](CHANGELOG.md)** — Version history and technical changes

### **Academic and Historical Context**

* **[Original_READ_ME](historical/original-source/Original_READ_ME)** — Fenlason's README as shipped with Brouwer's Hack
* **[BSD Games History](https://en.wikipedia.org/wiki/BSD_games)** — Unix gaming preservation
* **[Roguelike Development Community](https://www.roguebasin.com/)** — Modern roguelike evolution

### **Technical Implementation**

* **[CMakeLists.txt](../CMakeLists.txt)** — Modern build system implementation
* **[Source Code](../src/)** — Complete modernized codebase with documentation

---

## Contributing to This Document

This history represents ongoing research into Hack's development and cultural impact. Contributions are welcome:

1. **Corrections**: Historical inaccuracies or technical errors
2. **Additions**: Missing events, people, or technical details  
3. **Sources**: Additional primary source material or documentation
4. **Verification**: Independent confirmation of claims and dates

Please submit improvements via pull request to the [restoHack repository](https://github.com/Critlist/restoHack) with supporting documentation.

---

*This document is maintained as part of the restoHack software preservation project. Last updated: October 2026.*
