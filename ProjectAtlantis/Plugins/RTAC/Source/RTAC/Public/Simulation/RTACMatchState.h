// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Held BY VALUE below (FRTACGrid Grid, TArray<FRTACEntity> Entities), so complete types are
// required — these cannot be forward declarations. Decision #11 Ruling 1.
#include "Simulation/RTACEntity.h"
#include "Simulation/RTACGrid.h"
#include "Simulation/RTACGridPosition.h"
#include "Simulation/RTACTileOwner.h"

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
 * SCOPE — PHASE 1. Holds the whole per-match simulation state: the board, the entities on it, the
 * RNG state, and the entity-id counter. Decision #11 Ruling 1 wired the grid and entity storage in
 * here, closing this header's earlier "deliberately not wired in yet" note. Rule 6 requires
 * per-match temporal state to live in one explicit struct, and tile occupancy and entity positions
 * are exactly that.
 *
 * BY VALUE, NOT BY POINTER — and therefore expensive to copy, deliberately. Phase 1's determinism
 * test copies a whole match state to snapshot and compare two runs, which is the operation this
 * shape exists to make one line rather than a hand-written deep copy. See Decision #11 Ruling 1's
 * cost note. Nothing in Phase 1 copies match state per tick, and nothing should be built that does
 * without revisiting that ruling.
 *
 * Simulation-layer type (Rule 5): plain struct, UE Core value types only, no UObject/AActor
 * ownership, no UPROPERTY, no reflection.
 */
struct FRTACMatchState
{
	/** Seeded RNG state. Currently carries the master seed and no streams — see FRTACRngState. */
	FRTACRngState Rng;

	/**
	 * The combat board.
	 *
	 * Reset but UNINITIALISED after Initialize() — a caller runs Grid.Init(Rows, Columns) as the
	 * next setup step. See Initialize()'s own doc for why this struct does not take dimensions.
	 */
	FRTACGrid Grid;

	/**
	 * Every entity in the match, in spawn order.
	 *
	 * ---------------------------------------------------------------------------------
	 * ARRAY INDEX IS NOT ENTITY IDENTITY — Decision #11 Ruling 3.
	 *
	 * Index and EntityId happen to be equal today, because ids are handed out 0, 1, 2, ...
	 * (Decision #9) and Phase 1 has no removal. THAT IS A COINCIDENCE, NOT A CONTRACT. Do not
	 * index this array by EntityId, do not do arithmetic between the two, and do not write code
	 * whose correctness depends on them agreeing.
	 *
	 * It breaks the first time an entity is removed mid-match — and by then the assumption will
	 * have spread through every call site that found it convenient, each one individually correct
	 * when written, collectively a rewrite. FindEntity(EntityId) below is the only supported way
	 * to get from an id to an entity.
	 * ---------------------------------------------------------------------------------
	 *
	 * TArray, not TMap<int32, FRTACEntity>, per Decision #11 Ruling 2 — the same reason
	 * FRTACGrid::Tiles is a flat array: one unambiguous iteration order, which Rule 6's
	 * same-seed-same-result determinism requirement depends on. TMap's iteration order is not a
	 * stable contract across runs, builds, or platforms, and resting determinism on it would fail
	 * intermittently and unreproducibly — the shape Rule 6's "treat any divergence as a bug in
	 * this rule, not a curiosity" clause exists to prevent.
	 */
	TArray<FRTACEntity> Entities;

	/**
	 * Next EntityId to hand out, incrementing in spawn order (0, 1, 2, ...) per Decision #9.
	 *
	 * Sits BESIDE Rng rather than inside it, deliberately. This is a counter, not randomness —
	 * it draws from no stream and consumes no seed. Decision #9's August 30, 2026 clarification
	 * addendum records this explicitly: the counter engages Rule 6's no-hidden-state clause, not
	 * its RNG clause. Filing it under "RNG state" would conflate two domains in one container,
	 * the same Rule 10 error Decision #9 rejects at the field level.
	 *
	 * RTACSpawnEntity() is the only thing that should advance this, and it advances only on a
	 * successful spawn — see that function's doc for why that matters to Rule 6.
	 */
	int32 NextEntityId = 0;

	/**
	 * Returns the entity with the given EntityId, or nullptr when no entity has it.
	 *
	 * THE ONLY SUPPORTED WAY TO GET FROM AN ID TO AN ENTITY (Decision #11 Ruling 3). Same
	 * nullptr-on-absence contract and const/non-const overload pair as FRTACGrid::FindTile(), so
	 * the shape is already familiar in this tree rather than novel.
	 *
	 * A linear scan. At Phase 1's entity counts (single digits on an 18-tile board) that is not
	 * measurable. If counts ever grow to where it is, the fix is an id-to-index side map
	 * maintained by spawn and removal — never callers reintroducing index arithmetic.
	 *
	 * The returned pointer points into Entities and is invalidated by any subsequent spawn,
	 * removal, Initialize(), or Reset(). Do not store it across those calls.
	 */
	const FRTACEntity* FindEntity(int32 EntityId) const;
	FRTACEntity* FindEntity(int32 EntityId);

