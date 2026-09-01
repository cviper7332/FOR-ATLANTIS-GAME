// Copyright Epic Games, Inc. All Rights Reserved.

#include "Simulation/RTACMovementLegality.h"

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
