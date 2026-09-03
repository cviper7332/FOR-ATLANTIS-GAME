// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Simulation/RTACGrid.h"
#include "Simulation/RTACMovementLegality.h"
#include "Simulation/RTACSurfaceModifier.h"
#include "Simulation/RTACTile.h"
#include "Simulation/RTACTileOwner.h"

/**
 * Helpers shared between RTAC's automation tests.
 *
 * WHY THIS FILE EXISTS. Both RTACMovementTest.cpp and RTACDeterminismTest.cpp need the same two
 * helpers: a readable name for an ERTACMoveLegality value, and a fixture that stamps real ownership
 * onto a test board. They lived in RTACMovementTest.cpp's anonymous namespace, which gives them
 * internal linkage and makes them unreachable from a second translation unit. The two options were
 * to copy them or to hoist them; copying is exactly the drift Failure Mode 7 describes — "one
 * quantity, one authoritative location" — so they are hoisted here and the movement test now
 * includes this file instead of defining them.
 *
 * TEST-ONLY. Nothing here is production API and nothing here may be promoted into the plugin's
 * Public/ tree. It sits beside the tests that use it, inside the plugin per Rule 11, and is
 * included only from inside `#if WITH_AUTOMATION_TESTS` blocks.
 *
 * Included by quoted relative path (`#include "RTACTestFixtures.h"`) from files in this same
 * directory, so it does not depend on Private/ being on the module's include path.
 */

/**
 * Readable names for ERTACMoveLegality, so a failed assertion says "expected WrongOwner, got
 * Occupied" rather than "expected 3, got 2". Clause order is the thing most likely to be got
 * wrong in a movement test, and an integer failure message would hide exactly that.
 */
inline const TCHAR* RTACTest_LegalityName(ERTACMoveLegality Value)
{
	switch (Value)
	{
	case ERTACMoveLegality::Legal:         return TEXT("Legal");
	case ERTACMoveLegality::OutOfBounds:   return TEXT("OutOfBounds");
	case ERTACMoveLegality::Occupied:      return TEXT("Occupied");
	case ERTACMoveLegality::WrongOwner:    return TEXT("WrongOwner");
	case ERTACMoveLegality::Broken:        return TEXT("Broken");
	case ERTACMoveLegality::InvalidOrigin: return TEXT("InvalidOrigin");
	}
	return TEXT("<unrecognised>");
}

/** Readable names for ERTACTileOwner, for field-level difference messages. */
inline const TCHAR* RTACTest_TileOwnerName(ERTACTileOwner Value)
{
	switch (Value)
	{
	case ERTACTileOwner::Neutral: return TEXT("Neutral");
	case ERTACTileOwner::Player:  return TEXT("Player");
	case ERTACTileOwner::Enemy:   return TEXT("Enemy");
	}
	return TEXT("<unrecognised>");
}

/** Readable names for ERTACSurfaceModifier, for field-level difference messages. */
inline const TCHAR* RTACTest_SurfaceModifierName(ERTACSurfaceModifier Value)
{
	switch (Value)
	{
	case ERTACSurfaceModifier::None:   return TEXT("None");
	case ERTACSurfaceModifier::Broken: return TEXT("Broken");
	}
	return TEXT("<unrecognised>");
}

/**
 * Stamps a symmetric column split onto the board — left half Player, right half Enemy.
 *
 * ---------------------------------------------------------------------------------
 * THIS IS A TEST FIXTURE. IT IS NOT DECISION #10 RULING 3's AUTHORING MECHANISM, AND MUST
 * NEVER BE MISTAKEN FOR ONE OR PROMOTED OUT OF THIS DIRECTORY.
 *
 * Ruling 3 defers the authoring interface — "however a battle's tiles actually get set to
 * Player/Enemy/Neutral before combat starts" — and explicitly REJECTS deriving ownership from
 * Rows/Columns: mainline MMBN battles default to a symmetric 3x3/3x3 split, but Liberation
 * Mission-style battles start asymmetric (the player surrounded, or one side holding more
 * field), set by authored rules that a computed split cannot represent. Decision #11 records
 * the same boundary from the other side: this decision "adds no ownership-assignment function
 * to RTAC."
 *
 * A computed split is fine HERE precisely because this is a test arranging a known board, not
 * a system deciding what boards can exist. If this ever needs to move into the plugin proper,
 * that is a signal the authoring decision has come due — not a signal to copy this function.
 *
 * Hoisting it from RTACMovementTest.cpp into this shared test header does NOT change that: it
 * moved from one test file to a header included by test files only, and is still unreachable
 * from any non-test translation unit.
 * ---------------------------------------------------------------------------------
 *
 * Every tile must be assigned: ERTACTileOwner::Neutral is a construction-time sentinel, not a
 * resting gameplay state (ERTACTileOwner's own August 31, 2026 correction), and leaving tiles
 * Neutral is what makes the ownership clause unreachable — the degenerate setup the movement
 * test exists to avoid (Failure Mode 5).
 */
inline void RTACTest_ApplySymmetricSplitFixture(FRTACGrid& Grid)
{
	const int32 PlayerColumns = Grid.GetColumns() / 2;

	for (int32 Row = 0; Row < Grid.GetRows(); ++Row)
	{
		for (int32 Column = 0; Column < Grid.GetColumns(); ++Column)
		{
			FRTACTile* Tile = Grid.FindTile(Row, Column);
			check(Tile);
			Tile->Owner = (Column < PlayerColumns) ? ERTACTileOwner::Player : ERTACTileOwner::Enemy;
		}
	}
}
