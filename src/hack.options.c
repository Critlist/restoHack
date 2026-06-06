/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.options.c - version 1.0.3 */
/* $FreeBSD$ */

/*
 * Game options system for 1984 Hack - player preferences and configuration
 * Original 1984 source: docs/historical/original-source/hack.options.c
 *
 * Key modernizations: ANSI C function signatures
 */

#include "hack.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
extern char *eos();

/* Forward declaration */
void parseoptions(char *opts, boolean from_env);

void initoptions(void) {
  char *opts;

  flags.time = flags.nonews = flags.notombstone = flags.end_own =
      flags.standout = flags.nonull = FALSE;
  flags.no_rest_on_space = TRUE;
  flags.invlet_constant = TRUE;
  flags.end_top = 5;
  flags.end_around = 4;
  flags.female = FALSE; /* players are usually male */

  if ((opts = getenv("HACKOPTIONS"))) {
    /* MODERN: Validate environment variable length to prevent attacks */
    if (strlen(opts) > 1024) {
      pline("HACKOPTIONS too long (max 1024 chars) - ignoring.");
    } else {
      parseoptions(opts, TRUE);
    }
  }
}

/* MODERN: Track recursion depth to prevent stack overflow */
static int parse_depth = 0;
#define MAX_PARSE_DEPTH 50

void parseoptions(char *opts, boolean from_env) {
  char *op, *op2;
  unsigned num;
  boolean negated;

  /* MODERN: Prevent stack overflow from recursive parsing */
  if (++parse_depth > MAX_PARSE_DEPTH) {
    pline("Options parsing too deep - possible malformed input.");
    parse_depth--;
    return;
  }

  if ((op = index(opts, ','))) {
    *op++ = 0;
    parseoptions(op, from_env);
  }
  parse_depth--;
  if ((op = index(opts, ' '))) {
    op2 = op;
    /* MODERN: Safe space removal with bounds checking */
    char *end = opts + strlen(opts);
    while (op < end && *op) {
      if (*op != ' ' && op2 < end) {
        *op2++ = *op;
      }
      op++;
    }
    if (op2 < end) *op2 = '\0'; /* Ensure null termination */
  }
  if (!*opts)
    return;
  negated = FALSE;
  while ((*opts == '!') || !strncmp(opts, "no", 2)) {
    if (*opts == '!')
      opts++;
    else
      opts += 2;
    negated = !negated;
  }

  if (!strncmp(opts, "standout", 8)) {
    flags.standout = !negated;
    return;
  }

  if (!strncmp(opts, "null", 3)) {
    flags.nonull = negated;
    return;
  }

  if (!strncmp(opts, "tombstone", 4)) {
    flags.notombstone = negated;
    return;
  }

  if (!strncmp(opts, "news", 4)) {
    flags.nonews = negated;
    return;
  }

  if (!strncmp(opts, "time", 4)) {
    flags.time = !negated;
    flags.botl = 1;
    return;
  }

  if (!strncmp(opts, "restonspace", 4)) {
    flags.no_rest_on_space = negated;
    return;
  }

  if (!strncmp(opts, "fixinv", 4)) {
    if (from_env)
      flags.invlet_constant = !negated;
    else
      pline("The fixinvlet option must be in HACKOPTIONS.");
    return;
  }

  if (!strncmp(opts, "male", 4)) {
    flags.female = negated;
    return;
  }
  if (!strncmp(opts, "female", 6)) {
    flags.female = !negated;
    return;
  }

  /* name:string */
  if (!strncmp(opts, "name", 4)) {
    extern char plname[PL_NSIZ];
    if (!from_env) {
      pline("The playername can be set only from HACKOPTIONS.");
      return;
    }
    op = index(opts, ':');
    if (!op)
      goto bad;
    (void)strncpy(plname, op + 1, sizeof(plname) - 1);
    plname[sizeof(plname) - 1] = '\0'; /* MODERN: Ensure null termination */
    return;
  }

  /* endgame:5t[op] 5a[round] o[wn] */
  if (!strncmp(opts, "endgame", 3)) {
    op = index(opts, ':');
    if (!op)
      goto bad;
    op++;
    while (*op) {
      num = 1;
      if (digit(*op)) {
        num = atoi(op);
        /* MODERN: Validate numeric input to prevent integer overflow */
        if (num > 9999) {
          pline("Option value too large (max 9999): %u", num);
          goto bad;
        }
        while (digit(*op))
          op++;
      } else if (*op == '!') {
        negated = !negated;
        op++;
      }
      switch (*op) {
      case 't':
        flags.end_top = num;
        break;
      case 'a':
        flags.end_around = num;
        break;
      case 'o':
        flags.end_own = !negated;
        break;
      default:
        goto bad;
      }
      while (letter(*++op))
        ;
      if (*op == '/')
        op++;
    }
    return;
  }
bad:
  if (!from_env) {
    if (!strncmp(opts, "help", 4)) {
      pline("%s%s%s",
            "To set options use `HACKOPTIONS=\"<options>\"' in your "
            "environment, or ",
            "give the command 'o' followed by the line `<options>' while "
            "playing. ",
            "Here <options> is a list of <option>s separated by commas.");
      pline("%s%s%s",
            "Simple (boolean) options are rest_on_space, news, time, ",
            "null, tombstone, (fe)male. ",
            "These can be negated by prefixing them with '!' or \"no\".");
      pline("%s",
            "A string option is name, as in HACKOPTIONS=\"name:Merlin-W\".");
      pline(
          "%s%s%s",
          "A compound option is endgame; it is followed by a description of "
          "what ",
          "parts of the scorelist you want to see. You might for example say: ",
          "`endgame:own scores/5 top scores/4 around my score'.");
      return;
    }
    pline("Bad option: %.50s.", opts); /* MODERN: Limit format string length to prevent attacks */
    pline("Type `o help<cr>' for help.");
    return;
  }
  puts("Bad syntax in HACKOPTIONS.");
  puts("Use for example:");
  puts("HACKOPTIONS=\"!restonspace,notombstone,endgame:own/5 topscorers/4 "
       "around me\"");
  getret();
}

