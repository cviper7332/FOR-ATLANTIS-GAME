// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A position on the combat grid: a discrete (Row, Column) index, per Decision #5.
 *
 * Simulation-layer type (AGENTS.md Rule 5): plain struct, no engine ownership types, no
 * UPROPERTY, no reflection.
 *
 * DOMAIN (Rule 10): this is a grid index, never a world-space location. Converting a position
 * to world units is a presentation-layer concern and happens at exactly one named boundary
 * there — not here.
 */
struct FRTACGridPosition
{
	/**
	 * Row and Column are named outright rather than reused from FIntPoint's X/Y.
	 *
	 * FIntPoint would work mechanically, but its X/Y reads as screen space (X = horizontal),
	 * which runs opposite to this grid's rows x columns meaning (Decision #5). Reusing it would
	 * leave the mapping correct only for as long as every reader notices a comment saying so.
	 * Naming the fields makes it unmisreadable by construction instead.
	 */
	int32 Row = 0;
	int32 Column = 0;

	FRTACGridPosition() = default;

	FRTACGridPosition(int32 InRow, int32 InColumn)
		: Row(InRow)
		, Column(InColumn)
	{
	}

	bool operator==(const FRTACGridPosition& Other) const
	{
		return Row == Other.Row && Column == Other.Column;
	}

	bool operator!=(const FRTACGridPosition& Other) const
	{
		return !(*this == Other);
	}
};
