#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "RTACModule.h" // LogRTAC — Rule 9: dedicated category, never LogTemp.
#include "Presentation/RTACBoard.h"
#include "Presentation/RTACGridConversion.h"
#include "Simulation/RTACGrid.h"
#include "Simulation/RTACGridPosition.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

/**
 * RTACWorldPositionToGridPosition and RTACScreenToGridPosition's camera-independent guard
 * clauses -- the first Presentation-layer test in this suite (every other RTAC test is
 * Simulation-layer, Rule 5, and needs no UWorld at all).
 *
 * SCOPE, PRECISELY. RTACScreenToGridPosition's real deprojection path -- PlayerController's
 * camera turning a screen position into a world-space ray -- needs a live LocalPlayer with an
 * attached Viewport, which only exists inside a running PIE session (confirmed by reading
 * PlayerController.cpp's DeprojectScreenPositionToWorld and GameplayStatics.cpp's
 * DeprojectScreenToWorld directly, UE 5.8). That path is DELIBERATELY NOT exercised here — doing
 * so would mean starting PIE and possessing a real camera, which is exactly the Decision #15
 * camera dependency this test is scoped to stay out of. This test instead covers:
 *   (a) RTACWorldPositionToGridPosition -- the post-intersection geometry (local-space transform,
 *       floor-vs-round, bounds check) -- directly, with hand-picked world-space points and no
 *       PlayerController involved at all; and
 *   (b) RTACScreenToGridPosition's two guard branches that ARE reachable without a real camera:
 *       a null PlayerController, and a spawned-but-unpossessed one (no LocalPlayer), which fails
 *       the real DeprojectScreenPositionToWorld call rather than a stubbed one.
 * The full deprojection-to-hit-point path remains genuinely untested pending Decision #15.
 *
 * WORLD-SPACE INPUTS, NOT HAND-DERIVED TRIG. Board is placed at a deliberately non-identity
 * transform (nonzero Location, 90-degree Yaw) so InverseTransformPosition is actually exercised
 * rather than trivially passing at the world origin. Each case's WorldHit is computed at runtime
 * as Board->GetActorTransform().TransformPosition(LocalPoint) for a hand-chosen LocalPoint, not
 * derived by hand through trigonometry. This still catches a real regression: a bug that dropped
 * rotation, swapped TransformPosition for its own inverse, or ignored Board's Location would
 * recover a DIFFERENT LocalPoint than the one used to build the input, landing on a wrong tile or
 * a wrong bounds result (Failure Mode 8 — a test that cannot fail is not evidence).
 *
 * Tile (Row, Column)'s local region is X in [Column*TileSize, (Column+1)*TileSize),
 * Y in [Row*TileSize, (Row+1)*TileSize), per RTACGridToLocalOffset's own doc that a tile's
 * offset is its corner.
 *
 * WHY A UWORLD. ARTACBoard is an AActor and the guard-clause cases need a real (if unpossessed)
 * APlayerController, so this test needs a world to spawn into -- the first RTAC test that does.
 * The create/teardown sequence below mirrors Epic's own CQTest FActorTestSpawner::CreateWorld()
 * (Engine/Source/Developer/CQTest/Private/Components/ActorTestSpawner.cpp), inlined rather than
 * taking a dependency on the CQTest module, since RTAC.Build.cs needs no new dependency for it —
 * everything used here already comes from the Engine module RTAC already depends on.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTACGridConversionScreenToGridPositionTest,
	"RTAC.Presentation.GridConversion.ScreenToGridPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTACGridConversionScreenToGridPositionTest::RunTest(const FString& Parameters)
{
	int32 NumAssertions = 0;
	int32 NumPassed = 0;

	// Same assertion-mirroring pattern as every other RTAC test. See docs/reference.md ->
	// Automation Testing for why passes log at Log and failures at Warning.
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

	auto CheckTrue  = [&](const TCHAR* What, bool Value)                   { return Record(What, TestTrue(What, Value)); };
	auto CheckFalse = [&](const TCHAR* What, bool Value)                   { return Record(What, TestFalse(What, Value)); };
	auto CheckEqual = [&](const TCHAR* What, int32 Actual, int32 Expected) { return Record(What, TestEqual(What, Actual, Expected)); };

	UE_LOG(LogRTAC, Log, TEXT("=== RTAC.Presentation.GridConversion.ScreenToGridPosition — starting ==="));

	// --- World setup: create, spawn into, initialise for play ---
	const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass());
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
	World->AddToRoot();
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());

	// Board: deliberately non-identity transform (nonzero Location, 90-degree Yaw) so
	// InverseTransformPosition is actually exercised, not trivially passing at the world origin.
	const FVector BoardLocation(500.0f, -300.0f, 120.0f);
	const FRotator BoardRotation(0.0f, 90.0f, 0.0f); // Pitch 0, Yaw 90, Roll 0.
	ARTACBoard* Board = World->SpawnActor<ARTACBoard>(BoardLocation, BoardRotation);
	CheckTrue(TEXT("Setup: Board spawned"), Board != nullptr);

	FRTACGrid Grid;
	Grid.Init(FRTACGrid::DefaultRows, FRTACGrid::DefaultColumns); // 3x6, Decision #8.
	CheckTrue(TEXT("Setup: Grid initialised"), Grid.IsInitialized());

	const float TileSize = 100.0f;

	// A real, spawned PlayerController that is never possessed and never given a LocalPlayer --
	// used only for the two guard-clause cases below. GetLocalPlayer() is nullptr on it, for real,
	// not by a stub.
	APlayerController* BareController = World->SpawnActor<APlayerController>();
	CheckTrue(TEXT("Setup: bare PlayerController spawned"), BareController != nullptr);
	CheckTrue(TEXT("Setup: bare PlayerController control -- has no LocalPlayer"),
		BareController != nullptr && BareController->GetLocalPlayer() == nullptr);

	if (Board != nullptr)
	{
		// --- Case 1: tile center -- local (250,150,0), inside tile (1,2)'s region
		// [200,300) x [100,200). Reuses (1,2) as BasicLifecycle's own "known tile." ---
		{
			const FVector WorldHit = Board->GetActorTransform().TransformPosition(FVector(250.0f, 150.0f, 0.0f));
			FRTACGridPosition OutPosition;
			const bool bResult = RTACWorldPositionToGridPosition(WorldHit, *Board, Grid, TileSize, OutPosition);
			CheckTrue(TEXT("Tile center: resolves successfully"), bResult);
			CheckEqual(TEXT("Tile center: OutPosition.Row is 1"), OutPosition.Row, 1);
			CheckEqual(TEXT("Tile center: OutPosition.Column is 2"), OutPosition.Column, 2);
		}

		// --- Case 2: exact corner boundary -- local (300,200,0) is tile (2,3)'s own corner
		// (RTACGridToLocalOffset(2,3) returns exactly this). Proves the boundary belongs to the
		// tile whose corner it is, not the tile above or to the left. ---
		{
			const FVector WorldHit = Board->GetActorTransform().TransformPosition(FVector(300.0f, 200.0f, 0.0f));
			FRTACGridPosition OutPosition;
			const bool bResult = RTACWorldPositionToGridPosition(WorldHit, *Board, Grid, TileSize, OutPosition);
			CheckTrue(TEXT("Exact corner boundary: resolves successfully"), bResult);
			CheckEqual(TEXT("Exact corner boundary: OutPosition.Row is 2"), OutPosition.Row, 2);
			CheckEqual(TEXT("Exact corner boundary: OutPosition.Column is 3"), OutPosition.Column, 3);
		}

		// --- Case 3: floor-vs-round discriminator -- local (280,180,0), still inside tile
		// (1,2)'s region. floor() correctly gives (1,2); a round() regression would wrongly give
		// (2,3) (round(2.8)=3, round(1.8)=2). Case 2 alone can't catch this -- an exact multiple
		// rounds the same either way. ---
		{
			const FVector WorldHit = Board->GetActorTransform().TransformPosition(FVector(280.0f, 180.0f, 0.0f));
			FRTACGridPosition OutPosition;
			const bool bResult = RTACWorldPositionToGridPosition(WorldHit, *Board, Grid, TileSize, OutPosition);
			CheckTrue(TEXT("Floor-vs-round: resolves successfully"), bResult);
			CheckEqual(TEXT("Floor-vs-round: OutPosition.Row is 1, not rounded to 2"), OutPosition.Row, 1);
			CheckEqual(TEXT("Floor-vs-round: OutPosition.Column is 2, not rounded to 3"), OutPosition.Column, 2);
		}

		// --- Case 4: just outside the grid, upper bound -- local (600,150,0) -> Column = 6,
		// one past the last valid column (0..5). ---
		{
			const FVector WorldHit = Board->GetActorTransform().TransformPosition(FVector(600.0f, 150.0f, 0.0f));
			FRTACGridPosition OutPosition(-99, -99);
			const bool bResult = RTACWorldPositionToGridPosition(WorldHit, *Board, Grid, TileSize, OutPosition);
			CheckFalse(TEXT("Just outside grid (upper bound): returns false"), bResult);
			CheckEqual(TEXT("Just outside grid (upper bound): OutPosition.Row unmodified"), OutPosition.Row, -99);
			CheckEqual(TEXT("Just outside grid (upper bound): OutPosition.Column unmodified"), OutPosition.Column, -99);
		}

		// --- Case 5: just outside the grid, lower bound -- local (-50,150,0) -> Column = -1.
		// Both bounds directions tested, same discipline as BasicLifecycle's own two-directions
		// note: testing only the upper bound would let a missing `>= 0` half of a check pass
		// unnoticed. ---
		{
			const FVector WorldHit = Board->GetActorTransform().TransformPosition(FVector(-50.0f, 150.0f, 0.0f));
			FRTACGridPosition OutPosition(-99, -99);
			const bool bResult = RTACWorldPositionToGridPosition(WorldHit, *Board, Grid, TileSize, OutPosition);
			CheckFalse(TEXT("Just outside grid (lower bound): returns false"), bResult);
			CheckEqual(TEXT("Just outside grid (lower bound): OutPosition.Row unmodified"), OutPosition.Row, -99);
			CheckEqual(TEXT("Just outside grid (lower bound): OutPosition.Column unmodified"), OutPosition.Column, -99);
		}

		// --- Case 6: null PlayerController -- RTACScreenToGridPosition's own first guard. ---
		{
			FRTACGridPosition OutPosition(-99, -99);
			const bool bResult = RTACScreenToGridPosition(nullptr, FVector2D(0.0f, 0.0f), *Board, Grid, TileSize, OutPosition);
			CheckFalse(TEXT("Null PlayerController: RTACScreenToGridPosition returns false"), bResult);
			CheckEqual(TEXT("Null PlayerController: OutPosition.Row unmodified"), OutPosition.Row, -99);
			CheckEqual(TEXT("Null PlayerController: OutPosition.Column unmodified"), OutPosition.Column, -99);
		}

		// --- Case 7: deprojection fails entirely -- BareController has no LocalPlayer, so the
		// real DeprojectScreenPositionToWorld call fails before the board is ever touched. ---
		if (BareController != nullptr)
		{
			FRTACGridPosition OutPosition(-99, -99);
			const bool bResult = RTACScreenToGridPosition(BareController, FVector2D(400.0f, 300.0f), *Board, Grid, TileSize, OutPosition);
			CheckFalse(TEXT("Unpossessed PlayerController (no LocalPlayer): RTACScreenToGridPosition returns false"), bResult);
			CheckEqual(TEXT("Unpossessed PlayerController: OutPosition.Row unmodified"), OutPosition.Row, -99);
			CheckEqual(TEXT("Unpossessed PlayerController: OutPosition.Column unmodified"), OutPosition.Column, -99);
		}
	}

	// --- Teardown: mirrors CQTest's FActorTestSpawner destructor sequence. ---
	if (World->AreActorsInitialized())
	{
		for (FActorIterator It(World); It; ++It)
		{
			It->RouteEndPlay(EEndPlayReason::LevelTransition);
		}
	}
	GEngine->ShutdownWorldNetDriver(World);
	World->DestroyWorld(true);
	GEngine->DestroyWorldContext(World);
	World->RemoveFromRoot();

	if (NumPassed == NumAssertions)
	{
		UE_LOG(LogRTAC, Log,
			TEXT("=== RTAC.Presentation.GridConversion.ScreenToGridPosition — complete: %d/%d assertions passed ==="),
			NumPassed, NumAssertions);
	}
	else
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("=== RTAC.Presentation.GridConversion.ScreenToGridPosition — complete: %d/%d assertions passed, %d FAILED ==="),
			NumPassed, NumAssertions, NumAssertions - NumPassed);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
