<!-- Shared research document maintained across protoHack and restoHack.
     This is an evidentiary document intended to enumerate verifiable
     differences between source trees. It is not a narrative history.
     File path references (e.g., original/hack.vars:97) refer to the
     protoHack repository's directory structure.
     Maintained upstream in protoHack and periodically synced here.
     See: https://github.com/Critlist/protoHack -->

# Source Comparison: Repository Code vs. Known Hack Variants

This document compares the source code in `original/` against all three known
derivatives of Jay Fenlason's Hack plus Andries Brouwer's Hack 1.0.

**Sources compared:**

| Label | Location | Origin |
|-------|----------|--------|
| **Repo** | `original/` | This repository |
| **PDP-11** | `hack-pdp11/src/` | Usenet shar, Michiel Huisjes, VU Amsterdam, Feb 1985 |
| **PC/IX** | `pcix/src/` | Usenet shar, "peterb at pbear", May 1985 |
| **Hack 1.0** | `hack-v1.0/ab_hack-1.0/` | Andries Brouwer, posted to Usenet Dec 1984 |

hack121 (DOS binary, no source extant) is referenced from the NetHack Wiki
bestiary tables where applicable.

---

## 1. Monster Table

The main monster table is an 8x7 array (`struct permonst mon[8][7]`).
Levels 1--4 are nearly identical across all variants. Divergence begins at
level 5.

### Levels 1--4 (shared across all variants with D&D names)

All four source variants use the same monsters and stats for levels 1--4.
Minor spelling differences exist (repo has "homonculous" and "gelatenous").

- Repo: `original/hack.vars:57-87`
- PDP-11: `hack-pdp11/src/hack.monst.c:34-65`
- PC/IX: `pcix/src/hack.monst.c:34-65`
- Hack 1.0: `hack-v1.0/ab_hack-1.0/hack.monst.c:7-34`

### Level 5+ divergences

| Slot | Repo (`original/hack.vars`) | PDP-11 / PC/IX | Hack 1.0 |
|------|------------------------|----------------|----------|
| 5.3 | **rust monster** R 5hd `:77` | rust monster R 5hd | **rust monster** R **10hd** |
| 5.4 | **giant scorpion** s `:78` | giant scorpion s | **scorpion** s |
| 5.5 | **teleporter** t `:79` | teleporter t | **tengu** t |
| 5.7 | **yeti** Y `:95` | **long worm** w | long worm w (or **wumpus** w under `NOWORM`) |
| 6.1 | **displacer beast** d `:97` | **large dog** d | large dog d |
| 6.6 | **ugod** u `:102` | ugod u | **unicorn** u |
| 6.7 | **xerp** x `:103` | yeti Y | yeti Y |
| 7.4 | **wumpus** w `:108` | xorn X | xorn X |
| 7.5 | **xorn** X `:109` | xerp x | **xan** x |
| 7.6 | **zelomp** z `:110` | zelomp z | **zruty** z |
| 8.1 | **dragon** D 10hd `:113` | dragon D **20hd** | dragon D **10hd** |
| 8.3 | **lurker above** ~ `:115` | lurker above **'** | lurker above **~** |
| 8.4 | **neo-otyugh** n `:116` | neo-otyugh n | **nurse** n |
| 8.6 | **purple worm** P 15hd `:118` | purple worm P **20hd** | purple worm P **15hd** |
| 8.7 | **demon** & 10hd `:119` | demon & **14hd** | demon & **10hd** |

Key observations:

- **Displacer beast** (`original/hack.vars:97`): Present only in the repo.
  Absent from PDP-11, PC/IX, Hack 1.0, and hack121. Occupies the 'd'
  slot that all other variants assign to large dog (needed for pet system).

