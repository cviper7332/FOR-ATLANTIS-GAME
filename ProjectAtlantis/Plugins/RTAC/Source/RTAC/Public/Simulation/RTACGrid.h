// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Simulation/RTACGridPosition.h"
#include "Simulation/RTACTile.h"

/**
 * The combat grid: a rectangular board of FRTACTile, addressed rows x columns per Decision #5.
 *
 * Simulation-layer type (AGENTS.md Rule 5): plain struct, UE Core value types only, no
 * UObject/AActor ownership, no UPROPERTY, no reflection, no engine subsystem access. It does
 * not know an editor exists.
 *
 * Dimensions are constructor/Init parameters rather than compile-time constants, per
 * Decision #8. A presentation-layer wrapper (actor or component — where UPROPERTY(EditAnywhere)
 * belongs) exposes editable Rows/Columns defaulting to DefaultRows/DefaultColumns below, and
 * passes them in here at init time. That satisfies the editable-in-editor requirement through
 * Rule 5's separation working as designed, not as an exception to it.
 *
 * SCOPE — PHASE 1, DATA STRUCTURE ONLY. Storage, bounds checking, and tile access live here.
 * Movement rules do not, and are not to be added to this type without reading
 * docs/PHASES.md Phase 1 first.
 */
struct FRTACGrid
{
	/**
	 * Canonical default dimensions: 3 rows x 6 columns, matching MMBN3's actual board
	 * (3 columns per side), per Decision #8 and stated rows x columns per Decision #5.
	 *
	 * These are the single source of truth for the defaults. The presentation-layer wrapper's
	 * editable properties should initialise FROM these rather than restating 3 and 6 — two
	 * copies of the same number is exactly the drift Failure Mode 7 warns about.
	 */
	static constexpr int32 DefaultRows = 3;
	static constexpr int32 DefaultColumns = 6;

	/** Constructs an empty, uninitialised grid. Call Init() before use. */
	FRTACGrid() = default;

	/** Constructs and initialises in one step. Equivalent to default-construct then Init(). */
	FRTACGrid(int32 InRows, int32 InColumns);

	/**
	 * Builds the grid at the given dimensions, discarding any prior contents.
	 *
	 * This is a complete standalone reinitialisation, per Rule 6 — it does not depend on any
	 * earlier Init() or Reset() having run, and leaves no state carried over from a previous
	 * board. Every tile is freshly value-initialised and has its Position assigned.
	 *
	 * @param InRows     Row count, must be > 0.
	 * @param InColumns  Column count, must be > 0.
	 * @return true on success. On invalid dimensions, logs to LogRTAC, leaves the grid reset
	 *         and uninitialised, and returns false.
	 */
	bool Init(int32 InRows, int32 InColumns);

	/** Clears all tiles and dimensions, returning the grid to its default-constructed state. */
	void Reset();

	int32 GetRows() const { return Rows; }
	int32 GetColumns() const { return Columns; }

	/** Total tile count (Rows * Columns), or 0 when uninitialised. */
	int32 NumTiles() const { return Tiles.Num(); }

	/** True once Init() has succeeded with positive dimensions. */
	bool IsInitialized() const { return Rows > 0 && Columns > 0; }

	/** True when (Row, Column) lies inside the board. Safe to call on an uninitialised grid. */
	bool IsValidPosition(int32 Row, int32 Column) const;

	/** Overload taking a grid position. */
	bool IsValidPosition(FRTACGridPosition Position) const;

	/**
	 * Returns the tile at (Row, Column), or nullptr when out of bounds.
	 *
	 * Deliberately does not log on an out-of-bounds query: probing neighbours off the edge of
	 * the board is ordinary, expected behaviour for grid logic, and logging it would be exactly
	 * the per-frame spam Rule 9 requires be kept out of the production path. Genuine failures
	 * (bad dimensions at Init) are logged instead.
	 *
	 * The returned pointer points at a simulation type, not an engine object, and is invalidated
	 * by any subsequent Init() or Reset(). Do not store it across those calls.
	 */
	const FRTACTile* FindTile(int32 Row, int32 Column) const;
	FRTACTile* FindTile(int32 Row, int32 Column);
	const FRTACTile* FindTile(FRTACGridPosition Position) const;
	FRTACTile* FindTile(FRTACGridPosition Position);

	/**
	 * Returns the tile at (Row, Column), asserting the position is valid.
	 *
	 * For call sites that have already established the position is in bounds. Use FindTile()
	 * where out-of-bounds is a legitimate possible outcome rather than a bug.
	 */
	const FRTACTile& GetTileChecked(int32 Row, int32 Column) const;
	FRTACTile& GetTileChecked(int32 Row, int32 Column);

	/** Read-only access to the whole tile array, in row-major order. */
	const TArray<FRTACTile>& GetTiles() const { return Tiles; }

	/**
	 * Flat index for (Row, Column) in row-major order: Row * Columns + Column.
	 *
	 * Only meaningful for positions satisfying IsValidPosition(); callers are expected to have
	 * validated first. Row-major keeps the row as the slower-varying dimension, matching
	 * Decision #5's grid[row][col] convention exactly.
	 */
	int32 ToIndex(int32 Row, int32 Column) const;

private:
	/** Row count. 0 until Init() succeeds. */
	int32 Rows = 0;

	/** Column count. 0 until Init() succeeds. */
	int32 Columns = 0;

	/**
	 * Tile storage: one flat, contiguous array in row-major order, NOT TArray<TArray<...>>.
	 *
	 * Chosen over a nested array because:
	 *   1. One contiguous allocation — better locality for the whole-board iteration that
	 *      movement and resolution will do every tick, and one allocation instead of 1 + Rows.
	 *   2. Ragged rows become structurally impossible; every row is the same length by
	 *      construction rather than by convention.
	 *   3. One unambiguous iteration order, which Rule 6's same-seed-same-result determinism
	 *      requirement depends on.
	 *   4. Trivially serialisable as a block — future serialisation is one of the three
	 *      justifications Rule 5's Addendum #2 gives for keeping simulation state POD.
	 */
	TArray<FRTACTile> Tiles;
};
