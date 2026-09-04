# Phase 1 — Grid & Movement (Headless Simulation) — Completion Record

**Phase:** 1 — Grid & Movement (Headless Simulation)
**Started:** August 30, 2026 (`43166da`, first Phase 1 commit after Phase 0's close)
**Closed:** September 3, 2026 (`e0edee9`, citation completed in `2461861`)
**Status:** CLOSED — enacted in 6342e68, c436334, f1363b4, 9595330, 37f68cb, e0edee9
**Exit Review:** `docs/PHASE1_CHECK.md` (September 3, 2026)

---

## Summary

Phase 1 delivered the combat board as a real, addressable data structure with entities moving on
it, entirely headlessly — no renderer, no actor, no presentation layer, and no simulation type
depending on the editor. The editor appears in exactly one role: hosting the automation tests.

The simulation layer is 3,107 lines across 20 source files, all plain structs and free functions
over UE Core value types. It holds no `UObject*`/`AActor*` ownership, carries no reflection macros,
and contains no grid↔world unit conversion anywhere — the last two positively verified by grep at
the Exit Review rather than assumed.

Five UE Automation Tests cover it, 168 assertions total, all green against a build verified on disk
to postdate every source edit.

Phase 1 also produced this project's first deliberate departure from the structure it inherited:
the exit review is a separate artifact (`PHASE1_CHECK.md`) rather than a section inside this file,
because a review that poses open questions for a ruling cannot live inside a document defined as
containing nothing unresolved. See `PHASES.md` → Purpose.

---

## Definition of Done — Final Status

All seven items satisfied. **Verdicts and evidence pointers only** — the evidence itself is inline
in `PHASES.md`'s DoD items (see the migration note at the end of this document) and the review
verdicts are in `PHASE1_CHECK.md`. Neither is restated here.

| # | DoD Item | Verdict | Evidence lives in |
|---|---|---|---|
| 1 | Grid dimensions chosen and logged as their own Decision, rows×columns | ✅ | Decision #8; `FRTACGrid::DefaultRows/DefaultColumns` |
| 2 | Plain structs / UE Core value types; no `UObject*`/`AActor*` ownership; grid coords not world transforms | ✅ | `PHASE1_CHECK.md` §7 (greps re-run September 3) |
| 3 | Movement resolves through the simulation layer only; no presentation coupling either direction | ✅ | `RTACMovementLegality.cpp`; `PHASE1_CHECK.md` §1 |
| 4 | Tile model carries a surface-modifier slot and a mechanically inert elevation slot | ✅ | `FRTACTile::SurfaceModifier`, `::Elevation`; `PHASES.md` DoD item 4 |
| 5 | Same seed + same input sequence → identical state, verified by test | ✅ *(replay axis only)* | `RTAC.Simulation.Match.DeterministicReplay`; caveat in `PHASES.md` determinism note |
| 6 | No grid↔world unit conversion exists in this phase's code | ✅ | `PHASE1_CHECK.md` §7 — zero hits, token list extended beyond September 1's five terms |
| 7 | Tests run on the full configured grid, never degenerate | ✅ | `RTAC.Simulation.Movement.MultiEntity` — 3×6 board, four entities |

**Item 5 is checked on the input-sequence axis only, and that is permanent, not pending.** Phase 1
has no gameplay randomness, so the seed axis is inert and no seed-axis control is writable here.
It becomes testable when the first real RNG stream exists — a Phase 4/5 obligation inherited from
this phase, not a Phase 1 leftover. The full statement lives in `PHASES.md`'s determinism note and
is not re-derived here.

---

## What Was Built — Simulation Surface

**This inventory is authoritative here.** It previously lived in `CLAUDE.md`'s "Current state"
paragraph, which describes what exists *right now* and is rewritten as the project moves; Phase 1's
delivered surface would have vanished from the record the first time Phase 2 edited it. `CLAUDE.md`
now points here instead of carrying a second copy (Failure Mode 7).

All types below are simulation-layer: plain structs, no `UPROPERTY`/`UCLASS`/`USTRUCT`/`UENUM`, no
reflection, no engine ownership (Rule 5).

### Types

| Type | File | Role |
|---|---|---|
| `FRTACGridPosition` | `Public/Simulation/RTACGridPosition.h` | Discrete `(Row, Column)` index. Named fields rather than `FIntPoint`'s X/Y, which reads as screen space (Rule 5 Addendum #3) |
| `FRTACTile` | `Public/Simulation/RTACTile.h` | One board tile: `Position`, `OccupantEntityId`, `SurfaceModifier`, `Owner`, `Elevation` |
| `FRTACGrid` | `Public/Simulation/RTACGrid.h` | The board. Flat row-major `TArray<FRTACTile>`, not nested — one iteration order, which Rule 6 determinism depends on |
| `FRTACEntity` | `Public/Simulation/RTACEntity.h` | `EntityId`, `Position`, `Side`, `ArchetypeId` (Decision #9 + addenda) |
| `FRTACRngState` | `Public/Simulation/RTACMatchState.h` | Master seed. No stream fields yet, deliberately |
| `FRTACMatchState` | `Public/Simulation/RTACMatchState.h` | The whole per-match container: `Rng`, `Grid`, `Entities`, `NextEntityId` |
| `ERTACSurfaceModifier` | `Public/Simulation/RTACSurfaceModifier.h` | `None`, `Broken`. Rest of the list is Phase 3 |
| `ERTACTileOwner` | `Public/Simulation/RTACTileOwner.h` | `Neutral`, `Player`, `Enemy`. Serves both tile ownership and entity side |
| `ERTACMoveLegality` | `Public/Simulation/RTACMovementLegality.h` | Seven values: `Legal`, four Ruling 4 clauses, `InvalidOrigin`, `NotAdjacent` |

### Functions

| Function | File | Contract |
|---|---|---|
| `RTACDeriveStreamSeed` | `RTACStreamSeed.h/.cpp` | Pure `(MasterSeed, StreamName) → int32`. Stream names are a persisted contract, not labels |
| `RTACSpawnEntity` | `RTACMatchState.h/.cpp` | The one place an entity is placed on the board. Establishes the grid/entity consistency invariant. Advances `NextEntityId` only on success |
| `RTACCheckMoveLegality` | `RTACMovementLegality.h/.cpp` | Pure destination-occupancy predicate. Four clauses, first failure wins. Returns five of the seven enum values — never the two origin-facts |
| `RTACResolveMove` | `RTACMovementLegality.h/.cpp` | Rule 7 resolution stage. Re-validates internally; fixed order check → origin → adjacency → mutations; no partial application |
| `FRTACGrid::Init/Reset/FindTile/GetTileChecked/IsValidPosition/ToIndex` | `RTACGrid.h/.cpp` | Board storage, bounds checking, tile access. `Init()` is a complete standalone reinit (Rule 6) |
| `FRTACMatchState::Initialize/Reset/FindEntity` | `RTACMatchState.h/.cpp` | `FindEntity` is the only supported id→entity path; array index is explicitly not identity |

### Deliberately inert, and not dead code

`FRTACTile::Elevation` and `FRTACEntity::ArchetypeId` are reserved and unread by any non-test
`.cpp`. Both exist so Phase 6 (elevation) and Phases 3–4 (stats/AI lookup) can attach without
retrofitting a nested branch into logic that never anticipated them — Rule 8's "concrete risk"
note. The only thing that touches either is the determinism test's comparison-helper liveness
control, which exists precisely because a comparison of a permanently inert field is otherwise
indistinguishable from a missing one.

---

## Test Evidence

Five tests, **168 assertions**, all green. Run September 2, 2026.

| Test | Assertions |
|---|---|
| `RTAC.Simulation.Grid.BasicLifecycle` | 13/13 |
| `RTAC.Simulation.Match.DeterministicReplay` | 51/51 |
| `RTAC.Simulation.Movement.MultiEntity` | 74/74 |
| `RTAC.Simulation.Rng.MatchStateLifecycle` | 24/24 |
| `RTAC.Simulation.Rng.StreamSeedDerivation` | 6/6 |

Zero `[FAIL]` lines. Zero `LogRTAC` errors. Four `LogRTAC` warnings, all deliberately provoked by
`MultiEntity`: two spawn refusals, one `NotAdjacent`, one `InvalidOrigin`.

**Build verification chain** — `UnrealEditor-RTAC.dll` 23:24:25 postdates the last source edit at
23:21:58; module loaded 23:25:08; tests run 23:25:35. Read from
`Saved/Logs/ProjectAtlantis.log` after confirming which editor instance wrote it.

`MultiEntity` renders amber rather than green in the Session Frontend. That is Success-with-warnings,
not failure. The diagnosis lives in `RTACMovementTest.cpp`'s own header beside the warnings that
cause it and is not restated anywhere else.

---

## Decisions This Phase

| # | Title | Status as of this record |
|---|---|---|
| #8 | Grid Dimensions: 3×6 (configurable default) | `PARTIAL` — simulation side enacted in `9286975`; presentation wrapper is Phase 2 |
| #9 | Entity Identity: plain counter, separate from archetype | `CLOSED` — `43166da`, `6321472`, `1264631` |
| #10 | Movement Rules: discrete step, mutable ownership, reusable check | `CLOSED` — `c51027e`, `5231eac`, `1264631`, `1c27877`, `c436334` |
| #11 | Match-State Container and Entity Spawn | `CLOSED` — `6342e68` (closed in this commit; see below) |
| #12 | Movement Adjacency: enforced in resolution | `CLOSED` — `37f68cb` |
| #13 | Per-Entity Override Capabilities Are Archetype Data | `OPEN` — deferred by design, no consumer yet |
| #14 | Automation Test Naming | `RATIFIED` — first use of that status |

**Decision #11 was found enacted-but-unclosed while this record was being written, and is closed in
the same commit.** Its status had read `OPEN` since September 1 despite all four rulings landing in
`6342e68`: `FRTACMatchState` holds `Grid` and `Entities` (Ruling 1), storage is `TArray` not `TMap`
(Ruling 2), `FindEntity` exists as a const/non-const pair with the index-is-not-identity contract
stated at the storage field (Ruling 3), and `RTACSpawnEntity` is the sole placement path (Ruling 4).
Same shape as Decision #12 before `81659c4` closed it. The closure addendum is in
`combat_decisions.md` per Rule 4, not here — a completion record reports status, it does not set it.

**That this surfaced at all is the clearest argument for this artifact existing.** Nothing else in
the project cross-checks decision status against enacted code: `PHASES.md` tracks DoD items,
`PHASE1_CHECK.md` audits rules and failure modes, and neither walks the decision log. Enumerating
the phase's decisions to write them down is what caught it.

Decision #13 remains `OPEN` correctly — it rules on where a capability *would* live, and no
override mechanism exists to place. Two seams are documented and unbuilt: Decision #10 Ruling 5's
broken-tile override and Decision #12 Ruling 5's movement-range override. They are deliberately
kept distinct.

---

## Commits

Phase 1 spans **23 commits**, `43166da` (August 30) through `2461861` (September 3). `195a0de` is
Phase 0's closing commit and the boundary.

The `CLOSED` status cites six of them — the substantive enactment commits, not the doc syncs:
`6342e68`, `c436334`, `f1363b4`, `9595330`, `37f68cb`, `e0edee9`.

Load-bearing commits, in order:

| Commit | What it landed |
|---|---|
| `43166da` | Seeded-state groundwork (Decision #9) |
| `6321472` | `FRTACEntity` |
| `a651b0a` | Decision #10 logged |
| `c51027e` | `RTACCheckMoveLegality` (Ruling 4) |
| `5231eac` | Per-tile ownership (Ruling 3) |
| `1264631` | `FRTACEntity::Side` (blocker resolution) |
| `1c27877` | `ERTACSurfaceModifier::Broken` (blocker resolution) |
| `c436334` | `RTACResolveMove` |
| `6342e68` | Match-state container + entity spawn (Decision #11) |
| `f1363b4` | Multi-entity movement test |
| `9595330` | Determinism test — last DoD item |
| `37f68cb` | Decision #12 enacted — `NotAdjacent` |
| `81659c4` | Decision #12 closed |
| `e0edee9` | Exit Review + fifteen corrections; PARTIAL → CLOSED |
| `2461861` | Citation placeholder resolved |

---

## Files Changed

19 files, **+4,292 / −55** across `195a0de..HEAD`.

**New (12):** `RTACMatchState.h/.cpp`, `RTACMovementLegality.h/.cpp`, `RTACStreamSeed.h/.cpp`,
`RTACEntity.h`, `RTACTileOwner.h`, `RTACDeterminismTest.cpp`, `RTACMovementTest.cpp`,
`RTACStreamSeedTest.cpp`, `RTACTestFixtures.h`, plus `docs/PHASE1_CHECK.md`.

**Modified (7):** `RTACTile.h`, `RTACSurfaceModifier.h`, `CLAUDE.md`, `docs/PHASES.md`,
`docs/combat_decisions.md`, `docs/AGENTS.md`, `docs/reference.md`.

`RTACGridTest.cpp` was not touched in Phase 1 — its last change was in Phase 0's tail (`195a0de`).

---

## Exit Review Outcome

Full findings: `docs/PHASE1_CHECK.md`. Not restated here — this section records only the outcome.

All seven DoD items verified against code. All 8 Recurring Failure Modes checked: seven pass;
**Failure Mode 7 (duplication drifts) failed**, on the documentation, producing fifteen corrections
landed in `e0edee9`. No critical bugs. Safety Ruleset re-read live in full rather than inherited
from Phase 0's review. Rule 5 and Rule 10 absence-claims re-grepped from scratch, both clean.

Three things the review surfaced that were not on its input list: `CLAUDE.md`'s entire
"Next milestone" section was false in three ways, its layout tree still called the plugin
"scaffold only," and it carried build timestamps from a superseded run.

The review also introduced the `RATIFIED` status vocabulary term, defined in all three places the
vocabulary is duplicated, and was itself the evidence that promoted Failure Mode 8 to Rule 15
(Adversarial Verification).

---

## Deferred, and Carried Forward

| Item | Owner |
|---|---|
| Determinism's seed axis — no control writable until an RNG stream exists | Phase 4 or 5 |
| Decision #8's presentation-layer wrapper with editable Rows/Columns | Phase 2 |
| Decision #10 Ruling 5 — broken-tile override seam | unscheduled |
| Decision #12 Ruling 5 — movement-range override seam (Elebee warp) | unscheduled |
| Decision #13 — where capabilities live, once one exists | Phase 3/4 |
| Phase-tagging convention for the test gate | unscheduled |
| `RTACDeterminismTest.cpp:406` — "three" → "four" warnings, comment-only | next rebuild-forcing commit |

The last is deferred deliberately: landing it would make plugin source newer than the DLL and
falsify the build-verification claim `CLAUDE.md` and `PHASES.md` both depend on, for a comment.

---

## What Phase 2 Must Address

Per `PHASES.md` Phase 2 — Presentation & First Playable Board:

1. **A presentation layer that reads simulation state and renders it**, with no reverse dependency.
   Phase 1 verified that absence trivially because no presentation layer existed; Phase 2 is the
   first real test of Rule 5.
2. **The grid↔world conversion boundary** — Phase 1's DoD item 6 asserts this does not exist yet.
   Phase 2 creates it, and Rule 10 requires it happen at exactly one named function, presentation-side.
3. **Decision #8's editable Rows/Columns wrapper**, closing that decision's outstanding half.
4. **An input layer owning the real direction convention.** The determinism test defines one
   test-locally and says explicitly that nothing in the simulation should.
5. **Tick order and simultaneity (Rule 7)** — Phase 1's input sequences are a total order by
   construction, so "which entity resolves first when two move in the same tick" never arose. It
   becomes real here and wants its own decision entry.
6. **`PHASE2_CHECK.md` and `PHASE2_COMPLETED.md`**, both now required before Phase 2 may close.

---

## Migration Note — Where Phase 1's Evidence Actually Lives

`PHASES.md`'s Phase 1 DoD items carry their completion evidence **inline**, written before the
three-document split existed. That evidence is not duplicated into this record; this record cites
it. Going forward, DoD items carry a verdict and a pointer, and the evidence lives in the phase's
completion record.

Phase 1's inline evidence is deliberately **not** migrated. Moving it would rewrite a closed phase's
DoD items after the fact, and the whole point of the split is that one quantity has one home — the
home for Phase 1's evidence is already established and citable, and relocating it would break every
reference that currently points at it for no gain.

---

*Phase 1 closed September 3, 2026.*
*Lead: Omar. Implementation and Exit Review: Claude Code (Opus 5) with MCP to UE 5.8.*
*Engine: Unreal Engine 5.8 | Development Editor Win64 | RHI: DX12*
