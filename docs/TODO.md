# restoHack Development TODO

> **Status (2026-10): Done.** Shipped in v1.1.5 (Dec 2025) as save format v2. One deviation from the plan below: pre-versioned legacy saves are *rejected* (`src/hack.save.c`) rather than migrated. Kept for the record.

## **Completed: Option A — Save System Safety (Canonical Deployment Path)**

**Decision:** **OPTION A SELECTED**
**Timeline:** 4–6 hours focused work
**Priority:** **HIGH** — Required for Hardfought multi-user deployment
**Status:** **Shipped (v1.1.5)**

---

## The Problem

`struct you` contains pointer fields that are currently written as raw memory addresses. When restored—especially in a server or multi-user context—these pointers become invalid and can cause segfaults or corrupted state.

Affected fields:

* `u.usick_cause` (`const char *`) — pointer to sickness cause string
* `u.ustuck` (`struct monst *`) — pointer to monster the player is stuck to

This behavior is unsafe for canonical hosting and was identified post-release under real-world usage.

---

## The Chosen Solution (NetHack-Style, Proven)

**Option A is locked in, with compatibility preserved.**

The save system will:

* Preserve raw struct dumping to maintain historical accuracy and minimize behavioral drift
* Serialize pointer *targets* explicitly
* Restore pointers safely using reconstructed data
* **Maintain compatibility with all previous restoHack save versions via a migration loader**

Old save files will be loaded using their original layout, converted in-memory to the current representation, and then written back out using the new format.

This mirrors NetHack’s long-standing, battle-tested approach and is appropriate for a museum-quality preservation project.

No speculative redesign.
No broken saves.
No user drama.

Just removal of known crash vectors *and* long-term compatibility.

---

## Implementation Plan

### **Step 1: Serialize `usick_cause`** (≈5 minutes)

* **File:** `src/hack.save.c`
* **Location:** Immediately after existing `ustuck` handling (~line 329)
* Write string length followed by string data
* Write `0` if `NULL`

---

### **Step 2: Restore `usick_cause` safely** (≈10 minutes)

* **File:** `src/hack.save.c`
* **Function:** `dorecover()` (versioned save format section)
* Read string length
* Allocate buffer
* Restore string contents
* Assign pointer

---

### **Step 3: Implement legacy save migration loader** (≈20 minutes)

* **File:** `src/hack.save.c`
* Retain legacy struct-dump restore logic *only* for old save versions
* Dispatch based on save version:

```c
if (save_version <= 1) {
    restore_legacy_format(fd);
    migrate_legacy_state();
} else {
    restore_current_format(fd);
}
```

* Convert legacy in-memory state to the current safe representation
* Immediately re-save using the current format
* Legacy code is isolated, not deleted

This guarantees that **all historical restoHack saves continue to load** while allowing the active format to evolve safely.

---

### **Step 4: Add save-file corruption detection** (≈10 minutes)

* **File:** `src/hack.save.c`
* **Location:** Immediately after restoring or migrating `struct you`
* Validate critical invariants:

---

### **Step 5: Documentation** (≈20 minutes)

* **File:** `docs/CODING_STANDARDS.md`
* Add section: **Save System Pointer Handling**
* Document:

  * WHY this change exists
  * HOW pointers are handled safely
  * WHAT behavior is preserved
  * WHAT new guarantees are added

---

### **Step 6: Testing** (≈2–3 hours)

* Save/restore with `usick_cause = NULL`
* Induce poisoning → save → restore → verify string preserved
* Save while stuck to a monster → restore → verify pointer repair
* Attempt to load old save → verify clean rejection
* Corrupt save file → verify graceful failure
* Multiple save/restore cycles → check for leaks (valgrind)

---

## What This Achieves

✅ Safe for Hardfought multi-user deployment
✅ Matches NetHack’s proven save strategy (recognizable to K2)
✅ Minimal, targeted changes
✅ Low risk to existing gameplay behavior
✅ Establishes foundation for future serialization improvements

---

## What This Does *Not* Attempt to Solve

These are explicitly deferred:

* Cross-architecture portability (32-bit ↔ 64-bit)
* Forward/backward save compatibility across arbitrary versions
* Compiler struct padding differences

**Rationale:** Hardfought runs a single canonical binary. Option A is sufficient.

---

## Future Work: Option B — Field-by-Field Serialization

**Status:** Deferred
**Priority:** Medium
**Timeline:** 1–3 days (focused work)

Option B would replace raw struct dumping with explicit field-by-field serialization for `struct you`, similar to `pack_flags()` / `unpack_flags()`.

Benefits:

* Full cross-architecture compatibility
* Versioned save migration
* Immunity to compiler layout differences

This is intentionally postponed until Option A is deployed and stabilized.

---

## Release & Deployment Notes

* **Current Branch:** `master`
* **Next Release:** `v1.1.5`
* **Deployment Target:** Hardfought canonical hosting
* **Coordination:** K2

---

## Philosophy

> *Fix what breaks. Preserve what works.*

This change removes undefined behavior without altering the soul of the game.
