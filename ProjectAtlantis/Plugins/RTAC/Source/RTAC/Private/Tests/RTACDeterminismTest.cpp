// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "RTACModule.h" // LogRTAC — Rule 9: dedicated category, never LogTemp.
#include "RTACTestFixtures.h" // Shared with RTACMovementTest.cpp — see that header on why it exists.
#include "Simulation/RTACEntity.h"
#include "Simulation/RTACGrid.h"
#include "Simulation/RTACGridPosition.h"
#include "Simulation/RTACMatchState.h"
#include "Simulation/RTACMovementLegality.h"
#include "Simulation/RTACSurfaceModifier.h"
#include "Simulation/RTACTile.h"
#include "Simulation/RTACTileOwner.h"

namespace
{
	// =================================================================================
	// DIRECTION CONVENTION — DEFINED HERE, TEST-LOCAL, AND NOWHERE ELSE.
	//
	// DOMAIN (Rule 10): a direction is a delta in GRID SPACE — a (Row, Column) index offset per
	// Decision #5. It is never a world-space vector, never a screen-space direction, and never a
	// camera-relative one. The isometric camera (Decision #1) does not exist yet and does not
	// participate; when it does, screen-space input becomes grid-space through a projection owned
	// by Phase 2's presentation layer (PHASES.md Phase 2, Rule 10), not by anything here.
	//
	// NOTHING IN THE SIMULATION DEFINES THIS, AND NOTHING SHOULD. RTAC has no notion of "up." It
	// has RTACResolveMove(Entity, Destination, Grid) — an absolute destination and no direction
	// vocabulary at all. Decision #12 Ruling 3 names the four directions while deriving what
	// adjacency means, but derives a DISTANCE rule from them and adds no direction type. This enum
	// and the mapping below are therefore this test's own convention, defined so the test can speak
	// in relative terms; Phase 2's input layer owns the real one, and if it picks a different
	// mapping that is not a conflict with this file.
	//
	// THE MAPPING, stated so it cannot be inferred wrongly from the names:
	//
	//     Up    -> Row - 1        Row 0 is the top row.
	//     Down  -> Row + 1
	//     Left  -> Column - 1     Column 0 is the leftmost column.
	//     Right -> Column + 1
	//
	// Every delta changes exactly one axis by exactly one, so every destination this test produces
	// is at Manhattan distance 1 from its origin. That is deliberate and load-bearing for the
	// future: Decision #12's `NotAdjacent` outcome is decided but NOT YET ENACTED in
	// RTACResolveMove, and when it is enacted, no move in this test becomes non-adjacent. This
	// test's expected results survive that change untouched.
	// =================================================================================
	enum class ETestMoveDirection : uint8
	{
		Up,
		Down,
		Left,
		Right
	};

	const TCHAR* DirectionName(ETestMoveDirection Direction)
	{
		switch (Direction)
		{
		case ETestMoveDirection::Up:    return TEXT("Up");
		case ETestMoveDirection::Down:  return TEXT("Down");
		case ETestMoveDirection::Left:  return TEXT("Left");
		case ETestMoveDirection::Right: return TEXT("Right");
		}
		return TEXT("<unrecognised>");
	}

	/** Applies the convention above: origin + direction -> destination, in grid space. */
	FRTACGridPosition ApplyDirection(FRTACGridPosition Origin, ETestMoveDirection Direction)
	{
		switch (Direction)
		{
		case ETestMoveDirection::Up:    return FRTACGridPosition(Origin.Row - 1, Origin.Column);
		case ETestMoveDirection::Down:  return FRTACGridPosition(Origin.Row + 1, Origin.Column);
		case ETestMoveDirection::Left:  return FRTACGridPosition(Origin.Row, Origin.Column - 1);
		case ETestMoveDirection::Right: return FRTACGridPosition(Origin.Row, Origin.Column + 1);
		}
		return Origin;
	}

	/**
	 * One input event: which entity acts, and which way it tries to step.
	 *
	 * RELATIVE, NOT ABSOLUTE — the whole point of the sequence's shape. A list of absolute
	 * destinations would be self-correcting: if run B's entity ended up one tile off after event 3,
	 * event 4 would still name the same absolute tile and the runs could silently re-converge. A
	 * relative direction is resolved against wherever the entity ACTUALLY is at that moment, so a
	 * single divergence propagates into every destination computed after it. That is the property
	 * that makes a replay comparison mean something.
	 */
	struct FTestMoveInput
	{
		int32 EntityId = INDEX_NONE;
		ETestMoveDirection Direction = ETestMoveDirection::Up;
	};

