// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "RTACModule.h" // LogRTAC — Rule 9: dedicated category, never LogTemp.
#include "Simulation/RTACMatchState.h"
#include "Simulation/RTACStreamSeed.h"

/**
 * RTACDeriveStreamSeed's contract: purity, independence, and stability.
 *
 * Phase 1 has NO gameplay randomness — nothing in it draws from a stream, and EntityId is a
 * counter rather than a draw (Decision #9 + its August 30, 2026 clarification addendum). The
 * derivation mechanism is therefore groundwork, laid so Phase 4's AI rolls and Phase 5's chip
 * draws inherit a shape that does not couple them, rather than being retrofitted later once
 * seeds have already been recorded against a shared stream.
 *
 * That makes THIS test the mechanism's only verification. It is a direct unit test of a pure
 * function, which is legitimate on its own — but note honestly what it does not cover: no stream
 * is ever drawn from here, so the derivation is verified while its eventual integration (a
 * stream advancing across ticks, surviving Reset) is not, and cannot be until a real consumer
 * exists in Phase 4/5.
 *
 * The stability assertions are the load-bearing ones. Purity and independence would still hold
 * if someone swapped HashCombine for HashCombineFast, or switched hash functions entirely —
 * and every previously recorded repro seed would silently break. Pinning exact values is what
 * turns that from an invisible regression into a failing test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTACStreamSeedDerivationTest,
	"RTAC.Simulation.Rng.StreamSeedDerivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTACStreamSeedDerivationTest::RunTest(const FString& Parameters)
{
	int32 NumAssertions = 0;
	int32 NumPassed = 0;

	// Mirrors assertion outcomes into LogRTAC for MCP retrieval — the pattern established by
	// FRTACGridBasicLifecycleTest and documented in docs/reference.md. Passes log at Log
	// (not intercepted by the framework, so provably inert to the result); failures log at
	// Warning rather than Error, because the assertion's own AddError() has already registered
	// the failure and logging Error would double-count it.
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
	auto CheckEqual = [&](const TCHAR* What, int32 Actual, int32 Expected) { return Record(What, TestEqual(What, Actual, Expected)); };

	UE_LOG(LogRTAC, Log, TEXT("=== RTAC.Simulation.Rng.StreamSeedDerivation — starting ==="));

	// --- Purity: the same inputs always produce the same output ---
	// The whole scheme rests on this. If derivation ever consulted anything environmental
	// (time, an address, global state), Rule 6 determinism would be gone and this would catch it.
	CheckEqual(TEXT("Derivation is pure — same master seed and name give the same result"),
		RTACDeriveStreamSeed(12345, TEXT("EntitySpawn")),
		RTACDeriveStreamSeed(12345, TEXT("EntitySpawn")));

	// --- Independence: different streams from one master seed do not collide ---
	// This is the property that lets a new stream be added in a later phase without disturbing
	// any existing stream's sequence.
	CheckTrue(TEXT("Different stream names from one master seed derive different seeds"),
		RTACDeriveStreamSeed(12345, TEXT("EntitySpawn")) != RTACDeriveStreamSeed(12345, TEXT("ChipDraw")));

	// --- The master seed is load-bearing: changing it changes every stream ---
	// Without this, the master seed would be an inert constant (Failure Mode 1) and "same seed →
	// same match" would be satisfied vacuously.
	CheckTrue(TEXT("One stream name under different master seeds derives different seeds"),
		RTACDeriveStreamSeed(0, TEXT("EntitySpawn")) != RTACDeriveStreamSeed(12345, TEXT("EntitySpawn")));

	// --- Stability: exact pinned values ---
	//
	// Computed offline by reimplementing UE 5.8's own algorithm — FCrc::StrCrc32 (Crc.h:43,
	// standard reflected CRC-32, polynomial 0x04c11db7 per Crc.cpp:38 and its self-check at
	// Crc.cpp:444-463) composed with HashCombine (TypeHash.h:51). They were NOT captured from a
	// run of this engine, because they were written before this file had ever been compiled.
	//
	// If one of these fails on the first build, the likely cause is an error in that offline
	// reimplementation, not a defect in RTACDeriveStreamSeed. The actual values are logged
	// immediately below so a correction is a copy-paste. Once they have passed once, they are
	// authoritative and a later failure means the derivation genuinely changed — which is
	// precisely what this assertion exists to catch.
	UE_LOG(LogRTAC, Log, TEXT("  [INFO] Derived seeds — (0,\"EntitySpawn\")=%d (0,\"ChipDraw\")=%d (12345,\"EntitySpawn\")=%d"),
		RTACDeriveStreamSeed(0, TEXT("EntitySpawn")),
		RTACDeriveStreamSeed(0, TEXT("ChipDraw")),
		RTACDeriveStreamSeed(12345, TEXT("EntitySpawn")));

	CheckEqual(TEXT("Stable pinned value for master seed 0, stream \"EntitySpawn\""),
		RTACDeriveStreamSeed(0, TEXT("EntitySpawn")), 1448443275);
	CheckEqual(TEXT("Stable pinned value for master seed 0, stream \"ChipDraw\""),
		RTACDeriveStreamSeed(0, TEXT("ChipDraw")), 702107672);
	CheckEqual(TEXT("Stable pinned value for master seed 12345, stream \"EntitySpawn\""),
		RTACDeriveStreamSeed(12345, TEXT("EntitySpawn")), 710566879);

	if (NumPassed == NumAssertions)
	{
		UE_LOG(LogRTAC, Log,
			TEXT("=== RTAC.Simulation.Rng.StreamSeedDerivation — complete: %d/%d assertions passed ==="),
			NumPassed, NumAssertions);
	}
	else
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("=== RTAC.Simulation.Rng.StreamSeedDerivation — complete: %d/%d assertions passed, %d FAILED ==="),
			NumPassed, NumAssertions, NumAssertions - NumPassed);
	}

	return true;
}

/**
 * FRTACMatchState's Rule 6 obligations: state is explicit, and Initialize() is a complete
 * standalone reinitialisation rather than a partial refresh over whatever was there before.
 *
 * Slightly beyond the literal "unit test of the derivation function" scope, included because
 * Rule 6 states the standalone-reset requirement outright and leaving a brand-new state struct
 * untested would sit badly against the test discipline Phase 0 just closed on.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRTACMatchStateLifecycleTest,
	"RTAC.Simulation.Rng.MatchStateLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTACMatchStateLifecycleTest::RunTest(const FString& Parameters)
{
	int32 NumAssertions = 0;
	int32 NumPassed = 0;

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

	auto CheckEqual = [&](const TCHAR* What, int32 Actual, int32 Expected) { return Record(What, TestEqual(What, Actual, Expected)); };

	UE_LOG(LogRTAC, Log, TEXT("=== RTAC.Simulation.Rng.MatchStateLifecycle — starting ==="));

	FRTACMatchState State;

	// --- Default construction ---
	CheckEqual(TEXT("A default-constructed match state has master seed 0"), State.Rng.MasterSeed, 0);
	CheckEqual(TEXT("A default-constructed match state has NextEntityId 0"), State.NextEntityId, 0);

	// --- Initialize sets the seed and zeroes the counter ---
	State.Initialize(4242);
	CheckEqual(TEXT("Initialize() stores the master seed"), State.Rng.MasterSeed, 4242);
	CheckEqual(TEXT("Initialize() leaves NextEntityId at 0"), State.NextEntityId, 0);

	// --- The counter is a counter: incrementing hands out ids in spawn order ---
	const int32 FirstId = State.NextEntityId++;
	const int32 SecondId = State.NextEntityId++;
	CheckEqual(TEXT("First entity id is 0, per Decision #9's spawn-order counter"), FirstId, 0);
	CheckEqual(TEXT("Second entity id is 1"), SecondId, 1);
	CheckEqual(TEXT("NextEntityId has advanced to 2"), State.NextEntityId, 2);

	// --- Rule 6: Initialize() is standalone, not a patch over prior contents ---
	// Called on a dirtied state, it must produce exactly the same result as on a fresh one —
	// the counter must not carry over.
	State.Initialize(4242);
	CheckEqual(TEXT("Initialize() on a used state resets NextEntityId — standalone, not a patch (Rule 6)"),
		State.NextEntityId, 0);
	CheckEqual(TEXT("Initialize() on a used state still stores the master seed"), State.Rng.MasterSeed, 4242);

	// --- Reset returns the state to default-constructed form ---
	State.NextEntityId = 7;
	State.Reset();
	CheckEqual(TEXT("Reset() clears the master seed"), State.Rng.MasterSeed, 0);
	CheckEqual(TEXT("Reset() clears NextEntityId"), State.NextEntityId, 0);

	if (NumPassed == NumAssertions)
	{
		UE_LOG(LogRTAC, Log,
			TEXT("=== RTAC.Simulation.Rng.MatchStateLifecycle — complete: %d/%d assertions passed ==="),
			NumPassed, NumAssertions);
	}
	else
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("=== RTAC.Simulation.Rng.MatchStateLifecycle — complete: %d/%d assertions passed, %d FAILED ==="),
			NumPassed, NumAssertions, NumAssertions - NumPassed);
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
