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
 * TWO CATEGORIES OF VALUE, DELIBERATELY IN ONE ENUM:
 *   - Legal + the four Ruling 4 clauses (OutOfBounds, Occupied, WrongOwner, Broken) are all
 *     DESTINATION-legality outcomes. RTACCheckMoveLegality returns exactly these and nothing else.
 *   - InvalidOrigin is a precondition-on-the-mover failure, not a destination outcome. Only
 *     RTACResolveMove can return it, because only resolution inspects the mover's ORIGIN tile;
 *     RTACCheckMoveLegality never looks at the origin and so never returns InvalidOrigin. It
 *     shares this enum rather than getting its own type because callers of RTACResolveMove want
 *     one result vocabulary covering every way a resolve can decline — splitting it would force
 *     every caller to switch over two enums for one call.
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
	Broken,

	/**
	 * Precondition failure, NOT a Ruling 4 clause — the mover's own current Position does not
	 * correspond to a tile on the grid (default/unset, or otherwise off-board). Returned only by
	 * RTACResolveMove, which is the only function that reads the origin tile; RTACCheckMoveLegality
	 * inspects the destination alone and never yields this value. Appended after Broken (value 5)
	 * rather than slotted among the clauses so the four Ruling 4 outcomes keep their 1-4 ordering.
	 */
	InvalidOrigin
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
 * @return Legal if the move may proceed; otherwise the first clause that failed. One of exactly
 *         five values: Legal, OutOfBounds, Occupied, WrongOwner, Broken. Never InvalidOrigin —
 *         this function does not inspect the mover's origin; see ERTACMoveLegality's own doc and
 *         RTACResolveMove for that check.
 */
ERTACMoveLegality RTACCheckMoveLegality(const FRTACEntity& Entity, FRTACGridPosition Destination, const FRTACGrid& Grid);

/**
 * Resolves a move: validates it with RTACCheckMoveLegality, then — only if Legal — applies it.
 *
 * This is Rule 7's Resolution stage for movement: the one place a validated move is committed to
 * simulation state. It lives in this file, next to the check it wraps, because resolution is the
 * check's direct counterpart — same inputs, same result vocabulary, same Decision #10 Ruling 4
 * clauses — and splitting them would put two halves of one operation in two files with nothing
 * else in either. It is NOT added to FRTACGrid (Rule 5 / Grid.h's own scope note: "Movement
 * rules do not [live here]").
 *
 * VALIDATION IS INTERNAL, NOT CALLER-SUPPLIED. This function calls RTACCheckMoveLegality itself
 * rather than trusting a result the caller passes in. The tradeoff is a second run of the
 * four-clause check when a caller has already checked; that cost is four field comparisons and
 * is paid to guarantee resolution never applies a move against state that changed between a
 * caller's check and this call. A caller that only wants to test legality calls
 * RTACCheckMoveLegality directly and never reaches here.
 *
 * NO PARTIAL APPLICATION. If the check returns anything other than Legal, that result is
 * returned unchanged and no field of Entity or Grid is touched. The same rule covers the one
 * failure this function detects itself, InvalidOrigin (below): it too returns with nothing mutated.
 *
 * MUTATES IN PLACE. Entity and Grid are taken by non-const reference and modified directly;
 * this returns a status code, not new copies. Rule 6 mandates explicit state, not immutable
 * state — the caller owns both objects and passes them through the tick pipeline (Rule 7), and
 * copying an entire grid per move to satisfy a value-return style would be waste with no
 * determinism benefit. FRTACGrid already exposes a non-const FindTile() overload for exactly
 * this kind of write.
 *
 * On success, applies these three steps in this fixed order (stated explicitly per Rule 7 even
 * though origin and destination cannot coincide — a Legal move always changes Position, since
 * clause 2 rejects the occupied origin tile as a destination):
 *   1. Clear OccupantEntityId (-> INDEX_NONE) on the tile at Entity.Position (the origin).
 *   2. Set Entity.Position = Destination.
 *   3. Set OccupantEntityId = Entity.EntityId on the tile at Destination.
 *
 * ORIGIN PRECONDITION. Between the check passing and step 1, this function confirms Entity.Position
 * corresponds to a real tile. If it does not (unset/off-board mover), it returns InvalidOrigin and
 * applies nothing — a hard failure, not a warn-and-proceed. This is the one outcome RTACResolveMove
 * produces that RTACCheckMoveLegality cannot.
 *
 * SCOPE: resolution only. No test yet — that is the next task and depends on this existing.
 *
 * @param Entity        The mover. Mutated on success (Position); read-only on failure.
 * @param Destination   The target tile, in grid space (Rule 10).
 * @param Grid          The board. Two tiles' OccupantEntityId are mutated on success.
 * @return Legal when the move was applied; a Ruling 4 clause value when the legality check
 *         rejected the destination; or InvalidOrigin when the mover's own position is off-board.
 *         Nothing is applied in any non-Legal case.
 */
ERTACMoveLegality RTACResolveMove(FRTACEntity& Entity, FRTACGridPosition Destination, FRTACGrid& Grid);
