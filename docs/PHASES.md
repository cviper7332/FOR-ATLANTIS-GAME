# PHASES.md — RTAC Combat System

## Purpose

This document defines the development roadmap for
RTAC (realtime arena action strategic combat system) and establishes exit criteria for each phase. It is the roadmap only — completion evidence,
measured results, and corrections belong in the corresponding `PHASE{N}_COMPLETED.md`, created
only as each phase actually completes (none exist yet; do not pre-create empty files).

**Status vocabulary reused verbatim from [`combat_decisions.md`](combat_decisions.md)** — `OPEN`,
`PARTIAL — <done>; OUTSTANDING: <remains>`, `CLOSED — enacted in <commit>`,
`N/A — design rationale, no action implied`, etc. No separate ✅/⬜ system — see Failure Mode 7
below (one quantity, one authoritative location).

**Progression to the next phase is allowed only after the current phase's Definition of Done is
fully satisfied**, except Phase 7, which is explicitly floating (see its own section).

---

## Mechanical Fidelity Standard

RTAC targets **faithful BN3-style mechanical structure**, not merely similar-feeling output.

- Real-time, grid-based combat with no random encounters — engagement is world-initiated
  (Decision #4).
- Grid dimensions and tile addressing expressed as rows×columns (Decision #5); the grid itself
  is a flat 2D data structure regardless of camera (Decision #1, Rule 5).
- Buster (basic attack) and chip-style (resource-gated) attacks are two distinct economies, not
  one attack type with cosmetic variants.
- Tile modifiers (ice, grass, lava, steel, poison, cracked, broken panels — per Design
  Philosophy) persist and affect resolution; they are not decorative.
- **A deliberate departure from BN3 requires its own numbered Decision entry** (Rule 4) stating
  what changed and why — Decision #3 (elevation) is the first such departure and the template
  for any future one. A departure that isn't logged isn't decided (Rule 4's own text).

An implementation that *feels* like BN3 through a shortcut with the wrong structure (e.g., a
single attack type re-skinned as two, or tile modifiers implemented as pure VFX with no
resolution effect) is not acceptable.

---

## Phase Status Summary

| Phase | Description | Status |
|---|---|---|
| 0 | Foundation & Test Harness | `CLOSED — enacted in 9286975, c5527ca, 3df1fed` |
| 1 | Grid & Movement (Headless Simulation) | `PARTIAL — grid/tile types written, dimensions locked (Decision #8), seed-derivation groundwork tested (Decision #9), entity struct built (Decision #9 + Side addendum), tile ownership added (Decision #10 Ruling 3), movement rules fully specified (Decision #10), movement-legality check and resolution implemented (Decision #10 Ruling 4, RTACCheckMoveLegality + RTACResolveMove); OUTSTANDING: determinism test, multi-entity tests` |
| 2 | Presentation & First Playable Board | `OPEN` |
| 3 | Attacks, HP/Damage & Tile Modifiers | `OPEN` |
| 4 | Enemies & AI | `OPEN` |
| 5 | Round Structure & Chip-Equivalent Resource System | `OPEN` |
| 🛑 | **Hard Block — Core Loop Playable Certification** (gates Phase 5 → 6) | `OPEN` |
| 6 | Elevation | `OPEN` |
| 7 | Exploration ↔ Combat Transition Mechanism *(floating — see below)* | `OPEN` |
| 8 | Content, Balance & Portability Proof | `OPEN` |

---

# Phase 0 — Foundation & Test Harness

**Status:** `CLOSED — enacted in 9286975, c5527ca, 3df1fed`

All ten Definition of Done items satisfied, Part A and Part B alike. Part A closed by `9286975`
(simulation types) and `c5527ca` (compile confirmed, dependency registration, explicit state
struct, `LogRTAC` category); Part B closed by `3df1fed` (first automation test, discoverable and
run to `Success` in the Session Frontend on August 29, 2026) plus this commit's run-procedure
documentation in `CLAUDE.md`.

One item carried forward as non-blocking: assertion-outcome logging to `LogRTAC` was added to the
test *after* the confirmed `3df1fed` run, so the MCP log-retrieval path documented in `CLAUDE.md`
is written but unbuilt and unverified. It is an enhancement beyond this phase's DoD — no DoD item
requires it — and so does not hold Phase 0 open. Verify it after the next build.

## Prerequisite

**Development on this phase — and the project generally — requires UE 5.8.** Per Decision #7,
this project's workflow depends on MCP editor introspection, a UE 5.8 Experimental feature
absent from 5.6/5.7. Confirm the editor in use is 5.8 before starting any Part B item below.

## Goal

By the end of this phase the RTAC plugin exists as a real, loading, compiling module, and the
Safety Ruleset governing it is demonstrably enforceable rather than aspirational — Rule 5's
simulation/presentation boundary above all, with Rules 6, 9, and 11 alongside it.

Concretely: the plugin's descriptor, build script, and module boilerplate compile, and so does
everything under its `Simulation/` tree — a claim that requires an actual build, not an inference
drawn from source files existing. RTAC is registered as a dependency of the main project so the
two genuinely link. The simulation subtree holds plain structs and UE Core value types only,
with no `UObject`/`AActor` ownership anywhere in simulation state; that state lives in an
explicit struct with no hidden globals or statics (Rule 6); and diagnostics go through the
dedicated `LogRTAC` category rather than `LogTemp` (Rule 9).

The test harness is the other half of the phase, and the half its name points at. At least one
UE Automation Test exists inside the plugin, is discoverable from the editor's Session Frontend,
and runs — against a live UE 5.8 editor, which is a hard floor for this project rather than a
preference (Decision #7). That test exercises real simulation state and is demonstrably capable
of failing, since a test that cannot fail is not evidence (Failure Mode 8). The exact procedure
for running it is written down in `CLAUDE.md`, so the next person doesn't have to rediscover it.

## Test Harness

Simulation code is compiled and tested via **UE Automation Tests, running inside the editor**.
There is no standalone non-UE5 build and none is to be created — settled by Rule 5's first
August 26, 2026 addendum (unnumbered in `AGENTS.md`; it immediately precedes Addendum #2), which
corrected the rule's original "testable without a running engine" phrasing.
That phrasing was inherited from PRS, where a standalone engine-agnostic build was a genuine
project goal (PRSCore compiles with zero Unreal headers, CI-gated). RTAC's portability target is
"across UE5 projects," not away from UE5 (`RTAC.uplugin`), so no equivalent goal applies here.

Tests live inside the plugin per Rule 11 (`Plugins/RTAC/Source/…`), never in
`Source/ProjectAtlantis/`.

**"No engine types" enforcement is review-only, not machine-enforced** — see Decision #6. No
second UBT module and no grep/lint gate will be built; compliance is checked at each Phase Exit
Review instead.

## Definition of Done

**Part A — verifiable without a live editor session**
- [x] `RTAC.uplugin`, `RTAC.Build.cs`, `RTACModule.h/.cpp` exist and compile — confirmed by the
      August 26, 2026 build that produced `Binaries/Win64/UnrealEditor-RTAC.dll`
- [x] The `Simulation/` tree compiles — confirmed compiled August 29, 2026. The linked
      `Binaries/Win64/UnrealEditor-RTAC.dll` postdates every file under `Source/RTAC/*/Simulation/`
      (`CLAUDE.md` → build verification), and the MCP-read output log corroborates it independently:
      `LogModuleManager: InternalLoadLibrary: 'RTAC' (...UnrealEditor-RTAC.dll)` and
      `LogRTAC: RTAC module loaded.` both timestamped after that build. A comment-syntax bug in
      `RTACTile.h` (a literal `*/` inside a Doxygen comment's prose, closing the comment block early
      and turning the remainder of the file into unparsed code) caused the prior build failure and
      was fixed before this build; the fix is punctuation-only and changes no comment's meaning.
- [x] `RTAC` registered in `ProjectAtlantis.Build.cs`'s `PublicDependencyModuleNames`
- [x] A simulation subtree exists holding plain structs and UE Core value types only
      (`TArray`, `TMap`, `FString`, etc. — permitted and preferred per Rule 5 Addendum #2);
      no `UObject*`/`AActor*` **ownership** in simulation state. `FIntPoint` remains permitted
      generally, but grid positions specifically use `FRTACGridPosition` per Rule 5 Addendum #3.
- [x] Simulation state lives in an explicit state struct with no hidden globals or statics,
      per Rule 6 — `FRTACGrid` holds only its own member state (`Rows`, `Columns`, `Tiles`) plus
      `static constexpr` compile-time constants; no mutable globals or statics anywhere in it.
- [x] Dedicated log category in use (`LogRTAC` — confirmed working via the output log Omar
      pasted, and independently via `EditorToolset.LogsToolset`), not `LogTemp`, per Rule 9

**Part B — requires UE5.8 open**
- [x] Plugin loads in-editor; `LogRTAC: RTAC module loaded.` confirmed via
      `EditorToolset.LogsToolset` and on disk
- [x] At least one UE Automation Test exists, is discoverable from the editor's Session
      Frontend, and can be run — `RTAC.Simulation.Grid.BasicLifecycle`
      (`Plugins/RTAC/Source/RTAC/Private/Tests/RTACGridTest.cpp`, added in `3df1fed`), confirmed
      discoverable in the Session Frontend tree and run by Omar on August 29, 2026, result
      **Success**.
- [x] That test exercises simulation state and can *fail* — a test that cannot fail is not
      evidence (Failure Mode 8). **Stated precisely: this is a claim about the test's design, not
      something one green run proves.** The successful run above confirms the test currently
      passes; it does not and cannot demonstrate failability. What supports failability is the
      test's construction — each assertion is tied to a specific named regression that would trip
      it: a broken `ToIndex()` or a `Position` never assigned during `Init()` fails the
      position-agreement assertion; an off-by-one in either direction of `IsValidPosition()`'s
      `Row >= 0 && Row < Rows` guard fails one of the two bounds assertions; a `Reset()` that
      clears incompletely fails the post-reset assertions. The mechanism by which a failed
      assertion actually fails the test (`AddError()` → `HasAnyErrors()`, verified at
      `AutomationTest.cpp:1376`) is documented in `docs/reference.md`. A deliberate
      failure-injection run has **not** been performed; if one is wanted as harder evidence, that
      is a separate exercise from this DoD item.
- [x] The exact run procedure documented in `CLAUDE.md` — see "Running RTAC's Automation Tests",
      covering the verified Session Frontend path and, marked explicitly as not-yet-verified, the
      `LogRTAC` MCP-retrieval path.

## Exit

Rules 5, 6, 9, and 11 reviewed against the current simulation code and accepted.

---

# Phase 1 — Grid & Movement (Headless Simulation)

**Status:** `PARTIAL — grid/tile types written, dimensions locked (Decision #8), seed-derivation groundwork tested (Decision #9), entity struct built (Decision #9 + Side addendum), tile ownership added (Decision #10 Ruling 3), movement rules fully specified (Decision #10), movement-legality check and resolution implemented (Decision #10 Ruling 4, RTACCheckMoveLegality + RTACResolveMove); OUTSTANDING: determinism test, multi-entity tests`

## Goal

By the end of this phase the combat board exists as a real, addressable data structure and
entities move on it — entirely headlessly, with no renderer and no actor anywhere in the phase's
code, and no simulation type depending on the editor. The editor appears in exactly one role, as
a host: the tests run inside it as UE Automation Tests, per Phase 0's Test Harness section, which
settled that there is no standalone non-UE5 build and none is to be created. What is under test
is editor-independent; the runner is not, by design.

The board is a grid of deliberately chosen dimensions, logged as its own Decision entry and stated
rows×columns per Decision #5. Its tiles are plain structs built from UE Core value types, holding
no `UObject*`/`AActor*` ownership, and every tile carries both a surface-modifier slot and an
elevation slot that exists in the data model while remaining mechanically inert — reserved now so
Phase 6 can add elevation without retrofitting it as a nested branch inside logic that never
anticipated it (Rule 8). The surface-modifier slot is not entirely unpopulated here: it carries
one value, `Broken`, pulled forward from Phase 3's list because this phase's own movement-legality
check needs something real to compare against — see Resolves below.

Movement resolves through the simulation layer and nowhere else: no presentation layer reads
simulation state, and the simulation carries no dependency in the other direction. Grid
coordinates stay grid coordinates throughout — no grid↔world unit conversion exists anywhere in
this phase's code, and that absence is positively confirmed rather than assumed, since there is no
presentation layer yet to own that boundary (Rule 10).

All of it is deterministic and demonstrably so: the same seed and the same input sequence produce
an identical resulting state, verified by a test rather than asserted in prose (Rule 6). Those
tests exercise the full configured grid with multiple entities — never a 1×1 board or a
single-entity setup, either of which would collapse the very behaviour under test
(Failure Mode 5).

## Resolves

- **"Exact grid dimensions"** (Open Questions → Core BN3 Loop) — dimensions chosen and logged.
  Expressed as rows×columns per **Decision #5**; BN3's own board is 3 rows × 6 columns
  (3 columns per side) but this project's dimensions are not required to match it.
- **"Movement rules"** (Open Questions → Core BN3 Loop).

**Absorbed from a later phase — a scope note, not an Open Question resolution.**
`ERTACSurfaceModifier::Broken` is added in Phase 1 rather than Phase 3, authorized by Decision
#10's August 31, 2026 addendum: Ruling 4's fourth clause ("not broken") is Phase 1's own stated
concern and had no real value to check against. Phase 1 took the enum value only. Phase 3 keeps
the rest of the list (ice, grass, lava, steel, poison, cracked) **and** keeps every modifier's
resolution *effect*, `Broken`'s included — nothing here relaxes that deferral generally. The
reasoning lives in the decision addendum and is not restated here.

## Definition of Done

- [x] Grid dimensions chosen and logged as their own Decision entry, stated as rows×columns
      per Decision #5
- [x] Simulation types hold plain structs and UE Core value types (`TArray`, `TMap`, `FString`,
      etc. — permitted and preferred per Rule 5 Addendum #2); no `UObject*`/`AActor*`
      **ownership** in simulation state; grid coordinates are grid coordinates, not world
      transforms — confirmed September 1, 2026 by grep across every RTAC source file: no
      `UPROPERTY`/`UCLASS`/`USTRUCT`/`UENUM` anywhere in the tree, and every `UObject`/`AActor`
      token in it sits inside a comment explaining that type's absence. `FRTACEntity::ArchetypeId`
      is `FName` — a Core value type permitted by Rule 5 Addendum #2, flagged as such in its own
      header so the `UObject/` include path does not read as a violation on review.
- [x] Movement resolves through the simulation layer only; no presentation-layer read of
      simulation state and no reverse dependency (Rule 5)
- [x] The tile model carries a surface-modifier slot and an **elevation slot that exists but is
      mechanically inert** — see note below. `FRTACTile::SurfaceModifier` and
      `FRTACTile::Elevation` both exist; the modifier slot holds `None` and `Broken` (the latter
      pulled forward from Phase 3 — see Resolves above), and `Elevation`'s inertness is confirmed
      September 1, 2026 by grep: zero readers of the field in any `.cpp` in the plugin.
- [ ] Same seed + same input sequence → identical resulting state, verified by test (Rule 6)
- [x] Grid ↔ world unit conversion does not exist yet in this phase (no presentation layer) —
      confirmed by the absence of any such conversion in the simulation code (Rule 10).
      Positively confirmed September 1, 2026: grep for `FVector`, `FTransform`, `WorldLocation`,
      `ToWorld`, and `TileSize` across all RTAC source returns no hits. Re-run this at Phase Exit
      Review rather than carrying the result forward — the item asserts an absence, and absences
      regress silently.
- [ ] Tests run against the full configured grid, never a 1×1 or single-entity degenerate case
      (Failure Mode 5)

> **On the inert elevation slot — this is not designing elevation early.** Decision #3 defers
> elevation's *mechanical direction*, not its existence as a tile property; the decision's own
> text already treats elevation as "a tile property independent of those BN3-style modifiers."
> Rule 8's own "Concrete risk here" note warns that if the core loop's tile model is built so
> elevation can only be added later as a nested branch, the rule is violated *in advance*.
> Reserving the data slot with zero gameplay effect honors both constraints — flagged explicitly
> so it doesn't read as a sequencing violation of Decision #3's "core and elevation not designed
> simultaneously" requirement.

> **On the determinism DoD item — what's built, and what still isn't.** This item remains
> unchecked, correctly: no test exists yet that runs an actual input sequence against match state
> and confirms two runs from the same seed converge on the same final state, because no movement
> code exists yet for such a sequence to consist of. That test is real outstanding work, not
> paperwork — it stays on the list.
>
> What Phase 1 has built so far is groundwork toward it, not a substitute for it. Phase 1 has no
> gameplay randomness: nothing in it draws from a random stream. `EntityId` is a plain
> incrementing counter, not a random draw (Decision #9 and its August 30, 2026 clarification
> addendum). The seed-derivation mechanism itself (`RTACDeriveStreamSeed`, `FRTACRngState`) is
> unit-tested in isolation for purity, independence, and stability — this is real, passing,
> verified work, but it is a test of the mechanism a future stream will use, not a test of the
> DoD item's actual claim. A reader should take neither the passing derivation tests nor the
> absence of live randomness as evidence this item is closed; it isn't, until the input-sequence
> replay test exists.
>
> **Addendum, September 1, 2026 (the stated blocker is gone; the item is not).** The first
> paragraph above explains this test's absence "because no movement code exists yet for such a
> sequence to consist of." That reason is obsolete. `RTACCheckMoveLegality` landed in `c51027e`
> (August 31, 2026) and `RTACResolveMove` in `c436334` (September 1, 2026); an input sequence now
> has real move-events to consist of. The replay test is unblocked work, not blocked work. The
> original text is left unchanged per Rule 4.
>
> **The item stays unchecked, and the second paragraph above still stands unaltered.** No replay
> test exists. And its substantive point — that the seed-derivation tests verify the mechanism a
> future stream will use, not this item's actual claim — is untouched by movement existing, so it
> remains the reason those passing tests are not evidence here.

---

# Phase 2 — Presentation & First Playable Board

**Status:** `OPEN`

## Goal

By the end of this phase the board is visible and playable: it renders in an isometric 2.5D view
per Decision #1, at the dimensions chosen in Phase 1, and a player can move on it in PIE.

Getting there requires two conversions to exist and to live in exactly the right place. Grid
coordinates become world units in exactly one named function, on the presentation side and
nowhere else (Rule 10). Screen-space input becomes grid-space position through the isometric
projection — a real projection, not an axis swap — so hit-testing resolves in grid space rather
than being approximated in screen space (Rule 10, Decision #1).

The phase's actual proof is a falsifiable one: changing the camera — swapping the isometric
angle, say — must require zero changes to simulation code. That is the operational test of
Rule 5 and Decision #1 together, and the direct analogue of PRS Phase 3's "adding green phosphor
required zero structural changes." If a camera change reaches into grid code, the boundary this
phase exists to validate has already failed.

## Definition of Done

**Part A**
- [ ] Grid ↔ world-unit conversion exists in exactly one named function, in the presentation
      layer only (Rule 10)
- [ ] Screen-space ↔ grid-space hit-testing is implemented in grid space, related to screen
      space by the isometric projection, not an axis swap (Rule 10, Decision #1)

**Part B — requires UE5 open**
- [ ] **Falsifiable test:** changing the camera (e.g. swapping isometric angle) requires zero
      changes to simulation code. This is the operational test of Rule 5 and Decision #1 together
      — direct analogue of PRS Phase 3's "adding green phosphor required zero structural changes."
- [ ] Board renders and is playable in PIE at the dimensions chosen in Phase 1

---

# Phase 3 — Attacks, HP/Damage & Tile Modifiers

**Status:** `OPEN`

## Goal

By the end of this phase combat resolves: a buster attack lands, HP and damage are modeled, and
BN3's flat tile modifiers — ice, grass, lava, steel, poison, cracked, broken panels — affect the
outcome.

How that resolution is built matters as much as that it exists. The damage formula names the
domain of every stage explicitly — pre- versus post-mitigation, additive versus multiplicative —
so one constant cannot silently mean two different things at two points in the chain (Rule 10,
Failure Mode 2). Before any of it is implemented, one concrete scenario is traced by hand end to
end — this attacker, this attack, this tile, this defender — and the resulting number agreed;
the implementation then has to reproduce that number rather than define it (Failure Mode 4).

Every balance scalar introduced here — damage values, tile-modifier strengths — carries a test
that varies it and asserts the output moves in the intended direction and by the intended
magnitude, with the condition the test runs under stated explicitly. A scalar that cannot move
the output under any condition is inert, and gets deleted or documented as deliberately so
(Failure Mode 1). And each tile modifier's effect resolves in the simulation layer, not painted
on as presentation-only VFX — a modifier that looks active but changes no outcome fails the
Mechanical Fidelity Standard as surely as it fails Rule 5.

## Resolves

- **"Buster/basic attack implementation"** and **"HP/damage model"** (Open Questions → Core BN3
  Loop).

## Definition of Done

- [ ] Damage formula states its domain explicitly — pre- vs. post-mitigation, additive vs.
      multiplicative — for every stage (Rule 10, Failure Mode 2)
- [ ] One concrete scenario (this attacker, this attack, this tile, this defender) traced by
      hand and agreed before implementation; implementation reproduces the agreed number
      (Failure Mode 4)
- [ ] Every new balance scalar (damage value, tile-modifier strength) has a test that varies it
      and asserts the output moves in the intended direction **and magnitude**, with the test
      condition stated explicitly (Failure Mode 1)
- [ ] Each tile modifier's resolution effect is implemented in the simulation layer, not as
      presentation-only VFX (Rule 5, Mechanical Fidelity Standard)

---

# Phase 4 — Enemies & AI

**Status:** `OPEN`

## Goal

By the end of this phase enemies fight back, and they do so in more than one way. At least two
mechanically distinct AI behaviour patterns exist — not one pattern wearing two skins. A single
enemy cannot prove any of this: with one opponent on the board, targeting, threat selection, and
telegraph readability all collapse into a degenerate case that says nothing about the general one
(Failure Mode 5, which names this exact risk).

Telegraphing is what makes an attack answerable rather than arbitrary, and that depends on timing
being unambiguous. Telegraph timing is expressed in one authoritative domain — frames or seconds,
chosen and stated — converted exactly once at the boundary, with the conversion factor written
down (Rule 10). Two places holding the "same" wind-up duration in different units is how a
telegraph silently drifts out of sync with the attack it exists to announce.

Where the AI reads from, and how it is switched on, matter as much as what it does. AI queries
simulation state only, never the presentation layer — an enemy that decides based on where a mesh
is drawn has punched through the Rule 5 boundary no matter how correct it looks on screen. And
each AI system's active/inactive state is gated on its own condition, never nested inside another
system's guard (Rule 8), so switching one behaviour off cannot silently take a second one down
with it.

## Resolves

- **"Enemy AI behavior patterns and telegraphing"** (Open Questions → Core BN3 Loop).
- **Enemy Roster — partial.** Thematic direction only (retro-futuristic Atlantean tech per the
  faction lore) — full roster is explicitly deferred to Phase 8, since `combat_decisions.md`
  ties faction-specific enemies to PHIS faction identity being "more built out."

## Definition of Done

- [ ] At least two mechanically distinct AI behavior patterns exist — a single-enemy test
      collapses everything about targeting and telegraphing and cannot stand in for this
      (Failure Mode 5, named explicitly for this exact risk)
- [ ] Telegraph timing is expressed in one authoritative domain (frames or seconds), converted
      once, with the conversion factor written down (Rule 10)
- [ ] AI reads simulation state only — no presentation-layer queries (Rule 5)
- [ ] Each AI system's active/inactive state is gated independently — no AI behavior nested
      inside another system's guard condition (Rule 8)

---

# Phase 5 — Round Structure & Chip-Equivalent Resource System

**Status:** `OPEN`

## Goal

By the end of this phase the BN3 loop closes: a full match runs from engagement through
resolution to a win or a loss. Not merely once — it runs deterministically from a fixed seed, the
same seed producing the same match every time (Rule 6). Without that, a match that completes
proves only that it completed, and nothing found inside it can be reproduced.

Closing the loop means fixing its order. Round and turn timing follow one declared, documented
sequence — Input → Simulation tick → Resolution → Presentation — with no stage reading a later
stage's output (Rule 7). An update order that is merely emergent is one nobody can reason about,
and a single frame-late read is exactly the kind of defect that hides behind correct-looking
behaviour for entire phases.

The phase's second half is the resource economy the loop runs on. BN3's chip-and-folder system
gets a direction chosen here — kept as-is, replaced, or reworked into something Atlantis-specific
— and that choice is logged as its own `combat_decisions.md` Decision entry, not settled in code
and not settled in conversation. `combat_decisions.md` deliberately held this open "until the
core loop exists to build against"; this phase is where that condition is finally met, and the
whole point of the deferral is lost if the answer arrives as an implementation instead of as a
decision.

## Resolves

- **"Turn/round structure"** (Open Questions → Core BN3 Loop).
- **Chip/Folder Building Replacement** — `combat_decisions.md` explicitly left this "open until
  the core loop exists to build against"; that condition is met here, not before.

## Definition of Done

- [ ] Round/turn timing follows one declared, documented order (Rule 7): Input → Simulation
      tick → Resolution → Presentation, no stage reads a later stage's output
- [ ] Chip-equivalent resource system direction chosen (kept, replaced, or reworked) and logged
      as its own Decision entry
- [ ] A full match (engagement → resolution → win/lose) runs deterministically end-to-end from
      a fixed seed (Rule 6)

---

# 🛑 HARD BLOCK — Core Loop Playable Certification

**Phase 6 may not begin until this block is cleared.**

**Why this exists:** Decision #3 defers elevation until "the base BN3 combat loop is built and
playable" — and *"playable"* is otherwise arguable indefinitely without an operational
definition. This gate is that definition. Second reason, direct precedent from PRS: the
`opticalGather` bug was inert for an entire phase and caught only two phases later, because the
early phases predated PRS's own failure-mode checklist. Phases 1–5 here are written *with* the
Recurring Failure Modes checklist available from the start — this gate is what proves it was
actually run, not just available.

**Required work before Phase 6 (or Phase 7, if scheduled after this point) begins:**

1. All Phase 1–5 Definition of Done items satisfied, Part A and Part B alike.
2. All 8 Recurring Failure Modes explicitly checked against each of Phases 1–5 — checked, not
   skipped. Confirmed-clean findings are recorded, not just bugs, per PRS's own convention (the
   absence of a problem in a specific area is itself useful evidence).
3. A structured playtest record exists: named scenarios, specific written questions about feel
   and responsiveness, dated recorded answers. Not a checkbox — combat's core-loop feel is not
   objectively verifiable the way a physics pipeline's statistics are, and Failure Mode 1's own
   text says "it seemed to work in a playtest" is not evidence on its own; a structured record is
   the distinction between that and an actual gate.
4. Any finding gets its own `combat_decisions.md` entry (next sequential Decision number). Any
   finding that changes behavior requires a regression test before being considered closed.

**This block clears when:** all four items above are satisfied and documented.

---

# Phase 6 — Elevation

**Status:** `OPEN`

## Goal

By the end of this phase elevation is real: Decision #3's deferred mechanic has a direction, and
that direction is built and verified on the board. The direction itself is chosen first — from
the brainstormed options already logged, or from beyond them — and recorded as its own
sequential `combat_decisions.md` entry *before* implementation starts. Decided in the log, not
in code.

The hard part is proving the constraint Decision #3 attached to it. Elevation must not reduce to
a simple high-is-better axis, and that is a claim about *situational* strength, which no single
steady-state test can validate — Failure Mode 1 says so in as many words. So the proof takes the
shape of at least two distinct, named conditions with opposite outcomes: high ground winning
under condition A, low ground winning under condition B, not one scenario run until it looks
right. Those tests run on non-flat, multi-elevation boards, since a board where every tile sits
at the same height collapses precisely the axis this phase exists to introduce (Failure Mode 5).

Where the code goes matters as much as what it does. Elevation is not a nested branch bolted
inside the existing tick or tile-update loop (Rule 8) — reserving the inert elevation slot back
in Phase 1 was the preparation for exactly this, and spending it on a nested conditional would
waste it. And the phase answers a question it inherits rather than leaving it open: whether the
grid dimensions chosen in Phase 1 actually give elevation enough room to read clearly. That gets
an explicit answer, one way or the other, on the record — not a silence that later reads as
agreement.

## Resolves

- **Elevation — Mechanical Direction** (Open Questions, post-core, feeds Decision #3).

## Definition of Done

- [ ] Mechanical direction chosen from (or beyond) the brainstormed directions already logged,
      and recorded as its own sequential Decision entry before implementation starts
- [ ] **Elevation's situational-strength constraint is demonstrated by tests under at least two
      distinct named conditions** — e.g. high ground winning under condition A, low ground
      winning under condition B — never by a single steady-state test. Decision #3's own
      constraint ("must not be a simple high=better axis") is precisely the kind of claim
      Failure Mode 1 says "cannot be validated by a single steady-state test."
- [ ] Elevation is not implemented as a nested branch inside the existing core tick/tile-update
      loop (Rule 8) — reserving the inert slot in Phase 1 was the preparation for this
- [ ] Tests run on non-flat, multi-elevation boards — a flat all-same-elevation board collapses
      exactly the axis this phase introduces (Failure Mode 5, cited by name in `combat_decisions.md`
      for this exact risk)
- [ ] Whether the current grid dimensions (chosen in Phase 1) give elevation enough room to read
      clearly is answered explicitly, one way or the other — the open question raised in
      `combat_decisions.md`'s Elevation subsection

---

# Phase 7 — Exploration ↔ Combat Transition Mechanism

**Status:** `OPEN`
**Scheduling: floating, not strictly linear.**

## Why this phase is floating

`combat_decisions.md`'s "Exploration → Combat Transition Mechanism" Open Question states this
explicitly in its own text: *"Not blocking the core grid/movement work — per AGENTS.md Rule 5
(Simulation/Presentation Separation) and Rule 11 (Combat Code Lives Inside RTAC), the grid
simulation doesn't know or care how combat was entered."* Consequently this phase may be
scheduled any time after Phase 2 (once there is a board to transition into), independent of
where Phases 3–6 currently stand. This is a deliberate deviation from a strictly linear phase
chain — flagged as such rather than forcing false sequencing.

## Goal

By the end of this phase the player can actually cross from exploration into combat: engaging an
enemy in the world drops them into a contained, contextual arena, with no random encounters
anywhere in the path — matching Decision #4's description exactly rather than approximately.

The mechanism that does it is chosen before it is built. A GameMode swap, a mode flag on
GameState or PlayerController, or something else entirely — the choice is made and recorded as
its own sequential `combat_decisions.md` entry, citing Decision #4 as the structural philosophy
it implements, before any implementation begins.

Two boundaries constrain how it gets wired. Rule 11 caps what may be added to
`Source/ProjectAtlantis/`: the thin invoke path that triggers the transition, and nothing else —
no combat logic follows it across into the main module. And Rule 8 governs the gating. Entering
combat is conditioned on its own state alone, never nested inside or short-circuited by an
unrelated feature's guard, so an engagement can never quietly fail to start because something
else happened to be switched off.

## Resolves

- **Exploration → Combat Transition Mechanism** (Open Questions).

## Definition of Done

- [ ] Transition mechanism chosen and logged as its own sequential Decision entry, citing
      Decision #4 as the structural philosophy it implements
- [ ] The only combat-related code added to the main project (`Source/ProjectAtlantis/`) is the
      thin invoke path that triggers the transition — no combat logic itself lives there
      (Rule 11)
- [ ] The transition's gating is independent of any other system's state (Rule 8) — entering
      combat must not be nested inside or short-circuited by an unrelated feature's guard
- [ ] Player enters a contained, contextual arena on engagement with no random encounters,
      matching Decision #4's description exactly

---

# Phase 8 — Content, Balance & Portability Proof

**Status:** `OPEN`

## Goal

By the end of this phase the combat system is content-complete and the plugin's central claim has
been put to an actual test. The full enemy roster is implemented per the story outline's faction
lore, and a balance pass covers all of it — the whole roster *and* the elevation directions
Phase 6 introduced, since balancing the roster alone leaves the newest axis in the game untuned.

The portability proof is the item nothing before this phase verifies, and it is falsifiable
rather than rhetorical: RTAC compiles *and runs* inside a clean UE5.8 project containing no
`ProjectAtlantis` references of any kind. That is the direct test of the plugin's entire stated
reason for existing — `RTAC.uplugin`'s "portable across UE5 projects" — and the analogue of PRS
Phase 3's zero-structural-change sensor-abstraction test. Alongside it, `Plugins/RTAC/` is
re-audited for `ProjectAtlantis`-only dependencies that may have crept in since Phase 0: Rule 11
gets re-verified here, not assumed to have held on its own for eight phases.

The phase also settles a number this project has deliberately refused to guess. RTAC's
*consumer* engine floor — the oldest UE version a project dropping RTAC in can use — is
explicitly undetermined until tested, and Decision #7 does not answer it. Decision #7's UE 5.8
requirement is a *development*-workflow constraint born of MCP editor introspection, and a
consumer of the plugin never needs MCP. Whether RTAC's own code compiles against 5.6 or 5.7 is a
property of RTAC's code alone, and it gets determined by an actual build against a candidate
version — both are installed on this machine and available for exactly this — rather than by
picking a plausible-sounding number.

## Resolves

- **Enemy Roster — full** (Open Questions). Faction-specific enemies tied to PHIS faction
  identity, per `combat_decisions.md`'s own note that this category becomes viable "once PHIS
  faction identity is more built out."

## Definition of Done

- [ ] Full enemy roster implemented per the story outline's faction lore
- [ ] Balance pass complete across the full roster and elevation directions from Phase 6
- [ ] **Portability test:** RTAC compiles and runs in a clean UE5.8 project containing no
      `ProjectAtlantis` references of any kind. This is the falsifiable test of the plugin's
      entire stated reason for existing (`RTAC.uplugin` → "portable across UE5 projects") and the
      direct analogue of PRS Phase 3's zero-structural-change sensor-abstraction test.
- [ ] **Consumer engine floor — explicitly undetermined until tested.** This project's own UE
      5.8 requirement (Decision #7) is a *development*-workflow constraint (MCP editor
      introspection) and does not apply to consumers of the plugin — a project dropping RTAC in
      never needs MCP. Whether RTAC's own code compiles against an older engine (5.6, 5.7) is a
      property of RTAC's code, not decided by Decision #7, and is not yet known. Determine this
      by an actual build against a candidate version — both 5.6 and 5.7 are installed and
      available for this test — rather than assuming a number.
- [ ] No `ProjectAtlantis`-only dependency has silently crept into `Plugins/RTAC/` since Phase 0
      (Rule 11 re-verified, not assumed to have held)

---

## Architecture Re-evaluation Criteria

Re-evaluate the entire design if any of the following occur:

- The simulation layer requires a `UObject*`, `AActor*`, or `FVector` to function correctly
  (Rule 5 violated at the root)
- Adding a new tile modifier requires a structural change to the simulation's core loop rather
  than new data
- Elevation cannot be added without rewriting the core tick/tile-update loop (the exact failure
  Rule 8's "Concrete risk here" note warns against)
- A camera or presentation change requires touching grid/simulation code (Rule 5, Decision #1)
- RTAC cannot compile or run in a project with no `ProjectAtlantis` references (Rule 11's
  portability claim fails at the point it's actually tested, in Phase 8)
- A PHIS dependency, in either direction, becomes necessary for a combat feature (Decision #2)

---

## Phase Exit Review (Required Before Every Phase Transition)

- All Definition of Done items satisfied, Part A and Part B alike where the phase has both
- All 8 Recurring Failure Modes explicitly checked against this phase's new code — see
  Pre-Flight checklist below
- No critical bugs
- `docs/AGENTS.md` updated if a Safety Ruleset assumption changed during the phase
- `docs/combat_decisions.md` updated if a design point was settled during the phase — a chat
  conclusion is not decided until it is in the file (Rule 4)
- Safety Ruleset still holds under live re-read (Rule 2) — not assumed from an earlier phase's
  review

### Test gate

PRS's test gate is a concrete `ctest -L '^phase[0-N]$'` query. RTAC's equivalent is a name-prefix
filter over UE Automation Tests, run from the editor console:

```
Automation RunTests StartsWith:RTAC
```

Verified against local UE 5.8 source rather than assumed — all line references are to
`Engine/Source/Developer/AutomationController/Private/AutomationCommandline.cpp`:
`Automation RunTests <string>` is a registered console command (line 610, help text line 785),
arguments split on `+`, and a `StartsWith:` argument builds a prefix-match filter, appending the
trailing `.` when omitted (lines 149-163). The Session Frontend
procedure in `CLAUDE.md` → "Running RTAC's Automation Tests" runs the same tests through the UI
and is the path actually confirmed live (August 29, 2026); this console form is verified in
source but has not itself been run.

**This is not the phase-scoped query PRS's gate is, because no per-phase tag exists.** Test names
follow `RTAC.Simulation.<Area>.<Case>` — `RTAC.Simulation.Grid.BasicLifecycle`,
`RTAC.Simulation.Rng.StreamSeedDerivation`, `RTAC.Simulation.Rng.MatchStateLifecycle` — a
convention established by practice across the three existing tests and **never logged as a
decision**. It groups by subsystem, not by phase, so `StartsWith:RTAC` runs everything and cannot
answer "did *this phase's* tests pass." Until a phase-tagging convention is decided and logged,
that question is answered by the Phase Exit Review checking by hand that each of the phase's DoD
test artifacts exists and passes.

**Updated September 1, 2026.** This section previously read "Test gate — pending" and deferred
itself until "Phase 0 closes and the automation-test discovery/run procedure is documented in
`CLAUDE.md`," citing "Phase 0's outstanding Part B items." Both preconditions were met on
August 29, 2026 — Phase 0 is `CLOSED` and every Part B item is checked, the last of them being
the run procedure reaching `CLAUDE.md` — so the placeholder had outlived its own terms. What
remains open is not a precondition but unlogged work: deciding a phase-tagging convention, which
wants its own decision entry.

## Pre-Flight Checklist (Required Before Any Combat Stage, Balance Constant, or Test Oracle)

1. Run `docs/AGENTS.md § Recurring Failure Modes` — all 8 items, top to bottom.
2. Trace one concrete scenario through the change by hand; agree the result before writing code
   (Failure Mode 4).
3. Name the domain (grid coordinates, world units, frames, seconds, elevation level, screen
   space) of every input and output explicitly (Rule 10).
4. Confirm every new balance scalar has a sensitivity test that can fail under a stated condition
   (Failure Mode 1).
5. Confirm every test oracle measures the same quantity, statistic, and domain the system
   actually emits (Failure Mode 3).

---

## Explicit Non-Goals

RTAC will **not**:

- Depend on PHIS, or be depended on by it, in either direction (Decision #2)
- Require a standalone engine-agnostic build system (Rule 5, August 26, 2026 addendum)
- Implement multiplayer or netcode
- Become a general-purpose tactics framework — it is a specific BN3-inspired system for this
  game, not a reusable genre engine
- Faithfully emulate BN3 in every respect — deliberate departures are intended and expected,
  provided each is logged as its own Decision (Mechanical Fidelity Standard)

---

## Guiding Principle

> **Playable core loop first.**
> **Faithful BN3 mechanical structure second — the right shape, not just similar-feeling output.**
> **Architecture and Safety Ruleset third.**
> **Atlantis-specific modifications (elevation) fourth — only after the core is certified playable.**
> **Content, balance, and the portability proof last.**

This ordering is not a stylistic preference — it is Decision #3's own explicit sequencing
("the core and this modification are not to be designed simultaneously") generalized to the
whole roadmap. Every phase transition validates the previous phase's assumptions before adding
complexity on top of them.

---

*Created August 26, 2026. Structure adapted from `D:\Dev\PerceptionRenderingSystem\docs\PHASES.md`
(PRS PHASES.md), read-only reference — PRS's Physical Fidelity Standard, DoD-as-verifiable-claims
convention, Part A/Part B split, hard-block pattern, and Pre-Flight checklist all transfer
directly; PRS's own ✅/⬜ status system does not, replaced with `combat_decisions.md`'s existing
controlled vocabulary per Failure Mode 7. All rule and decision citations verified against live
`docs/AGENTS.md` (Rules 1–12) and `docs/combat_decisions.md` (Decisions #1–#5) on August 26, 2026,
not carried over from an earlier draft in conversation.*
