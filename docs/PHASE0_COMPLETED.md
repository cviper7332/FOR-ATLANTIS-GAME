# Phase 0 — Foundation & Test Harness — Completion Record

**Phase:** 0 — Foundation & Test Harness
**Started:** on or before August 26, 2026 — see the reconstruction note below; no earlier boundary is recoverable
**Closed:** August 29, 2026 (`195a0de`, 14:16)
**Status:** CLOSED — enacted in 9286975, c5527ca, 3df1fed
**Exit Review:** none exists — see "Exit Review Outcome" below
**Record written:** September 21, 2026 — retroactively, 23 days after closure

---

## Reconstruction Note — Read This Before Trusting Anything Below

**This record is weaker evidence than `PHASE1_COMPLETED.md`, and the difference is not cosmetic.**
Phase 1's record was written at closure by the agent that did the work, from live state, with the
Exit Review it cites sitting beside it. This one was reconstructed on September 21, 2026 from git
history and the documents Phase 0 left behind. Nobody wrote it at the time because the requirement
to write it did not exist yet.

What that costs, concretely:

- **No contemporaneous observation survives.** Every claim below is derived from a commit diff, a
  commit message, or a document. Where Phase 0's own documents assert something that git cannot
  confirm — a build timestamp, a Session Frontend result Omar read off a screen — this record
  repeats the assertion *and says whose assertion it is*, rather than laundering it into a fact.
- **Phase 0's build-verification artifacts are gone.** Its DoD rests on
  `UnrealEditor-RTAC.dll` timestamps from August 26 and August 29. The DLL on disk today is dated
  **2026-09-02 23:24** — Phase 1's build. Those two timestamps are no longer independently
  checkable by anyone, and will not become checkable again.
- **The start boundary is genuinely unknown.** The project had no version control before
  `207d361` (August 26, 2026, 01:22). `CLAUDE.md`, `docs/AGENTS.md`, and `docs/combat_decisions.md`
  (already carrying Decisions #1–#3) are *present in that first commit*, so an unrecorded amount of
  Phase 0 predates git entirely. "August 26" is the first day git can see, not the day work began.
- **Authorship is mostly unrecoverable.** Only 4 of Phase 0's 23 commits carry a `Co-Authored-By`
  trailer; the convention was not yet in place. All four name Claude Sonnet 5.

Every claim below that *could* be re-derived from source was re-derived from source for this
record, not inherited from `PHASES.md` or from `PHASE1_COMPLETED.md`'s statements about Phase 0
(Rule 15). Three of those re-derivations changed the answer; they are flagged where they appear.

---

## Summary

Phase 0 turned the RTAC plugin from nothing into a real, loading, compiling UE5 module with a
simulation subtree and a working automation test, and established the Safety Ruleset's
architectural half as something enforced against actual code rather than asserted about future
code.

At its close the plugin held **8 C++ files, 583 lines** — of which the `Simulation/` tree was
**391 lines** — plus `RTAC.uplugin` (24) and `RTAC.Build.cs` (19). For scale: Phase 1 ended at 20
files and 3,107 lines by the identical measure, which reproduces exactly at `HEAD` and was used
here unchanged.

Phase 0 also produced two of this project's four governing documents. `docs/PHASES.md` — the
roadmap that now defines every phase's exit criteria, including the three-document model this
record exists to satisfy — was created inside Phase 0 (`3d58785`). So was `docs/reference.md`
(`3df1fed`).

It also carried the project's only pre-git→git transition: `207d361` is the initial commit, and
Rule 3's addendum recording that the safety net finally existed landed one commit later.

---

## Definition of Done — Final Status

All ten items satisfied — six in Part A, four in Part B. **Verdicts and evidence pointers only**;
the evidence itself is inline in `PHASES.md`'s DoD items (see the migration note at the end of this
document) and is not restated here.

**Part A — verifiable without a live editor session**