- **Wumpus** (`original/hack.vars:108`): Present in the repo and in hack121.
  Absent from PDP-11 and PC/IX. In Hack 1.0, wumpus exists only as a
  fallback when the worm system is disabled:
  `hack-v1.0/ab_hack-1.0/hack.monst.c:41-45`:

  ```c
  #ifdef NOWORM
      { "wumpus",    'w',8,3,2,3,6,0 },
  #else
      { "long worm", 'w',8,3,5,1,4,0 },
  #endif NOWORM
  ```

  This confirms wumpus was in whatever code Brouwer started from and was
  replaced when the worm system was added.

- **Hit dice**: The repo matches Hack 1.0 for dragon (10hd), purple worm
  (15hd), and demon (10hd). PDP-11 and PC/IX inflate these to 20, 20,
  and 14 respectively.

- **Lurker above symbol**: Repo uses `~` (`original/hack.vars:115`),
  matching Hack 1.0 (`hack.monst.c:61`). PDP-11 and PC/IX use `'`.

### Monsters in separate structures

| Repo | PDP-11 / PC/IX | Hack 1.0 |
|------|----------------|----------|
| vampire bat (`original/hack.vars:124`) | li_dog, dog, la_dog, shk, treasurer, ghost, eel | li_dog, dog, la_dog, shopkeeper |

The repo has only the vampire bat in a separate struct. All other variants
have full dog/pet hierarchies and shopkeeper definitions.

### Monster count

- Repo: 56 (8x7 main) + 1 (vampire bat) = **57**
- PDP-11: 56 + shopkeeper, treasurer, li_dog, dog, la_dog, ghost, eel = **63+**
- PC/IX: same as PDP-11
- Hack 1.0: 58 (main, including shopkeeper and minotaur appended) + li_dog, dog, la_dog = **61**

---

## 2. Amulet

| Variant | Name | Reference |
|---------|------|-----------|
| Repo | **Amulet of Frobozz** | `original/hack.c:394`, `original/exp/exp1/hack.obj.c:10` |
| PDP-11 | Amulet of Frobozz | `hack-pdp11/src/hack.c:228`, `hack-pdp11/src/show.c:283` |
| PC/IX | Amulet of Frobozz | `pcix/src/hack.c:228`, `pcix/src/show.c:284` |
| Hack 1.0 | **Amulet of Yendor** | `hack-v1.0/ab_hack-1.0/hack.objnam.c:46`, `def.objects.h:11` |
| hack121 | *(not documented)* | *(binary only, no source)* |

PDP-11 help text also contains a third name, "Amulet of Guyver"
(`hack-pdp11/src/help:4`), inconsistent with its own code.

---

## 3. Potions

| Slot | Repo (`original/hack.vars:172-188`) | PDP-11 | PC/IX (`pcix/src/hack.vars.h:29-45`) | Hack 1.0 |
|------|------|--------|-------|----------|
| 1 | restore strength | restore strength | restore strength | restore strength |
| 2 | **alcohol** | **booze** | **confuse monster** | **booze** |
| 3 | invisibility | invisibility | invisibility | invisibility |
| 4 | fruit juice | fruit juice | fruit juice | fruit juice |
| 5 | healing | healing | healing | healing |
| 6 | paralysis | **paralysis** | paralysis | paralysis |
| 7 | monster detection | monster detection | monster detection | monster detection |
| 8 | object detection | object detection | object detection | object detection |
| 9 | sickness | sickness | sickness | sickness |
| 10 | confusion | confusion | confusion | confusion |
| 11 | gain strength | gain strength | gain strength | gain strength |
| 12 | speed | speed | speed | speed |
| 13 | blindness | blindness | blindness | blindness |
| 14 | gain level | gain level | gain level | gain level |
| 15 | extra healing | extra healing | extra healing | extra healing |
| 16 | -- | -- | -- | **levitation** |

The repo's "alcohol" (`original/hack.vars:174`) is unique among extant source
variants. PDP-11 and Hack 1.0 use "booze". PC/IX replaced booze with
"confuse monster" (per wiki, reinstating confuse monster as a potion after
PDP-11 removed the scroll).

---

