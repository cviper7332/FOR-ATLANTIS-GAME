# Combat System — Design Decisions

**Engine target:** Unreal Engine 5.8
**Scope of this doc:** Real-time grid-based combat system. Fully decoupled from PHIS (the narrative/world-state belief engine) — no shared dependency between the two systems.

> Format convention for this doc follows the project's established `DESIGN_DECISIONS.md`
> pattern: numbered decisions, each carrying `Date`, `Phase`, `Author`, `Status` in that
> order directly under its heading, `Status` set at creation (not backfilled), append-don't-
> rewrite once an entry exists (corrections are added as addenda, not silent edits).

**Status values (controlled vocabulary):**

- `CLOSED — fixed in <commit>` — a finding remediated.
- `CLOSED — enacted in <commit>` — a ruling implemented.
- `PARTIAL — <what is done>; OUTSTANDING: <what remains>` — partly actioned.
- `OPEN` — logged, not yet actioned.
- `N/A — design rationale, no action implied` — a choice that never implied a change.
- `CLOSED — superseded by Decision #N` — fully absorbed by a later decision; citing the
superseding decision number is mandatory.

---



## Design Philosophy

- **Combat mechanics** are based on Mega Man Battle Network 3 (MMBN3): real-time, grid-based, buster + chip-style attacks, tile modifiers.
- **Transition/structural philosophy** is based on Clair Obscur: Expedition 33 — enemies exist in the explorable world (no random encounters); engaging one transitions the player into a contained, contextual battle arena rather than an abstracted menu-only screen.
- **Build order is deliberate:** implement a faithful BN3-style combat core first, get it fully playable, and only then layer Atlantis-specific modifications (starting with elevation) on top. Do not design the core and the modifications simultaneously.

---



## Decision #1 — Combat Camera: Isometric 2.5D

**Date:** August 26, 2026
**Phase:** Combat system design (pre-implementation)
**Author:** Omar
**Status:** OPEN

**Decision:** Combat renders in isometric 2.5D. Grid logic remains flat/2D underneath; camera and art are angled isometric on top. Height is a visual and (per Decision #3) mechanical layer, not a change to the underlying 2D grid data model.

**Rejected alternatives:**

- Orthographic/flat side-view (true BN3 presentation) — simpler hit-testing and closest to source material, but doesn't sell verticality or the sunken/ruined-Atlantis environment.

**Why this matters:** Camera choice is a rendering decision, not a data-structure decision — the grid's internal representation doesn't change based on which camera is used. Isometric was chosen specifically because Atlantis's ruined, semi-flooded architecture benefits from a real depth cue that orthographic can't provide.

---



## Decision #2 — Combat System Fully Decoupled from PHIS

**Date:** August 26, 2026
**Phase:** Combat system design (pre-implementation)
**Author:** Omar
**Status:** N/A — design rationale, no action implied

**Decision:** Combat and PHIS (the narrative/world-state belief engine) share no dependency. Faction belief state does not drive combat behavior; combat outcomes do not feed PHIS.

**Why this matters:** PHIS governs story/world evolution around the player; combat is a separate gameplay system that happens to exist in the same game. Keeping them decoupled avoids scope creep in both directions. Revisit only if a specific design reason emerges — not a default assumption to build toward.

---



## Decision #3 — Elevation Will Be Mechanically Meaningful (direction deferred)

**Date:** August 26, 2026
**Phase:** Combat system design (pre-implementation)
**Author:** Omar
**Status:** OPEN

**Decision:** Tile elevation will affect gameplay meaningfully — a deliberate departure from MMBN3, which has no elevation dimension (only flat tile modifiers: ice, grass, lava, steel, poison, cracked, broken panels). Elevation is treated as a tile property independent of those BN3-style modifiers, not a replacement for them — a tile can carry both an elevation value and a surface modifier.

**Constraint:** Elevation must not be a simple "high = strictly better, low = strictly worse" axis. Both must be situationally strong depending on context (what's attacking, what the player is trying to do that turn) — not universally ranked, or the system collapses into "always fight for high ground."

**Explicitly deferred:** Mechanical direction is not locked (see Open Questions). Implementation is deferred until after the base BN3 combat loop is built and playable — the core and this modification are not to be designed simultaneously.

**Rejected alternatives:** None formally rejected yet — see Open Questions for brainstormed directions (offense/defense split, ability-gating, environmental interaction, faction-specific relationships, instability) still under consideration.

---



## Decision #4 — Combat Structure: Expedition-33-Style World Engagement

**Date:** August 26, 2026
**Phase:** Combat system design (pre-implementation)
**Author:** Omar
**Status:** N/A — design rationale, no action implied

**Decision:** Combat structure follows Clair Obscur: Expedition 33's transition philosophy. Enemies exist in the explorable world — no random encounters. Engaging an enemy transitions the player into a contained, contextual battle arena rather than an abstracted menu-only screen.

**Why this matters:** This governs how the player *enters* combat, distinct from Decision #1 (which governs the camera *within* combat) and distinct from the combat mechanics themselves (Design Philosophy, MMBN3-based). The specific implementation mechanism (GameMode swap, mode flag, or otherwise) is not decided by this entry — see Open Questions → "Exploration → Combat Transition Mechanism."

**Note:** This decision was implicit in the Design Philosophy section from the start of this document but was never given its own numbered entry. Logged retroactively on August 26, 2026 to bring it in line with this doc's convention that a settled design point should be a discrete, citable Decision — not left as unnumbered prose.

---



## Decision #5 — Grid Axis-Order Convention: Rows×Columns

**Date:** August 26, 2026
**Phase:** Combat system design (pre-implementation)
**Author:** Omar
**Status:** N/A — design rationale, no action implied

**Decision:** Grid dimensions are stated and coded as rows×columns (matrix/array convention — an m×n grid has m rows, n columns), matching standard 2D array indexing (`grid[row][col]`). BN3's actual battle grid is 3 rows tall by 6 columns wide (3 columns per side) — verified against multiple sources (Wikibooks combat guide, community strategy analysis, in-game mechanics descriptions of row-based targeting) rather than assumed from memory. "3x6" as already used throughout this doc is accurate under this convention and requires no correction.

**Why this matters:** "3x6" and "6x3" both correctly describe the same physical board depending on which number-order convention is used — this is not a contradiction between sources, just an unstated convention that could silently drift into inconsistent code (tile addressing, array bounds, loop order) if never pinned down explicitly. Screen/display resolution conventions (width×height) go the opposite direction from matrix convention and are a common source of this exact class of bug — worth naming so nobody imports that convention here by habit.

**Note:** This does not itself decide RTAC (realtime arena action strategic combat system)'s final grid dimensions (still an Open Question — see "Core BN3 Loop") — only the axis-order convention to express whatever dimensions are eventually chosen.

---



## Decision #6 — Rule 5's "No Engine Types" Enforcement: Review-Only, Not Machine-Enforced

**Date:** August 26, 2026
**Phase:** Combat system design (pre-implementation)
**Author:** Omar
**Status:** N/A — design rationale, no action implied

**Decision:** Rule 5's requirement that simulation state hold no `AActor*`/`UObject*` ownership is enforced by code review and the Phase Exit Review checklist, not by a machine-enforced mechanism. Neither of the two mechanical options considered — a second, narrow-dependency UBT module (`RTACSim`) or a grep/lint gate over the simulation subtree — will be built.

**Why this matters:** Both candidate mechanisms were evaluated as mechanical proxies for the same goal: engine-independence enforcement, modeled on PRS's CI-gated standalone build (PRSCore compiles with zero Unreal headers, checked by CI). That goal was dropped from Rule 5 in two stages — first the "testable without a running engine" half (Rule 5's August 26, 2026 addendum), then the blanket engine-type ban itself, narrowed to actor-lifecycle ownership specifically (Rule 5's second addendum, same date). Once the goal both mechanisms existed to serve is gone, building either is machinery solving a problem this project no longer has — not a case of the options being bad in isolation.

**Note:** This does not relax the underlying rule — no `UObject*`/`AActor*` ownership in simulation state is still mandatory, per Rule 5. It changes only how compliance is verified: reviewed at each Phase Exit, not compiled or linted automatically.

**Addendum, August 28, 2026 (date correction):** This entry's `**Date:**` field above reads "August 26, 2026." That date is incorrect. Decision #6 was actually written and committed **August 27, 2026**, per commit `10b88d7` (2026-08-27 20:04). The original `**Date:**` field is left unchanged per Rule 4 (append, don't rewrite); this addendum records the correction.

This also resolves the "Why this matters" paragraph's internal reference to *"Rule 5's second addendum, same date"* — meaning the same date as this Decision. As originally written (this entry's date: August 26), that phrase pointed to the wrong day. Rule 5's Addendum #2 was written in the same commit as this Decision (`10b88d7`), so once both are corrected, "same date" resolves correctly: both were genuinely written August 27, 2026.

---



## Decision #7 — Development Engine Floor: UE 5.8, Hard Requirement

**Date:** August 26, 2026
**Phase:** Combat system design (pre-implementation)
**Author:** Omar
**Status:** N/A — design rationale, no action implied

**Decision:** Development on this project below UE 5.8 is not supported. This is a floor on the *development environment*, distinct from and not to be conflated with RTAC's *consumer* portability floor (still undetermined — see `PHASES.md` Phase 8's portability test).

