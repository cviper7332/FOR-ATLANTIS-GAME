// Copyright Epic Games, Inc. All Rights Reserved.

#include "Simulation/RTACMatchState.h"

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

void FRTACMatchState::Initialize(int32 InMasterSeed)
{
	// Assigned outright rather than incrementally patched, so this is a complete standalone
	// reinitialisation per Rule 6 rather than a partial refresh over prior contents.
	Rng.Initialize(InMasterSeed);
	NextEntityId = 0;
}

void FRTACMatchState::Reset()
{
	Rng.Reset();
	NextEntityId = 0;
}
