// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * RNG state for one match. Seeded explicitly, per AGENTS.md Rule 6.
 *
 * Holds the single master seed every stream is derived from (see RTACDeriveStreamSeed).
 *
 * NO STREAM FIELDS EXIST YET, DELIBERATELY. Phase 1 has no gameplay randomness at all — nothing
 * in it draws. EntityId is a plain counter, not a draw (Decision #9 and its August 30, 2026
 * clarification addendum). Streams are added here one field per consuming system as those
 * systems arrive: an AI-roll stream in Phase 4, a chip-draw stream in Phase 5. Adding one never
 * perturbs an existing stream's sequence, because each stream's seed is derived independently
 * from (MasterSeed, its own name).
 *
 * Named fields rather than a TMap<FName, FRandomStream> on purpose. The independence property
 * comes from the derivation function, not from the storage, so a map buys only "no struct edit
 * when adding a stream" — and costs real guarantees: lazily-created entries make the live stream
 * set depend on execution path, which is the hidden-state shape Rule 6 exists to prevent, and a
 * typo'd name would silently create a new stream instead of failing to compile.
 *
 * WHEN STREAMS DO EXIST — a warning the type system will not give you. FRandomStream::Seed is
 * `mutable` and its draw methods (GetUnsignedInt, RandRange, GetFraction) are all `const`, so a
 * stream advances happily through a const reference. Passing FRTACMatchState by const& therefore
 * does NOT guarantee the RNG was left untouched. Rule 6's "passed in and out each tick by the
 * caller" is a discipline here, not something the compiler enforces.
 *
 * Also: never construct a stream via FRandomStream(FName). Its documented behaviour is to seed
 * from the CURRENT TIME when given NAME_None — a direct Rule 6 violation. Always
 * Initialize(int32) with a value from RTACDeriveStreamSeed.
 *
 * Simulation-layer type (Rule 5): plain struct, UE Core value types only, no UObject/AActor
 * ownership, no UPROPERTY, no reflection.
 */
struct FRTACRngState
{
	/** The match's single root seed. Every stream is derived from this plus a stream name. */
	int32 MasterSeed = 0;

	/**
	 * Complete standalone reinitialisation, per Rule 6 — does not depend on any earlier
	 * Initialize() or Reset() having run, and leaves nothing carried over from a prior match.
	 */
	void Initialize(int32 InMasterSeed);

	/** Returns this state to its default-constructed form. */
	void Reset();
};

/**
 * Per-match simulation state: the explicit struct Rule 6 requires, passed in and out each tick
 * by the caller. No hidden globals, no mutable statics, no function-local statics.
 *
 * SCOPE — PHASE 1, PARTIAL. Currently holds only RNG state and the entity-id counter. Grid and
 * entity storage join this struct as Phase 1's movement work lands; they are deliberately not
 * wired in yet rather than being speculatively arranged now.
 *
 * Simulation-layer type (Rule 5): plain struct, UE Core value types only, no UObject/AActor
 * ownership, no UPROPERTY, no reflection.
 */
struct FRTACMatchState
{
	/** Seeded RNG state. Currently carries the master seed and no streams — see FRTACRngState. */
	FRTACRngState Rng;

	/**
	 * Next EntityId to hand out, incrementing in spawn order (0, 1, 2, ...) per Decision #9.
	 *
	 * Sits BESIDE Rng rather than inside it, deliberately. This is a counter, not randomness —
	 * it draws from no stream and consumes no seed. Decision #9's August 30, 2026 clarification
	 * addendum records this explicitly: the counter engages Rule 6's no-hidden-state clause, not
	 * its RNG clause. Filing it under "RNG state" would conflate two domains in one container,
	 * the same Rule 10 error Decision #9 rejects at the field level.
	 */
	int32 NextEntityId = 0;

	/**
	 * Complete standalone reinitialisation, per Rule 6 — does not depend on any earlier call.
	 * Resets the entity counter to 0, which is what makes two runs' id sequences comparable as
	 * the determinism check Decision #9 describes.
	 */
	void Initialize(int32 InMasterSeed);

	/** Returns this state to its default-constructed form. */
	void Reset();
};
