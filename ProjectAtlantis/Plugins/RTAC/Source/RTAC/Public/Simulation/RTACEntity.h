// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Simulation/RTACGridPosition.h"
#include "Simulation/RTACTileOwner.h"

/**
 * One entity on the combat grid — the minimal occupant, per Decision #9 and its August 31, 2026
 * addendum.
 *
 * Decision #9 fixes this shape: instance identity, position, and archetype key are three
 * separate fields, and identity is never merged with archetype into a single descriptive name.
 * Its August 31, 2026 addendum adds a fourth — Side — closing a gap Decision #10 Ruling 4
 * exposed: the movement-legality check's ownership clause needs to know which side an entity
 * belongs to, and nothing in the original three fields provided that.
 *
 * SCOPE — PHASE 1, DATA ONLY. This is the type; nothing here spawns entities, moves them, or
 * resolves anything about them. Movement rules are still an open design question in
 * combat_decisions.md and are deliberately not anticipated by this struct — no facing, no speed,
 * no cooldown, no movement state, because none of that is settled and inventing it here would
 * commit the design by implication.
 *
 * Simulation-layer type (AGENTS.md Rule 5): plain struct, UE Core value types only, no
 * UObject/AActor ownership, no UPROPERTY, no reflection. Presentation reads this and renders it;
 * this never reaches back the other way.
 */
struct FRTACEntity
{
	/**
	 * Instance identity — which occupant this is, and nothing more.
	 *
	 * Assigned from FRTACMatchState::NextEntityId in spawn order (0, 1, 2, ...) per Decision #9.
	 * It is not random, not derived from a name string, and not derived from anything
	 * environmental (spawn timestamp, memory address) — each of those was considered and
	 * rejected in Decision #9's "Rejected alternatives" section, on Rule 6 determinism grounds.
	 *
	 * No RNG stream seeds this. Decision #9's August 30, 2026 clarification addendum records
	 * that "sourced from explicit seeded state" means Rule 6's no-hidden-state clause — the
	 * counter lives in the explicit per-match state struct — not that the value is drawn from a
	 * seeded generator. Do not build one behind it.
	 *
	 * Defaults to INDEX_NONE rather than 0 because 0 is a legitimate id — the first one handed
	 * out. Defaulting to 0 would make "never assigned" indistinguishable from "the first entity."
	 * Same sentinel convention as FRTACTile::OccupantEntityId, which this field pairs with.
	 * INDEX_NONE is an unscoped enum constant (not a preprocessor macro), value -1, per
	 * CoreMiscDefines.h:145. -1 is disjoint from EntityId's entire value space by construction,
	 * not merely by convention: EntityIds are assigned 0, 1, 2, ... per Decision #9, and no
	 * non-negative counter can produce -1 without underflow, which isn't in play here.
	 */
	int32 EntityId = INDEX_NONE;

	/**
	 * Where this entity currently is, rows x columns per Decision #5.
	 *
	 * DOMAIN (Rule 10): a discrete grid index, never a world-space location. Converting to world
	 * units is a presentation-layer concern and happens at exactly one named boundary there.
	 */
	FRTACGridPosition Position;

	/**
	 * Which side this entity belongs to — Player, Enemy, or Neutral.
	 *
	 * Added by Decision #9's August 31, 2026 addendum, not by the original decision. Decision
	 * #9's text explicitly excluded "no facing, no speed, no cooldown, no movement state" —
	 * that exclusion targeted turn-to-turn movement/animation state, not identity. Side is
	 * identity: set once at spawn and unchanging for the rest of the match, the same character
	 * as EntityId and ArchetypeId, not the movement-state category the original text ruled out.
	 *
	 * Reuses ERTACTileOwner rather than introducing a second, parallel enum — one type answers
	 * "which side" for both a tile and an entity, so the movement-legality check's ownership
	 * clause (Decision #10 Ruling 4) compares like against like without a conversion step.
	 *
	 * Defaults to Neutral, the same sentinel-and-real-value default ERTACTileOwner::Owner uses
	 * on FRTACTile — an entity that hasn't been assigned a side yet reports Neutral rather than
	 * silently aliasing to Player or Enemy.
	 */
	ERTACTileOwner Side = ERTACTileOwner::Neutral;

	/**
	 * What KIND of entity this is — a lookup key into a future stats/behavior table.
	 *
	 * ---------------------------------------------------------------------------------
	 * INTENTIONALLY UNPOPULATED AT PHASE 1 — THIS IS NOT DEAD CODE. DO NOT DELETE.
	 *
	 * Nothing reads this field yet, and that is correct. Decision #9 reserves it now, at the
	 * cost of one unused field, so a future stats/AI-pattern lookup has a seam to plug into
	 * without a later struct rework — the same reserved-but-inert pattern FRTACTile::Elevation
	 * already uses, and Decision #9 cites that parallel explicitly.
	 *
	 * The lookup table itself — stat definitions, AI behavior mapping — is out of scope by
	 * Decision #9's own "Explicitly deferred" section, until Phase 3 (HP/damage) and Phase 4
	 * (enemy AI) exist to consume it. There is deliberately no archetype enum and no registry
	 * anywhere in the plugin; a placeholder value is sufficient for Phase 1's tests.
	 *
	 * This is a KEY, not a display name and not an instance label. "Sentinel" is correct;
	 * "Sentinel_03" is not — that would smuggle instance identity back into the archetype field,
	 * which is the exact conflation Decision #9 exists to prevent (Rule 10).
	 *
	 * FName is a UE Core value type — an interned index handle with no reflection, no GC, and no
	 * object lifetime — so it satisfies Rule 5 as narrowed by Addendum #2 ("UE Core value types
	 * are explicitly permitted, and preferred"). It is not a UObject, despite its header living
	 * under Core's UObject/ folder; noting that here so the include path doesn't read as a
	 * Rule 5 violation on review.
	 * ---------------------------------------------------------------------------------
	 */
	FName ArchetypeId;

	/**
	 * True when this entity has been assigned an id.
	 *
	 * Kept as an accessor because it encodes the INDEX_NONE sentinel convention, which is not
	 * evident from the field alone — the same reasoning FRTACTile::IsOccupied() is kept for.
	 */
	bool IsValid() const { return EntityId != INDEX_NONE; }
};