## 4. Scrolls

| Slot | Repo (`original/hack.vars:207-223`) | PDP-11 / PC/IX (`pcix/src/hack.vars.h:75-91`) | Hack 1.0 |
|------|------|----------------|----------|
| 1 | enchant armor | enchant armor | enchant armor |
| 2 | **confuse monster** | **losing** (S_DROP) | confuse monster |
| 3 | **blank** | **curse levels** (S_CURSE_LEV) | *(blank paper, different position)* |
| 4 | remove curse | remove curse | remove curse |
| 5 | enchant weapon | enchant weapon | enchant weapon |
| 6 | create monster | create monster | create monster |
| 7 | damage weapon | damage weapon | damage weapon |
| 8 | **rust armor** | **genocide** | **destroy armor** |
| 9 | genocide | **destroy armor** (S_DESTR_ARMOR) | genocide |
| 10 | light | light | light |
| 11 | **teleportation** | **transportation** (elevator) | teleportation |
| 12 | gold detection | gold detection | gold detection |
| 13 | identify | identify | identify |
| 14 | magic mapping | magic mapping | magic mapping |
| 15 | fire | fire | fire |

- **confuse monster** (`original/hack.vars:209`): Repo and Hack 1.0 retain it.
  PDP-11 and PC/IX replaced it with "losing" (`S_DROP`), documented in
  `hack-pdp11/src/hack.do.c:69-73`:

  ```c
  /* New scroll instead of scroll of monster confusion. This scroll
   * drops everything you have on you. And put it somewhere on the level
   * Michiel and Fred */
  ```

- **rust armor** (`original/hack.vars:215`): Unique to the repo. All other
  variants with extant source use "destroy armor".

- **teleportation** (`original/hack.vars:218`): Repo and Hack 1.0 use simple
  teleportation. PDP-11/PC/IX changed it to an elevator with level-jumping
  (`hack-pdp11/src/hack.do.c:195-198`):

  ```c
  /* Extended by Michiel and Fred:
   * One can jump between levels */
  ```

---

## 5. Wands

| Repo (`original/hack.vars:243-268`) | PDP-11 / PC/IX (`pcix/src/hack.vars.h:117-136`) | Hack 1.0 |
|------|----------------|----------|
| light | light | light |
| secret door + trap detection | secret door + trap detection | secret door detection |
| create monster | create monster | create monster |
| **striking** | **exhaustion** | **striking** |
| slow monster | slow monster | slow monster |
| speed monster | speed monster | speed monster |
| undead turning | undead turning | undead turning |
| polymorph | polymorph | polymorph |
| cancelation | cancellation | cancellation |
| teleport monster | teleport monster | teleport monster |
| digging | digging | digging |
| magic missile | magic missile | magic missile |
| fire | fire | fire |
| sleep | sleep | sleep |
| cold | cold | cold |
| death | death | death |
| -- | **confusion** | -- |
| -- | **door closing** | -- |
| -- | -- | **wishing** |
| -- | -- | **make invisible** |

The repo retains "striking" (`original/hack.vars:251`), matching Hack 1.0.
PDP-11/PC/IX replaced it with "exhaustion".

The repo lacks wands of confusion and door closing (VU Amsterdam additions)
and wands of wishing and make invisible (Brouwer additions).

---

## 6. Rings

The repo defines 17 ring types (`original/hack.vars:288-306`). PDP-11/PC/IX
define 16. The difference:

- **increase hit points** (`original/hack.vars:305`): Present in the repo
  and hack121. Absent from PDP-11/PC/IX.

---

## 7. Food

| Repo (`original/hack.vars:146-149`) | PDP-11 / PC/IX | Hack 1.0 |
|------|----------------|----------|
| food ration | food ration | food ration |
| fruit | tripe ration | tripe ration |
| -- | pancake | pancake |
| -- | dead lizard | dead lizard |
| -- | cookie | fortune cookie |
| -- | orange | orange |
| -- | apple | apple |
| -- | pear | pear |
| -- | melon | melon |
| -- | banana | banana |
| -- | candy bar | candy bar |
| -- | egg | egg |
| -- | -- | carrot |
| -- | -- | tin |
| -- | -- | clove of garlic |
| -- | -- | corpse |