	/**
	 * Builds the match from nothing. Called once per run, never copied between runs.
	 *
	 * THIS IS THE FUNCTION THAT MAKES THE TEST MEAN ANYTHING. Run B must be BUILT, not cloned:
	 * copying run A's state into B and replaying would compare a state against itself and pass no
	 * matter how non-deterministic setup or resolution actually is — an experiment that cannot fail
	 * is not evidence (Failure Mode 8). Both runs therefore call this, from a default-constructed
	 * FRTACMatchState, with the same master seed.
	 *
	 * State.Initialize() is relied on as a complete standalone reinitialisation per Rule 6. It is
	 * called on a freshly default-constructed state here, so nothing in this fixture depends on that
	 * property — but every member it touches (Rng.MasterSeed, Grid, Entities, NextEntityId) is
	 * already covered against a USED state by RTAC.Simulation.Rng.MatchStateLifecycle, and this
	 * fixture exercises no member beyond those four, so that test needs no extension for this one.
	 *
	 * Board and entity layout follow RTACMovementTest.cpp's, deliberately: the full configured 3x6
	 * grid (dimensions referenced from FRTACGrid, never restated — Failure Mode 7), four entities
	 * two per side, one broken tile inside PLAYER territory so the Broken outcome is reachable
	 * rather than pre-empted by WrongOwner. Dimensions and ownership come from the shared fixture
	 * header, so there is one statement of "how a test board is arranged," not two.
	 */
	void BuildFixture(FRTACMatchState& State, int32 MasterSeed)
	{
		State.Initialize(MasterSeed);
		State.Grid.Init(FRTACGrid::DefaultRows, FRTACGrid::DefaultColumns);
		RTACTest_ApplySymmetricSplitFixture(State.Grid);

		// Player territory, so the movement-legality check reaches clause 4 instead of stopping at
		// clause 3. Same reasoning as the multi-entity test's header records at length.
		State.Grid.GetTileChecked(2, 0).SurfaceModifier = ERTACSurfaceModifier::Broken;

		const FName ArchetypeId(TEXT("Sentinel")); // A KIND, not an instance label — Decision #9.

		RTACSpawnEntity(State, FRTACGridPosition(1, 0), ERTACTileOwner::Player, ArchetypeId); // id 0
		RTACSpawnEntity(State, FRTACGridPosition(1, 1), ERTACTileOwner::Player, ArchetypeId); // id 1
		RTACSpawnEntity(State, FRTACGridPosition(1, 3), ERTACTileOwner::Enemy,  ArchetypeId); // id 2
		RTACSpawnEntity(State, FRTACGridPosition(0, 4), ERTACTileOwner::Enemy,  ArchetypeId); // id 3
	}

	/**
	 * Replays an input sequence against a match state, returning the ordered result codes.
	 *
	 * The result sequence is a first-class output, not a by-product. Two runs could in principle
	 * diverge mid-sequence and re-converge on an identical final state — this test proves that is
	 * not hypothetical, see the "reconvergence" run below — so comparing final states alone would
	 * report a clean pass over a genuine divergence.
	 *
	 * OutMissingMovers counts inputs naming an id no entity holds. It must be zero: a missing mover
	 * would silently shorten the result sequence and desynchronise the comparison against every
	 * later event, so the count is surfaced rather than folded into a legality value that would
	 * conflate "the mover isn't there" with "the move was rejected."
	 */
	TArray<ERTACMoveLegality> RunInputSequence(FRTACMatchState& State, const TArray<FTestMoveInput>& Inputs, int32& OutMissingMovers)
	{
		TArray<ERTACMoveLegality> Results;
		Results.Reserve(Inputs.Num());
		OutMissingMovers = 0;

		for (const FTestMoveInput& Input : Inputs)
		{
			// FindEntity is the only supported id-to-entity path (Decision #11 Ruling 3). The returned
			// pointer stays valid across RTACResolveMove: nothing here spawns or removes, so Entities
			// never reallocates underneath it.
			FRTACEntity* Mover = State.FindEntity(Input.EntityId);
			if (Mover == nullptr)
			{
				++OutMissingMovers;
				continue;
			}

			// The destination is computed from the mover's CURRENT position, every time. This is where
			// a divergence compounds.
			const FRTACGridPosition Destination = ApplyDirection(Mover->Position, Input.Direction);
			Results.Add(RTACResolveMove(*Mover, Destination, State.Grid));
		}

		return Results;
	}

	// =================================================================================
	// STATE COMPARISON — FIELD BY FIELD, BY HAND.
	//
	// NOT FMemory::Memcmp. Two reasons, either one fatal:
	//   - Struct padding is uninitialised and compares as garbage, producing failures with no
	//     defect behind them.
	//   - FRTACGrid holds a TArray and FRTACMatchState holds another; a byte comparison would be
	//     comparing heap POINTERS, which are guaranteed to differ between two independently built
	//     runs and say nothing at all about contents.
	//
	// NOT a bool operator==, either — and specifically not one added to the production structs. A
	// bool answers "did they differ" and cannot answer "which field, in which entity or tile," which
	// is the only answer that makes a determinism failure diagnosable. Adding operator== to
	// FRTACEntity/FRTACTile/FRTACMatchState would also be production API existing solely to serve a
	// test; nothing in the simulation compares two match states.
	//
	// FIELD-TOTAL AS OF TODAY, AND THAT IS A MAINTENANCE OBLIGATION. Every data member of
	// FRTACMatchState, FRTACGrid, FRTACTile and FRTACEntity is compared below. ADDING A FIELD TO ANY
	// OF THOSE STRUCTS WITHOUT ADDING IT HERE SILENTLY NARROWS THIS TEST — it will keep passing while
	// checking less. There is no compile-time guard against that; this comment is the guard.
	// =================================================================================

