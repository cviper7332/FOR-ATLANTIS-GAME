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

---

*Last Updated: August 26, 2026*