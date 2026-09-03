# Phase 1 Exit Review — Findings

**Date of review:** September 2, 2026 (system clock checked: `Wed Sep 2 23:43:55 EDT 2026`).
**Working tree:** clean. **HEAD:** `81659c4`.

## Build/evidence chain (established first, since everything below rests on it)

| Artifact | Timestamp | Verdict |
|---|---|---|
| Last RTAC source edit | `2026-09-02 23:21:58` (`RTACDeterminismTest.cpp`) | — |
| `UnrealEditor-RTAC.dll` | `2026-09-02 23:24:25` | postdates all source ✅ |
| Test run in `ProjectAtlantis.log` | `2026.09.03-03.25.35` UTC = `23:25:35` EDT | postdates DLL ✅ |

Read from disk, not MCP — and I checked for the two-instance trap: `ProjectAtlantis_2.log` exists but was last written `19:06:25`, hours before this run. The primary log is the right subject.

**Live run, all five tests:**

| Test | Result |
|---|---|
| `RTAC.Simulation.Grid.BasicLifecycle` | 13/13 |
| `RTAC.Simulation.Match.DeterministicReplay` | **51/51** |
| `RTAC.Simulation.Movement.MultiEntity` | **74/74** |
| `RTAC.Simulation.Rng.MatchStateLifecycle` | 24/24 |
| `RTAC.Simulation.Rng.StreamSeedDerivation` | 6/6 |

Zero `[FAIL]` lines. Zero `LogRTAC` errors. **Four** `LogRTAC` warnings — two spawn refusals, one `NotAdjacent`, one `InvalidOrigin`.

The arithmetic on both deltas checks out exactly: `AttemptRejectedMove` fires 5 assertions (findable + legality + three no-partial-application), so 69 + 5 = 74; the determinism test gained one negative-coverage assertion, so 50 + 1 = 51.

---

## 1. DoD items — all seven genuinely satisfied

I checked each against code, not against its own checkbox.

- **Item 1 (dimensions logged)** — Decision #8, 3×6, rows×columns. `FRTACGrid::DefaultRows/DefaultColumns` are the single source; both tests reference them rather than restating 3 and 6. ✅
- **Item 2 (POD, no `UObject*`/`AActor*` ownership)** — re-grepped fresh, see §7. ✅
- **Item 3 (movement through simulation only)** — `RTACResolveMove` is a free function over plain structs; no presentation layer exists to read or be read. ✅
- **Item 4 (surface-modifier + inert elevation slot)** — both fields exist; elevation is mechanically inert. ✅ **but its stated evidence is now false** — see Finding E1.
- **Item 5 (determinism)** — `DeterministicReplay` 51/51, with three real controls (divergence, reconvergence, comparison-helper liveness) plus a hand-derived oracle. The seed-axis caveat is stated correctly and honestly in PHASES.md; I agree with both the caveat and the decision to check the box on the replay half. ✅
- **Item 6 (no grid↔world conversion)** — re-grepped fresh, zero hits. ✅
- **Item 7 (non-degenerate tests)** — full 3×6, four entities two per side, with an explicit `NumTiles() > 1` assertion and fixture controls proving ownership isn't vacuously Neutral. ✅

**Rule 14 spot-check:** Phase 1's Goal covers all seven DoD items. I mapped each one to its sentence; none is absent. ✅

---

## 2. Recurring Failure Modes — checked against Phase 1's new code