| # | DoD Item | Verdict | Evidence lives in |
|---|---|---|---|
| 1 | `RTAC.uplugin`, `RTAC.Build.cs`, `RTACModule.h/.cpp` exist and compile | ✅ | `PHASES.md` Part A item 1 — August 26, 2026 build |
| 2 | The `Simulation/` tree compiles | ✅ | `PHASES.md` Part A item 2 — August 29 build, DLL timestamp plus two MCP log queries |
| 3 | `RTAC` registered in `ProjectAtlantis.Build.cs`'s `PublicDependencyModuleNames` | ✅ | `ProjectAtlantis.Build.cs:22` — re-verified live for this record |
| 4 | Simulation subtree holds plain structs / UE Core value types only, no `UObject*`/`AActor*` ownership | ✅ | Re-grepped for this record — see "Exit Review Outcome" |
| 5 | Simulation state in an explicit state struct, no hidden globals or statics (Rule 6) | ✅ | Re-grepped for this record — `FRTACGrid` members plus exactly two `static constexpr` |
| 6 | Dedicated log category `LogRTAC`, not `LogTemp` (Rule 9) | ✅ | `RTACModule.h:8`, `RTACModule.cpp:7` — re-verified live |

**Part B — requires UE5.8 open**

| # | DoD Item | Verdict | Evidence lives in |
|---|---|---|---|
| 7 | Plugin loads in-editor; `LogRTAC: RTAC module loaded.` confirmed | ✅ | `PHASES.md` Part B item 1 — MCP and on disk |
| 8 | ≥1 Automation Test exists, is Session-Frontend-discoverable, and runs | ✅ | `RTAC.Simulation.Grid.BasicLifecycle`; run by Omar August 29, 2026 |
| 9 | That test exercises simulation state and *can fail* | ✅ *(design claim only)* | `PHASES.md` Part B item 3 — stated there as a claim about construction, explicitly **not** proven by the passing run |
| 10 | Exact run procedure documented in `CLAUDE.md` | ✅ | `CLAUDE.md` → "Running RTAC's Automation Tests" |

**Item 9 is the one to read carefully.** `PHASES.md` checks it on the test's *design* — each
assertion tied to a named regression that would trip it — and says outright that one green run
cannot demonstrate failability. No failure-injection run was performed in Phase 0, and none has
been performed since. The box is correctly checked against what it actually claims; it does not
claim what a failure-injection run would.

---

## What Was Built — Plugin, Simulation Surface, and Harness

All simulation types below are plain structs: no `UPROPERTY`/`UCLASS`/`USTRUCT`/`UENUM`, no
reflection, no engine ownership (Rule 5). Verified by grep against the tree as it stood at
`195a0de`, not against today's tree.

### Plugin infrastructure

| Artifact | File | Role |
|---|---|---|
| Plugin descriptor | `RTAC.uplugin` | One Runtime module, `LoadingPhase: Default`, `CanContainContent: true`. Created in `ad0590c` |
| Build script | `Source/RTAC/RTAC.Build.cs` | `Core`, `CoreUObject`, `Engine` public; private list empty. Unchanged since `f018a0b` — still true at `HEAD` |
| Module | `RTACModule.h/.cpp` | `IModuleInterface` implementation; `LogRTAC` declared and defined here; logs `RTAC module loaded.` on startup |
| Dependency registration | `ProjectAtlantis.Build.cs:22` | `"RTAC"` in `PublicDependencyModuleNames` — the line that makes the two modules genuinely link (`e89266f`) |

### Simulation types

| Type | File | Role |
|---|---|---|
| `FRTACGridPosition` | `Public/Simulation/RTACGridPosition.h` | Discrete `(Row, Column)` with `operator==`/`!=`. Introduced specifically to avoid `FIntPoint`'s screen-space-flavored X/Y, which would silently invert against rows×columns (Decision #5, Rule 5 Addendum #3) |
| `FRTACTile` | `Public/Simulation/RTACTile.h` | Four fields only at this phase: `Position`, `OccupantEntityId` (`int32`, `INDEX_NONE` sentinel, never a pointer), `SurfaceModifier`, `Elevation`. **No `Owner` field** — that is Phase 1's `5231eac` |
| `FRTACGrid` | `Public/Simulation/RTACGrid.h` | The board. Flat row-major `TArray<FRTACTile>`, not nested — one unambiguous iteration order, which Rule 6 determinism depends on. `DefaultRows = 3` / `DefaultColumns = 6` as `static constexpr`, per Decision #8 |
| `ERTACSurfaceModifier` | `Public/Simulation/RTACSurfaceModifier.h` | `None = 0` **and nothing else** at this phase. `Broken` arrives in Phase 1 (`1c27877`); the rest of the BN3 list is Phase 3 |

