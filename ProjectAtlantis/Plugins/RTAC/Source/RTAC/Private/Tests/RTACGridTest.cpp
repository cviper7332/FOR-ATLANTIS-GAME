// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Simulation/RTACGrid.h"

/**
 * Phase 0 Part B's first automation test: FRTACGrid's basic lifecycle.
 *
 * Deliberately minimal per docs/PHASES.md Phase 0 — "at least one test that exercises simulation
 * state and can fail," not comprehensive coverage. Multi-entity movement and determinism testing
 * is Phase 1's own Definition of Done item, not this one.
 *
 * Exercises real simulation state (FRTACGrid, a plain struct with no engine dependency — Rule 5)
 * and is capable of failing under a real regression: a broken ToIndex(), a Position that isn't
 * written on Init(), or an off-by-one in IsValidPosition() would each fail one of the assertions
 * below (Failure Mode 8 — a test that cannot fail is not evidence).
 *
 * On RunTest returning true unconditionally: that is correct, not a "cannot fail" bug. The engine
 * computes success as RunTest's return value AND !HasAnyErrors() AND HasMetExpectedMessages()
 * (Runtime/Core/Private/Misc/AutomationTest.cpp:1376). A failing assertion calls AddError()
 * internally, which forces the test to fail regardless of what this function returned. See
 * docs/reference.md → Automation Testing.
 *
 * Flags: EditorContext because Phase 0 Part B is scoped to "requires UE5.8 open" (Decision #7),
 * and ProductFilter — not SmokeFilter — because the engine's filter categories classify tests by
 * kind rather than speed, and this is project gameplay logic, not engine/build sanity. Exactly one
 * Filter-mask flag is permitted; two is a compile error (AutomationTest.h:4128).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTACGridBasicLifecycleTest,
	"RTAC.Simulation.Grid.BasicLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTACGridBasicLifecycleTest::RunTest(const FString& Parameters)
{
	FRTACGrid Grid;

	// --- Init() at the project's own default dimensions (3 rows x 6 columns, Decision #8) ---
	// Reusing FRTACGrid::DefaultRows/DefaultColumns rather than writing 3 and 6 here directly —
	// two copies of the same number is exactly the drift Failure Mode 7 warns about, and the
	// grid header's own comment names this same temptation.
	const bool bInitResult = Grid.Init(FRTACGrid::DefaultRows, FRTACGrid::DefaultColumns);
	TestTrue(TEXT("Init() succeeds at default dimensions"), bInitResult);
	TestTrue(TEXT("IsInitialized() is true after a successful Init()"), Grid.IsInitialized());
	TestEqual(TEXT("GetRows() reports the initialised row count"), Grid.GetRows(), FRTACGrid::DefaultRows);
	TestEqual(TEXT("GetColumns() reports the initialised column count"), Grid.GetColumns(), FRTACGrid::DefaultColumns);
	TestEqual(TEXT("NumTiles() is Rows * Columns"), Grid.NumTiles(), FRTACGrid::DefaultRows * FRTACGrid::DefaultColumns);

	// --- A known in-bounds position resolves to a tile whose own Position agrees ---
	// This is the assertion that would catch a broken ToIndex() or a Position never assigned
	// during Init(): the tile found at (1,2) must be the tile that believes it is at (1,2).
	const FRTACGridPosition KnownPosition(1, 2);
	const FRTACTile* FoundTile = Grid.FindTile(KnownPosition);
	if (TestNotNull(TEXT("FindTile() finds an in-bounds position"), FoundTile))
	{
		// Guarded: TestNotNull logs and returns false rather than aborting, so an unguarded
		// dereference here would crash the run instead of failing it cleanly.
		TestTrue(TEXT("The found tile's own Position matches the position it was found at"),
			FoundTile->Position == KnownPosition);
	}

	// --- Out-of-bounds queries return nullptr, not a stale or garbage tile ---
	// Both directions are exercised deliberately. IsValidPosition() guards with
	// `Row >= 0 && Row < Rows`, so testing only the upper bound would let a regression that
	// deleted the `Row >= 0` half pass unnoticed — in a test whose main job is bounds-checking.
	const FRTACGridPosition PastLastRowPosition(FRTACGrid::DefaultRows, 0); // one past the last row
	TestNull(TEXT("FindTile() returns nullptr past the last row (upper bound)"),
		Grid.FindTile(PastLastRowPosition));

	const FRTACGridPosition NegativeRowPosition(-1, 0);
	TestNull(TEXT("FindTile() returns nullptr for a negative row (lower bound)"),
		Grid.FindTile(NegativeRowPosition));

	// --- Reset() actually returns the grid to its default-constructed, uninitialised state ---
	Grid.Reset();
	TestFalse(TEXT("IsInitialized() is false after Reset()"), Grid.IsInitialized());
	TestEqual(TEXT("GetRows() is 0 after Reset()"), Grid.GetRows(), 0);
	TestEqual(TEXT("GetColumns() is 0 after Reset()"), Grid.GetColumns(), 0);
	TestNull(TEXT("FindTile() returns nullptr on an uninitialised grid"),
		Grid.FindTile(FRTACGridPosition(0, 0)));

	return true;
}

#endif // WITH_AUTOMATION_TESTS
