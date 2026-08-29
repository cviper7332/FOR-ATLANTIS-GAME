// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "RTACModule.h" // LogRTAC — Rule 9: dedicated category, never LogTemp.
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
 * written on Init(), or an off-by-one in either bounds direction of IsValidPosition() would each
 * fail one of the assertions below (Failure Mode 8 — a test that cannot fail is not evidence).
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
	int32 NumAssertions = 0;
	int32 NumPassed = 0;

	/**
	 * Mirrors each assertion's outcome into LogRTAC, so MCP's GetLogEntries(category="LogRTAC")
	 * can retrieve per-assertion test detail rather than only the module-load line. This is
	 * PURELY ADDITIVE — by the time Record() runs, the wrapped assertion helper has already
	 * executed and already driven the framework's own pass/fail state through AddError(). These
	 * log lines never determine the result.
	 *
	 * The verbosity split is deliberate and load-bearing, not stylistic. The framework installs
	 * an output device that intercepts UE_LOG for the duration of a test run and captures
	 * Error/Warning/Display — but NOT Log — converting a captured Error into a test error event
	 * (AutomationTest.cpp:218-254, capture predicate at :233, Error mapping at :245-248).
	 * Therefore:
	 *   - Passes log at Log, which is not intercepted at all, so they provably cannot affect the
	 *     result no matter how many there are.
	 *   - Failures log at Warning rather than Error, because the assertion's own AddError() has
	 *     already registered that failure. Logging at Error would register a SECOND error event
	 *     for a single failed assertion, inflating the Session Frontend's error count and
	 *     misreporting one failure as two. Warning is captured as a warning event and does not
	 *     itself fail the test (bElevateLogWarningsToErrors defaults to false,
	 *     AutomationTest.cpp:181, and is not set in this project's config).
	 *
	 * This is test-only instrumentation inside #if WITH_AUTOMATION_TESTS, runs once per test
	 * rather than per frame, and never exists in the production path — so it is not the per-frame
	 * diagnostic spam Rule 9 requires be removed before work is called done.
	 *
	 * See docs/reference.md → Automation Testing for the full mechanism.
	 */
	auto Record = [&](const TCHAR* What, bool bResult) -> bool
	{
		++NumAssertions;
		if (bResult)
		{
			++NumPassed;
			UE_LOG(LogRTAC, Log, TEXT("  [PASS] %s"), What);
		}
		else
		{
			UE_LOG(LogRTAC, Warning, TEXT("  [FAIL] %s"), What);
		}
		return bResult;
	};

	// Thin wrappers so each description string is written once, at the call site, and cannot
	// drift between the assertion and its log line (Failure Mode 7).
	auto CheckTrue    = [&](const TCHAR* What, bool Value)                   { return Record(What, TestTrue(What, Value)); };
	auto CheckFalse   = [&](const TCHAR* What, bool Value)                   { return Record(What, TestFalse(What, Value)); };
	auto CheckEqual   = [&](const TCHAR* What, int32 Actual, int32 Expected) { return Record(What, TestEqual(What, Actual, Expected)); };
	auto CheckNull    = [&](const TCHAR* What, const void* Pointer)          { return Record(What, TestNull(What, Pointer)); };
	auto CheckNotNull = [&](const TCHAR* What, const FRTACTile* Pointer)     { return Record(What, TestNotNull(What, Pointer)); };

	UE_LOG(LogRTAC, Log, TEXT("=== RTAC.Simulation.Grid.BasicLifecycle — starting ==="));

	FRTACGrid Grid;

	// --- Init() at the project's own default dimensions (3 rows x 6 columns, Decision #8) ---
	// Reusing FRTACGrid::DefaultRows/DefaultColumns rather than writing 3 and 6 here directly —
	// two copies of the same number is exactly the drift Failure Mode 7 warns about, and the
	// grid header's own comment names this same temptation.
	const bool bInitResult = Grid.Init(FRTACGrid::DefaultRows, FRTACGrid::DefaultColumns);
	CheckTrue(TEXT("Init() succeeds at default dimensions"), bInitResult);
	CheckTrue(TEXT("IsInitialized() is true after a successful Init()"), Grid.IsInitialized());
	CheckEqual(TEXT("GetRows() reports the initialised row count"), Grid.GetRows(), FRTACGrid::DefaultRows);
	CheckEqual(TEXT("GetColumns() reports the initialised column count"), Grid.GetColumns(), FRTACGrid::DefaultColumns);
	CheckEqual(TEXT("NumTiles() is Rows * Columns"), Grid.NumTiles(), FRTACGrid::DefaultRows * FRTACGrid::DefaultColumns);

	// --- A known in-bounds position resolves to a tile whose own Position agrees ---
	// This is the assertion that would catch a broken ToIndex() or a Position never assigned
	// during Init(): the tile found at (1,2) must be the tile that believes it is at (1,2).
	const FRTACGridPosition KnownPosition(1, 2);
	const FRTACTile* FoundTile = Grid.FindTile(KnownPosition);
	if (CheckNotNull(TEXT("FindTile() finds an in-bounds position"), FoundTile))
	{
		// Guarded: TestNotNull logs and returns false rather than aborting, so an unguarded
		// dereference here would crash the run instead of failing it cleanly.
		CheckTrue(TEXT("The found tile's own Position matches the position it was found at"),
			FoundTile->Position == KnownPosition);
	}

	// --- Out-of-bounds queries return nullptr, not a stale or garbage tile ---
	// Both directions are exercised deliberately. IsValidPosition() guards with
	// `Row >= 0 && Row < Rows`, so testing only the upper bound would let a regression that
	// deleted the `Row >= 0` half pass unnoticed — in a test whose main job is bounds-checking.
	const FRTACGridPosition PastLastRowPosition(FRTACGrid::DefaultRows, 0); // one past the last row
	CheckNull(TEXT("FindTile() returns nullptr past the last row (upper bound)"),
		Grid.FindTile(PastLastRowPosition));

	const FRTACGridPosition NegativeRowPosition(-1, 0);
	CheckNull(TEXT("FindTile() returns nullptr for a negative row (lower bound)"),
		Grid.FindTile(NegativeRowPosition));

	// --- Reset() actually returns the grid to its default-constructed, uninitialised state ---
	Grid.Reset();
	CheckFalse(TEXT("IsInitialized() is false after Reset()"), Grid.IsInitialized());
	CheckEqual(TEXT("GetRows() is 0 after Reset()"), Grid.GetRows(), 0);
	CheckEqual(TEXT("GetColumns() is 0 after Reset()"), Grid.GetColumns(), 0);
	CheckNull(TEXT("FindTile() returns nullptr on an uninitialised grid"),
		Grid.FindTile(FRTACGridPosition(0, 0)));

	if (NumPassed == NumAssertions)
	{
		UE_LOG(LogRTAC, Log,
			TEXT("=== RTAC.Simulation.Grid.BasicLifecycle — complete: %d/%d assertions passed ==="),
			NumPassed, NumAssertions);
	}
	else
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("=== RTAC.Simulation.Grid.BasicLifecycle — complete: %d/%d assertions passed, %d FAILED ==="),
			NumPassed, NumAssertions, NumAssertions - NumPassed);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