	/** Returns an empty string when the two states are identical, or a description of the FIRST difference found. */
	FString FindFirstStateDifference(const FRTACMatchState& Left, const FRTACMatchState& Right)
	{
		// --- Match-level ---
		if (Left.Rng.MasterSeed != Right.Rng.MasterSeed)
		{
			return FString::Printf(TEXT("Rng.MasterSeed: %d vs %d"), Left.Rng.MasterSeed, Right.Rng.MasterSeed);
		}
		if (Left.NextEntityId != Right.NextEntityId)
		{
			return FString::Printf(TEXT("NextEntityId: %d vs %d"), Left.NextEntityId, Right.NextEntityId);
		}
		if (Left.Grid.GetRows() != Right.Grid.GetRows())
		{
			return FString::Printf(TEXT("Grid rows: %d vs %d"), Left.Grid.GetRows(), Right.Grid.GetRows());
		}
		if (Left.Grid.GetColumns() != Right.Grid.GetColumns())
		{
			return FString::Printf(TEXT("Grid columns: %d vs %d"), Left.Grid.GetColumns(), Right.Grid.GetColumns());
		}
		if (Left.Grid.NumTiles() != Right.Grid.NumTiles())
		{
			return FString::Printf(TEXT("Grid tile count: %d vs %d"), Left.Grid.NumTiles(), Right.Grid.NumTiles());
		}

		// --- Entities ---
		// Compared IN ARRAY ORDER, index against index, not matched up by id through FindEntity.
		// Array order is itself part of what determinism means here: Decision #11 Ruling 2 chose
		// TArray over TMap precisely because it gives "one unambiguous iteration order," and Rule 6's
		// same-seed-same-result requirement rests on that. Matching by id would hide a run that
		// produced the right entities in a different order — the exact failure the storage choice was
		// made to prevent.
		if (Left.Entities.Num() != Right.Entities.Num())
		{
			return FString::Printf(TEXT("Entity count: %d vs %d"), Left.Entities.Num(), Right.Entities.Num());
		}
		for (int32 Index = 0; Index < Left.Entities.Num(); ++Index)
		{
			const FRTACEntity& A = Left.Entities[Index];
			const FRTACEntity& B = Right.Entities[Index];

			if (A.EntityId != B.EntityId)
			{
				return FString::Printf(TEXT("Entities[%d].EntityId: %d vs %d"), Index, A.EntityId, B.EntityId);
			}
			if (A.Position.Row != B.Position.Row)
			{
				return FString::Printf(TEXT("Entities[%d].Position.Row: %d vs %d"), Index, A.Position.Row, B.Position.Row);
			}
			if (A.Position.Column != B.Position.Column)
			{
				return FString::Printf(TEXT("Entities[%d].Position.Column: %d vs %d"), Index, A.Position.Column, B.Position.Column);
			}
			if (A.Side != B.Side)
			{
				return FString::Printf(TEXT("Entities[%d].Side: %s vs %s"), Index,
					RTACTest_TileOwnerName(A.Side), RTACTest_TileOwnerName(B.Side));
			}
			if (A.ArchetypeId != B.ArchetypeId)
			{
				return FString::Printf(TEXT("Entities[%d].ArchetypeId: %s vs %s"), Index,
					*A.ArchetypeId.ToString(), *B.ArchetypeId.ToString());
			}
		}

		// --- Tiles ---
		// Also index-by-index over the flat row-major array, for the same reason: FRTACGrid::Tiles is
		// one flat array specifically so the iteration order is unambiguous, and comparing in that
		// order checks the order too.
		const TArray<FRTACTile>& LeftTiles = Left.Grid.GetTiles();
		const TArray<FRTACTile>& RightTiles = Right.Grid.GetTiles();
		for (int32 Index = 0; Index < LeftTiles.Num(); ++Index)
		{
			const FRTACTile& A = LeftTiles[Index];
			const FRTACTile& B = RightTiles[Index];

			if (A.Position.Row != B.Position.Row || A.Position.Column != B.Position.Column)
			{
				return FString::Printf(TEXT("Tiles[%d].Position: (%d,%d) vs (%d,%d)"), Index,
					A.Position.Row, A.Position.Column, B.Position.Row, B.Position.Column);
			}
			if (A.OccupantEntityId != B.OccupantEntityId)
			{
				return FString::Printf(TEXT("Tile (%d,%d) OccupantEntityId: %d vs %d"),
					A.Position.Row, A.Position.Column, A.OccupantEntityId, B.OccupantEntityId);
			}
			if (A.SurfaceModifier != B.SurfaceModifier)
			{
				return FString::Printf(TEXT("Tile (%d,%d) SurfaceModifier: %s vs %s"),
					A.Position.Row, A.Position.Column,
					RTACTest_SurfaceModifierName(A.SurfaceModifier), RTACTest_SurfaceModifierName(B.SurfaceModifier));
			}
			if (A.Owner != B.Owner)
			{
				return FString::Printf(TEXT("Tile (%d,%d) Owner: %s vs %s"),
					A.Position.Row, A.Position.Column,
					RTACTest_TileOwnerName(A.Owner), RTACTest_TileOwnerName(B.Owner));
			}
			if (A.Elevation != B.Elevation)
			{
				return FString::Printf(TEXT("Tile (%d,%d) Elevation: %d vs %d"),
					A.Position.Row, A.Position.Column, A.Elevation, B.Elevation);
			}
		}

		return FString();
	}

	/** Returns an empty string when the two result sequences are identical, or the FIRST difference. */
	FString FindFirstResultDifference(const TArray<ERTACMoveLegality>& Left, const TArray<ERTACMoveLegality>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return FString::Printf(TEXT("result count: %d vs %d"), Left.Num(), Right.Num());
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (Left[Index] != Right[Index])
			{
				return FString::Printf(TEXT("result[%d]: %s vs %s"), Index,
					RTACTest_LegalityName(Left[Index]), RTACTest_LegalityName(Right[Index]));
			}
		}
		return FString();
	}

	/** Renders a result sequence as one readable line, so a LogRTAC reader can see what actually happened. */
	FString DescribeResults(const TArray<ERTACMoveLegality>& Results)
	{
		FString Out;
		for (int32 Index = 0; Index < Results.Num(); ++Index)
		{
			if (Index > 0)
			{
				Out += TEXT(", ");
			}
			Out += RTACTest_LegalityName(Results[Index]);
		}
		return Out;
	}
}

