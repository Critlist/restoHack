# restoHack Coding Standards

## Documentation Policy for Non-Original Code

This project preserves the original Hack 1.0.3 (1985) source code while making minimal modern additions for buildability and stability. To maintain clear distinction between original and modern code:

### Documentation Requirement

**ALL** code that is not part of the original Hack 1.0.3 codebase or simple K&R to ANSI C conversion MUST be documented with structured comments explaining:

1. **WHY** the addition was necessary
2. **HOW** it was implemented  
3. **WHAT** it preserves from the original
4. **WHAT** it adds that's modern

### Documentation Format

```c
/**
 * MODERN ADDITION (YYYY): Brief description
 * 
 * WHY: Explanation of the problem that required this addition
 * 
 * HOW: Technical explanation of the implementation approach
 * 
 * PRESERVES: What original 1984 behavior/logic is maintained
 * ADDS: What modern functionality or compatibility is provided
 */
```

### Examples of Code Requiring Documentation

- **Bug fixes** that add new logic (not just syntax fixes)
- **Defensive programming** additions (error checking, resource cleanup)
- **Compatibility shims** for modern systems
- **New functions** not present in original codebase
- **Modified algorithms** or data structures
- **Added includes** for modern headers not in original

### Examples of Code NOT Requiring Documentation

- **K&R to ANSI C conversion**: Function signature modernization
- **Simple syntax fixes**: Adding void, const keywords
- **Header organization**: Moving declarations to proper files
- **Build system**: CMake, compiler flags, etc.
- **Whitespace/formatting**: Code style consistency

### Original Source Preservation Rule

### NEVER DELETE ORIGINAL CODE

*Note: comment markers in the source say "1984" (e.g. `ORIGINAL 1984 CODE`). They predate a date correction and refer to Hack 1.0.3, released in 1985. Keep using the existing marker so it stays greppable.*

All original source code must be preserved using one of these methods:

1. **Comment Preservation** (Preferred):

```c
#if 0
/* ORIGINAL 1984 CODE - preserved for reference */
original_function() {
    /* original implementation */
}
#endif
```

2. **Inline Comments** (For small changes):

```c
/* Original 1984: old_approach(); */
modern_approach();  /* MODERN: explanation */
```

3. **Documentation References**:

```c
/* See docs/historical/original-source/filename.c for original implementation */
```

**Rationale**: Enables future researchers to understand evolution, verify authenticity claims, and potentially revert changes if needed.

### Preservation Principle

The goal is to make it easy for future developers to:

1. **Identify original Hack 1.0.3 code** vs modern additions
2. **Understand why** modern additions were necessary
3. **Evaluate whether** additions could be removed or improved
4. **Learn from** the historical code while understanding modern adaptations

### Existing Documented Additions

As of August 2025, the following modern additions have been documented:

1. **hack.u_init.c**: Mutable role strings for ANSI C compatibility
2. **hack.end.c**: Stale lock detection and cleanup for robustness
3. **hack.termcap.c**: delay_output() fallback implementation
4. **hack.timeout.c**: timeout() renamed to hack_timeout() for curses compatibility
5. **hack.cmd.c**: unctrl() renamed to hack_unctrl() for curses compatibility
6. **hack.rip.c**: Mutable tombstone strings for memory safety
7. **hack.lock.c**: Modern flock()-based locking with 1984 fallback

### Original Code Preservation Audit

**✅ COMPLIANT FILES** (Original code preserved):

- hack.termcap.c: delay_output() preserved under #if 0
- hack.unix.c: setrandom() and link() locking preserved
- hack.timeout.c: timeout() preserved under #if 0  
- hack.cmd.c: unctrl() preserved under #if 0
- hack.end.c: Original locking mechanisms preserved
- hack.ioctl.c: Original sgtty includes preserved in comments

All future additions should follow this documentation standard to maintain the educational and preservation value of the project.
