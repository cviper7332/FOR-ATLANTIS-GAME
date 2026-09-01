// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Simulation/RTACGridPosition.h"

struct FRTACEntity;
struct FRTACGrid;

/**
 * Result of a movement-legality check, per Decision #10 Ruling 4.
 *
 * Named so a caller can know WHICH of the four clauses failed, not just that the move is
 * illegal. Ruling 4's own reasoning for keeping the clauses separable is defeated if the result
 * collapses back to a bare bool at the boundary — two future consumers (a per-entity broken-tile
 * override, Ruling 5; a reach-attack system reusing this check from outside movement, Ruling 6)
 * each need to know specifically which clause they're dealing with.
 *
 * Deliberately not a UENUM: simulation-layer type, no reflection (Rule 5).
 */
enum class ERTACMoveLegality : uint8
{
	/** All four clauses passed. The move may proceed. */
	Legal = 0,

	/** Clause 1 — the destination lies outside the grid. */
	OutOfBounds,

	/** Clause 2 — the destination tile already has an occupant. */
	Occupied,

	/** Clause 3 — the destination tile's owner does not match the mover's side. */
	WrongOwner,

	/** Clause 4 — the destination tile is broken, and the mover has no qualifying override. */
	Broken
};

/**
 * Checks whether Entity may legally occupy Destination on Grid, per Decision #10 Ruling 4.
 *
 * Four clauses, checked in order, first failure wins — exactly Ruling 4's specification:
 *   1. In bounds       — reuses FRTACGrid::IsValidPosition(), not reimplemented.
 *   2. Unoccupied       — FRTACTile::OccupantEntityId == INDEX_NONE.
 *   3. Owned by the mover's side — FRTACTile::Owner matches FRTACEntity::Side (Decision #9's
 *      August 31, 2026 addendum, which added Side specifically so this clause has something to
 *      compare against).
 *   4. Not broken       — FRTACTile::SurfaceModifier != ERTACSurfaceModifier::Broken, for an
 *      entity without a qualifying override. No override mechanism exists yet (Ruling 5 is
 *      explicitly deferred) — this clause is a flat check for every entity today. See this
 *      function's .cpp for exactly where a future override plugs in without restructuring
 *      this function.
 *
 * SCOPE: the check only. Does not move Entity, does not mutate Grid or Entity in any way, and
 * has no test yet — both are separate follow-on tasks. Pure function over simulation-layer
 * state: no UObject/AActor, no engine dependency (Rule 5).
 *
 * @param Entity        The mover. Only its Side is read.
 * @param Destination   The tile being validated, in grid space (Rule 10 — never world units).
 * @param Grid          The board Destination is checked against.
 * @return Legal if the move may proceed; otherwise the first clause that failed.
 */
ERTACMoveLegality RTACCheckMoveLegality(const FRTACEntity& Entity, FRTACGridPosition Destination, const FRTACGrid& Grid);