/**
 * Input-sequence replay determinism: the same match, built twice and driven by the same input
 * sequence, produces byte-for-byte equivalent state and an identical sequence of outcomes.
 *
 * ---------------------------------------------------------------------------------
 * WHAT THIS TEST DOES *NOT* VERIFY, AND WHY THAT IS NOT A GAP TO BE FILLED TODAY.
 *
 * Phase 1's Definition of Done item reads "Same seed + same input sequence -> identical resulting
 * state." THERE IS NO SEED-AXIS CONTROL IN THIS TEST — no run that changes only the master seed and
 * asserts a different result — and none is possible right now. The seed is inert: Phase 1 has no
 * gameplay randomness, nothing draws from a stream, `FRTACRngState` holds a master seed and no
 * stream fields at all, and `RTACDeriveStreamSeed` is consumed by nothing that `RTACResolveMove`
 * touches. A "different seed -> different result" assertion would therefore FAIL against correct
 * code, and a "different seed -> same result" assertion would be asserting the seed's own inertness
 * as if it were a determinism property. Neither is evidence about determinism.
 *
 * This is not a new observation and is not re-derived here. PHASES.md's Phase 1 note on this DoD
 * item already records it: "Phase 1 has no gameplay randomness: nothing in it draws from a random
 * stream," and the seed-derivation tests are "a test of the mechanism a future stream will use, not
 * a test of the DoD item's actual claim." That note's September 1, 2026 addendum records the other
 * half — that the movement code the input sequence needs now exists, so the replay half is
 * unblocked work. This test is that half, and only that half.
 *
 * The seed axis becomes testable when the first real RNG stream exists (Phase 4's AI rolls or
 * Phase 5's chip draws, per FRTACRngState's own header). At that point this test gains a fourth run
 * differing only in master seed. Until then, both runs below are seeded identically and the seed is
 * compared as ordinary state, not exercised as an input.
 * ---------------------------------------------------------------------------------
 *
 * HOW THE TWO RUNS ARE PRODUCED. Both are built from a default-constructed FRTACMatchState by the
 * same fixture function, independently. Run B is never copied from run A: a copy compared against
 * its source passes regardless of whether anything is deterministic, which is Failure Mode 8's
 * "an experiment that cannot fail is not evidence" in its purest form.
 *
 * WHAT IS COMPARED. Four surfaces, not one:
 *   - Every entity's EntityId, Position.Row, Position.Column, Side, ArchetypeId — in array order.
 *   - Every tile's Position, OccupantEntityId, SurfaceModifier, Owner, Elevation — in array order.
 *   - Match-level Rng.MasterSeed, NextEntityId, grid rows, columns, tile count.
 *   - The full ORDERED SEQUENCE of ERTACMoveLegality codes both runs returned.
 * The result sequence is not redundant with the state. Two runs can diverge and re-converge, and
 * the "reconvergence" run below demonstrates a concrete case where they do — same final state,
 * different outcome sequence. Final-state-only comparison would have called that identical.
 *
 * THREE CONTROLS, because an all-passing comparison proves nothing on its own:
 *   1. Divergence control — a run with a genuinely different input sequence must produce a
 *      different final state. Without it, a comparison helper that always reported "identical"
 *      would pass every assertion in this file.
 *   2. Reconvergence control — a run with a different input sequence that reaches the SAME final
 *      state must still produce a different result sequence. This is what proves the result-sequence
 *      comparison earns its place rather than restating the state comparison.
 *   3. Comparison-helper liveness — each compared field is mutated in a copy and the helper is
 *      required to notice. `Elevation` and `ArchetypeId` are the reason this block is not optional:
 *      nothing in Phase 1 ever writes either, so their comparisons are structurally unreachable
 *      during a replay and would be indistinguishable from missing (Failure Mode 1, generalised
 *      from constants to comparison fields the way the movement test generalises it to clauses).
 * A fourth, weaker guard sits alongside them: run A's outcomes are also checked against a
 * hand-derived expected sequence. A -vs- B agreement alone would still pass if RTACResolveMove were
 * a no-op returning a constant; the expected sequence is what makes the replay assert that the
 * right things happened, not merely that they happened twice.
 *
 * ON THE EXPECTED LogRTAC WARNINGS. There are none by design — every mover in every run is a
 * spawned entity on a real tile, so `InvalidOrigin` is unreachable and nothing here logs at Warning.
 * A Warning appearing in this test's block is a genuine signal, not expected noise. That is the
 * opposite of the multi-entity test's three deliberate warnings, and worth knowing when reading the
 * two tests' output in one log.
 *
 * Simulation-layer only (Rule 5): plain structs and free functions, no engine objects, no
 * presentation layer. Lives inside the plugin per Rule 11.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTACMatchDeterministicReplayTest,
	"RTAC.Simulation.Match.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTACMatchDeterministicReplayTest::RunTest(const FString& Parameters)
{
	int32 NumAssertions = 0;
	int32 NumPassed = 0;

	// Same assertion-mirroring pattern as the Grid, MatchState and Movement tests: purely additive
	// logging on top of the framework's own AddError()-driven pass/fail, with passes at Log (never
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

	/** Asserts two states are identical, naming the first diverging field when they are not. */
	auto CheckStatesIdentical = [&](const TCHAR* What, const FRTACMatchState& Left, const FRTACMatchState& Right)
	{
		const FString Difference = FindFirstStateDifference(Left, Right);
		const FString Message = Difference.IsEmpty()
			? FString(What)
			: FString::Printf(TEXT("%s — first difference: %s"), What, *Difference);
		return Record(*Message, TestTrue(*Message, Difference.IsEmpty()));
	};

	/** Asserts two states DIFFER — the shape every control assertion in this test needs. */
	auto CheckStatesDiffer = [&](const TCHAR* What, const FRTACMatchState& Left, const FRTACMatchState& Right)
	{
		const FString Difference = FindFirstStateDifference(Left, Right);
		const FString Message = Difference.IsEmpty()
			? FString::Printf(TEXT("%s — but no difference was detected"), What)
			: FString::Printf(TEXT("%s — detected: %s"), What, *Difference);
		return Record(*Message, TestTrue(*Message, !Difference.IsEmpty()));
	};

	UE_LOG(LogRTAC, Log, TEXT("=== RTAC.Simulation.Match.DeterministicReplay — starting ==="));

	// ---------------------------------------------------------------------------------
	// The input sequence — relative directions, resolved against wherever each entity actually is.
	//
	// Fifteen events across all four entities. The expected outcome of each is stated in the
	// comment beside it; those comments are not decoration, they are the hand-derivation the
	// ExpectedResults table below encodes, kept next to the input that produces it.
	//
	// Board: 3x6. Columns 0-2 Player, columns 3-5 Enemy. Tile (2,0) broken.
	// Spawns: id 0 @(1,0)P, id 1 @(1,1)P, id 2 @(1,3)E, id 3 @(0,4)E.
	//
	// ORDER-DEPENDENCE IS DELIBERATE. Event 9 is Legal only because event 7 vacated (1,3) first;
	// event 10 is Occupied only because event 9 filled it again; event 11 is Legal only because
	// event 5 vacated (0,0). A sequence whose events were independent of each other would not
	// distinguish a deterministic replay from a lucky one.
	// ---------------------------------------------------------------------------------
	const TArray<FTestMoveInput> InputSequence = {
		{ 0, ETestMoveDirection::Up    }, //  1. (1,0)->(0,0)  Player, empty        -> Legal
		{ 1, ETestMoveDirection::Right }, //  2. (1,1)->(1,2)  Player, empty        -> Legal
		{ 2, ETestMoveDirection::Left  }, //  3. (1,3)->(1,2)  occupied by id 1     -> Occupied
		{ 3, ETestMoveDirection::Down  }, //  4. (0,4)->(1,4)  Enemy, empty         -> Legal
		{ 0, ETestMoveDirection::Down  }, //  5. (0,0)->(1,0)  vacated at event 1   -> Legal
		{ 0, ETestMoveDirection::Down  }, //  6. (1,0)->(2,0)  own territory, broken-> Broken
		{ 2, ETestMoveDirection::Up    }, //  7. (1,3)->(0,3)  Enemy, empty         -> Legal
		{ 1, ETestMoveDirection::Right }, //  8. (1,2)->(1,3)  now empty, Enemy-owned-> WrongOwner
		{ 3, ETestMoveDirection::Left  }, //  9. (1,4)->(1,3)  empty, Enemy==Enemy  -> Legal
		{ 1, ETestMoveDirection::Right }, // 10. (1,2)->(1,3)  now occupied by id 3 -> Occupied
		{ 0, ETestMoveDirection::Up    }, // 11. (1,0)->(0,0)  vacated at event 5   -> Legal
		{ 0, ETestMoveDirection::Up    }, // 12. (0,0)->(-1,0) off the top edge     -> OutOfBounds
		{ 1, ETestMoveDirection::Up    }, // 13. (1,2)->(0,2)  Player, empty        -> Legal
		{ 1, ETestMoveDirection::Left  }, // 14. (0,2)->(0,1)  Player, empty        -> Legal
		{ 1, ETestMoveDirection::Left  }  // 15. (0,1)->(0,0)  occupied by id 0     -> Occupied
	};

	// The independent oracle. Derived by hand from the board layout and RTACCheckMoveLegality's
	// first-failure-wins clause order, NOT recorded from a previous run of this code — a table
	// captured from the implementation would agree with any behaviour the implementation had,
	// including wrong behaviour (Failure Mode 3: an oracle must measure the same quantity the system
	// does, independently).
	const TArray<ERTACMoveLegality> ExpectedResults = {
		ERTACMoveLegality::Legal,
		ERTACMoveLegality::Legal,
		ERTACMoveLegality::Occupied,
		ERTACMoveLegality::Legal,
		ERTACMoveLegality::Legal,
		ERTACMoveLegality::Broken,
		ERTACMoveLegality::Legal,
		ERTACMoveLegality::WrongOwner,
		ERTACMoveLegality::Legal,
		ERTACMoveLegality::Occupied,
		ERTACMoveLegality::Legal,
		ERTACMoveLegality::OutOfBounds,
		ERTACMoveLegality::Legal,
		ERTACMoveLegality::Legal,
		ERTACMoveLegality::Occupied
	};

	// Logged in the test's own direction vocabulary, so a LogRTAC reader can reconstruct exactly what
	// was replayed without cross-referencing this file.
	{
		FString SequenceDescription;
		for (int32 Index = 0; Index < InputSequence.Num(); ++Index)
		{
			SequenceDescription += FString::Printf(TEXT("%s%d:%s"),
				(Index > 0) ? TEXT(", ") : TEXT(""),
				InputSequence[Index].EntityId,
				DirectionName(InputSequence[Index].Direction));
		}
		UE_LOG(LogRTAC, Log, TEXT("  input sequence (entity:direction, grid-space): %s"), *SequenceDescription);
	}

	// ---------------------------------------------------------------------------------
	// Run A and run B — built independently, seeded identically, driven by the same sequence.
	// ---------------------------------------------------------------------------------
	const int32 MasterSeed = 20260902;

	FRTACMatchState RunA;
	BuildFixture(RunA, MasterSeed);

	FRTACMatchState RunB;
	BuildFixture(RunB, MasterSeed);

	// Setup controls. If the fixture silently failed — an uninitialised grid, refused spawns — every
	// comparison below would compare two empty boards and pass (Failure Mode 5 and Failure Mode 8
	// meeting in one place).
	CheckTrue(TEXT("Setup: run A's grid initialised at the configured default dimensions"),
		RunA.Grid.IsInitialized());
	CheckEqual(TEXT("Setup: run A's board holds Rows x Columns tiles"),
		RunA.Grid.NumTiles(), FRTACGrid::DefaultRows * FRTACGrid::DefaultColumns);
	CheckTrue(TEXT("Setup: the board is larger than one tile, so no degenerate case (Failure Mode 5)"),
		RunA.Grid.NumTiles() > 1);
	CheckEqual(TEXT("Setup: run A spawned all four entities"), RunA.Entities.Num(), 4);
	CheckTrue(TEXT("Setup: more than one entity, so no single-entity degenerate case (Failure Mode 5)"),
		RunA.Entities.Num() > 1);
	CheckEqual(TEXT("Setup: run B spawned all four entities independently"), RunB.Entities.Num(), 4);
	CheckTrue(TEXT("Setup: run B is a separately built state, not an alias of run A"),
		&RunA != &RunB && RunA.Grid.GetTiles().GetData() != RunB.Grid.GetTiles().GetData());
	CheckTrue(TEXT("Setup: the ownership fixture ran — a left-half tile is Player-owned, not the Neutral default"),
		RunA.Grid.GetTileChecked(1, 1).Owner == ERTACTileOwner::Player);
	CheckTrue(TEXT("Setup: the ownership fixture ran — a right-half tile is Enemy-owned"),
		RunA.Grid.GetTileChecked(1, 4).Owner == ERTACTileOwner::Enemy);
	CheckTrue(TEXT("Setup: the broken tile sits in Player territory, so the Broken outcome is reachable"),
		RunA.Grid.GetTileChecked(2, 0).Owner == ERTACTileOwner::Player
		&& RunA.Grid.GetTileChecked(2, 0).SurfaceModifier == ERTACSurfaceModifier::Broken);
	CheckTrue(TEXT("Setup: the input sequence is long enough for a divergence to compound"),
		InputSequence.Num() >= 10);

	// The two freshly built states must already agree, before any input runs. A divergence here
	// would be in setup, not in resolution, and reporting it separately keeps the two causes apart.
	CheckStatesIdentical(TEXT("Pre-replay: two independently built states from the same seed are identical"),
		RunA, RunB);

	// ---------------------------------------------------------------------------------
	// Replay.
	// ---------------------------------------------------------------------------------
	int32 MissingMoversA = 0;
	int32 MissingMoversB = 0;
	const TArray<ERTACMoveLegality> ResultsA = RunInputSequence(RunA, InputSequence, MissingMoversA);
	const TArray<ERTACMoveLegality> ResultsB = RunInputSequence(RunB, InputSequence, MissingMoversB);

	UE_LOG(LogRTAC, Log, TEXT("  run A outcomes: %s"), *DescribeResults(ResultsA));
	UE_LOG(LogRTAC, Log, TEXT("  run B outcomes: %s"), *DescribeResults(ResultsB));

	CheckEqual(TEXT("Replay: run A named no missing movers — the result sequence is not short"),
		MissingMoversA, 0);
	CheckEqual(TEXT("Replay: run B named no missing movers"), MissingMoversB, 0);
	CheckEqual(TEXT("Replay: run A produced one result per input event"),
		ResultsA.Num(), InputSequence.Num());
	CheckEqual(TEXT("Replay: run B produced one result per input event"),
		ResultsB.Num(), InputSequence.Num());

	// --- The DoD item's actual assertion, on all four surfaces ---
	{
		const FString Difference = FindFirstResultDifference(ResultsA, ResultsB);
		const FString Message = Difference.IsEmpty()
			? FString(TEXT("Determinism: the two runs' ordered outcome sequences are identical"))
			: FString::Printf(TEXT("Determinism: the two runs' ordered outcome sequences are identical — first difference: %s"), *Difference);
		Record(*Message, TestTrue(*Message, Difference.IsEmpty()));
	}
	CheckStatesIdentical(TEXT("Determinism: the two runs' final states are identical across every entity, tile, and match-level field"),
		RunA, RunB);

	// --- The independent oracle: the outcomes are the RIGHT ones, not merely repeatable ---
	{
		const FString Difference = FindFirstResultDifference(ResultsA, ExpectedResults);
		const FString Message = Difference.IsEmpty()
			? FString(TEXT("Oracle: run A's outcomes match the hand-derived expected sequence"))
			: FString::Printf(TEXT("Oracle: run A's outcomes match the hand-derived expected sequence — first difference: %s"), *Difference);
		Record(*Message, TestTrue(*Message, Difference.IsEmpty()));
	}

	// Outcome coverage, asserted rather than assumed from reading the table. A replay in which every
	// move returned Legal would exercise one code path fifteen times.
	{
		bool bSawLegal = false, bSawOccupied = false, bSawWrongOwner = false, bSawBroken = false, bSawOutOfBounds = false, bSawInvalidOrigin = false;
		for (const ERTACMoveLegality Result : ResultsA)
		{
			bSawLegal         |= (Result == ERTACMoveLegality::Legal);
			bSawOccupied      |= (Result == ERTACMoveLegality::Occupied);
			bSawWrongOwner    |= (Result == ERTACMoveLegality::WrongOwner);
			bSawBroken        |= (Result == ERTACMoveLegality::Broken);
			bSawOutOfBounds   |= (Result == ERTACMoveLegality::OutOfBounds);
			bSawInvalidOrigin |= (Result == ERTACMoveLegality::InvalidOrigin);
		}
		CheckTrue(TEXT("Coverage: the replay reached Legal"), bSawLegal);
		CheckTrue(TEXT("Coverage: the replay reached Occupied"), bSawOccupied);
		CheckTrue(TEXT("Coverage: the replay reached WrongOwner"), bSawWrongOwner);
		CheckTrue(TEXT("Coverage: the replay reached Broken"), bSawBroken);
		CheckTrue(TEXT("Coverage: the replay reached OutOfBounds"), bSawOutOfBounds);
		// Not a gap: every mover is a spawned entity standing on a real tile, so RTACResolveMove's
		// origin precondition cannot fail. InvalidOrigin is covered by the multi-entity test, which
		// hand-assembles the off-board mover that spawn refuses to create.
		CheckFalse(TEXT("Coverage: the replay did NOT reach InvalidOrigin — unreachable from spawned entities, by design"),
			bSawInvalidOrigin);
	}

	// The board invariant RTACResolveMove assumes but never verifies, re-checked after the replay.
	{
		bool bConsistent = true;
		for (const FRTACEntity& Entity : RunA.Entities)
		{
			const FRTACTile* Tile = RunA.Grid.FindTile(Entity.Position);
			if (Tile == nullptr || Tile->OccupantEntityId != Entity.EntityId)
			{
				bConsistent = false;
				break;
			}
		}
		CheckTrue(TEXT("Board consistency: after the replay, every entity's tile records that entity's id"), bConsistent);
	}

	// ---------------------------------------------------------------------------------
	// CONTROL 1 — DIVERGENCE (Failure Mode 8).
	//
	// A third run, same seed and same fixture, one input event changed: event 13 steps Down instead
	// of Up. Everything after it compounds — the entity ends at (2,1) instead of (0,1) and takes a
	// different final outcome — so this run MUST produce a different final state. If it does not,
	// either the comparison helper is blind or resolution is not actually applying moves, and every
	// "identical" assertion above is worthless.
	// ---------------------------------------------------------------------------------
	{
		TArray<FTestMoveInput> DivergentSequence = InputSequence;
		CheckTrue(TEXT("Control 1 setup: the divergent sequence is the same length as the original"),
			DivergentSequence.Num() == InputSequence.Num());
		DivergentSequence[12].Direction = ETestMoveDirection::Down; // was Up: (1,2)->(2,2) instead of (0,2)

		FRTACMatchState RunC;
		BuildFixture(RunC, MasterSeed);

		int32 MissingMoversC = 0;
		const TArray<ERTACMoveLegality> ResultsC = RunInputSequence(RunC, DivergentSequence, MissingMoversC);
		UE_LOG(LogRTAC, Log, TEXT("  run C outcomes (divergence control): %s"), *DescribeResults(ResultsC));

		CheckEqual(TEXT("Control 1: the divergent run named no missing movers"), MissingMoversC, 0);
		CheckStatesDiffer(TEXT("Control 1: a genuinely different input sequence produces a DIFFERENT final state"),
			RunA, RunC);

		const FString ResultDifference = FindFirstResultDifference(ResultsA, ResultsC);
		const FString Message = FString::Printf(
			TEXT("Control 1: the divergent run also produces a different outcome sequence — %s"),
			ResultDifference.IsEmpty() ? TEXT("but none was detected") : *ResultDifference);
		Record(*Message, TestTrue(*Message, !ResultDifference.IsEmpty()));

		// The compounding itself, named rather than left implicit: the changed event is at index 12,
		// and the divergence reaches an entity position, not just one outcome.
		const FRTACEntity* MoverInA = RunA.FindEntity(1);
		const FRTACEntity* MoverInC = RunC.FindEntity(1);
		if (CheckTrue(TEXT("Control 1: the diverged mover is findable in both runs"),
			MoverInA != nullptr && MoverInC != nullptr))
		{
			CheckTrue(TEXT("Control 1: one changed direction compounded into a different final POSITION, not just a different outcome"),
				MoverInA->Position != MoverInC->Position);
		}
	}

	// ---------------------------------------------------------------------------------
	// CONTROL 2 — RECONVERGENCE.
	//
	// This is the control that justifies comparing the outcome sequence at all, and it is not a
	// hypothetical: the sequence below is a real, hand-derived case.
	//
	// Event 1 steps Right instead of Up. Entity 0 is immediately blocked by its teammate at (1,1)
	// (Occupied instead of Legal), so it is still at (1,0) when event 5 fires — which makes event 5
	// a step onto the broken tile (Broken instead of Legal) rather than a step back down. Event 6
	// repeats that. By event 11 the entity steps up to (0,0) and the two runs are back in step.
	//
	// The result: a DIFFERENT input sequence that reaches an IDENTICAL final state, with a different
	// outcome sequence. A final-state-only determinism test would have reported these two runs as
	// the same run. That is the failure the result-sequence comparison exists to catch, demonstrated
	// rather than argued.
	// ---------------------------------------------------------------------------------
	{
		TArray<FTestMoveInput> ReconvergingSequence = InputSequence;
		ReconvergingSequence[0].Direction = ETestMoveDirection::Right; // was Up: blocked by the teammate at (1,1)

		FRTACMatchState RunD;
		BuildFixture(RunD, MasterSeed);

		int32 MissingMoversD = 0;
		const TArray<ERTACMoveLegality> ResultsD = RunInputSequence(RunD, ReconvergingSequence, MissingMoversD);
		UE_LOG(LogRTAC, Log, TEXT("  run D outcomes (reconvergence control): %s"), *DescribeResults(ResultsD));

		CheckEqual(TEXT("Control 2: the reconverging run named no missing movers"), MissingMoversD, 0);

		CheckStatesIdentical(TEXT("Control 2: a DIFFERENT input sequence reconverges on an identical final state"),
			RunA, RunD);

		const FString ResultDifference = FindFirstResultDifference(ResultsA, ResultsD);
		const FString Message = FString::Printf(
			TEXT("Control 2: yet its outcome sequence DIFFERS — final-state-only comparison would have missed this — %s"),
			ResultDifference.IsEmpty() ? TEXT("but none was detected") : *ResultDifference);
		Record(*Message, TestTrue(*Message, !ResultDifference.IsEmpty()));
	}

	// ---------------------------------------------------------------------------------
	// CONTROL 3 — COMPARISON-HELPER LIVENESS.
	//
	// Every field FindFirstStateDifference compares is mutated in a copy of run A, and the helper is
	// required to report a difference. Without this, a comparison that silently skipped a field
	// would be indistinguishable from one that checked it and found it equal — Failure Mode 1's
	// "a constant the output is invariant to is inert" applied to comparison fields.
	//
	// Two fields make this mandatory rather than thorough: FRTACTile::Elevation and
	// FRTACEntity::ArchetypeId are both deliberately inert at Phase 1 (their own headers say so at
	// length), so no replay can ever make them differ. Their comparisons are unreachable during the
	// replay and can only be exercised here.
	//
	// Indexing Entities[0] and Tiles by position is fine here: this is a test reaching into its own
	// scratch copy to perturb a field, not code using an array index as entity identity — the thing
	// Decision #11 Ruling 3 forbids. No id arithmetic is done anywhere in this block.
	// ---------------------------------------------------------------------------------
	{
		auto CheckFieldIsCompared = [&](const TCHAR* FieldName, TFunctionRef<void(FRTACMatchState&)> Mutate)
		{
			FRTACMatchState Mutated = RunA; // Whole-state copy — the operation Decision #11 Ruling 1 shaped this struct for.
			Mutate(Mutated);

			const FString Difference = FindFirstStateDifference(RunA, Mutated);
			const FString Message = FString::Printf(TEXT("Control 3: a change to %s is detected by the comparison (%s)"),
				FieldName, Difference.IsEmpty() ? TEXT("NOT DETECTED") : *Difference);
			return Record(*Message, TestTrue(*Message, !Difference.IsEmpty()));
		};

		CheckFieldIsCompared(TEXT("Rng.MasterSeed"),
			[](FRTACMatchState& S) { S.Rng.MasterSeed += 1; });
		CheckFieldIsCompared(TEXT("NextEntityId"),
			[](FRTACMatchState& S) { S.NextEntityId += 1; });
		CheckFieldIsCompared(TEXT("grid dimensions and tile count"),
			[](FRTACMatchState& S) { S.Grid.Init(FRTACGrid::DefaultRows + 1, FRTACGrid::DefaultColumns + 1); });
		CheckFieldIsCompared(TEXT("the entity count"),
			[](FRTACMatchState& S) { S.Entities.Pop(); });
		CheckFieldIsCompared(TEXT("an entity's EntityId"),
			[](FRTACMatchState& S) { S.Entities[0].EntityId += 100; });
		CheckFieldIsCompared(TEXT("an entity's Position.Row"),
			[](FRTACMatchState& S) { S.Entities[0].Position.Row += 1; });
		CheckFieldIsCompared(TEXT("an entity's Position.Column"),
			[](FRTACMatchState& S) { S.Entities[0].Position.Column += 1; });
		CheckFieldIsCompared(TEXT("an entity's Side"),
			[](FRTACMatchState& S) { S.Entities[0].Side = ERTACTileOwner::Neutral; });
		CheckFieldIsCompared(TEXT("an entity's ArchetypeId (inert during a replay — only reachable here)"),
			[](FRTACMatchState& S) { S.Entities[0].ArchetypeId = FName(TEXT("Different")); });
		CheckFieldIsCompared(TEXT("a tile's Position"),
			[](FRTACMatchState& S) { S.Grid.GetTileChecked(0, 0).Position.Column += 1; });
		CheckFieldIsCompared(TEXT("a tile's OccupantEntityId"),
			[](FRTACMatchState& S) { S.Grid.GetTileChecked(2, 5).OccupantEntityId = 77; });
		CheckFieldIsCompared(TEXT("a tile's SurfaceModifier"),
			[](FRTACMatchState& S) { S.Grid.GetTileChecked(0, 0).SurfaceModifier = ERTACSurfaceModifier::Broken; });
		CheckFieldIsCompared(TEXT("a tile's Owner"),
			[](FRTACMatchState& S) { S.Grid.GetTileChecked(0, 0).Owner = ERTACTileOwner::Neutral; });
		CheckFieldIsCompared(TEXT("a tile's Elevation (inert during a replay — only reachable here)"),
			[](FRTACMatchState& S) { S.Grid.GetTileChecked(0, 0).Elevation += 1; });

		// The other half of the liveness argument: an unmutated copy must compare EQUAL. A helper
		// that reported "different" unconditionally would pass all fourteen assertions above.
		FRTACMatchState UnmutatedCopy = RunA;
		CheckStatesIdentical(TEXT("Control 3: an unmutated copy compares identical — the helper does not report differences unconditionally"),
			RunA, UnmutatedCopy);
	}

	if (NumPassed == NumAssertions)
	{
		UE_LOG(LogRTAC, Log,
			TEXT("=== RTAC.Simulation.Match.DeterministicReplay — complete: %d/%d assertions passed ==="),
			NumPassed, NumAssertions);
	}
	else
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("=== RTAC.Simulation.Match.DeterministicReplay — complete: %d/%d assertions passed, %d FAILED ==="),
			NumPassed, NumAssertions, NumAssertions - NumPassed);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
