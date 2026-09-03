// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "RTACModule.h" // LogRTAC — Rule 9: dedicated category, never LogTemp.
#include "RTACTestFixtures.h" // LegalityName + the symmetric-split board fixture, shared with the determinism test.
#include "Simulation/RTACEntity.h"
#include "Simulation/RTACGrid.h"
#include "Simulation/RTACGridPosition.h"
#include "Simulation/RTACMatchState.h"
#include "Simulation/RTACMovementLegality.h"
#include "Simulation/RTACSurfaceModifier.h"
#include "Simulation/RTACTile.h"
#include "Simulation/RTACTileOwner.h"

// LegalityName and ApplySymmetricSplitFixture used to live here, in this file's anonymous
// namespace. The determinism test (RTACDeterminismTest.cpp) needs both, and internal linkage put
// them out of its reach; they were hoisted into RTACTestFixtures.h rather than copied, per
// Failure Mode 7 ("one quantity, one authoritative location"). Their documentation moved with
// them unchanged — in particular the note that the split fixture is NOT Decision #10 Ruling 3's
// authoring mechanism and must never be promoted out of the test tree.

/**
 * Movement legality and resolution across a populated, multi-entity board.
 *
 * Phase 1's Definition of Done requires tests that "run against the full configured grid, never a
 * 1x1 or single-entity degenerate case (Failure Mode 5)." This is that test.
 *
 * WHY FOUR ENTITIES, TWO PER SIDE — the count is chosen, not arbitrary:
 *   - One entity collapses clause 2 to self-moves only; being blocked by ANOTHER entity, the case
 *     that actually matters, cannot occur at all.
 *   - One per side reaches the ownership boundary but cannot produce same-side blocking.
 *   - Two per side is the smallest configuration producing all three interaction kinds: same-side
 *     blocking, cross-boundary rejection, and cross-boundary rejection in BOTH directions — which
 *     is what guards against an ownership comparison that happens to work for one side only.
 *   - A fifth entity adds setup cost without adding a new kind of interaction.
 * Four entities on eighteen tiles also leaves enough free space that moves are not trivially
 * blocked, which would mask legal-path behaviour.
 *
 * CLAUSE ORDER IS LOAD-BEARING IN THE BOARD LAYOUT. RTACCheckMoveLegality is first-failure-wins in
 * a fixed order — bounds, occupied, owner, broken — so a tile that is both enemy-owned AND broken
 * can only ever report WrongOwner. The Broken case below therefore uses a broken tile inside the
 * MOVER'S OWN territory; anywhere else and clause 4 is never reached, and the test would report a
 * green Broken case it never actually exercised.
 *
 * CLAUSE LIVENESS. Failure Mode 1 is written about constants, but it generalises to clauses: one
 * that cannot change the outcome under any tested condition is indistinguishable from one that is
 * inert. Tile (1,2) is therefore attempted twice under directly comparable conditions — an Enemy
 * mover gets WrongOwner, a Player mover gets Legal — with the Enemy attempt first, while the tile
 * is still empty, because moving the Player there first would make the Enemy's attempt return
 * Occupied and silently stop testing ownership at all.
 *
 * ON THE THREE EXPECTED LogRTAC WARNINGS. Two deliberate spawn-failure cases make RTACSpawnEntity
 * log at Warning, and the InvalidOrigin case makes RTACResolveMove do the same — all three by
 * design (Rule 9), and all three provoked on purpose. They are not suppressed with
 * AddExpectedMessage: that API matches an exact occurrence count and feeds HasMetExpectedMessages(),
 * so a miscount would fail this test for a reason unrelated to movement. Warnings do not fail a
 * test on their own (bElevateLogWarningsToErrors defaults false), so they are left visible and
 * documented instead of made brittle.
 *
 * ---------------------------------------------------------------------------------
 * THIS TEST RENDERS AMBER/YELLOW IN THE SESSION FRONTEND, NOT GREEN. THAT IS EXPECTED, AND IT IS
 * NOT A FAILURE. DO NOT RE-DIAGNOSE IT.
 *
 * Diagnosed September 2, 2026, from log evidence and UE 5.8 engine source rather than from UI
 * convention. The automation framework reports two INDEPENDENT things per test: a result, and a
 * captured-event list. This test's result is Success — `LogAutomationController: Display: Test
 * Completed. Result={Success} Name={MultiEntity}` — and its 69/69 assertions all pass. Separately,
 * the three warnings above are captured as Warning-severity events inside this test's
 * BeginEvents/EndEvents block, each tagged `[log]` because they came from UE_LOG rather than an
 * explicit AddWarning. It is the only RTAC test with a non-empty event block.
 *
 * The colour follows from that event list, not from the result:
 *   - SAutomationGraphicalResultBox::GetColorForTestState (UE 5.8,
 *     Developer/AutomationWindow/Private/SAutomationGraphicalResultBox.cpp:296) returns
 *     FLinearColor(1.0, 0.5, 0.0) — amber — for EAutomationState::Success WHEN bHasWarnings, and
 *     FLinearColor(0.0, 0.5, 0.0) — green — for Success without. Failure is a separate dark red.
 *     Success-with-warnings is a genuine third colour, not a shade of failure.
 *   - SAutomationTestItem.cpp:1086 and :1133 swap the row's status icon to the "Automation.Warning"
 *     brush under the same condition — that is the yellow triangle.
 *   - Both are driven by FAutomationReport::HasWarnings(), which keys on GetWarningTotal() > 0.
 *
 * THE DISCRIMINATOR IS SEVERITY, NOT THE PRESENCE OF LOG OUTPUT. Every other RTAC test also writes
 * heavily to LogRTAC — one line per assertion — and all of them render green. Log-verbosity output
 * does not tint anything; only Warning and above does. RTAC.Simulation.Match.DeterministicReplay
 * is the direct control: ~56 LogRTAC lines, zero at Warning, green.
 *
 * Making this test green would require AddExpectedMessage, which is refused for the reason stated
 * in the paragraph above. The amber is the accepted cost of that choice.
 * ---------------------------------------------------------------------------------
 *
 * Simulation-layer only (Rule 5): plain structs and free functions, no engine objects, no
 * presentation layer. Lives inside the plugin per Rule 11.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTACMovementMultiEntityTest,
	"RTAC.Simulation.Movement.MultiEntity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTACMovementMultiEntityTest::RunTest(const FString& Parameters)
{
	int32 NumAssertions = 0;
	int32 NumPassed = 0;

	// Same assertion-mirroring pattern as the Grid and MatchState tests: purely additive logging
	// on top of the framework's own AddError()-driven pass/fail, with passes at Log (never
	// intercepted) and failures at Warning (the assertion's own AddError already registered the
	// failure; logging at Error would double-count it). See docs/reference.md → Automation Testing.
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

	auto CheckLegality = [&](const TCHAR* What, ERTACMoveLegality Actual, ERTACMoveLegality Expected)
	{
		const FString Message = FString::Printf(TEXT("%s (expected %s, got %s)"),
			What, RTACTest_LegalityName(Expected), RTACTest_LegalityName(Actual));
		return Record(*Message, TestTrue(*Message, Actual == Expected));
	};

	UE_LOG(LogRTAC, Log, TEXT("=== RTAC.Simulation.Movement.MultiEntity — starting ==="));

	// ---------------------------------------------------------------------------------
	// Board setup — the full configured grid, never a degenerate one (Failure Mode 5).
	// Dimensions are referenced from FRTACGrid, never restated as 3 and 6: two copies of one
	// number is exactly the drift Failure Mode 7 warns about.
	// ---------------------------------------------------------------------------------
	FRTACMatchState State;
	State.Initialize(9001);
	State.Grid.Init(FRTACGrid::DefaultRows, FRTACGrid::DefaultColumns);

	CheckTrue(TEXT("Setup: the grid initialised at the configured default dimensions"),
		State.Grid.IsInitialized());
	CheckEqual(TEXT("Setup: the board holds Rows x Columns tiles"),
		State.Grid.NumTiles(), FRTACGrid::DefaultRows * FRTACGrid::DefaultColumns);
	CheckTrue(TEXT("Setup: the board is larger than one tile, so no degenerate case (Failure Mode 5)"),
		State.Grid.NumTiles() > 1);

	RTACTest_ApplySymmetricSplitFixture(State.Grid);

	// Control on the fixture itself. Without real ownership every tile stays Neutral, every move
	// passes clause 3 vacuously, and the WrongOwner cases below would be testing nothing.
	CheckTrue(TEXT("Fixture control: a left-half tile is Player-owned, not the Neutral default"),
		State.Grid.GetTileChecked(1, 1).Owner == ERTACTileOwner::Player);
	CheckTrue(TEXT("Fixture control: a right-half tile is Enemy-owned, not the Neutral default"),
		State.Grid.GetTileChecked(1, 4).Owner == ERTACTileOwner::Enemy);

	// One broken tile, inside PLAYER territory — see the clause-order note in this test's header.
	State.Grid.GetTileChecked(2, 0).SurfaceModifier = ERTACSurfaceModifier::Broken;
	CheckTrue(TEXT("Fixture control: the broken tile sits in the mover's OWN territory, so clause 4 is reachable"),
		State.Grid.GetTileChecked(2, 0).Owner == ERTACTileOwner::Player
		&& State.Grid.GetTileChecked(2, 0).SurfaceModifier == ERTACSurfaceModifier::Broken);

	// ---------------------------------------------------------------------------------
	// Entities — four, two per side, all placed through RTACSpawnEntity so the grid/entity
	// consistency invariant is established by the one function that owns it (Decision #11
	// Ruling 4) rather than by hand.
	// ---------------------------------------------------------------------------------
	const FName ArchetypeId(TEXT("Sentinel")); // A KIND, not an instance label — Decision #9.

	const int32 PlayerA = RTACSpawnEntity(State, FRTACGridPosition(1, 0), ERTACTileOwner::Player, ArchetypeId);
	const int32 PlayerB = RTACSpawnEntity(State, FRTACGridPosition(1, 1), ERTACTileOwner::Player, ArchetypeId);
	const int32 EnemyA  = RTACSpawnEntity(State, FRTACGridPosition(1, 3), ERTACTileOwner::Enemy,  ArchetypeId);
	const int32 EnemyB  = RTACSpawnEntity(State, FRTACGridPosition(0, 4), ERTACTileOwner::Enemy,  ArchetypeId);

	CheckEqual(TEXT("Spawn: first entity is id 0, per Decision #9's spawn-order counter"), PlayerA, 0);
	CheckEqual(TEXT("Spawn: second entity is id 1"), PlayerB, 1);
	CheckEqual(TEXT("Spawn: third entity is id 2"), EnemyA, 2);
	CheckEqual(TEXT("Spawn: fourth entity is id 3"), EnemyB, 3);
	CheckEqual(TEXT("Spawn: four entities are on the board"), State.Entities.Num(), 4);
	CheckEqual(TEXT("Spawn: NextEntityId has advanced to 4"), State.NextEntityId, 4);
	CheckTrue(TEXT("Spawn: more than one entity, so no single-entity degenerate case (Failure Mode 5)"),
		State.Entities.Num() > 1);

	// Counted per side rather than inferred from the total. Entities.Num() / 2 would report "2"
	// for four entities all on one side, which is precisely the configuration this test exists to
	// avoid — an assertion that cannot distinguish the degenerate case from the intended one is
	// not evidence (Failure Mode 8).
	int32 NumPlayerSide = 0;
	int32 NumEnemySide = 0;
	for (const FRTACEntity& Entity : State.Entities)
	{
		NumPlayerSide += (Entity.Side == ERTACTileOwner::Player) ? 1 : 0;
		NumEnemySide  += (Entity.Side == ERTACTileOwner::Enemy)  ? 1 : 0;
	}
	CheckEqual(TEXT("Spawn: exactly two entities are on the Player side"), NumPlayerSide, 2);
	CheckEqual(TEXT("Spawn: exactly two entities are on the Enemy side"), NumEnemySide, 2);

	// Spawn writes BOTH halves of the invariant — the entity's Position and the tile's occupancy.
	CheckEqual(TEXT("Spawn: the tile under the first entity records that entity's id"),
		State.Grid.GetTileChecked(1, 0).OccupantEntityId, PlayerA);
	CheckEqual(TEXT("Spawn: the tile under the fourth entity records that entity's id"),
		State.Grid.GetTileChecked(0, 4).OccupantEntityId, EnemyB);

	// --- Spawn refuses to break the invariant it exists to establish (Decision #11 Ruling 4) ---
	// Both of these log a LogRTAC Warning by design. See this test's header on why they are not
	// suppressed with AddExpectedMessage.
	const int32 RejectedOccupied = RTACSpawnEntity(State, FRTACGridPosition(1, 0), ERTACTileOwner::Player, ArchetypeId);
	CheckEqual(TEXT("Spawn: spawning onto an occupied tile is refused"), RejectedOccupied, INDEX_NONE);
	CheckEqual(TEXT("Spawn: a refused spawn does not advance NextEntityId (Rule 6 — id sequences stay comparable)"),
		State.NextEntityId, 4);
	CheckEqual(TEXT("Spawn: a refused spawn adds no entity"), State.Entities.Num(), 4);
	CheckEqual(TEXT("Spawn: the occupied tile still holds its ORIGINAL occupant, not the refused one"),
		State.Grid.GetTileChecked(1, 0).OccupantEntityId, PlayerA);

	const int32 RejectedOffBoard = RTACSpawnEntity(State, FRTACGridPosition(-1, 0), ERTACTileOwner::Player, ArchetypeId);
	CheckEqual(TEXT("Spawn: spawning off the board is refused"), RejectedOffBoard, INDEX_NONE);
	CheckEqual(TEXT("Spawn: an off-board refusal does not advance NextEntityId"), State.NextEntityId, 4);

	// --- FindEntity: the only supported id-to-entity path (Decision #11 Ruling 3) ---
	const FRTACEntity* FoundPlayerA = State.FindEntity(PlayerA);
	if (CheckTrue(TEXT("FindEntity: finds a spawned entity by its id"), FoundPlayerA != nullptr))
	{
		// Guarded: an unguarded dereference would crash the run instead of failing it cleanly.
		CheckEqual(TEXT("FindEntity: the entity found by id reports that same id"),
			FoundPlayerA->EntityId, PlayerA);
		CheckTrue(TEXT("FindEntity: the entity found by id is at the position it was spawned at"),
			FoundPlayerA->Position == FRTACGridPosition(1, 0));
		CheckTrue(TEXT("FindEntity: the entity found by id carries the side it was spawned with"),
			FoundPlayerA->Side == ERTACTileOwner::Player);
	}
	CheckTrue(TEXT("FindEntity: returns nullptr for INDEX_NONE, which is never a real id"),
		State.FindEntity(INDEX_NONE) == nullptr);
	CheckTrue(TEXT("FindEntity: returns nullptr for an id no entity holds"),
		State.FindEntity(9999) == nullptr);

	// ---------------------------------------------------------------------------------
	// Invariant sweep — every entity's tile records that entity. This is the invariant
	// RTACSpawnEntity exists to establish and RTACResolveMove assumes without verifying, so it is
	// re-checked after every successful move below rather than trusted once.
	// ---------------------------------------------------------------------------------
	auto CheckBoardConsistent = [&](const TCHAR* What) -> bool
	{
		bool bConsistent = true;
		for (const FRTACEntity& Entity : State.Entities)
		{
			const FRTACTile* Tile = State.Grid.FindTile(Entity.Position);
			if (Tile == nullptr || Tile->OccupantEntityId != Entity.EntityId)
			{
				bConsistent = false;
				break;
			}
		}
		return Record(What, TestTrue(What, bConsistent));
	};

	CheckBoardConsistent(TEXT("Board consistency: after setup, every entity's tile records that entity's id"));

	// ---------------------------------------------------------------------------------
	// Rejected moves. Each asserts the returned clause AND that nothing was applied — the
	// no-partial-application guarantee RTACResolveMove's header states three times and which
	// nothing tested before this.
	// ---------------------------------------------------------------------------------
	auto AttemptRejectedMove = [&](const TCHAR* What, int32 MoverId, FRTACGridPosition Destination, ERTACMoveLegality Expected)
	{
		FRTACEntity* Mover = State.FindEntity(MoverId);

		const FString FoundMessage = FString::Printf(TEXT("%s — the mover is findable by id"), What);
		if (!Record(*FoundMessage, TestTrue(*FoundMessage, Mover != nullptr)))
		{
			return;
		}

		// Capture everything the move could touch, before touching it.
		const FRTACGridPosition PositionBefore = Mover->Position;
		const FRTACTile* OriginTile = State.Grid.FindTile(PositionBefore);
		const int32 OriginOccupantBefore = OriginTile != nullptr ? OriginTile->OccupantEntityId : INDEX_NONE;
		const FRTACTile* DestinationTile = State.Grid.FindTile(Destination);
		const int32 DestinationOccupantBefore = DestinationTile != nullptr ? DestinationTile->OccupantEntityId : INDEX_NONE;

		const ERTACMoveLegality Result = RTACResolveMove(*Mover, Destination, State.Grid);
		CheckLegality(What, Result, Expected);

		const FString PositionMessage = FString::Printf(TEXT("%s — no partial application: the mover's Position is unchanged"), What);
		Record(*PositionMessage, TestTrue(*PositionMessage, Mover->Position == PositionBefore));

		const FRTACTile* OriginTileAfter = State.Grid.FindTile(PositionBefore);
		const int32 OriginOccupantAfter = OriginTileAfter != nullptr ? OriginTileAfter->OccupantEntityId : INDEX_NONE;
		const FString OriginMessage = FString::Printf(TEXT("%s — no partial application: the origin tile's occupant is unchanged"), What);
		Record(*OriginMessage, TestTrue(*OriginMessage, OriginOccupantAfter == OriginOccupantBefore));

		const FRTACTile* DestinationTileAfter = State.Grid.FindTile(Destination);
		const int32 DestinationOccupantAfter = DestinationTileAfter != nullptr ? DestinationTileAfter->OccupantEntityId : INDEX_NONE;
		const FString DestinationMessage = FString::Printf(TEXT("%s — no partial application: the destination tile's occupant is unchanged"), What);
		Record(*DestinationMessage, TestTrue(*DestinationMessage, DestinationOccupantAfter == DestinationOccupantBefore));
	};

	// Clause 1 — off the left edge of the board.
	AttemptRejectedMove(TEXT("OutOfBounds: a Player steps off the left edge"),
		PlayerA, FRTACGridPosition(1, -1), ERTACMoveLegality::OutOfBounds);

	// Clause 2 — onto a teammate. Needs a SECOND entity to exist at all; this is the case a
	// single-entity test cannot produce.
	AttemptRejectedMove(TEXT("Occupied: a Player steps onto its own teammate"),
		PlayerA, FRTACGridPosition(1, 1), ERTACMoveLegality::Occupied);

	// Clause 4 — a broken tile inside the mover's own territory, so clauses 1-3 all pass first and
	// clause 4 is genuinely the one that rejects.
	AttemptRejectedMove(TEXT("Broken: a Player steps onto a broken tile in its OWN territory"),
		PlayerA, FRTACGridPosition(2, 0), ERTACMoveLegality::Broken);

	// Clause 3, liveness half A — an Enemy reaching for an empty PLAYER tile. Runs BEFORE the
	// Player's own move to the same tile, while it is still empty; reversed, this would return
	// Occupied and test nothing about ownership.
	AttemptRejectedMove(TEXT("WrongOwner: an Enemy steps onto an empty Player-owned tile (1,2)"),
		EnemyA, FRTACGridPosition(1, 2), ERTACMoveLegality::WrongOwner);

	// ---------------------------------------------------------------------------------
	// Legal moves. The first is clause 3's liveness half B: the SAME destination that just
	// returned WrongOwner for an Enemy now returns Legal for a Player.
	// ---------------------------------------------------------------------------------
	{
		FRTACEntity* Mover = State.FindEntity(PlayerB);
		if (CheckTrue(TEXT("Legal: the Player mover is findable by id"), Mover != nullptr))
		{
			const ERTACMoveLegality Result = RTACResolveMove(*Mover, FRTACGridPosition(1, 2), State.Grid);
			CheckLegality(TEXT("Legal: a Player steps onto the SAME tile (1,2) that rejected the Enemy — clause 3 is live, not inert"),
				Result, ERTACMoveLegality::Legal);
			CheckTrue(TEXT("Legal: the mover's Position advanced to the destination"),
				Mover->Position == FRTACGridPosition(1, 2));
			CheckEqual(TEXT("Legal: the destination tile now records the mover's id"),
				State.Grid.GetTileChecked(1, 2).OccupantEntityId, PlayerB);
			CheckFalse(TEXT("Legal: the origin tile was vacated"),
				State.Grid.GetTileChecked(1, 1).IsOccupied());
		}
	}
	CheckBoardConsistent(TEXT("Board consistency: after the Player's legal move, every entity's tile records that entity's id"));

	// The mirror case: an Enemy moving inside ENEMY territory is legal too. Without this, a
	// clause 3 that only ever worked for one side would pass everything above.
	{
		FRTACEntity* Mover = State.FindEntity(EnemyB);
		if (CheckTrue(TEXT("Legal: the Enemy mover is findable by id"), Mover != nullptr))
		{
			const ERTACMoveLegality Result = RTACResolveMove(*Mover, FRTACGridPosition(0, 3), State.Grid);
			CheckLegality(TEXT("Legal: an Enemy steps within its own territory — ownership works in BOTH directions"),
				Result, ERTACMoveLegality::Legal);
			CheckTrue(TEXT("Legal: the Enemy mover's Position advanced to the destination"),
				Mover->Position == FRTACGridPosition(0, 3));
			CheckEqual(TEXT("Legal: the Enemy's destination tile records its id"),
				State.Grid.GetTileChecked(0, 3).OccupantEntityId, EnemyB);
			CheckFalse(TEXT("Legal: the Enemy's origin tile was vacated"),
				State.Grid.GetTileChecked(0, 4).IsOccupied());
		}
	}
	CheckBoardConsistent(TEXT("Board consistency: after the Enemy's legal move, every entity's tile records that entity's id"));

	// ---------------------------------------------------------------------------------
	// InvalidOrigin — deliberately ISOLATED from the board.
	//
	// The mover is a local entity that was never spawned and is never added to State.Entities, so
	// the malformed state it represents cannot reach the consistency sweep above. RTACSpawnEntity
	// could not have produced it: spawning off-board is refused, which is the point — the only way
	// to reach InvalidOrigin is to hand-assemble the exact state spawn exists to prevent.
	//
	// The destination must be genuinely Legal — in bounds, empty, Player-owned, unbroken — or
	// RTACCheckMoveLegality rejects it first and the origin precondition is never reached.
	// ---------------------------------------------------------------------------------
	{
		FRTACEntity Stray;
		Stray.EntityId = 4242;
		Stray.Side = ERTACTileOwner::Player;
		Stray.Position = FRTACGridPosition(-1, -1);
		Stray.ArchetypeId = ArchetypeId;

		const FRTACGridPosition StrayDestination(2, 1);
		CheckTrue(TEXT("InvalidOrigin setup: the destination is a legal Player tile, so the origin check is what rejects"),
			State.Grid.GetTileChecked(2, 1).Owner == ERTACTileOwner::Player
			&& !State.Grid.GetTileChecked(2, 1).IsOccupied()
			&& State.Grid.GetTileChecked(2, 1).SurfaceModifier != ERTACSurfaceModifier::Broken);

		const ERTACMoveLegality Result = RTACResolveMove(Stray, StrayDestination, State.Grid);
		CheckLegality(TEXT("InvalidOrigin: a mover whose own Position is off-board is hard-rejected"),
			Result, ERTACMoveLegality::InvalidOrigin);
		CheckTrue(TEXT("InvalidOrigin: no partial application — the stray mover's Position is unchanged"),
			Stray.Position == FRTACGridPosition(-1, -1));
		CheckFalse(TEXT("InvalidOrigin: no partial application — the destination tile was not occupied"),
			State.Grid.GetTileChecked(2, 1).IsOccupied());

		// RTACCheckMoveLegality inspects the destination alone and can never return InvalidOrigin,
		// however malformed the mover is. Same inputs, different answer — this is the asymmetry
		// ERTACMoveLegality's own documentation calls out, asserted rather than assumed.
		const ERTACMoveLegality CheckOnly = RTACCheckMoveLegality(Stray, StrayDestination, State.Grid);
		CheckLegality(TEXT("InvalidOrigin: RTACCheckMoveLegality never returns it — it reports the destination as Legal"),
			CheckOnly, ERTACMoveLegality::Legal);
	}

	CheckBoardConsistent(TEXT("Board consistency: the isolated InvalidOrigin case left the real board untouched"));
	CheckEqual(TEXT("The isolated stray was never added to the match's entity array"), State.Entities.Num(), 4);

	if (NumPassed == NumAssertions)
	{
		UE_LOG(LogRTAC, Log,
			TEXT("=== RTAC.Simulation.Movement.MultiEntity — complete: %d/%d assertions passed ==="),
			NumPassed, NumAssertions);
	}
	else
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("=== RTAC.Simulation.Movement.MultiEntity — complete: %d/%d assertions passed, %d FAILED ==="),
			NumPassed, NumAssertions, NumAssertions - NumPassed);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