The repo has **2 food types**. PDP-11/PC/IX have 12. Hack 1.0 has 16.

---

## 8. Armor

| Repo (`original/hack.vars:138-145`) | PDP-11 / PC/IX | Hack 1.0 |
|------|----------------|----------|
| leather (AC 8) | leather armor | leather armor |
| ring (AC 7) | ring mail | ring mail |
| scale (AC 6) | scale mail | scale mail |
| chain (AC 5) | chain mail | chain mail |
| splint (AC 4) | splint mail | splint mail |
| plate (AC 3) | plate mail | plate mail |
| -- | banded mail | banded mail |
| -- | studded leather armor | studded leather armor |
| -- | elven cloak | elven cloak |
| -- | -- | helmet |
| -- | -- | shield |
| -- | -- | pair of gloves |

The repo has **6 armor types** (suits only). PDP-11/PC/IX have 9.
Hack 1.0 has 12.

---

## 9. Game Systems Absent from Repo

The following systems are present in PDP-11, PC/IX, and/or Hack 1.0
but entirely absent from the repo:

### Pet / dog system

PDP-11/PC/IX define three dog types and a full movement AI:

- `hack-pdp11/src/hack.monst.c:101-103` (li_dog, dog, la_dog)
- `hack-pdp11/src/hack.mon.do.c:468-469` (dog_move dispatch)
- `hack-pdp11/src/hack.h:218` (mtame bit field in struct monst)

Hack 1.0: `hack-v1.0/ab_hack-1.0/hack.dog.c` (entire file, 400+ lines)

Brouwer's own README confirms this was his addition:
`hack-v1.0/ab_hack-1.0/READ_ME:11-12`:
> "The current version is more than twice as large (with such new features as
> **the dog, the long worms, the shops**, etc.) and almost entirely rewritten"

The repo's `struct monst` (`original/hack.h:70-82`) has no `mtame`, `angry`,
`ale`, `wormno`, or `mxlth` fields.

### Worm segment system

PDP-11/PC/IX: `struct wseg` at `hack-pdp11/src/hack.h:231-234`,
worm functions throughout.

Hack 1.0: `hack-v1.0/ab_hack-1.0/hack.worm.c` (entire file),
`def.wseg.h`.

Not present in the repo. The repo uses yeti in the slot PDP-11/PC/IX
assign to long worm, and wumpus in a slot where other variants have xorn.
Hack 1.0's `#ifdef NOWORM` conditional (`hack.monst.c:41-45`) confirms
wumpus was the original monster before the worm system existed.

### Special room types

PDP-11/PC/IX define and generate four special room types:

- `mkzoo()`: `hack-pdp11/src/mklev.make.c:275`, `pcix/src/mklev.make.c:275`
- `mkyard()` (graveyard): `hack-pdp11/src/mklev.make.c:297`, `pcix/src/mklev.make.c:297`
- `mkswamp()`: `hack-pdp11/src/mklev.make.c:397`, `pcix/src/mklev.make.c:397`
- `mk_knox()` (vault): `hack-pdp11/src/mklev.make.c:450`, `pcix/src/mklev.make.c:450`

Hack 1.0 has shops (`mklv.shk.c`), vaults (`hack.vault.c`), and zoos.

The VU Amsterdam developers documented these as their own additions in
the PDP-11 Hack readme (`hack-pdp11/src/READ_ME:33-41`):
> "Some additions we (Fred de Wilde and Michiel Huisjes) made are
>
> - The Vault / - The Swamp / - The Zoo / - The Graveyard"

The repo has none of these. Its `mklev.c` generates only basic rooms,
corridors, and mazes.

### MAXLEVEL