**Why this matters:** The binding reason is not API stability — it is that this project's development workflow depends on live MCP (Model Context Protocol) connectivity into a running editor session (CC/CC-Opus reading the output log, inspecting editor state, and helping build/verify automation tests), and MCP is provided by UE 5.8's built-in Experimental `ModelContextProtocol` plugin. Verified directly against the local engine installs on this machine: the plugin is present under UE 5.8's `Engine/Plugins/Experimental/`. UE 5.7's install here is complete (includes `Source/`, not just binaries) and shows a clean absence of the plugin anywhere in its `Plugins` tree — the stronger data point. UE 5.6's install here is partial (only `Binaries/Intermediate/Plugins/Programs` present, no `Source/`), so its absence is weaker, corroborating evidence rather than equally strong proof. Below 5.8, this connectivity does not degrade — it does not exist, and CC loses the ability to verify anything live in the editor at all. Phase 0's Part B Definition of Done items (`PHASES.md`) already depend on this capability implicitly; this decision makes the dependency explicit.

**Secondary, corroborating note:** this floor is also consistent with, though not driven by, this project's own documented API churn — `BL_BeforeTonemapping` was removed in UE 5.5+ (`CLAUDE.md` → UE5.8 API Gotchas), so 5.8 sits safely above that break. This is not the reason for the floor and should not be cited as if it were.

**Note:** This ruling says nothing about what engine version a consumer project (one RTAC is dropped into, per its stated portability goal) requires — that is a property of RTAC's own code, not of this project's development workflow, and remains open until tested (Phase 8).

**Addendum, August 28, 2026 (date correction):** This entry's `**Date:**` field above reads "August 26, 2026." That date is incorrect. Decision #7 was actually written and committed **August 27, 2026**, per commit `10b88d7` (2026-08-27 20:04), the same commit as Decision #6 and Rule 5 Addendum #2. The original `**Date:**` field is left unchanged per Rule 4; this addendum records the correction. No internal reference within this entry restates another entry's date, so no further correction is needed here.

---



## Decision #8 — Grid Dimensions: 3 Rows × 6 Columns (Configurable, Default)

**Date:** August 26, 2026
**Phase:** Combat system design (pre-implementation)
**Author:** Omar
**Status:** PARTIAL — simulation-side dimensions enacted in `9286975` (`FRTACGrid` takes rows/columns as `Init()` parameters, `DefaultRows`/`DefaultColumns` = 3/6); OUTSTANDING: the presentation-layer wrapper exposing editable `Rows`/`Columns` in the Details panel (Phase 2)

**Decision:** The combat grid defaults to **3 rows × 6 columns**, matching BN3's actual board exactly (per Decision #5's rows×columns convention). Dimensions are **not** a compile-time constant — rows and columns are configurable, exposed to the level author for editing in the UE5 Editor's Details panel, with 3×6 as the shipped default value.

**Architecture — this does not create tension with Rule 5.** The editable-in-editor requirement is satisfied entirely on the presentation side, not by relaxing the simulation boundary:

- The simulation-layer grid type takes rows and columns as **plain integer parameters** (constructor or init call) — no engine dependency, no `UPROPERTY`, nothing Rule 5 would object to. The struct itself has no idea an editor exists.
- A thin presentation-layer wrapper — an actor or component, exactly where `UPROPERTY(EditAnywhere)` and engine types belong per Rule 5's own separation — exposes `Rows`/`Columns` as editable integer properties defaulting to 3/6, and passes them into the simulation at init time.
- This is stated explicitly so it reads as the separation being **used as designed**, not as an exception carved out for it. Rule 5 already draws exactly this boundary (simulation owns state and rules; presentation owns everything engine-facing, including where editor-exposed properties live) — configurable dimensions are simply the first concrete case that exercises it.

**Why this matters:** "Exact grid dimensions" has sat as an open question since the Design Philosophy section was written, explicitly deferred until the core loop needed a real number to build against (Open Questions → Core BN3 Loop). Locking the *default* to BN3's own board removes the guesswork the open question raised, while making it editor-configurable — rather than a hardcoded constant — keeps the door open for the "does 3×6 give elevation enough room to read clearly, or does the grid need to grow slightly (4×6/5×6)?" question already logged under Open Questions → Elevation — Mechanical Direction, without requiring a second decision or a code change to test that later.

**Note:** This satisfies Phase 1's Definition of Done item ("Grid dimensions chosen and logged as their own Decision entry, stated as rows×columns per Decision #5") in `PHASES.md` — this entry is that "own Decision entry," and its dimensions are expressed as rows×columns per Decision #5's convention as required.

**Addendum, August 28, 2026 (date correction):** This entry's `**Date:**` field above reads "August 26, 2026." That date is incorrect. Decision #8 was actually written and committed **August 28, 2026**, per commit `ad1fe4b` (2026-08-28 10:07). The original `**Date:**` field is left unchanged per Rule 4; this addendum records the correction. No internal reference within this entry restates another entry's date.