### Functions

| Function | Contract |
|---|---|
| `FRTACGrid::Init(InRows, InColumns)` | Complete standalone reinitialization (Rule 6). Dimensions are parameters, never compile-time constants — Decision #8's simulation half |
| `FRTACGrid::Reset()` | Clears to uninitialized state |
| `FRTACGrid::IsValidPosition()` ×2 | Bounds guard, `int32` and `FRTACGridPosition` overloads |
| `FRTACGrid::FindTile()` ×4 | Const/non-const × `int32`/`FRTACGridPosition`. Returns `nullptr` out of bounds |
| `FRTACGrid::GetTileChecked()` ×2 | Const/non-const asserting accessor |
| `FRTACGrid::ToIndex()` | Row-major index arithmetic, private |
| `GetRows`/`GetColumns`/`NumTiles`/`IsInitialized`/`GetTiles` | Inline observers |

### Deliberately inert, and not dead code

`FRTACTile::Elevation` (`int32 = 0`) is reserved and unread. It exists so Phase 6 can add elevation
without retrofitting a nested branch into logic that never anticipated it (Rule 8, Decision #3).
`ERTACSurfaceModifier` is likewise a reserved slot carrying one enumerator — the enum exists so
Phase 3 can extend it additively with no structural change to `FRTACTile`.

### Documents created

| Document | Commit | What it established |
|---|---|---|
| `docs/PHASES.md` | `3d58785` | The eight-phase roadmap and every phase's exit criteria — including, eventually, the three-document model this record satisfies |
| `docs/reference.md` | `3df1fed`, extended `195a0de` | Verified UE5.8 API notes. First sections: automation-test registration, how a test actually passes/fails, `EAutomationTestFlags`' one-filter-flag constraint, the framework's `UE_LOG` interception, and the mirror-to-`LogRTAC` pattern |

### Safety Ruleset changes

**Correcting the premise this record was commissioned under:** Phase 0 did **not** adopt Rules 5
and 9. Both are present in the initial commit `207d361`, which means they were written before this
project had version control and their original adoption is **not recoverable from git at all**.
What Phase 0 did was amend Rule 5 three times and originate four other rules.

| Change | Commit | Date |
|---|---|---|
| Rule 5 Addendum #1 — retires "testable without a running engine"; settles that there is no standalone non-UE5 build | `2d769c3` | Aug 26 |
| Rule 5 Addendum #2 — UE Core value types (`TArray`, `TMap`, `FString`) permitted and preferred | `10b88d7` | Aug 27 |
| Rule 5 Addendum #3 — `FRTACGridPosition` supersedes `FIntPoint` for grid positions specifically | `a6d34fb` | Aug 28 |
| **Rule 11** — Combat Code Lives Inside the RTAC Plugin | `c3c8c81` | Aug 26 |
| **Rule 12** — Diffs Shown as Their Own Block | `e8bc0b2` | Aug 26 |
| **Rule 13** — Verify Date Before Dated Content | `f70805f` | Aug 28 |
| **Rule 14** — Goal Describes Its Full DoD | `f70805f` | Aug 28 |
| Rule 3 addendum — git and remote now exist; rule not retired | `46e0eba` | Aug 26 |

Rule 13 was established *by* a Phase 0 failure: seven dated entries written on August 27–28 were
labeled August 26, because the date was carried forward within a session instead of checked. Five
of them were corrected by dated addenda in `f70805f`, per Rule 4. This directly affects the
decision dates below.

---

## Test Evidence

One test. **`RTAC.Simulation.Grid.BasicLifecycle`** (`FRTACGridBasicLifecycleTest`,
`Plugins/RTAC/Source/RTAC/Private/Tests/RTACGridTest.cpp`), flags
`EditorContext | ProductFilter`. Added in `3df1fed` at 88 lines; extended to 157 in `195a0de`.

It covers `Init()` and dimension reporting, in-bounds `FindTile` position agreement, both
out-of-bounds row directions, and that `Reset()` actually clears state.

**Result at Phase 0's close: `Success`, and that is the entire recorded result.** Omar ran it in
the Session Frontend on August 29, 2026, against commit `3df1fed`.

**No assertion count exists for Phase 0's run, and one must not be borrowed.** The `LogRTAC`
mirroring that emits `complete: N/N assertions passed` did not exist when that run happened — it
landed in `195a0de`, *after* the run, as the same commit that closed the phase. The **13/13**
figure that appears in `CLAUDE.md` and in `PHASE1_COMPLETED.md` comes from the **September 2, 2026**
run, during Phase 1. Attributing it to Phase 0 would be attributing a measurement to a run that
could not have produced it.

**Two things follow, and both are load-bearing:**

- The test file has not changed since `195a0de` — verified by content hash against `HEAD` for this
  record. So the 13-assertion September 2 run did exercise *exactly* the code Phase 0 shipped. The
  number is trustworthy about Phase 0's test; it is simply not Phase 0's own evidence.
- The `LogRTAC` MCP-retrieval path was **written but never built or verified** at closure.
  `PHASES.md` carried it explicitly as non-blocking, correctly — no DoD item required it. It was
  verified live on September 1, 2026, per `CLAUDE.md`.

**Build verification at closure:** `PHASES.md` records the `Simulation/` tree compiling on
August 29, 2026, with the DLL timestamp corroborated independently by two MCP log queries
(`LogModuleManager: InternalLoadLibrary` and `LogRTAC: RTAC module loaded.`, both postdating the
build). As stated in the reconstruction note, that DLL no longer exists to re-check. This record
reports the claim and its stated basis; it cannot re-run it.

**One build failure is recorded and worth keeping.** The first build attempt of the simulation tree
failed with ~15 cascading errors from a single character sequence: a literal `*/` inside a Doxygen
comment's prose (`UObject*/AActor*`) closed the comment block early and turned the remainder of
`RTACTile.h` into unparsed code. Fixed in `c5527ca`, punctuation only, no comment's meaning
changed. The fix is still visible in the live file as `UObject* / AActor*` with spaces.

---

## Decisions This Phase

**Decisions #1–#8 were logged during Phase 0's window — not #1–#7.** This was re-derived per Rule 15
rather than inherited, and it changed the answer. Decision #8 landed in `ad1fe4b` on August 28,
inside Phase 0's commit range, and its `Phase:` field reads "Combat system design
(pre-implementation)" exactly as #1–#7 do. Decision #9 (`43166da`, August 30) is the first genuinely
Phase 1 entry.

| # | Title | First appeared in | Actual write date | Status (live, September 21, 2026) |
|---|---|---|---|---|
| #1 | Combat Camera: Isometric 2.5D | `207d361` (pre-git) | ≤ Aug 26 | `OPEN` |
| #2 | Combat System Fully Decoupled from PHIS | `207d361` (pre-git) | ≤ Aug 26 | `N/A — design rationale` |
| #3 | Elevation Will Be Mechanically Meaningful | `207d361` (pre-git) | ≤ Aug 26 | `OPEN` |
| #4 | Combat Structure: Expedition-33-Style World Engagement | `45f5b6b` | Aug 26 | `N/A — design rationale` |
| #5 | Grid Axis-Order Convention: Rows×Columns | `9e2ae97` | Aug 26 | `N/A — design rationale` |
| #6 | Rule 5 Enforcement: Review-Only, Not Machine-Enforced | `10b88d7` | **Aug 27** | `N/A — design rationale` |
| #7 | Development Engine Floor: UE 5.8, Hard Requirement | `10b88d7` | **Aug 27** | `N/A — design rationale` |
| #8 | Grid Dimensions: 3 Rows × 6 Columns (Configurable) | `ad1fe4b` | **Aug 28** | `PARTIAL` |

**Decisions #6, #7 and #8 carry `**Date:** August 26, 2026` headers that are known to be wrong.**
Each has a dated correction addendum from `f70805f` giving the true date and commit; the original
field is left untouched per Rule 4. Anyone citing these three by date should cite the addendum, not
the header. The bolded dates in the table above are the corrected ones.

**Decisions #1–#3 cannot be dated more precisely than "at or before the initial commit."** They
predate version control. Their `**Date:**` fields say August 26, 2026, and nothing independent
corroborates that.

### One commit is claimed by both phases

**`9286975` — "RTAC: add simulation-layer grid and tile types" — declares itself Phase 1 work in
its own subject line and body ("Data structure only, per Phase 1 scope"), yet it is cited as one of
Phase 0's three enactment commits in `PHASES.md`'s status line, and separately cited by
`PHASE1_COMPLETED.md` as where Decision #8's simulation half landed.**

All three citations are individually defensible. `9286975` is what made Phase 0's Part A items 4
and 5 true — a simulation subtree cannot be audited for engine types before it exists — while the
types it added are equally the substrate Phase 1 built on. The commit was authored believing it was
Phase 1 work; Phase 0's closing commit three days later claimed it retroactively.

**This is recorded, not resolved.** Reassigning it would require editing a closed phase's status
line, and the overlap is a fact about how the boundary actually fell rather than an error in either
record. The one thing not to do is cite it as evidence that Phase 0 and Phase 1 have a clean commit
boundary — they do not. `195a0de` is the boundary for every *other* purpose.

---

## Commits

Phase 0 spans **24 commits**, `207d361` (August 26, 01:22) through `195a0de` (August 29, 14:16) —
23 of them after the baseline commit. Phase 1, for scale, spans 23.

The `CLOSED` status cites three: `9286975`, `c5527ca`, `3df1fed`. Note that `195a0de`, the commit
that *performed* the closure and carries "Phase 0 CLOSED" in its subject, is **not** among the
three it cites.

Load-bearing commits, in order:

| Commit | What it landed |
|---|---|
| `207d361` | Initial commit — stock UE5.8 template, plus `CLAUDE.md`, `AGENTS.md`, `combat_decisions.md` (Decisions #1–#3) already present |
| `46e0eba` | Git recorded in `CLAUDE.md`; Rule 3 addendum |
| `ad0590c` | `RTAC.uplugin` created |
| `f018a0b` | Module scaffold — `RTAC.Build.cs`, `RTACModule.h/.cpp` |
| `c3c8c81` | Rule 11 — combat code lives inside the plugin |
| `e89266f` | `LogRTAC` category + startup log; **`RTAC` registered as a project dependency** |
| `e8bc0b2` | Rule 12 |
| `2d769c3` | Rule 5 Addendum #1 — no standalone non-UE5 build |
| `3d58785` | **`docs/PHASES.md` created** |
| `10b88d7` | Decisions #6 and #7; Rule 5 Addendum #2 |
| `ad1fe4b` | Decision #8 — grid dimensions 3×6 |
| `a6d34fb` | Rule 5 Addendum #3 — `FRTACGridPosition` |
| `9286975` | **Simulation-layer grid and tile types** (self-labeled Phase 1 — see above) |
| `f70805f` | Rules 13 and 14; five date corrections; `PHASES.md` accuracy pass |
| `a66f700` | Rule 14 applied across all eight phases — full Goal rewrite |
| `c5527ca` | **Comment-syntax build break fixed; Part A confirmed compiling** |
| `3df1fed` | **First automation test; `docs/reference.md` created** |
| `195a0de` | Assertions mirrored to `LogRTAC`; run procedure in `CLAUDE.md`; **Phase 0 `PARTIAL` → `CLOSED`** |

---

## Files Changed

17 files, **+2,068 / −11** across `207d361..195a0de` — that is, excluding the baseline commit's own
856-file template import.

**New (10 under the plugin, 2 docs):** `RTAC.uplugin`, `RTAC.Build.cs`, `RTACModule.h/.cpp`,
`RTACGrid.h/.cpp`, `RTACGridPosition.h`, `RTACSurfaceModifier.h`, `RTACTile.h`, `RTACGridTest.cpp`,
plus `docs/PHASES.md` and `docs/reference.md`.

**Modified (5):** `CLAUDE.md` (+152), `docs/AGENTS.md` (+249), `docs/combat_decisions.md` (+109),
`ProjectAtlantis.uproject` (+4, `MCPClientToolset`), `ProjectAtlantis.Build.cs` (+1/−1).

**`ProjectAtlantis.Build.cs` is the only file touched under `Source/ProjectAtlantis/` in the entire
phase, and the change is three lines of dependency registration.** No combat logic was added
outside the plugin — Rule 11 held for the phase that created it.

---

## Exit Review Outcome

**No Exit Review was run, and no `PHASE0_CHECK.md` exists.** The requirement did not exist at the
time: the three-document model was established September 3, 2026 in `d313976`, five days after
Phase 0 closed, and `PHASE1_CHECK.md` is the first exit review this project has ever produced.
This section states that plainly rather than leaving a heading with nothing under it.

`PHASES.md`'s Phase 0 section does carry an **Exit** clause — *"Rules 5, 6, 9, and 11 reviewed
against the current simulation code and accepted"* — but no artifact anywhere records it being
performed. What exists instead is scattered through commit messages: `c5527ca` notes that "the
engine-type audit and log-category items were already grep-verified earlier tonight," and `9286975`
claims a whole-tree grep for engine types with every hit landing in a comment. That is evidence of
checks having been run. It is not a review.

**Two documents refer to "Phase 0's review" as though the artifact existed.** `CLAUDE.md` and
`PHASE1_COMPLETED.md` both say Phase 1's Safety Ruleset read was done live "rather than inherited
from Phase 0's review." The *point* those sentences make is correct and remains correct — Phase 1
did not inherit. But the phrase presupposes a Phase 0 review that was never written. Recorded here
so the next reader looking for it stops looking.

**This record is the closest available substitute audit**, written under Rule 15's re-derivation
requirement — which is itself being applied retroactively, since Rule 15 postdates Phase 0's
closure by five days. Below is what that re-derivation actually found, run against the tree as it
stood at `195a0de`, reporting what held alongside what did not.

| Check | Falsifier | Result |
|---|---|---|
| Rule 5 — no `UObject*`/`AActor*`/`FVector` in simulation | Any hit in live code | **1 hit, in a comment** (`RTACTile.h:25`) — no usage. Holds |
| Rule 5 — no reflection macros | Any `UPROPERTY`/`UCLASS`/`USTRUCT`/`UENUM`/`GENERATED_BODY` in live code | **5 hits, all in comments** explaining their own absence. Holds |
| Rule 6 — no hidden globals or statics | Any non-`constexpr` static in `Simulation/` | **Zero.** Only `DefaultRows = 3` / `DefaultColumns = 6`, both `static constexpr`. Holds |
| Rule 9 — dedicated log category | Any `LogTemp` in plugin source | **Zero.** `LogRTAC` declared `RTACModule.h:8`, defined `RTACModule.cpp:7`. Holds |
| Rule 10 — no grid↔world conversion | Any of `WorldLocation`, `WorldPosition`, `TileSize`, `CellSize`, `ToWorld`, `FromWorld`, `GridToWorld`, `WorldToGrid`, `GetActorLocation`, `FTransform` | **Zero hits.** Holds — trivially, as no presentation layer existed |
| Rule 11 — combat code inside the plugin | Combat logic under `Source/ProjectAtlantis/` | **Zero.** Only the 3-line dependency registration. Holds |
| DoD item 3 — dependency registered | Absence from `PublicDependencyModuleNames` | Present at `ProjectAtlantis.Build.cs:22`. Holds |

**The 8 Recurring Failure Modes were not checked against Phase 0 at the time and are not
retroactively certified here.** Doing so honestly would require the contemporaneous context this
record does not have. Two are worth noting as *observed* rather than audited: Failure Mode 8
(confident answers wrong on mechanism) is what `c5527ca`'s build break and `f70805f`'s seven
misdated entries both are, and both were caught and corrected inside the phase. Failure Mode 5
(degenerate test configuration) is arguably live — `BasicLifecycle` is a single-grid lifecycle test
and Phase 0's DoD asked for nothing more, but "runs on the full configured grid, never degenerate"
did not become a DoD item until Phase 1.

---

## Deferred, and Carried Forward

| Item | Disposition |
|---|---|
| `LogRTAC` MCP-retrieval path — written at `195a0de`, unbuilt and unverified at closure | **Resolved.** Verified live September 1, 2026 |
| Decision #8's presentation-layer wrapper with editable `Rows`/`Columns` | Still outstanding — Phase 2. Why Decision #8 reads `PARTIAL`, not `CLOSED` |
| Failure-injection run for DoD item 9 | **Never performed**, in Phase 0 or since. Item 9 is checked on design, and says so |
| Phase 0's Exit clause — formal Rules 5/6/9/11 review | Never run. This record's audit table above is the substitute |
| MCP has no console-command or test-listing query, so Session Frontend discovery cannot be verified programmatically | Logged honestly in `reference.md` at the time; still unresolved |
| `PHASE0_CHECK.md` | **Will not be written.** A review is written before a phase closes, to pose questions for a ruling; fabricating one 23 days after closure would invent deliberation that never happened |

---

## What Phase 1 Had to Address

**This section is structurally different from its counterpart in `PHASE1_COMPLETED.md`, and the
difference is unavoidable.** Phase 1's version looks forward at an open phase. This one is written
with 23 days of hindsight about a phase that has since closed, so it states what Phase 0 handed
forward *and* how it actually landed. It is a record, not a prediction, and should not be read as
though Phase 0 anticipated any of it.

1. **Entities.** Phase 0 shipped a board with an `OccupantEntityId` field and nothing to put in it.
   → Decision #9, `FRTACEntity` (`6321472`).
2. **A populated `ERTACSurfaceModifier`.** The enum shipped with `None` alone and no movement rule
   to compare against. → `Broken` pulled forward from Phase 3 (`1c27877`) because the
   movement-legality check needed something real.
3. **Tile ownership.** `FRTACTile` had four fields; BN3's board is territorial. → `Owner` added
   (`5231eac`), Decision #10 Ruling 3.
4. **Movement.** Phase 0 built the board and nothing that moves on it. → `RTACCheckMoveLegality`
   (`c51027e`), `RTACResolveMove` (`c436334`), adjacency in `37f68cb`.
5. **Determinism as a tested property.** Rule 6 was asserted and grep-audited in Phase 0, never
   exercised. → `RTAC.Simulation.Match.DeterministicReplay`, and the seed-axis caveat that Phase 1
   in turn handed to Phase 4/5.
6. **A test that is not a single-grid lifecycle check.** → `MultiEntity`, on the full 3×6 board
   with four entities.

---

## Migration Note — Where Phase 0's Evidence Actually Lives

`PHASES.md`'s Phase 0 DoD items carry their completion evidence **inline**, written long before the
three-document split existed. That evidence is not duplicated into this record; this record cites it
and adds only what it could independently re-derive.

Phase 0's inline evidence is deliberately **not** migrated, for the same reason Phase 1's was not:
moving it would rewrite a closed phase's DoD items after the fact, and one quantity gets one home
(Failure Mode 7). The home for Phase 0's evidence is already established and citable.

**One asymmetry with Phase 1's migration note.** Phase 1's inline evidence was written by the people
who produced it, at the time. Phase 0's was too — but parts of it point at artifacts that no longer
exist, chiefly the August 26 and August 29 DLL timestamps. Citing that evidence means citing a
contemporaneous claim, not a re-checkable fact. That is a real limitation of Phase 0's record and
it does not get better with time.

---

*Phase 0 closed August 29, 2026. This record written September 21, 2026, retroactively.*
*Lead: Omar. Implementation: Claude Code — attribution recoverable for only 4 of 23 commits, all
naming Claude Sonnet 5. Reconstruction: Claude Code (Opus 5).*
*Engine: Unreal Engine 5.8 | Development Editor Win64 | RHI: DX12*