	/**
	 * Complete standalone reinitialisation, per Rule 6 — does not depend on any earlier call, and
	 * leaves no member carrying state from a prior match. Resets the entity counter to 0, which is
	 * what makes two runs' id sequences comparable as the determinism check Decision #9 describes.
	 *
	 * DOES NOT TAKE GRID DIMENSIONS, and so leaves Grid reset-but-uninitialised; a caller runs
	 * Grid.Init(Rows, Columns) as the next setup step. This keeps Decision #8's model intact —
	 * dimensions are configuration passed in at init time by whoever sets up the battle, not
	 * something match state decides — and leaves this function's existing signature unchanged.
	 * Whether Initialize() should instead take dimensions and do both is a question Decision #11
	 * did not settle, and is deliberately not answered here.
	 */
	void Initialize(int32 InMasterSeed);

	/** Returns this state to its default-constructed form: empty board, no entities, seed 0. */
	void Reset();
};

/**
 * Places a new entity on the board and returns its EntityId, or INDEX_NONE if it could not be
 * placed. Decision #11 Ruling 4.
 *
 * THE ONE PLACE AN ENTITY IS PUT ON THE BOARD. It is the only function that sets Entity.Position
 * and the destination tile's OccupantEntityId together, which is what establishes the invariant
 * every mover depends on:
 *
 *     for every entity E:  Grid.FindTile(E.Position)->OccupantEntityId == E.EntityId
 *
 * RTACResolveMove assumes that invariant and never verifies it — its step 1 clears the origin
 * tile's OccupantEntityId unconditionally, without confirming the origin tile actually held the
 * entity being moved. On a well-formed board that is correct and cheap. On a hand-assembled one it
 * silently clears a DIFFERENT entity's occupancy and corrupts the board with no error and no log
 * line. Before this function existed nothing established that invariant and nothing enforced it;
 * assembling entities by hand is what this exists to stop.
 *
 * Free function rather than a member, following RTACResolveMove's precedent: the operation lives
 * next to the state it advances — NextEntityId, here — rather than in a file of its own.
 *
 * FAILURE CASES. Both return INDEX_NONE, log at Warning to LogRTAC (Rule 9), and mutate nothing:
 *   - Position is not a tile on the grid. An uninitialised grid fails this too, since
 *     FRTACGrid::IsValidPosition() is false for every position while Rows/Columns are 0 — which
 *     is the right answer: there is no board to spawn onto.
 *   - The tile at Position is already occupied. Spawning there would overwrite the existing
 *     occupant's claim, breaking the very invariant this function exists to establish.
 *
 * NextEntityId ADVANCES ONLY ON SUCCESS. Load-bearing for Rule 6, not tidiness: if a failed spawn
 * consumed an id, two runs differing only in one failed spawn would hand different ids to every
 * entity after it, and every downstream comparison would diverge for a reason unrelated to
 * whatever was being tested.
 *
 * WHAT IT DELIBERATELY DOES NOT DO:
 *   - It does not read or write FRTACTile::Owner. Assigning tile ownership is authoring, which
 *     Decision #10 Ruling 3 explicitly defers.
 *   - It does not check the tile's Owner against the entity's Side. Movement legality governs
 *     movement, not placement, and battle setup may legitimately place an entity anywhere the
 *     author intends. A consequence worth knowing: an entity placed inside opposing territory is
 *     immobile, because RTACCheckMoveLegality's clause 3 rejects every neighbouring tile. That is
 *     a property of the placement, not a bug in the check.
 *
 * Returns INDEX_NONE rather than a named result enum. An ERTACSpawnResult mirroring
 * ERTACMoveLegality was considered and rejected (Decision #11): that enum's justification was two
 * named future consumers each needing one clause distinguished from the others, and spawn has no
 * such consumer. The LogRTAC warning carries the diagnostic detail instead.
 *
 * @param State        The match to spawn into. Entities, NextEntityId, and one tile are mutated
 *                     on success; nothing is mutated on failure.
 * @param Position     Where to place the entity, in grid space (Rule 10 — never world units).
 * @param Side         Which side the entity belongs to. Set once at spawn and unchanging for the
 *                     match, per Decision #9's August 31, 2026 addendum.
 * @param ArchetypeId  Archetype lookup key. Reserved and unread at Phase 1 (Decision #9).
 * @return The new entity's EntityId, or INDEX_NONE if it could not be placed.
 */
int32 RTACSpawnEntity(FRTACMatchState& State, FRTACGridPosition Position, ERTACTileOwner Side, FName ArchetypeId);
