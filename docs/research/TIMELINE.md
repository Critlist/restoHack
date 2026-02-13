<!-- Shared research document maintained across protoHack and restoHack.
     Maintained upstream in protoHack and periodically synced here.
     See: https://github.com/Critlist/protoHack -->

# Hack Development Timeline

## Corrected Chronology

Primary source evidence from *;login:* (Vol. 7, No. 3, June 1982) shows the
code was already complete and submitted for the distribution tapes **before**
the Summer 1982 USENIX conference took place.

### Timeline

- **1980** — **Rogue** released for Unix by Michael Toy and Glenn Wichman at
  UC Santa Cruz, with later contributions by Ken Arnold.

- **~Summer 1980** — Brian Harvey invites Lincoln-Sudbury students to
  California for a summer class he is teaching at San Francisco State
  University. During the trip, Jay Fenlason visits UC Berkeley and plays
  *Rogue* for the first time: "The summer between my sophomore and junior
  years, Brian [Harvey] invited some of us students to come out to California
  to [work as teacher's assistants during] a summer class he was teaching at
  San Francisco State University. While we were out there, we took a trip to
  UC Berkeley, where I got to play *Rogue* for the first time." He is
  immediately hooked: "I think I got a little bit obsessed then, and like any
  obsessed person who'd been cut off, I decided to build my own." (Craddock,
  pp. 92)

- **~1980–81** — Fenlason creates the earliest version of Hack in **Logo**
  (Turtle Graphics) on an **Apple II**. Once satisfied with the Logo
  prototype, he migrates the game to C on the PDP-11/70 at Lincoln-Sudbury.
  (Craddock, p. 94)

- **1981–82** — **Jay Fenlason** develops Hack in C at Lincoln-Sudbury
  Regional High School — his "first semester programming project" (READ_Me).
  The development environment is a PDP-11/70 running V7 Unix (alpha test site
  for 2.9BSD), administered by a student-run Computer Center Users Society
  with ~50 members who have keys and unsupervised access. The game is an
  implementation of Rogue with 56 monster types (vs. Rogue's 26) and expanded
  dungeon features. Collaborators and their contributions:
  - **Kenny Woodland** (KW) — Maze-generating code for the bottom level, the
    original BUZZ() function, and "general random things" (READ_Me). Also
    contributed code for a game called *Bombs*. (Craddock, p. 94)
  - **Mike Thome** (MT) — Invented the chameleon, a monster that "could take
    on the appearance and abilities of other types of monsters" (Craddock,
    p. 94). Credited in READ_Me as "The original chamelian" [sic].
  - **Jonathan Payne** (JP) — The lock file system and "the massive CURS()"
    (READ_Me). Also the author of JOVE (Jonathan's Own Version of Emacs).
    Payne and Fenlason had "a friendly rivalry going on as to whose program,
    *JOVE* or *Hack*, could update the screen most efficiently" (Craddock,
    p. 94). Both programs shipped on the same USENIX 82-1 tape.

- **First half of 1982** — Brian Harvey, Computer Director at Lincoln-Sudbury
  (1979-1982), submits student projects — including Hack and JOVE — for
  inclusion on the USENIX distribution tapes (82-1). Harvey, whose background
  was in the MIT and Stanford AI labs, had built the school's computing
  environment to resemble those labs: "a powerful computer system, with lots
  of software tools, an informal community spirit, and not much formal
  curriculum."

- **June 1982** — *;login:* Vol. 7, No. 3 reports that the first 1982 USENIX
  distribution tape has been completed, noting the Lincoln-Sudbury Regional
  High School submission containing "quite a few games."

- **July 8, 1982 (Boston USENIX)** — Michael C. Toy and Kenneth C. R. C.
  Arnold present "Rogue: Where It Has Been, Why It Was There, and Why It
  Shouldn't Have Been There in the First Place." The 82-1 distribution tapes
  — with Hack already on them — are distributed at the same conference.

- **~1983/84** — **Michiel Huisjes** and **Fred de Wilde** at Vrije Universiteit,
  Amsterdam produce a PDP-11 version based on pre-1.0 Hack that Andries
  Brouwer later described (in an April 1985 Usenet reply) as copied from his
  directory without permission while 1.0 was still in development; he noted it
  was not in shape for distribution and lacked many features present in Hack
  1.0.

- **Dec 17, 1984** — **Andries Brouwer** at Stichting Mathematisch Centrum
  (CWI), Amsterdam distributes **Hack 1.0** to `net.sources` in 15 parts.
  Sender: `play@mcvax.UUCP (funhouse)`. The announcement promised 10 parts
  but there were actually 15, all sent on the same day. Near-complete rewrite
  of Fenlason's code — Brouwer later wrote that 1.0.3 "contains very little
  if anything from the original sources."

- **~Dec 1984** — `net.games.hack` newsgroup created by Gene Spafford due to
  the volume of Hack traffic on net.games and net.games.rogue.

- **Jan 1985** — **Hack 1.0.1** patch adding a few features. Sender:
  `play@turing.UUCP`.

