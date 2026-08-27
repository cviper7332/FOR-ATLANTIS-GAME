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
| 0 | Foundation & Rule 5 Enforceability | `PARTIAL — plugin scaffold live; test harness enforcement OUTSTANDING` |
| 1 | Grid & Movement (Headless Simulation) | `OPEN` |
| 2 | Presentation & First Playable Board | `OPEN` |
| 3 | Attacks, HP/Damage & Tile Modifiers | `OPEN` |
| 4 | Enemies & AI | `OPEN` |
| 5 | Round Structure & Chip-Equivalent Resource System | `OPEN` |
| 🛑 | **Hard Block — Core Loop Playable Certification** (gates Phase 5 → 6) | `OPEN` |
| 6 | Elevation | `OPEN` |
| 7 | Exploration ↔ Combat Transition Mechanism *(floating — see below)* | `OPEN` |
| 8 | Content, Balance & Portability Proof | `OPEN` |

---

# Phase 0 — Foundation & Rule 5 Enforceability

**Status:** `PARTIAL — plugin scaffold live; test harness enforcement OUTSTANDING`

## Goal

Prove the Safety Ruleset is enforceable, not aspirational — specifically Rule 5's
simulation/presentation boundary. Mirrors PRS Phase 0's goal (establish structure, prove the
ruleset is enforceable) adapted to this project's actual architecture.

## Test Harness

Simulation code is compiled and tested via **UE Automation Tests, running inside the editor**.
There is no standalone non-UE5 build and none is to be created — settled by Rule 5's August 26,
2026 addendum, which corrected the rule's original "testable without a running engine" phrasing.
That phrasing was inherited from PRS, where a standalone engine-agnostic build was a genuine
project goal (PRSCore compiles with zero Unreal headers, CI-gated). RTAC's portability target is
"across UE5 projects," not away from UE5 (`RTAC.uplugin`), so no equivalent goal applies here.

Tests live inside the plugin per Rule 11 (`Plugins/RTAC/Source/…`), never in
`Source/ProjectAtlantis/`.

## ⚠ Open Sub-Item — Enforcement of the "No Engine Types" Half

**Genuinely undecided. Not to be marked resolved or silently defaulted to one of the candidates
below.** Rule 5's addendum removed the standalone-build requirement, but that build was also
doing enforcement work for the half of the rule that *does* still stand — no `AActor*`,
`UObject*`, or `FVector` in the simulation layer. On PRS, the CMake build failed outright if an
Unreal header appeared in PRSCore; that check doesn't exist here yet, and UE Automation Tests
give no equivalent guarantee — nothing stops an engine type compiling cleanly into the rules
layer.

Candidates, none selected:

- **A second UBT module inside the plugin** (e.g. `RTACSim`) with a deliberately narrow
  dependency list, so UBT itself rejects engine-type creep. **Verified caveat:** `FVector`
  resolves to `UE::Math::TVector`, defined in `Runtime/Core/Public/Math/Vector.h` — i.e. inside
  the `Core` module. A `Core`-only dependency list does not by itself exclude `FVector`; the
  exact dependency list needs deciding, not assuming.
- **A grep/lint gate** over the simulation subtree, run as part of the Phase Exit Review.
- **Review-only**, accepting the rule as convention-enforced rather than machine-enforced.

This needs its own Decision entry before Phase 0 can close.

## Definition of Done

**Part A — verifiable without a live editor session**
- [x] `RTAC.uplugin`, `RTAC.Build.cs`, `RTACModule.h/.cpp` exist and compile
- [x] `RTAC` registered in `ProjectAtlantis.Build.cs`'s `PublicDependencyModuleNames`
- [ ] A simulation subtree exists containing no `AActor*`, `UObject*`, or `FVector` (Rule 5)
- [ ] Enforcement mechanism for the above decided and logged as a `combat_decisions.md` entry
- [ ] Simulation state lives in an explicit state struct with no hidden globals or statics,
      per Rule 6
- [ ] Dedicated log category in use (`LogRTAC` — already confirmed working), not `LogTemp`,
      per Rule 9

**Part B — requires UE5 open**
- [x] Plugin loads in-editor; `LogRTAC: RTAC module loaded.` confirmed via
      `EditorToolset.LogsToolset` and on disk
- [ ] At least one UE Automation Test exists, is discoverable from the editor's Session
      Frontend, and can be run
- [ ] That test exercises simulation state and can *fail* — a test that cannot fail is not
      evidence (Failure Mode 8)
- [ ] The exact run procedure documented in `CLAUDE.md`

## Exit

Rules 5, 6, 9, and 11 reviewed against the current scaffold and accepted. **Phase 1 may not
begin until the enforcement sub-item above is decided** — without it, Rule 5's surviving half
has no teeth.

---

# Phase 1 — Grid & Movement (Headless Simulation)

**Status:** `OPEN`

## Goal

The board exists and entities move on it, provably, with no renderer involved at all.

## Resolves

- **"Exact grid dimensions"** (Open Questions → Core BN3 Loop) — dimensions chosen and logged.
  Expressed as rows×columns per **Decision #5**; BN3's own board is 3 rows × 6 columns
  (3 columns per side) but this project's dimensions are not required to match it.
- **"Movement rules"** (Open Questions → Core BN3 Loop).

## Definition of Done

- [ ] Grid dimensions chosen and logged as their own Decision entry, stated as rows×columns
      per Decision #5
- [ ] Simulation types hold POD/standard containers only — no engine types (Rule 5); grid
      coordinates are grid coordinates, not world transforms
- [ ] Movement resolves through the simulation layer only; no presentation-layer read of
      simulation state and no reverse dependency (Rule 5)
- [ ] The tile model carries a surface-modifier slot and an **elevation slot that exists but is
      mechanically inert** — see note below
- [ ] Same seed + same input sequence → identical resulting state, verified by test (Rule 6)
- [ ] Grid ↔ world unit conversion does not exist yet in this phase (no presentation layer) —
      confirmed by the absence of any such conversion in the simulation code (Rule 10)
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

---

# Phase 2 — Presentation & First Playable Board

**Status:** `OPEN`

## Goal

See the grid, move on it, isometric per Decision #1. The Rule 5 boundary survives contact with
an actual renderer.

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

Combat resolution exists: buster, HP/damage, and BN3's flat tile modifiers (ice, grass, lava,
steel, poison, cracked, broken panels).

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

Something fights back, legibly.

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

The BN3 loop closes: a full match runs start → win/lose.

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

Decision #3 becomes real. Mechanical direction is chosen and logged as its own new Decision
entry *before* implementation begins — not decided in code.

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

Decision #4's transition mechanism (GameMode swap vs. a mode flag on
GameState/PlayerController vs. something else) is decided and logged *before* implementation.

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

Enemy roster filled out, balance passes complete, and — the item nothing before this phase
verifies — RTAC's actual portability claim is tested, not assumed.

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

### Test gate — pending

PRS's test gate is a concrete `ctest -L '^phase[0-N]$'` query. RTAC has no equivalent yet,
because the exact UE Automation Test naming/tagging convention has not been decided (see Phase
0's outstanding Part B items). **This is a live placeholder, not an oversight** — it is filled
in once Phase 0 closes and the automation-test discovery/run procedure is documented in
`CLAUDE.md`.

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