**Addendum, September 1, 2026 (status transition — OPEN → PARTIAL):** This entry's `**Status:**` field above was changed from `OPEN` on this date. The simulation half of this decision is enacted: `FRTACGrid` takes rows and columns as plain integer `Init()` parameters and carries `DefaultRows = 3` / `DefaultColumns = 6` as `static constexpr` defaults, landed in commit `9286975` (August 28, 2026). The presentation half is not: the thin actor-or-component wrapper exposing `Rows`/`Columns` as `UPROPERTY(EditAnywhere)` integers in the Details panel does not exist, and cannot until a presentation layer does (Phase 2). Until it lands, this entry's headline claim — dimensions "exposed to the level author for editing in the UE5 Editor's Details panel" — is undelivered, which is why this reads `PARTIAL` and not `CLOSED — enacted`.

**On amending `Status` in place at all** (applies equally to Decisions #9 and #10, transitioned the same date): this doc's format convention says `Status` is "set at creation (not backfilled)." That is read here as barring the retroactive invention of a status for an entry that never carried one — not as freezing the field for an entry's whole life. The controlled vocabulary's own `CLOSED — enacted in <commit>` and `PARTIAL` values cannot be written at creation time, since the commits they cite do not exist yet; a status field that could never change would make two of the six permitted values unreachable. Each transition is recorded in an addendum like this one rather than made silently, so the append-only record still shows what changed, when, and on what evidence. No original entry text is altered by any of the three.

---

## Decision #9 — Entity Identity: Plain Instance Counter, Separate from Archetype Key

**Date:** August 29, 2026
**Phase:** RTAC Phase 1 (Grid & Movement — Headless Simulation)
**Author:** Omar
**Status:** CLOSED — enacted in `43166da`, `6321472`, `1264631`

**Decision:** An entity occupying a grid tile is identified by two separate fields, not one:

- `EntityId` — instance identity only. A plain, monotonically incrementing integer assigned in
  spawn order (0, 1, 2, ...), sourced from explicit seeded state per Rule 6. Carries no meaning
  beyond "which occupant is this" — it is not descriptive, not derived from a name string, and
  not derived from anything environmental (spawn timestamp, memory address).
- `ArchetypeId` — a separate lookup key (e.g. `FName`) identifying *what kind* of entity this is
  (e.g. `"Sentinel"`), for a future stats/behavior lookup. Reserved as a field now; the lookup
  table itself is out of scope for Phase 1 and is not being built by this decision.

These two are not the same field and must not be merged into one (e.g. a descriptive name string
used as the instance ID). `Position` (`FRTACGridPosition`, already existing) completes the
minimal entity: `EntityId` + `Position` + `ArchetypeId`.

**Rejected alternatives:**

- A single descriptive-name identity (e.g. `"Sentinel_03"`) serving as both instance ID and
  implied archetype. Rejected because it conflates two domains in one field (Rule 10) and
  threatens Rule 6 determinism if any part of the name is ever derived from something
  non-reproducible (formatted counters, disambiguation suffixes, spawn timestamps). A plain
  integer counter has no such dependency.
- A GUID or hash-based instance ID. Rejected for the same Rule 6 reason: not trivially
  reproducible run-to-run, which would make two runs' entity-ID sequences unusable as a
  determinism check even though gameplay itself would be unaffected.

**Why this matters:** Phase 1's Definition of Done (`PHASES.md`) requires movement to resolve
through the simulation layer and requires tests to run against the full configured grid "never
a 1×1 or single-entity degenerate case" (Failure Mode 5) — both presuppose some entity concept
occupying `FRTACTile.OccupantEntityId`, though neither DoD line names entity identity explicitly;
this decision fills that gap rather than leaving it to be improvised inside movement-logic code,
which is the exact Rule 8 risk pattern ("logic that never anticipated it") Phase 1's own Goal
section already flags for elevation, one layer earlier in the same phase. `ArchetypeId` is
reserved now, at the cost of one unused field, specifically so a future stats/AI-pattern lookup
(Phase 3 HP/damage, Phase 4 AI) has a seam to plug into without a later struct rework — this
mirrors the same reserved-but-inert pattern Phase 1's DoD already uses for the elevation slot.

**Explicitly deferred:** The archetype lookup table itself (stat definitions, AI behavior
mapping) is not built by this decision and is not Phase 1 scope — `ArchetypeId` is reserved
unpopulated-in-practice (a placeholder value in Phase 1's own tests is sufficient) until Phase 3
(HP/damage model) and Phase 4 (enemy AI) exist to consume it.

**Addendum, August 30, 2026 (clarification):** The phrase "sourced from explicit seeded state
per Rule 6" in this entry's `EntityId` bullet above is ambiguous and should be read narrowly.
Rule 6 contains two independent clauses: (1) all per-match state, random or not, lives in an
explicit struct with no hidden globals/statics; (2) RNG state specifically is seeded explicitly.
`EntityId` engages clause (1) only — it is a plain incrementing counter, not a draw from any
random stream, and no RNG stream seeds it. "Sourced from explicit seeded state" was intended to
mean "lives in the explicit per-match state struct," not "derived from a seeded RNG." The
original text is left unchanged per Rule 4; this addendum records the clarification so a future
reader does not build an unneeded RNG stream behind the counter. Surfaced during CC/Opus's
seeded-state-container design task on August 29, 2026.

**Addendum, August 31, 2026 (scope gap closed):** Decision #9's original text fixed
FRTACEntity's shape as EntityId + Position + ArchetypeId and stated the entity carries "no
facing, no speed, no cooldown, no movement state." That exclusion was aimed at turn-to-turn
movement/animation state, not at identity — and it left a real gap: Decision #10 Ruling 4's
ownership clause requires knowing "the entity's side" (Player/Enemy), and nothing in Decision
#9 provides that value. `ArchetypeId` cannot stand in for it — side and archetype are
independent axes (a given archetype is not inherently one side or the other).

This addendum adds a fourth field: `Side` (same enum family as `FRTACTile::Owner`,
`ERTACTileOwner`, or a compatible type — implementer's call on exact shape). `Side` is set once
at spawn and does not change during a match, matching the "identity, not state" character of
`EntityId` and `ArchetypeId` rather than the movement-state category Decision #9 excluded. The
minimal entity is now `EntityId` + `Position` + `ArchetypeId` + `Side`.

Surfaced during Decision #10 Ruling 4 implementation, when the movement-legality check's
ownership clause had no entity-side value to check against.

**Addendum, September 1, 2026 (status transition — OPEN → CLOSED):** This entry's `**Status:**`
field above was changed from `OPEN` on this date. Every field this decision fixes now exists in
`FRTACEntity` as specified — `EntityId` (`int32`, defaulting to `INDEX_NONE` so "never assigned"
cannot alias the first real id), `Position`, `ArchetypeId` (`FName`), and `Side` per this entry's
August 31, 2026 addendum — and the spawn-order counter lives in the explicit per-match state
struct as `FRTACMatchState::NextEntityId`, where Rule 6's no-hidden-state clause requires it,
with its `Initialize()`/`Reset()` behaviour covered by `RTAC.Simulation.Rng.MatchStateLifecycle`.

**What `CLOSED` does not claim here.** The archetype lookup table remains deferred by this
entry's own "Explicitly deferred" section: `ArchetypeId` is a reserved, unread field, and closing
this decision starts none of Phase 3's or Phase 4's work on consuming it. Separately, no
production code yet draws an id from `NextEntityId` — the counter is exercised only by its own
test, because no entity-spawn path exists to consume it. That is downstream implementation this
decision governs but does not itself require, so it is stated here rather than carried as an
`OUTSTANDING` clause; every ruling the entry actually makes is enacted. See Decision #8's
addendum of the same date on amending `Status` in place.

---

## Decision #10 — Movement Rules: Discrete Step, Mutable Ownership, Reusable Legality Check

**Date:** August 30, 2026
**Phase:** RTAC Phase 1 (Grid & Movement — Headless Simulation)
**Author:** Omar
**Status:** CLOSED — enacted in `c51027e`, `5231eac`, `1264631`, `1c27877`, `c436334`

**Decision:** Movement resolves as one discrete tile-step per input event, validated against a
reusable legality check with four separable clauses, against a grid whose per-tile ownership is
externally assigned rather than computed. Six sub-rulings, detailed below.

**1. Movement is discrete, one tile per input — not continuous, not select-and-confirm.**

One button press moves the entity exactly one tile in the pressed direction. Holding a direction
does not produce continuous glide-style motion; repeated presses produce repeated discrete steps.
This is closer to tile-by-tile stepping than to BN3's real-time continuous movement feel, but it
is *responsive* tile-stepping — the player's own input rate drives repetition, not a
game-interpreted "held = moving" state. Confirmed against actual BN3 behavior (Omar), not
assumed from genre convention.

**Why this matters for Phase 1 specifically:** this makes "one input" trivially well-defined for
Rule 6's determinism test. An input sequence is a list of discrete move-events (up/down/left/
right), applied one at a time — no continuous position, no sub-tile interpolation, no tick-rate
decision needed at the simulation layer to define what a single unit of input means.

**2. Movement speed modifiers are presentation-layer only — not a Phase 1 (or any simulation-layer)
concern.**

BN3 has movement-speed-affecting items (implied by Omar's "idk, I think it's just the anim played
faster"). Given Ruling 1 — movement is always exactly one discrete tile-step at the simulation
level — "faster movement" cannot be a simulation-layer concept: the simulation has no notion of
speed, only "did the step resolve." Any such modifier can only mean "how long the presentation
layer takes to animate an already-resolved step." This follows from Ruling 1 by construction, not
as a separate design choice requiring its own justification. No `FRTACEntity` field, no
simulation-layer hook, is reserved for this — there is nothing for such a hook to attach to.

**3. Tile ownership is a per-tile field on `FRTACTile`, externally assigned per battle — not
computed from a symmetric grid-split assumption.**

Every mainline MMBN battle *defaults* to a symmetric 3×3/3×3 player/enemy split, but this is not
universal: Liberation Mission-style battles (MMBN5, per Omar) can start with asymmetric
configurations — the player surrounded (3×2 enemy / 3×2 player / 3×2 enemy), or one side starting
with more field than the other (3×4/3×2 or 3×2/3×4) — set by "authored rules," not derived from
grid dimensions at runtime.

**Consequence:** ownership cannot be computed once from `Rows`/`Columns` and a hardcoded
left-half/right-half split. It must be data — assigned externally per tile, at battle setup, by
whoever configures the battle (eventually a level-author-facing concept, in the same spirit
Decision #8 already exposes `Rows`/`Columns` for editor configuration; the authoring *interface*
itself is out of scope for this decision and for Phase 1).

**Type shape (illustrative, not binding on the implementer):** an enum on `FRTACTile` — e.g.
`ETileOwner { Player, Enemy, Neutral }` — is the expected shape, but exact naming is an
implementation detail, not a design ruling this decision is fixing.

**4. Movement legality is a reusable, separably-clause-based check — not movement-exclusive
logic, and not one opaque boolean.**

A move (or any request to validate "can entity E legally occupy tile T") is legal only if **all**
of the following hold, checked as **separately named clauses**, not folded into one undifferentiated
boolean:

- **In bounds** — `IsValidPosition()` already exists (Phase 0); reused, not reimplemented.
- **Unoccupied** — `FRTACTile.OccupantEntityId` is `INDEX_NONE`.
- **Owned by the mover's side** — the destination tile's ownership (Ruling 3) matches the
  entity's side. Normal movement can never cross the ownership boundary; this is the mechanism
  that enforces "player half stays player half" as an actual rule, not merely convention.
- **Not broken** — a broken tile (`ERTACSurfaceModifier`, already reserved in Phase 0) blocks
  entry for an entity without a qualifying modifier (see Ruling 5).

**Why separable clauses, specifically, rather than one combined boolean:** two independent future
consumers need to interact with individual clauses without touching the others, and neither
consumer exists yet, which is exactly why the shape must be decided now rather than discovered
under pressure later (the same Rule 8 logic Decision #9 already applied to `ArchetypeId`):

- A future per-entity modifier (Ruling 5 — hover, Air Shoes) needs to override the *not-broken*
  clause specifically, for that entity only, without altering in-bounds/unoccupied/ownership
  checking for anyone else.
- A future attack/chip system (Ruling 6 — Step-Sword-style reach attacks) needs to ask the
  identical "can entity E legally occupy tile T" question from *outside* movement entirely,
  without pretending to be a movement action to get an answer.

Building the check as separable, independently-named clauses now costs nothing extra in Phase 1
(nothing consumes the separability yet) and avoids movement's core legality logic being rewritten
from a monolithic boolean into separable pieces under time pressure once Phase 3 or later actually
needs one of the above.

**5. A tile's `SurfaceModifier` state can be legally bypassed by a per-entity property — the tile's
rule does not change; what the check is evaluated against, per-entity, does.**

Some entities can legally occupy tiles that would otherwise be illegal for movement: BN3's Air
Shoes (a program and/or a chip, per Omar) lets the player step onto and occupy broken tiles; some
enemies "hover" and can move across broken tiles similarly. Omar further notes hovering entities
may also be exempt from *other* tile-modifier effects while occupying such a tile (e.g. poison
tiles not depleting HP for a hovering occupant) — flagged here as a related but **distinct**
mechanic (an in-combat *effect-application* rule, not a *movement-legality* rule) and explicitly
**not** resolved by this decision; it belongs with tile-modifier resolution (Phase 3) rather than
movement.

**The shape this implies:** the broken-tile clause in Ruling 4's legality check is not a fixed
rule uniformly applied to every entity — it is evaluated per-entity, and a future entity-side
property can change its outcome for that entity specifically. Nothing about Ruling 4's four
clauses changes structurally to support this later; the clause being independently named
(Ruling 4) is what makes this override attachable without restructuring the check.

**Explicitly deferred:** no entity-modifier system (Air Shoes, hover, or any other property that
would alter clause evaluation) is built by this decision or is Phase 1 scope. This ruling commits
only to the legality check's *shape* being override-compatible, not to building an override
mechanism now.

**6. Step-Sword-style reach attacks are not movement — they are attacks with a positional
side-effect, entirely out of scope for movement validation.**

BN3 has attacks (player chips like Step-Sword/Step-Cross; more complex enemy-side equivalents,
per Omar) where the entity's position visibly shifts — including into enemy territory, bypassing
Ruling 4's ownership clause entirely — resolves an attack, then the entity returns to its prior
position. Initially considered as a possible exception embedded inside movement's ownership check;
rejected on inspection.

**Why this is not a movement-legality exception:** the entity never persistently occupies the
reach destination. There is no settling-in, no lasting position change — confirmed explicitly by
Omar ("once the attack resolves the entity is moved right back to the position... not from
regular movement"). Folding a temporary, attack-scoped position change into movement's own
ownership clause would require movement's core legality logic to special-case one specific attack
by name, which is precisely the Rule 8 failure pattern this project has flagged repeatedly
("logic that never anticipated it"). Movement's ownership clause (Ruling 4) is correct and
untouched for genuine movement; Step-Sword simply never invokes it.

**Where Step-Sword's own legality lives instead:** if a reach-attack's destination is a broken
tile, whether the attack is even permitted is governed by the *mover's own properties*
(Ruling 5's per-entity override mechanism) — the same underlying "can entity E legally occupy
tile T" question, reused from outside movement by the attack's own resolution logic, not
movement's ownership clause being bypassed. Confirmed explicitly by Omar: an entity without a
qualifying modifier (e.g. Air Shoes) cannot complete a Step-Sword-style attack whose destination
is broken — the attack simply fails that precondition, the same way it would if attempted by an
entity lacking the modifier during ordinary movement.

**Explicitly deferred:** no attack, chip, or reach-attack system is built by this decision — this
is out of scope until Phase 3 (attacks) and Phase 5 (chip-equivalent resources) exist. This
ruling exists so movement's legality check (Ruling 4) is not later contorted to accommodate a
mechanic it was never meant to handle, and so a future implementer building Step-Sword-style
attacks knows this interaction was considered and deliberately routed elsewhere, not missed.

**Rejected alternatives:**

- Continuous/held-direction movement (true BN3 real-time glide feel). Rejected for Phase 1 in
  favor of Ruling 1's discrete-step model — confirmed against actual BN3 mechanics (one tile per
  press), not a simplification chosen for implementation convenience, though it also happens to
  sidestep a harder Rule 6/Rule 7 tick-rate question a continuous model would require.
- A single monolithic "is this move legal" boolean, rather than Ruling 4's separable clauses.
  Rejected because two independent, not-yet-built future systems (Ruling 5's per-entity
  overrides, Ruling 6's attack-side reuse) each need to interact with one clause without
  disturbing the others — discovered as a real requirement during this decision's own drafting,
  not speculative.
- Territory computed from a hardcoded symmetric grid-split, rather than Ruling 3's externally-
  assigned per-tile ownership. Rejected because Liberation Mission-style asymmetric authored
  starts are real, confirmed BN3 mechanics, not a hypothetical this project chose to accommodate
  preemptively.
- Folding Step-Sword-style reach-attacks into movement's ownership clause as a named exception.
  Rejected per Ruling 6 — Rule 8's "logic that never anticipated it" failure pattern.

**Why this matters:** this is the largest remaining open item blocking Phase 1's actual
implementation. Nothing about movement — the phase's single largest outstanding Definition of
Done item — could be built correctly without these six rulings settled first: an entity struct
existing (Decision #9) is necessary but not sufficient, since movement also needs to know what
one input means (Ruling 1), what a legal destination is (Ruling 4), and what "territory" even
means as data (Ruling 3), before any movement code can be written without silently baking in an
unstated assumption. Rulings 5 and 6 are included in full despite committing to no new Phase 1
code, specifically because both surfaced directly from reasoning through Ruling 4's shape — Rule
4 (Append, Don't Rewrite)'s own discipline argues for capturing a design conversation's full
reasoning at the point it happens, not compressing it down to only the rulings with immediate
code consequence and losing the "why" a future reader would need to avoid re-deriving the same
edge cases from scratch.

**Explicitly deferred, project-wide summary (see individual rulings for detail):** movement-speed
presentation (Ruling 2), the tile-ownership authoring interface (Ruling 3), any entity-modifier
system — Air Shoes, hover (Ruling 5), any attack/chip/reach-attack system — Step-Sword and its
enemy-side equivalents (Ruling 6), and the Area Grab-style territory-swap mechanism itself. On
that last item specifically: the *rule* is decided now — an occupied tile is immune to an
ownership-swap effect and remains with whoever currently occupies it, resolved per-tile rather
than as one atomic region-swap (confirmed against actual BN3 Area Grab-family chip behavior,
Omar) — but the swap mechanism that would apply this rule is not built by this decision and
remains Phase 5 scope, per the same "chip-equivalent resource system... open until the core loop
exists to build against" deferral already governing chip-adjacent design in this document's Open
Questions.

**Addendum, August 31, 2026 (Broken value scoped exception):** Ruling 4's fourth clause ("not
broken") requires comparing against a broken-tile value in `ERTACSurfaceModifier`, which per
its own header is deliberately unpopulated beyond `None` until Phase 3. This addendum
authorizes one narrow exception: `Broken` is added to `ERTACSurfaceModifier` now, so Ruling 4's
clause has something real to check. Nothing else in the eventual full modifier list (ice, grass,
lava, steel, poison, cracked) is added by this addendum, and Phase 3's scope over the rest of
that list is unchanged — `Broken` is added specifically because movement-legality checking is
Phase 1's own stated concern, not because Phase 3's deferral is being relaxed generally.

**Addendum, September 1, 2026 (status transition — OPEN → CLOSED):** This entry's `**Status:**`
field above was changed from `OPEN` on this date. Every ruling that commits to code has landed,
and the three that commit to none are satisfied by construction:

- **Ruling 1** (discrete one-tile step per input) — `RTACResolveMove` applies exactly one discrete
  step. No continuous position, no sub-tile interpolation, no tick-rate concept exists anywhere in
  the simulation layer. The input layer that generates the steps is Phase 2 presentation work, not
  this ruling's.
- **Ruling 2** (movement-speed modifiers are presentation-only) — commits explicitly to no
  simulation-layer field and no hook. Confirmed: neither exists.
- **Ruling 3** (per-tile ownership, externally assigned) — `FRTACTile::Owner` and `ERTACTileOwner`
  in `5231eac`, with the entity-side counterpart `FRTACEntity::Side` in `1264631`. Ownership is a
  stored per-tile field, never computed from `Rows`/`Columns`. The authoring interface is out of
  scope by this ruling's own text and is not implied by this closure.
- **Ruling 4** (separable four-clause legality check) — `RTACCheckMoveLegality` in `c51027e`, the
  `Broken` value its fourth clause compares against in `1c27877` (per this entry's August 31, 2026
  addendum), and `RTACResolveMove` in `c436334`. The four clauses are separately named values on
  `ERTACMoveLegality`, not a collapsed boolean, as the ruling requires.
- **Rulings 5 and 6** (per-entity overrides; Step-Sword-style reach attacks) — both commit
  explicitly to no Phase 1 code, and none was written. Ruling 5's actual requirement is
  structural — that the broken-tile clause be independently named, so an override can attach later
  without restructuring the check — and it is met: the clause is its own `ERTACMoveLegality::Broken`
  value.

**What `CLOSED` does not claim here.** This means the entry's rulings are enacted, not that the
mechanics it defers are built. The Area Grab-style territory-swap mechanism, any entity-modifier
system (Air Shoes, hover), and any attack/chip/reach-attack system all remain deferred exactly as
this entry's project-wide deferral summary states. See Decision #8's addendum of the same date on
amending `Status` in place.

---

## Decision #11 — Match-State Container and Entity Spawn: Array Storage, Id-Not-Index Identity, One Invariant Owner

**Date:** September 1, 2026
**Phase:** RTAC Phase 1 (Grid & Movement — Headless Simulation)
**Author:** Omar
**Status:** OPEN

**Decision:** `FRTACGrid` and entity storage move inside `FRTACMatchState`, stored as a flat
`TArray<FRTACEntity>`; array index is explicitly **not** entity identity, and lookup by
`EntityId` is provided so no caller has cause to assume otherwise; and a single spawn function
becomes the one place an entity is placed on the board, establishing the grid/entity consistency
invariant that `RTACResolveMove` currently assumes without confirming. Four sub-rulings, plus one
boundary this decision deliberately does not cross.

**1. `FRTACGrid` and entity storage live inside `FRTACMatchState`, wired in as their own change
before either Phase 1 test is written.**

Rule 6 requires this independently of any test: "All per-match temporal state lives in an explicit
state struct, passed in and out each tick by the caller." Entity positions and tile occupancy are
per-match temporal state — they change during a match — so holding them as loose locals in a
caller is the hidden-state shape Rule 6 exists to prevent. `RTACMatchState.h`'s own scope note
already committed to this: "Grid and entity storage join this struct as Phase 1's movement work
lands; they are deliberately not wired in yet rather than being speculatively arranged now." That
condition is now met.

**Three options were considered, and the third is chosen:**

- **(A) Wire into `FRTACMatchState`.** Rule 6 mandates it; the header promised it; and Phase 1's
  determinism Definition of Done item says "identical resulting **state**," a sentence that is
  only honest if match state is one real object. Against: it is a container-shape decision made
  with a test as the proximate trigger.
- **(B) A test-local container.** Commits to no production API, and lets the shape be decided when
  a real tick function exists to constrain it. Rejected: Rule 6 puts this state in the struct
  regardless, so (B) defers a required change rather than avoiding one, the tests get rewritten
  when it lands, and `NextEntityId` keeps having no consumer for another phase. It also weakens
  the determinism oracle — a test comparing its own bookkeeping rather than what the simulation
  owns is measuring the wrong thing (Failure Mode 3).
- **(C) (A), done as its own commit with this decision entry, landing before the tests.** Chosen.
  Same outcome as (A), but the architectural decision gets reviewed on its own merits instead of
  arriving inside a test commit where it would read as test scaffolding.

**Cost, stated plainly:** `FRTACMatchState` stops being cheap to copy once it holds a tile array.
For Phase 1's determinism test this is a *feature* — snapshot-and-compare across two runs is
exactly the operation wanted, and a struct that copies its whole board is what makes it one line
instead of a manual deep copy. For a future per-tick copy it would not be a feature. Nothing in
Phase 1 copies match state per tick, and nothing should be built that does without revisiting this.

**2. Entity storage is `TArray<FRTACEntity>`, not `TMap<int32, FRTACEntity>`.**

This applies an existing project convention rather than inventing one. `RTACGrid.h`'s tile-storage
comment already made this exact argument for `TArray<FRTACTile>`, and reason 3 transfers verbatim:
"One unambiguous iteration order, which Rule 6's same-seed-same-result determinism requirement
depends on."

`TMap` iteration order is not a stable contract across runs, builds, or platforms. Resting a
determinism guarantee on it would be precisely the hidden dependency Rule 6 targets — and worse,
it would fail *intermittently and unreproducibly*, which is the failure mode Rule 6's "treat any
divergence as a bug in this rule, not a curiosity" clause is written against. The keyed lookup a
map would buy is provided by Ruling 3 instead, at a cost this project's entity counts make
irrelevant.

**3. Array index is not entity identity. Callers look entities up by `EntityId`, and the
index-equals-id coincidence is documented as a coincidence.**

With ids handed out `0, 1, 2, …` (Decision #9) and nothing ever removed in Phase 1, array index
happens to equal `EntityId` today. **This must not be encoded as an invariant, relied on in call
sites, or used for arithmetic.** It breaks the first time an entity is removed mid-match, and by
then it will have spread silently through every call site that found it convenient — each one
individually correct at the time it was written, collectively a rewrite.

**Resolution — both halves, deliberately, not either alone:**

- A lookup by `EntityId` is provided on `FRTACMatchState`, returning `nullptr` when no such entity
  exists. This matches `FRTACGrid::FindTile()`'s existing convention exactly — same
  nullptr-on-absence contract, same const and non-const overload pair — so the shape is already
  familiar in this tree rather than novel.
- The contract "index is not id" is stated in the header, at the storage field.

The documented contract alone would be insufficient: a caller told not to assume index-equals-id,
and given no correct alternative, writes the index arithmetic anyway. The accessor alone would be
insufficient: without the stated contract, the first caller who notices the coincidence bypasses
the accessor "for speed." The pair closes both paths.

**Why this is decided now rather than when removal lands:** the cost of deciding it now is one
accessor and one header sentence. The cost of deciding it later is auditing every call site
written in between. This is the same reasoning Decision #10 Ruling 4 applied to the legality
check's separable clauses — shape decided while it is free, rather than discovered under pressure
once something depends on the wrong shape.

**Performance note, not a deferral:** the lookup is a linear scan. At Phase 1's entity counts
(single digits on an 18-tile board) this is not measurable. If entity counts ever grow to where it
matters, the correct fix is an id-to-index side map maintained by spawn and despawn — never
callers reintroducing index arithmetic. That map is not built here.

**4. `RTACSpawnEntity(FRTACMatchState&, Position, Side, ArchetypeId)` becomes the one place an
entity is placed on the board.**

This enacts semantics Decision #9 already specified — `EntityId` "assigned from
`FRTACMatchState::NextEntityId` in spawn order (0, 1, 2, ...)" — rather than deciding anything new
about identity. What is new is giving that assignment a home, and with it, an owner for an
invariant that currently has none.

**The gap this closes, found during test design and not asked for.** `RTACResolveMove`'s step 1
clears the origin tile unconditionally:

> `OriginTile->OccupantEntityId = INDEX_NONE;`

It never confirms the origin tile actually held *the entity being moved*. In intended use this is
correct, and the function is not being called buggy: the invariant
`Grid.FindTile(E.Position)->OccupantEntityId == E.EntityId` does hold for every well-formed entity.
But **nothing establishes that invariant** — no code path sets `Entity.Position` and the tile's
`OccupantEntityId` together — and **nothing enforces it**. A setup that gets it wrong silently
clears a *different* entity's occupancy and corrupts the board with no error, no log line, and no
failed check. Both Phase 1 tests sit directly on top of this invariant.

Folding spawn into this commit rather than leaving it to the tests also avoids a concrete Failure
Mode 7 drift: without it, the determinism test and the multi-entity test each hand-increment
`NextEntityId` and each hand-write the two-field placement, duplicating spawn semantics across two
files with no single source of truth. `FRTACMatchState`'s existing lifecycle test already
hand-increments the counter, so this is a pattern that has started rather than one being predicted.

**Sub-rulings on its behaviour:**

- **Returns the new `EntityId`, or `INDEX_NONE` on failure**, matching the sentinel convention
  `FRTACTile::OccupantEntityId` and `FRTACEntity::EntityId` already use throughout this tree. A
  failure logs at `Warning` to `LogRTAC` (Rule 9), the same way `RTACResolveMove` already reports
  `InvalidOrigin`.
- **`NextEntityId` advances only on success.** This is load-bearing for Rule 6, not bookkeeping
  tidiness: if a failed spawn consumed an id, two runs differing only in a failed spawn would
  produce different id sequences for every entity after it, and every downstream comparison would
  diverge for a reason unrelated to what was being tested.
- **Spawn fails on an out-of-bounds position or an already-occupied tile.** Both would otherwise
  break the invariant this function exists to establish — spawning onto an occupied tile
  overwrites an existing occupant's claim.
- **Spawn does not read or write tile ownership.** It sets the entity's `Side` and the tile's
  occupancy, nothing else. Assigning `FRTACTile::Owner` is authoring, which is Decision #10
  Ruling 3's explicitly deferred territory.
- **Spawn does not check the tile's `Owner` against the entity's `Side`.** This asymmetry with
  `RTACCheckMoveLegality` is deliberate: movement legality governs movement, not placement, and
  battle setup may legitimately place an entity anywhere the author intends. Noted as an
  observation for whoever builds the authoring system: an entity placed inside opposing territory
  is immobile by clause 3, since every neighbouring tile is opposing too. That is a property of
  the placement, not a bug in the check, and this decision does not rule on whether the authoring
  layer should prevent it.
- **Declared in `RTACMatchState.h`, defined in `RTACMatchState.cpp`**, as a free function rather
  than a member. This follows `RTACMovementLegality.h`'s stated precedent for `RTACResolveMove`:
  the operation lives next to the state it advances, because "splitting them would put two halves
  of one operation in two files with nothing else in either." A separate `RTACEntitySpawn.h/.cpp`
  was the alternative and would also be defensible; it was not chosen because the counter being
  consumed lives in `RTACMatchState`.

**A boundary this decision respects and deliberately does not resolve.**

Phase 1's tests need real tile ownership assigned — every `FRTACTile::Owner` and
`FRTACEntity::Side` defaults to `Neutral`, so an unconfigured board makes every in-bounds,
unoccupied, unbroken move `Legal` regardless of ownership, and clause 3 never fires. A test built
on a default board would pass without exercising the clause it appears to exercise (Failure Mode 5,
with Failure Mode 8's "an experiment that cannot fail is not evidence" sitting behind it).

**The fixture that assigns ownership stays test-local, under `#if WITH_AUTOMATION_TESTS`, and is
never promoted to plugin-public API.** This decision adds no ownership-assignment function to
RTAC. Doing so would build the authoring mechanism Decision #10 Ruling 3 explicitly defers — and
worse, any convenient shape for it (a symmetric column split) would bake in the assumption Ruling 3
spends its length rejecting: mainline battles default to a symmetric 3×3/3×3 split, but Liberation
Mission-style battles start asymmetric, and ownership "cannot be computed once from `Rows`/`Columns`
and a hardcoded left-half/right-half split."

This is recorded here as a constraint this decision is aware of and works within, not as a ruling
it makes. Ruling 3 already owns it.

**Rejected alternatives:**

- **A test-local entity container (option B above).** Rejected per Ruling 1 — Rule 6 requires this
  state in the explicit struct regardless, so it defers required work rather than avoiding it, and
  it leaves the determinism test comparing its own bookkeeping instead of simulation-owned state.
- **`TMap<int32, FRTACEntity>` storage.** Rejected per Ruling 2 — unstable iteration order is a
  determinism dependency Rule 6 forbids, and the keyed lookup it buys is supplied by Ruling 3's
  accessor at negligible cost.
- **Documenting `index == EntityId` as a supported invariant.** Rejected per Ruling 3. It is true
  today only because Phase 1 has no removal, and it fails silently and everywhere at once when
  removal arrives.
- **No spawn function; each test hand-assigns ids and placements.** Rejected per Ruling 4 — it
  leaves the consistency invariant unowned and duplicates spawn semantics across test files
  (Failure Mode 7).
- **An `ERTACSpawnResult` enum mirroring `ERTACMoveLegality`'s named-clause shape.** Considered
  seriously, since Decision #10 Ruling 4 argues at length against collapsing a multi-clause result
  to a bare value. Rejected because that ruling's justification does not transfer: it named two
  specific future consumers (Ruling 5's per-entity overrides, Ruling 6's attack-side reuse) that
  each need one clause distinguished from the others. Spawn has two failure causes and no
  identified consumer that needs to tell them apart; the `LogRTAC` warning carries the diagnostic
  detail. If such a consumer appears, this is a small change made at that point with a real
  requirement behind it.
- **Changing `RTACResolveMove`'s signature to take `FRTACMatchState&`.** Rejected for now. With
  grid and entities both inside the struct, callers pass two references into one object, which is
  legal but slightly awkward. Taking the entity and grid separately is what keeps `RTACResolveMove`
  a pure function over simulation values with no knowledge of match-state layout — the property
  that makes it trivially testable. Noted as a seam, not a debt.

**Why this matters:** this is the container shape both of Phase 1's two remaining Definition of
Done items will be built directly on top of, and container shapes are cheap to choose and
expensive to change once tests depend on them. Every ruling here is one that costs a line or two
now and an audit later: whether iteration order is stable (Ruling 2) decides whether the
determinism test means anything; whether index is identity (Ruling 3) decides whether entity
removal is a localized change or a call-site sweep; and whether the grid/entity consistency
invariant has an owner (Ruling 4) decides whether a malformed setup fails loudly or corrupts a
board in silence. None of the three is urgent in the sense of blocking compilation, and all three
are the kind of thing that is only ever decided once.

**Explicitly deferred:**

- **The tile-ownership authoring mechanism** — untouched, still Decision #10 Ruling 3's, and see
  the boundary section above for why this entry adds no API toward it.
- **Entity removal / despawn.** No removal exists in Phase 1. Ruling 3's id-not-index contract is
  what makes adding it later a localized change; the removal semantics themselves (does the array
  compact? does a removed id ever get reused?) are not decided here and need their own entry when
  a phase needs them.
- **An id-to-index acceleration map.** Not built. See Ruling 3's performance note for the
  conditions that would justify it and the wrong fix to avoid.
- **Tick order and simultaneity (Rule 7).** Phase 1's input sequences are a total order by
  construction, so "which entity resolves first when two move in the same tick" never arises and
  is not answered here. It becomes real when Phase 2's input layer lands, and wants its own
  decision entry rather than being settled by whatever the first implementation happens to do.

---



## Open Questions



### Core BN3 Loop (build first)

- ~~Exact grid dimensions~~ — resolved, see Decision #8.
- ~~Movement rules~~ — resolved, see Decision #10.
- Buster/basic attack implementation
- Chip-equivalent resource system — see "Chip/Folder Replacement" below
- Turn/round structure (BN3 uses real-time with a Custom Screen pause for selecting chips — confirm whether this is kept as-is)
- Enemy AI behavior patterns and telegraphing
- HP/damage model



### Chip/Folder Building Replacement

- Undecided whether the chip-drafting/folder-building resource system from BN3 is kept, replaced, or reworked.
- No direction chosen yet — intentionally left open until the core loop exists to build against.



### Enemy Roster

- Undecided. Thematically should fit retro-futuristic Atlantean tech — likely candidates per the story outline's faction lore: Guardian automated defense systems/sentinels, corrupted Artisan constructs, etc. Faction bosses are a likely category once PHIS faction identity is more built out.
- No specific enemy types locked yet.



### Elevation — Mechanical Direction (post-core, feeds Decision #3)

Brainstormed directions, none locked, to revisit once the core loop is playable:

- High ground = offensive bonus (damage/accuracy/range) vs. melee, but exposed to ranged attacks and vulnerable to being knocked down (fall damage/lost turn)
- Low ground = defensive bonus (cover from ranged/AOE) but vulnerable to melee and to being pushed further down into hazards (water, pits)
- Elevation-gated abilities — moves that only work from specific elevations (e.g., a slam usable only jumping down from high ground; a low-ground move that pulls enemies toward a hazard), so elevation is a toolkit choice, not just a passive stat modifier
- Environmental interaction tied to elevation — e.g. low ground near water enabling offensive plays (flooding a tile, shorting out enemy systems), high ground enabling collapses/rockslides onto tiles below
- Faction/enemy-specific elevation relationships — e.g. Guardian sentinels favor/punish high ground (turret-like), ambush-style enemies favor low ground — ties elevation strategy to which faction's remnants the player is fighting
- Instability as a dimension — some high tiles risk collapse if stood on too long or hit while occupied
- Open question raised: does a 3x6 grid provide enough spatial room for elevation to read clearly, or does it need to be zone-based (e.g. shared elevation per column/row) rather than per-tile, and/or does the grid need to grow slightly (e.g. 4x6/5x6)?



### Exploration → Combat Transition Mechanism

- Undecided how the player actually enters combat, mechanically. Per Decision #4, enemies exist in the explorable world and engaging one transitions into a contained arena — but the *implementation* of that transition (GameMode swap vs. a mode flag on GameState/PlayerController that swaps pawn/control scheme and streams in the arena, vs. something else) is not decided.
- A GameMode swap is UE5's more traditional pattern but is a heavier operation (typically a level transition or substantial state teardown/rebuild) that may fight against the contextual, in-place-feeling transition Decision #4 implies.
- Not blocking the core grid/movement work — per AGENTS.md Rule 5 (Simulation/Presentation Separation) and Rule 11 (Combat Code Lives Inside RTAC), the grid simulation doesn't know or care how combat was entered. This becomes relevant once the core loop exists and needs to actually be triggered from the explorable world.

---

### Entity Allegiance Change — Mid-Battle Defection to a Third Party (post-core, speculative)

**Not a BN3 mechanic.** Confirmed against source material (Omar): in the mainline games, tile
ownership and entity side are always exactly Player or Enemy — no third faction, no mid-battle
allegiance change, ever. This entire entry is a deliberate departure this project is considering,
not a gap in BN3 knowledge. If pursued, it would need its own numbered Decision entry and its own
Mechanical Fidelity Standard justification (`PHASES.md`), the same treatment Decision #3
(elevation) already received as this project's first deliberate BN3 departure.

**The idea:** an enemy entity can be authored to switch its `Side` from Enemy to `Neutral`
mid-battle, for narrative reasons — Omar's example: fighting three enemies, and one of them
defects to Neutral once specific story conditions are met, becoming neither the player's ally
nor the original enemy group's.

**Explicitly not player/enemy-controllable.** This is not a chip, not a button, not an input
combo, not anything either combatant can trigger or choose. It is authored — triggered by
whatever narrative/condition system decides it happens (undesigned; possibly a PHIS-adjacent
concern, though Decision #2's PHIS/combat decoupling would need explicit reconsideration if so,
not silently bridged). No player agency initiates a defection; it happens *to* the battle, not
*by* a participant's action within it.

**Why this needs its own entry, not silent extrapolation from Decision #9's `Side` field:**
`FRTACEntity::Side` (added by Decision #9's August 31, 2026 addendum) was scoped as a static,
spawn-time identity field — "set once at spawn and does not change during a match," matching the
identity character of `EntityId`/`ArchetypeId` rather than the movement-state category Decision
#9 originally excluded. This idea directly contradicts that: it requires `Side` to be mutable
mid-match, for at least one specific authored case. That is not something Decision #9's addendum
anticipated or permits as written — pursuing this idea means revisiting that addendum's "does not
change during a match" clause, explicitly, not assuming it already allows for an exception.

**The tile-ownership consistency problem this surfaces — a real mechanical requirement, not a
detail:**

If Enemy 3 flips from `Enemy` to `Neutral` while physically standing on a tile whose `Owner` is
still `Enemy`, Decision #10 Ruling 4's clause 3 (owned by the mover's side) now considers that
entity's own current position illegal — not because of anything it did, but because its side
changed out from under it while its position stayed fixed. This is a real contradiction the
legality check would surface, not a cosmetic issue: `RTACCheckMoveLegality`'s clause 3 implicitly
assumes an entity's current tile always agrees with its own side, and nothing before this idea
was ever proposed had reason to question that assumption.

**Proposed resolution, stated as a rule rather than left implicit:** for the specific tile an
entity currently occupies, the entity's `Side` is authoritative, and the tile's `Owner` is kept
in sync with it. Concretely: the moment Enemy 3's `Side` becomes `Neutral`, the tile beneath it
also becomes `Neutral`, as part of the same authored event — not as a separate, potentially
out-of-sync step. This is not proposed as "the tile becomes Neutral because Neutral tiles are
allowed generally" (they are not — see the correction below); it is proposed narrowly, as a
direct, atomic consequence of an entity's own authored side-change, scoped to that one tile only.

**Interaction with the tile-`Neutral`-as-default correction (RTACTileOwner.h's own header
comment, corrected same date):**
this does not reopen or weaken that correction. That correction addressed `Neutral` appearing as
an *unauthored default* — a tile nobody assigned real ownership to, which real BN3 tiles never
are. This idea's tile-Neutral state is the opposite: a *deliberately authored*, narratively
triggered event with a specific cause and a specific single tile affected. The two are not in
tension; they concern different origins of the same enum value.

**Player-facing legibility — Omar's own concern, recorded rather than resolved:** if Enemy 3
still *looks* like an ordinary enemy at the moment it defects, the player has no visual cue that
its allegiance has changed. Omar's own proposed partial answer: the tile-ownership sync above
already provides one cue for free, before any character-model change exists — the ground itself
visibly shifting ownership (territory boundary/tile coloring changing) at the moment of defection
is a signal in its own right, potentially a stronger and more BN3-native one than a character
reskin, since it uses a visual language (tile color = ownership) the game already teaches the
player from the very first battle. Whether this is sufficient on its own, or needs to be paired
with an actual visual change to the entity itself, is unresolved — flagged as a presentation-layer
question for whenever this idea is actually pursued, not decided here.

**Explicitly deferred, in full:**
- The trigger/condition system that would cause a defection (narrative logic, PHIS-adjacent or
  not — undecided, and not to be assumed without revisiting Decision #2).
- Whether `FRTACEntity::Side` becomes mutable mid-match at all, and if so, under what governance
  (only via this specific authored mechanism, presumably — not opened as general mutable state).
- The presentation-layer question above (tile-sync-only vs. tile-sync-plus-entity-visual-change).
- Whether a defected-to-Neutral entity can subsequently be attacked, targeted, or interacted with
  by either original side, and under what rules — entirely unspecified.
- Whether this ever needs to support more than one simultaneous third party, or is scoped to
  exactly one "Neutral" bucket.

**Status of this entry:** speculative and unscoped beyond what's written above. Not blocking any
current Phase 1 work — Phase 1's `FRTACEntity::Side` remains static per Decision #9's addendum
until and unless this idea is formally decided, which would require its own numbered Decision
entry, not an extrapolation from this Open Question.

---

*Last Updated: August 31, 2026 — Decisions #1–#10 current, #9's clarification addendum included.*