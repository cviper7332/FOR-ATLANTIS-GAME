#include "Presentation/RTACGridConversion.h"

FVector RTACGridToLocalOffset(const FRTACGridPosition& Position, float TileSize)
{
	return FVector(Position.Column * TileSize, Position.Row * TileSize, 0.0f);
}