- **Feb 1985** — **Hack for PDP-11** published on Usenet (`net.sources`) by
  Huisjes. Five shar parts.

- **Apr 1-14, 1985** — **Hack 1.0.2** re-distributed as a fresh copy of the
  full source, spread over two weeks. Sender: `aeb@mcvax.UUCP`. The 1.0
  distribution's single-day dump had overwhelmed many sites, and the 1.0.1
  patch required the `patch` utility which was not universally available. Part
  2 was famously missing from Google Groups until Ray Chason located it in
  2005 in the DECUS library at `vmsone.com/~decuslib/unixsig/uni87a/hack/`.

- **May 1985** — **PC/IX Hack** published on Usenet (`net.sources.games`).
  Port of the PDP-11 version to IBM PC UNIX. Five shar parts.

- **Jul 23, 1985** — **Hack 1.0.3** distributed as an ed script against 1.0.2.
  Last version distributed by Brouwer. Well preserved — copies found across
  the net.

- **Jul 28, 1987** — **NetHack 1.3d** released. Mike Stephenson, Izchak
  Miller, and Janet Walz fork Hack 1.0.3 and begin independent development.

### Quest (Brouwer, undistributed)

Brouwer also wrote **Quest**, a game sharing most of Hack's source but using
its own level generator (`quest.mklev.c`) that produced more interesting cave
shapes instead of "boring rectangles." Quest was never officially distributed
but leaked — a copy appeared at Vrije Universiteit, and it was listed among
evidence seized in the 1990 Secret Service raids documented in Bruce Sterling's
*The Hacker Crackdown*. Brouwer's own copy was lost when an email transfer
from Amsterdam to Denmark was silently discarded by a gateway for exceeding
100 KB.

## Key Evidence

The *;login:* evidence shows Hack was already submitted for the distribution
tapes before the conference. Jay encountered Rogue during a trip to UC Berkeley
while attending Harvey's summer class at SFSU — confirming his READ_Me
acknowledgment: "This entire program would not have been possible without the
SFSU Logo Workshop ... without whom I would never have seen Rogue." The
Craddock interview further reveals that the earliest version of Hack was
written in Logo (Turtle Graphics) for the Apple II before being ported to C,
explaining the depth of Fenlason's gratitude to the Logo Workshop: it provided
not just the encounter with Rogue but also the first development environment
for Hack itself.

## Licensing

Both original authors have issued BSD-type licenses allowing free
redistribution and modification:

**Jay Fenlason** (covers all code he wrote):
> It is shared under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC-BY-NC-SA 4.0), as specified by Jay Fenlason when the source was archived by the Snap!Hack project. A copy of the license is available at: https://creativecommons.org/licenses/by-nc-sa/4.0/

**CWI** (covers Brouwer's additions, formerly "Stichting Mathematisch
Centrum"):
> Copyright (c) 1985, Stichting Centrum voor Wiskunde en Informatica,
> Amsterdam — 3-clause BSD license

Statement issued by Dick Broekhuis, controller CWI.

Full license texts preserved in Brouwer's published account.

## Primary Sources

- David L. Craddock, *Dungeon Hacks: How NetHack, Angband, and Other Roguelikes
  Changed the Course of Video Games* (Press Start Press, 2015) — Contains
  interview quotes from Jay Fenlason conducted via email in 2014. Pages 92 and
  94 detail the origin of Hack: the encounter with Rogue at UC Berkeley, the
  Logo prototype on Apple II, and the contributions of each collaborator.
- *;login:* Vol. 7, No. 3 (June 1982) — USENIX distribution tape announcement
  - [Archive.org scan](https://archive.org/details/login_june-1982/page/n13/mode/2up)
  - [Full text](https://archive.org/stream/login_june-1982/login_june-1982_djvu.txt)
- Jay Fenlason's `Original_READ_ME` (preserved in Brouwer's Hack 1.0)
- Jay Fenlason's `READ_Me` (preserved in the protoHack repository at `original/READ_Me`)
- Andries Brouwer's [Hack history page](https://homepages.cwi.nl/~aeb/games/hack/hack.html)
- Brian Harvey, ["Computer Hacking and Ethics" — A Case Study: The Lincoln-Sudbury
  Regional High School](https://people.eecs.berkeley.edu/~bh/lsrhs.html) (appendix
  to ACM Select Panel on Hacking position paper, 1985)
- Neozeed, "While hunting for Hack 1.0 in usenet" (reproduces Brouwer's 1985
  Usenet response about the PDP-11 version)
- Usenet archives via SuperGlobalMegaCorp Altavista Archive

## Lineage

```
Rogue (1980, Toy/Wichman/Arnold)
  |
  v
Hack in Logo (~1980-81, Apple II, Fenlason)
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
  |       +---> Hack 1.0.3 (Jul 1985)
  |               |
  |               +---> NetHack 1.3d (Jul 1987, Stephenson/Miller/Walz)
  |
  +---> Quest (Brouwer, undistributed — lost)
```