| # | Verdict |
|---|---|
| 1 — inert constant | **PASS, and generalized well.** No balance scalars exist yet. The tests apply FM1's logic to *clauses* instead: the movement test proves clause 3 is live (tile (1,2) → `WrongOwner` for Enemy, `Legal` for Player), clause 4 genuinely reachable (broken tile inside the mover's own territory, so clause 3 can't pre-empt it), and the new `NotAdjacent` case passes all four Ruling 4 clauses *and* the origin precondition so step 3 is what actually rejects. The determinism test's liveness control mutates each of 14 compared fields and requires the helper to notice. |
| 2 — balance domain | **N/A.** No balance numbers this phase. |
| 3 — oracle mismatch | **PASS.** Field-by-field comparison in a test-local helper — explicitly not `Memcmp` (padding, `TArray` heap pointers), not `operator==` (can't name the diverging field). |
| 4 — whole chain | **PASS.** The hand-derived expected-outcome sequence for run A is exactly FM4's worked example; A-vs-B agreement alone would pass a constant-returning resolver. |
| 5 — degenerate setup | **PASS.** See DoD item 7. |
| 6 — read source first | **PASS.** `INDEX_NONE` verified at `CoreMiscDefines.h:145`; the amber-colour diagnosis cites `SAutomationGraphicalResultBox.cpp:296` and `SAutomationTestItem.cpp:1086/1133`; the test gate cites `AutomationCommandline.cpp:610/785/149-163`. |
| 7 — duplication drifts | **FAIL — this is the review's main finding.** Fired exactly as predicted, on the docs. Details in §5. |
| 8 — confident-but-wrong / stale binary | **PASS.** Build chain verified above. The divergence control is FM8's "an experiment that cannot fail is not evidence" applied directly — it reported `Entities[1].Position.Row: 0 vs 2`. |

---

## 3. Critical bugs — my judgment: none

I'll state the standard I used: a critical bug is one that silently corrupts simulation state, crashes, or makes a passing test not mean what it claims. I found nothing meeting it.

The one thing worth naming explicitly, because it's the highest-risk construct in the tree:

**`RTACResolveMove` step 4.1 clears the origin tile's `OccupantEntityId` unconditionally**, without confirming that tile actually held the mover. On a hand-assembled board this would silently clear a *different* entity's occupancy with no error and no log line. This is **not** a bug — it's a documented, deliberate design with a named invariant owner (`RTACSpawnEntity`), the invariant is stated in three places, and the multi-entity test asserts board consistency after every successful move. I traced the degenerate case: if `Destination == Position` on an inconsistent board, the move falls through to `ManhattanDistance == 0` and returns `NotAdjacent` with nothing mutated. Safe.

Flagging it as the thing to re-examine when Phase 2's input layer starts producing moves the tests didn't author — not as a Phase 1 blocker.

---

## 4. Scope items 4, 5, 6 — AGENTS.md, combat_decisions.md, Safety Ruleset

**Scope 6 — Safety Ruleset re-read live: done.** Full read of all 687 lines this turn, not inherited from Phase 0. Rules 1–14, all addenda, Recurring Failure Modes, audit table.

**Scope 4 — yes, Phase 1's work invalidates three claims in AGENTS.md's Provenance section:**

- **`AGENTS.md:27`** — *"Rules 5–11 (Architecture) bind combat code as it is written; **there is none yet**, so they currently read as design constraints rather than audit targets."* Combat code now exists and was just audited against those rules. This is the assumption the phase broke.
- **`AGENTS.md:22`** — *"**No incidents have been logged against FOR ATLANTIS yet.**"* Contradicted by the file's own later content: Rule 13 carries a `Caught (August 28, 2026)` and a `Caught (August 30, 2026)`, both FOR ATLANTIS incidents citing FOR ATLANTIS commits (`10b88d7`, `a6d34fb`, `ad1fe4b`).
- **`AGENTS.md:26`** — the adoption-status line enumerates Rules 1–4 as Conduct and 5–11 as Architecture, omitting Rules 12, 13, 14 entirely, though each is documented below it and each explains its own out-of-sequence numbering.

**Scope 5 — one unlogged design point, and it's already self-flagged:**

The **test-naming convention** `RTAC.Simulation.<Area>.<Case>`. PHASES.md:779 says so in its own words — *"a convention established by practice across the three existing tests and **never logged as a decision**"* — and the Test gate section notes the related phase-tagging question *"wants its own decision entry."* Five tests now follow the convention. This is a settled-by-practice design point with no entry.

Everything else settled during implementation **is** logged: `NotAdjacent` logging at `Warning` is captured in Decision #12's closure addendum, correctly labelled an enactment choice rather than a ruling.

---

## 5. Doc drift — the four you queued, confirmed, plus three more

### Your four (all confirmed against live evidence)

**A. `69/69` → `74/74`** — CLAUDE.md:37, 49, 533; PHASES.md:47, 175, 290.
**B. `50/50` → `51/51`** — CLAUDE.md:36, 43, 65, 533; PHASES.md:47, 175, 243.
**C. three → four warnings** — CLAUDE.md:41; `RTACDeterminismTest.cpp:406`.
**D. "six outcomes" → "seven"** — PHASES.md:272 and :296. Note these need *different* edits:
- :296 (MultiEntity) — all seven are now genuinely **reached**.
- :272 (determinism) — seven **accounted for**: five reached, `InvalidOrigin` *and* `NotAdjacent` asserted absent.

`RTACMovementTest.cpp` is already fully correct at four/74. `combat_decisions.md`'s Decision #12 closure is already fully correct at 74/74, 51/51, seven.

### Three more I found

**E1 — PHASES.md:240 states an absence that no longer holds.** DoD item 4 claims elevation's inertness is *"confirmed September 1, 2026 by grep: zero readers of the field in any `.cpp` in the plugin."* That is now false — `RTACDeterminismTest.cpp` reads `Elevation` at lines 299–302 and mutates it at 803. **The substance is fine** (mechanical inertness holds; a test comparison helper is not a mechanical effect), but the stated evidence contradicts PHASES.md's *own* DoD item 5 forty lines later, which says the liveness control *"is the only thing that exercises `FRTACTile::Elevation`"*. Both cannot be true as written. Suggested fix: rephrase the evidence as "zero readers in any non-test `.cpp`," which is what I verified and what the DoD actually means.

**E2 — `combat_decisions.md:1402` footer:** *"Last Updated: August 31, 2026 — Decisions #1–#10 current"*. The file holds Decisions #1–#13 and was last written September 2 (`81659c4`).

**E3 — PHASES.md:779:** *"the three existing tests"* — there are five.

### One deliberate non-edit, and one Rule 4 question for you

**CLAUDE.md:327** also says `69/69 and 50/50`, but that sentence narrates the September 2 MCP two-instance incident and was accurate *at that moment*. I'd leave it as history rather than retroactively falsify the anecdote. Your call.

**PHASES.md:347 is inside a dated `Addendum, September 2, 2026` blockquote** that says `50/50`. That was true when written — Decision #12 hadn't been enacted yet. Rule 4 governs `combat_decisions.md` by its letter, but PHASES.md has adopted the same dated-addendum convention throughout Phase 1's determinism note. **I recommend not editing it**, and correcting via a new dated addendum instead. This is the one item where I'd rather have your ruling than pick a default, because it sets the precedent for whether PHASES.md addenda are append-only.

---

## 6. Phase 1 status string — the false clause

Current (PHASES.md:47 and :175, identical):

> `PARTIAL — … OUTSTANDING: Phase Exit Review not yet run, and the determinism work is uncommitted — the CLOSED status requires a commit hash that does not exist yet`

**"the determinism work is uncommitted" is false.** It landed in `9595330` (Sept 2, 23:10), and Decision #12's enactment followed in `37f68cb` (23:33) with its closure in `81659c4` (23:40). Working tree is clean.

---

## 7. Rule 5 / Rule 10 re-greps — run fresh, not inherited

Run against the whole tree (which subsumes the six files that landed since September 1: `RTACMovementLegality.cpp/.h`, `RTACDeterminismTest.cpp`, `RTACMovementTest.cpp`, `RTACTestFixtures.h`, `RTACMatchState.cpp/.h`, `RTACStreamSeedTest.cpp`).

**Rule 5 — reflection macros** (`UPROPERTY|UCLASS|USTRUCT|UENUM|UFUNCTION|GENERATED_BODY|UINTERFACE`): 10 hits, **every one inside a `*`-prefixed comment** explaining the macro's deliberate absence. Zero code hits. ✅

**Rule 5 — `UObject`/`AActor` tokens:** 10 hits, **every one inside a comment**, plus `"CoreUObject"` at `RTAC.Build.cs:13` — a module dependency name, not a type. Zero ownership. ✅

**Rule 10 — grid↔world conversion** (`FVector|FTransform|WorldLocation|ToWorld|TileSize|FRotator|GetActorLocation|FQuat|WorldToGrid|GridToWorld|Centimet|UnitsPer`): **grep exit 1 — zero hits of any kind.** ✅

Both absence-claims hold. I extended the Rule 10 token list beyond September 1's five terms rather than just repeating it.

**Bonus rules audited while I was in there:**
- **Rule 6** — only `static constexpr` compile-time constants and `static_cast`. No mutable statics, no globals, no function-local statics. ✅
- **Rule 9** — `LogRTAC` only, never `LogTemp`. Five `UE_LOG` sites in production code, all `Warning` on genuine failures. `FindTile` deliberately doesn't log out-of-bounds probes. No per-frame spam. ✅
- **Rule 11** — the only `RTAC` token anywhere in `Source/ProjectAtlantis/` is the `"RTAC"` dependency line in `ProjectAtlantis.Build.cs:22`. Zero combat logic in the main module. ✅

---

## Bottom line: is Phase 1 ready for `CLOSED`?

**Yes — the phase itself is done.** All seven DoD items are genuinely satisfied with test artifacts behind them, all 8 Failure Modes are checked with one finding (FM7, docs-only), there are no critical bugs, and the Safety Ruleset holds under live re-read with all three absence-claims re-verified.

**But it cannot be marked `CLOSED` in this turn's state**, for a reason that's procedural rather than substantive: the review just surfaced nine documentation corrections, and the `CLOSED — enacted in <commits>` vocabulary needs a commit hash that will only exist once they land. Closing on `81659c4` would stamp a hash onto a state whose own status line still says the work is uncommitted.

The sequence I'd propose: **land the corrections, then close on that commit.** When it lands, the citation should be

> `CLOSED — enacted in 6342e68, c436334, f1363b4, 9595330, 37f68cb, <this commit>`

— the five commits that built Phase 1's simulation and tests, plus the exit-review commit itself.

**Nothing blocks it beyond that.** No code change is required to close Phase 1. Every finding above is documentation, and E1 is the only one where a doc claim is actually *false* about the code rather than merely stale on a number.

---

Ready to write when you give the word. Two things I'd like your ruling on first, since they change what I produce:

1. **PHASES.md:347** — new addendum (my recommendation) or in-place edit?
2. **The test-naming convention** — want me to draft it as Decision #14, or leave it noted and unlogged for now?
