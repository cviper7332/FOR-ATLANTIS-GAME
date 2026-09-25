#include "Presentation/RTACGridConversion.h"

#include "GameFramework/PlayerController.h"
#include "Presentation/RTACBoard.h"
#include "Simulation/RTACGrid.h"

FVector RTACGridToLocalOffset(const FRTACGridPosition& Position, float TileSize)
{
	return FVector(Position.Row * TileSize, Position.Column * TileSize, 0.0f);
}

bool RTACWorldPositionToGridPosition(
	const FVector& WorldPosition,
	const ARTACBoard& Board,
	const FRTACGrid& Grid,
	float TileSize,
	FRTACGridPosition& OutPosition)
{
	// RTACGridToLocalOffset's offsets are board-local, not world-absolute -- match that frame.
	const FVector LocalPoint = Board.GetActorTransform().InverseTransformPosition(WorldPosition);

	// floor(), not round() -- a tile's offset is its corner, per RTACGridToLocalOffset's own doc.
	// Row from X, Column from Y (Decision #16) -- must stay paired with RTACGridToLocalOffset.
	const int32 Row = FMath::FloorToInt32(LocalPoint.X / TileSize);
	const int32 Column = FMath::FloorToInt32(LocalPoint.Y / TileSize);

	if (!Grid.IsValidPosition(Row, Column))
	{
		return false;
	}

	OutPosition.Row = Row;
	OutPosition.Column = Column;
	return true;
}

bool RTACScreenToGridPosition(
	APlayerController* PlayerController,
	FVector2D ScreenPosition,
	const ARTACBoard& Board,
	const FRTACGrid& Grid,
	float TileSize,
	FRTACGridPosition& OutPosition)
{
	if (!PlayerController)
	{
		return false;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!PlayerController->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldOrigin, WorldDirection))
	{
		return false;
	}

	// Board's Z=0 plane in world space -- Decision #1's flat grid, wherever the board actor sits.
	const FPlane BoardPlane(Board.GetActorLocation(), Board.GetActorTransform().GetUnitAxis(EAxis::Z));
	const FVector WorldHit = FMath::RayPlaneIntersection(WorldOrigin, WorldDirection, BoardPlane);

	return RTACWorldPositionToGridPosition(WorldHit, Board, Grid, TileSize, OutPosition);
}