int doset(void) {
  char buf[BUFSZ];

  pline("What options do you want to set? ");
  getlin(buf);
  if (!buf[0] || buf[0] == '\033') {
    /* MODERN: Safe options string building with bounds tracking */
    int len = 0, remaining = BUFSZ;

    len = snprintf(buf, remaining, "HACKOPTIONS=");
    if (len >= remaining)
      len = remaining - 1;
    remaining -= len;

    if (remaining > 0) {
      int added = snprintf(buf + len, remaining, "%s",
                           flags.female ? "female," : "male,");
      if (added > 0 && added < remaining) {
        len += added;
        remaining -= added;
      }
    }
    if (flags.standout && remaining > 0) {
      int added = snprintf(buf + len, remaining, "standout,");
      if (added > 0 && added < remaining) {
        len += added;
        remaining -= added;
      }
    }
    if (flags.nonull && remaining > 0) {
      int added = snprintf(buf + len, remaining, "nonull,");
      if (added > 0 && added < remaining) {
        len += added;
        remaining -= added;
      }
    }
    if (flags.nonews && remaining > 0) {
      int added = snprintf(buf + len, remaining, "nonews,");
      if (added > 0 && added < remaining) {
        len += added;
        remaining -= added;
      }
    }
    if (flags.time && remaining > 0) {
      int added = snprintf(buf + len, remaining, "time,");
      if (added > 0 && added < remaining) {
        len += added;
        remaining -= added;
      }
    }
    if (flags.notombstone && remaining > 0) {
      int added = snprintf(buf + len, remaining, "notombstone,");
      if (added > 0 && added < remaining) {
        len += added;
        remaining -= added;
      }
    }
    if (flags.no_rest_on_space && remaining > 0) {
      int added = snprintf(buf + len, remaining, "!rest_on_space,");
      if (added > 0 && added < remaining) {
        len += added;
        remaining -= added;
      }
    }
    if ((flags.end_top != 5 || flags.end_around != 4 || flags.end_own) &&
        remaining > 0) {
      int added =
          snprintf(buf + len, remaining, "endgame: %u topscores/%u around me",
                   flags.end_top, flags.end_around);
      if (added > 0 && added < remaining) {
        len += added;
        remaining -= added;
      }
      if (flags.end_own && remaining > 0) {
        added = snprintf(buf + len, remaining, "/own scores");
        if (added > 0 && added < remaining) {
          len += added;
          remaining -= added;
        }
      }
    } else {
      /* Remove trailing comma if present */
      if (len > 0 && buf[len - 1] == ',') {
        buf[len - 1] = 0;
      }
    }
    pline("%s", buf); /* Modern: Fix format string vulnerability */
  } else
    parseoptions(buf, FALSE);

  return (0);
}
