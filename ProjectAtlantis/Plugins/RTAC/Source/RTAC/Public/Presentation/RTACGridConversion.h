#pragma once

#include "CoreMinimal.h"
#include "Simulation/RTACGridPosition.h"

/**
 * Grid <-> world-unit conversion (AGENTS.md Rule 10): the one named boundary where a
 * FRTACGridPosition becomes a world-space quantity. Presentation-layer only -- nothing in
 * Simulation/ calls this, and it never calls back into Simulation/ to mutate anything.
 *
 * Deliberately flat (Decision #1): grid logic remains flat/2D underneath regardless of camera
 * angle, so this formula carries no isometric skew, no rotation, and no elevation contribution.
 * The isometric look is applied entirely by the camera on top of this. Elevation
 * (FRTACTile::Elevation) is mechanically inert per Phase 1's DoD and is not read here; a
 * world-space height derived from elevation would be a separate, later presentation-side lookup
 * per Rule 10, not folded in ahead of Decision #3's core-before-elevation sequencing.
 *
 * Returns an offset relative to the owning board's own local origin, not an absolute world
 * location -- the board actor supplies absolute placement via its own transform. Column maps to
 * the local X axis, Row maps to the local Y axis, matching Decision #5's wider-than-deep board
 * (6 columns x 3 rows default); this pairing is the single source of truth for that mapping
 * rather than left to whichever axis a caller assumes (Rule 5 Addendum #3).
 *
 * A tile's offset is its CORNER, not its center: tile (Row, Column) spans
 * [Column*TileSize, (Column+1)*TileSize) x [Row*TileSize, (Row+1)*TileSize). An inverse
 * (screen/world -> grid) conversion must floor(), not round(), to recover the containing tile
 * from an arbitrary interior point -- round() misidentifies any point past a tile's midpoint.
 *
 * @param Position  Grid position (Decision #5 rows x columns), read-only.
 * @param TileSize  World units (cm) per grid step. Caller-supplied, not hardcoded here.
 * @return Local-space offset: (Column * TileSize, Row * TileSize, 0).
 */
FVector RTACGridToLocalOffset(const FRTACGridPosition& Position, float TileSize);
