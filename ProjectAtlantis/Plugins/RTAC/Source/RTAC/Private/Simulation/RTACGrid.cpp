// Copyright Epic Games, Inc. All Rights Reserved.

#include "Simulation/RTACGrid.h"

#include "RTACModule.h" // LogRTAC — Rule 9: dedicated category, never LogTemp.

FRTACGrid::FRTACGrid(int32 InRows, int32 InColumns)
{
	Init(InRows, InColumns);
}

bool FRTACGrid::Init(int32 InRows, int32 InColumns)
{
	// Rejecting bad dimensions is a genuine, once-per-setup failure — the one place in this
	// type where logging is warranted (Rule 9). Out-of-bounds tile queries are not logged;
	// see FindTile's comment for why.
	if (InRows <= 0 || InColumns <= 0)
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("FRTACGrid::Init rejected dimensions %d rows x %d columns — both must be greater than 0. Grid left uninitialised."),
			InRows, InColumns);

		Reset();
		return false;
	}

	Rows = InRows;
	Columns = InColumns;

	// Empty() before SetNum() so no tile state survives from a previous board. Rule 6 requires
	// this be a complete standalone reinitialisation rather than a patch over prior contents.
	Tiles.Empty(Rows * Columns);
	Tiles.SetNum(Rows * Columns);

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			FRTACTile& Tile = Tiles[ToIndex(Row, Column)];

			// Explicit fresh value even though SetNum default-constructs, so the
			// "nothing carries over" guarantee is visible here rather than inferred.
			Tile = FRTACTile();

			// This assignment is the single place Position is ever written, so it cannot drift
			// out of agreement with the tile's index.
			Tile.Position = FRTACGridPosition(Row, Column);
		}
	}

	return true;
}

void FRTACGrid::Reset()
{
	Rows = 0;
	Columns = 0;
	Tiles.Empty();
}

int32 FRTACGrid::ToIndex(int32 Row, int32 Column) const
{
	// Row-major: the row is the slower-varying dimension, matching Decision #5's grid[row][col].
	return Row * Columns + Column;
}

bool FRTACGrid::IsValidPosition(int32 Row, int32 Column) const
{
	return Row >= 0 && Row < Rows
		&& Column >= 0 && Column < Columns;
}

bool FRTACGrid::IsValidPosition(FRTACGridPosition Position) const
{
	return IsValidPosition(Position.Row, Position.Column);
}

const FRTACTile* FRTACGrid::FindTile(int32 Row, int32 Column) const
{
	if (!IsValidPosition(Row, Column))
	{
		return nullptr;
	}

	return &Tiles[ToIndex(Row, Column)];
}

FRTACTile* FRTACGrid::FindTile(int32 Row, int32 Column)
{
	return const_cast<FRTACTile*>(const_cast<const FRTACGrid*>(this)->FindTile(Row, Column));
}

const FRTACTile* FRTACGrid::FindTile(FRTACGridPosition Position) const
{
	return FindTile(Position.Row, Position.Column);
}

FRTACTile* FRTACGrid::FindTile(FRTACGridPosition Position)
{
	return FindTile(Position.Row, Position.Column);
}

const FRTACTile& FRTACGrid::GetTileChecked(int32 Row, int32 Column) const
{
	check(IsValidPosition(Row, Column));
	return Tiles[ToIndex(Row, Column)];
}

FRTACTile& FRTACGrid::GetTileChecked(int32 Row, int32 Column)
{
	check(IsValidPosition(Row, Column));
	return Tiles[ToIndex(Row, Column)];
}
