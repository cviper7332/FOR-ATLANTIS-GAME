// Copyright Epic Games, Inc. All Rights Reserved.

#include "Simulation/RTACMovementLegality.h"

#include "RTACModule.h" // LogRTAC — Rule 9: dedicated category, never LogTemp.
#include "Simulation/RTACEntity.h"
#include "Simulation/RTACGrid.h"
#include "Simulation/RTACSurfaceModifier.h"
#include "Simulation/RTACTile.h"

ERTACMoveLegality RTACCheckMoveLegality(const FRTACEntity& Entity, FRTACGridPosition Destination, const FRTACGrid& Grid)
{
	// --- Clause 1: in bounds ---
	// Reused, not reimplemented, per Ruling 4's own instruction. Also guards every FindTile()
	// call below: a position that fails this clause returns nullptr from FindTile(), so bounds
	// must be checked first, before any tile field is read.
	if (!Grid.IsValidPosition(Destination))
	{
		return ERTACMoveLegality::OutOfBounds;
	}

	// Safe to dereference: clause 1 already established Destination is in bounds, so this
	// cannot be nullptr. Not re-checked, to avoid two bounds checks doing the same job.
	const FRTACTile* DestinationTile = Grid.FindTile(Destination);
	check(DestinationTile);

	// --- Clause 2: unoccupied ---
	if (DestinationTile->IsOccupied())
	{
		return ERTACMoveLegality::Occupied;
	}

	// --- Clause 3: owned by the mover's side ---
	// FRTACTile::Owner vs. FRTACEntity::Side — both ERTACTileOwner, per Decision #9's August 31,
	// 2026 addendum, which added Side to FRTACEntity specifically so this comparison has
	// something real on both sides rather than one.
	if (DestinationTile->Owner != Entity.Side)
	{
		return ERTACMoveLegality::WrongOwner;
	}

	// --- Clause 4: not broken ---
	//
	// FUTURE OVERRIDE SEAM (Decision #10 Ruling 5 — Air Shoes, hover; explicitly deferred, not
	// built here): this is a flat check today because no per-entity override mechanism exists.
	// When one does, it plugs in as an added condition on THIS line only —
	// e.g. `if (IsBroken && !Entity.HasBrokenTileOverride())` — without touching clauses 1-3 or
	// changing this function's signature or return type. Nothing elsewhere in this function
	// needs to change to support that; that is the entire point of Ruling 4's separable-clause
	// shape.
	const bool bIsBroken = DestinationTile->SurfaceModifier == ERTACSurfaceModifier::Broken;
	if (bIsBroken)
	{
		return ERTACMoveLegality::Broken;
	}

	return ERTACMoveLegality::Legal;
}

ERTACMoveLegality RTACResolveMove(FRTACEntity& Entity, FRTACGridPosition Destination, FRTACGrid& Grid)
{
	// Validate first. Anything other than Legal is returned unchanged, with no state touched —
	// no partial application. See this function's header doc for why the check is re-run here
	// rather than trusted from the caller.
	const ERTACMoveLegality Legality = RTACCheckMoveLegality(Entity, Destination, Grid);
	if (Legality != ERTACMoveLegality::Legal)
	{
		return Legality;
	}

	// Clause 1 of the check established Destination is in bounds, so this cannot be nullptr.
	FRTACTile* DestinationTile = Grid.FindTile(Destination);
	check(DestinationTile);

	// The origin tile. A well-formed mover sits on a real tile; a null here means Entity.Position
	// is unset or off-board. That is a precondition failure on the mover, distinct from the four
	// Ruling 4 destination clauses (RTACCheckMoveLegality never inspects the origin, so it cannot
	// catch this). Treat it as a hard failure — return InvalidOrigin with nothing mutated, the
	// same no-partial-application rule the check's four values already get — rather than mutating
	// a board into an inconsistent state from a bad input.
	FRTACTile* OriginTile = Grid.FindTile(Entity.Position);
	if (!OriginTile)
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("RTACResolveMove: entity %d has off-grid origin (%d,%d); move to (%d,%d) rejected (InvalidOrigin), nothing applied."),
			Entity.EntityId, Entity.Position.Row, Entity.Position.Column, Destination.Row, Destination.Column);
		return ERTACMoveLegality::InvalidOrigin;
	}

	// Fixed step order (Rule 7), stated explicitly. Origin and destination cannot be the same
	// tile here — a Legal move always changes Position, since the check's clause 2 rejects an
	// occupied destination and the origin is occupied by this entity — but the order is declared
	// regardless so no step silently depends on that being true.
	//
	// Step 1: vacate the origin tile. OriginTile is non-null — the precondition check above
	// returned already if it wasn't.
	OriginTile->OccupantEntityId = INDEX_NONE;

	// Step 2: move the entity.
	Entity.Position = Destination;

	// Step 3: occupy the destination tile.
	DestinationTile->OccupantEntityId = Entity.EntityId;

	return ERTACMoveLegality::Legal;
}