PDP-11/PC/IX: `#define MAXLEVEL 40` (`hack-pdp11/src/hack.save.c:15`,
`pcix/src/hack.do.misc.c:14`).

Not defined in the repo.

### Pool / water locations

PDP-11/PC/IX: `#define POOL 7` (`hack-pdp11/src/hack.h:42`), with ale
(aquatic) monster support.

Not present in the repo. The repo's `original/hack.h:7-11` defines location
types WALL through ROOM only.

---

## 10. Structural Differences

### struct monst

| Field | Repo (`original/hack.h:70-82`) | PDP-11 (`hack-pdp11/src/hack.h:202-222`) | Hack 1.0 (`def.monst.h`) |
|-------|------|--------|----------|
| nmon | yes | yes | yes |
| mx, my | yes | yes | yes |
| mhp, orig_hp | yes (char) | yes (int) | yes (int) |
| invis | yes | yes | yes |
| cham | yes | yes | yes |
| mspeed | yes | yes | yes |
| mcan | yes | yes | yes |
| mstuck | yes | -- | -- |
| sinv | yes | -- | -- |
| mstat | yes (2 bits) | -- | -- |
| angry | -- | yes | yes |
| ale | -- | yes | -- |
| msleep | -- | yes (bit) | yes |
| mfroz | -- | yes (bit) | yes |
| mconf | -- | yes (bit) | yes |
| mflee | -- | yes (bit) | yes |
| mtame | -- | yes (bit) | yes |
| wormno | -- | yes (5 bits) | yes |
| mxlth | -- | yes | yes |
| mextra | -- | yes | yes |

The repo's struct is notably simpler. It uses a combined `mstat` field
(2 bits for flee/sleep/frozen) rather than separate bit fields.
It includes `sinv` and `mstuck` fields not present in other variants.

### struct obj

The repo's `struct obj` (`original/hack.h:101-110`) lacks the `unpaid` field
present in PDP-11 (`hack-pdp11/src/hack.h:188`) and Hack 1.0 (for shop tracking).

### struct you (player)

The repo (`original/hack.h:134-164`) lacks:

- `uinshop`, `uinzoo`, `uinyard`, `uinswamp`, `uinknox` (PDP-11/PC/IX special room tracking)
- `uhcursed` (PDP-11/PC/IX hero curse flag, tied to S_CURSE_LEV scroll)

---

## 11. README Comparison

Fenlason's README exists in three forms:

| Location | Content |
|----------|---------|
| Repo `original/READ_Me` | 11 setup steps, mentions MAGIC mode (gid==42), SMALL/VTONL paragraph |
| Hack 1.0 `Original_READ_ME` | 9 setup steps (renumbered, "2.5"), no SMALL/VTONL paragraph |
| Hack 1.0 `READ_ME` | Brouwer's own, credits Fenlason "at lincolnsudbury" |

All three credit the same collaborators: Kenny Woodland (KW), Mike Thome
(MT), Jon Payne (JP), and reference the SFSU Logo Workshop and Mike Clancy.

Brouwer's README (`hack-v1.0/ab_hack-1.0/READ_ME:6`) identifies
Fenlason's location as "lincolnsudbury" -- consistent with
Lincoln-Sudbury Regional High School.

---

## 12. Scroll Name Strings

The scroll incantation strings provide a fingerprint. The repo
(`original/hack.vars:224-241`) uses lowercase; PDP-11/PC/IX
(`pcix/src/hack.vars.h:94-111`) use uppercase. Both share the same
words in the same order, with one exception:

| Repo | PDP-11 / PC/IX |
|------|----------------|
| "Velox Neb" | "VELOX NEB" |
| "Foobie Bletch" | "FOOBIE BLETCH" |
| "Temov" | "TEMOV" |
| "Garven Deh" | "GARVEN DEH" |
| "Zelgo Mer" | "ZELGO MER" |
| "Andova Begarin" | "ANDOVA BEGARIN" |
| "Elam Ebow" | "ELAM EBOW" |
| "Kernod Wel" | "KERNOD WEL" |
| "Tharr" | "THARR" |
| "Venzar Borgavve" | "VENZAR BORGAVVE" |
| **"Elbib Yloh"** | *(commented out)* |
| "Verr Yed Horre" | "VERR YED HORRE" |
| "Juyed Awk Yacc" | "JUYED AWK YACC" |
| "Hackem Muche" | "HACKEM MUCHE" |
| "Lep Gex Ven Zea" | "LEP GEX VEN ZEA" |
| "Daiyen Fooels" | "DAIYEN FOOELS" |

"Elbib Yloh" ("Holy Bible" reversed) is present in the repo but commented
out in PC/IX (`pcix/src/hack.vars.h:105`: `/* "ELBIB YLOH", */`).

---

## 13. Files Unique to This Repository

The repo contains files not present in any known Hack distribution:

- `original/.cshrc`, `original/.login`, `original/.exrc`, `original/.msgsrc` --
  Shell and editor configuration from a development environment.
  `.login` sets prompt to `Hack:` and references IBM terminal via
  `term -c -M ibm`.

- `original/zork/` -- Zork reference material (`zork.h`, `dungeon.c`),
  likely used as design reference during Hack development.

- `original/exp/` and `original/exp/exp1/` -- Development branches showing code
  evolution. `exp/exp1/` contains a modularized refactoring splitting code
  into fight, move, screen, and object subsystems (~4600 lines).
  `exp/exp1/bugs1` contains V7 lint output.

- `original/record` -- High score table with entries from users "karll",
  "nike", "shm", "jaws", "thm", "adam", and "Jay the Hacker"
  (`original/record:9`). Jay's entry shows death by "Bus error" (SIGBUS crash).

---

## 14. Summary of Unique Features

Features present **only** in the repo (not in any variant with extant source):

| Feature | Reference |
|---------|-----------|
| Displacer beast monster | `original/hack.vars:97` |
| "alcohol" potion (not "booze") | `original/hack.vars:174` |
| "rust armor" scroll (not "destroy armor") | `original/hack.vars:215` |
| 2-item food list (food ration, fruit) | `original/hack.vars:146-149` |
| 6-type armor list (suits only) | `original/hack.vars:138-145` |
| `sinv` and `mstuck` fields in struct monst | `original/hack.h:73,79` |
| Combined `mstat` field (2-bit) | `original/hack.h:77` |
| "Elbib Yloh" scroll name (uncommented) | `original/hack.vars:235` |
| Personal dev environment files | `original/.cshrc`, `original/.login`, `original/.exrc` |
| Zork reference material | `original/zork/` |
| Experimental refactoring branches | `original/exp/`, `original/exp/exp1/` |

Features the repo **shares with Hack 1.0** but not PDP-11/PC/IX:

| Feature | Repo | Hack 1.0 |
|---------|------|----------|
| Wumpus monster | `original/hack.vars:108` | `hack.monst.c:42` (under `NOWORM`) |
| Dragon 10hd | `original/hack.vars:113` | `hack.monst.c:59` |
| Demon 10hd | `original/hack.vars:119` | `hack.monst.c:65` |
| Purple worm 15hd | `original/hack.vars:118` | `hack.monst.c:64` |
| Lurker above ~ | `original/hack.vars:115` | `hack.monst.c:61` |
| Wand of striking | `original/hack.vars:251` | `def.objects.h:209` |
| Scroll of confuse monster | `original/hack.vars:209` | `def.objects.h:179` |
| Scroll of teleportation (simple) | `original/hack.vars:218` | *(same name)* |

Features the repo **shares with hack121** (DOS binary, per NetHack Wiki)
but not PDP-11/PC/IX:

- Wumpus monster
- Ring of increase hit points (`original/hack.vars:305`)
- "alcohol" potion (hack121 also uses "alcohol")
