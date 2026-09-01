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
**Status:** OPEN

**Decision:** The combat grid defaults to **3 rows × 6 columns**, matching BN3's actual board exactly (per Decision #5's rows×columns convention). Dimensions are **not** a compile-time constant — rows and columns are configurable, exposed to the level author for editing in the UE5 Editor's Details panel, with 3×6 as the shipped default value.

**Architecture — this does not create tension with Rule 5.** The editable-in-editor requirement is satisfied entirely on the presentation side, not by relaxing the simulation boundary:

- The simulation-layer grid type takes rows and columns as **plain integer parameters** (constructor or init call) — no engine dependency, no `UPROPERTY`, nothing Rule 5 would object to. The struct itself has no idea an editor exists.
- A thin presentation-layer wrapper — an actor or component, exactly where `UPROPERTY(EditAnywhere)` and engine types belong per Rule 5's own separation — exposes `Rows`/`Columns` as editable integer properties defaulting to 3/6, and passes them into the simulation at init time.
- This is stated explicitly so it reads as the separation being **used as designed**, not as an exception carved out for it. Rule 5 already draws exactly this boundary (simulation owns state and rules; presentation owns everything engine-facing, including where editor-exposed properties live) — configurable dimensions are simply the first concrete case that exercises it.

**Why this matters:** "Exact grid dimensions" has sat as an open question since the Design Philosophy section was written, explicitly deferred until the core loop needed a real number to build against (Open Questions → Core BN3 Loop). Locking the *default* to BN3's own board removes the guesswork the open question raised, while making it editor-configurable — rather than a hardcoded constant — keeps the door open for the "does 3×6 give elevation enough room to read clearly, or does the grid need to grow slightly (4×6/5×6)?" question already logged under Open Questions → Elevation — Mechanical Direction, without requiring a second decision or a code change to test that later.

**Note:** This satisfies Phase 1's Definition of Done item ("Grid dimensions chosen and logged as their own Decision entry, stated as rows×columns per Decision #5") in `PHASES.md` — this entry is that "own Decision entry," and its dimensions are expressed as rows×columns per Decision #5's convention as required.

**Addendum, August 28, 2026 (date correction):** This entry's `**Date:**` field above reads "August 26, 2026." That date is incorrect. Decision #8 was actually written and committed **August 28, 2026**, per commit `ad1fe4b` (2026-08-28 10:07). The original `**Date:**` field is left unchanged per Rule 4; this addendum records the correction. No internal reference within this entry restates another entry's date.

---

## Decision #9 — Entity Identity: Plain Instance Counter, Separate from Archetype Key

**Date:** August 29, 2026
**Phase:** RTAC Phase 1 (Grid & Movement — Headless Simulation)
**Author:** Omar
**Status:** OPEN

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

---

## Decision #10 — Movement Rules: Discrete Step, Mutable Ownership, Reusable Legality Check

**Date:** August 30, 2026
**Phase:** RTAC Phase 1 (Grid & Movement — Headless Simulation)
**Author:** Omar
**Status:** OPEN

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

---



## Open Questions



### Core BN3 Loop (build first)

- Exact grid dimensions (stick with BN3's 3x6, or adjust?)
- Movement rules (free movement within your side vs. tile-by-tile step, cooldowns)
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

*Last Updated: August 30, 2026 — Decisions #1–#9 current, #9's clarification addendum included.*