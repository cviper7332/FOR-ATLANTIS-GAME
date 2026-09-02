// Copyright Epic Games, Inc. All Rights Reserved.

#include "Simulation/RTACMatchState.h"

#include "RTACModule.h" // LogRTAC — Rule 9: dedicated category, never LogTemp.
#include "Simulation/RTACTile.h"

void FRTACRngState::Initialize(int32 InMasterSeed)
{
	MasterSeed = InMasterSeed;

	// No streams to derive yet. When stream fields exist, they are derived HERE, once, and
	// stored — never re-derived per draw. A freshly derived stream returns the same value every
	// time, so re-deriving at each use would produce a constant, not a sequence.
}

void FRTACRngState::Reset()
{
	MasterSeed = 0;
}

const FRTACEntity* FRTACMatchState::FindEntity(int32 EntityId) const
{
	// INDEX_NONE is never a real id — Decision #9 hands them out 0, 1, 2, ... — so it is rejected
	// up front rather than left to fall out of the scan. This is not defensive padding: the
	// natural call site is FindEntity(Tile.OccupantEntityId), and an empty tile carries exactly
	// this sentinel. Answering it in O(1) with an explicit "no" is clearer than a scan that
	// happens to miss, and it stays correct if a malformed default-constructed entity ever ends
	// up in the array.
	if (EntityId == INDEX_NONE)
	{
		return nullptr;
	}

	for (const FRTACEntity& Entity : Entities)
	{
		if (Entity.EntityId == EntityId)
		{
			return &Entity;
		}
	}

	return nullptr;
}

FRTACEntity* FRTACMatchState::FindEntity(int32 EntityId)
{
	// Same const_cast delegation FRTACGrid::FindTile() uses, so the two overloads cannot drift
	// apart: one implementation, one set of rules about what counts as found.
	return const_cast<FRTACEntity*>(const_cast<const FRTACMatchState*>(this)->FindEntity(EntityId));
}

void FRTACMatchState::Initialize(int32 InMasterSeed)
{
	// Every member assigned outright rather than incrementally patched, so this is a complete
	// standalone reinitialisation per Rule 6 rather than a partial refresh over prior contents.
	//
	// Grid is reset, not initialised: this function takes no dimensions, so a caller runs
	// Grid.Init(Rows, Columns) next. See the header for why.
	Rng.Initialize(InMasterSeed);
	Grid.Reset();
	Entities.Reset();
	NextEntityId = 0;
}

void FRTACMatchState::Reset()
{
	Rng.Reset();
	Grid.Reset();
	Entities.Reset();
	NextEntityId = 0;
}

int32 RTACSpawnEntity(FRTACMatchState& State, FRTACGridPosition Position, ERTACTileOwner Side, FName ArchetypeId)
{
	// Bounds first, for the same reason RTACCheckMoveLegality orders its clauses this way:
	// FindTile() returns nullptr off-grid, so no tile field may be read before this passes. An
	// uninitialised grid fails here too — IsValidPosition() is false for every position while
	// Rows/Columns are 0 — which is the intended answer, not an edge case to special-case.
	if (!State.Grid.IsValidPosition(Position))
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("RTACSpawnEntity: position (%d,%d) is not on the %dx%d grid; nothing spawned."),
			Position.Row, Position.Column, State.Grid.GetRows(), State.Grid.GetColumns());
		return INDEX_NONE;
	}

	// Safe to dereference: the bounds check above already established Position is on the grid.
	FRTACTile* Tile = State.Grid.FindTile(Position);
	check(Tile);

	if (Tile->IsOccupied())
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("RTACSpawnEntity: tile (%d,%d) is already occupied by entity %d; nothing spawned."),
			Position.Row, Position.Column, Tile->OccupantEntityId);
		return INDEX_NONE;
	}

	// Past every failure case. The counter advances HERE and nowhere earlier, so a rejected spawn
	// leaves the id sequence untouched — see the header on why that is a Rule 6 requirement rather
	// than bookkeeping tidiness.
	const int32 NewEntityId = State.NextEntityId++;

	// Tile stays valid across this Add: it points into Grid.Tiles, a different array from
	// Entities, so growing Entities cannot reallocate underneath it.
	FRTACEntity& Entity = State.Entities.AddDefaulted_GetRef();
	Entity.EntityId = NewEntityId;
	Entity.Position = Position;
	Entity.Side = Side;
	Entity.ArchetypeId = ArchetypeId;

	// The other half of the invariant. Entity.Position and the tile's OccupantEntityId are written
	// together, in one function, so there is no window in which they can disagree and no second
	// place that has to remember to do both.
	Tile->OccupantEntityId = NewEntityId;

	return NewEntityId;
}
