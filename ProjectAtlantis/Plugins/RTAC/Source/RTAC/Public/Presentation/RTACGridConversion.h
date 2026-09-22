#pragma once

#include "CoreMinimal.h"
#include "Simulation/RTACGridPosition.h"

class APlayerController;
class ARTACBoard;
struct FRTACGrid;

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

/**
 * Inverse of RTACGridToLocalOffset (AGENTS.md Rule 10): screen-space input becomes a grid
 * position through a real isometric-camera-aware projection (Decision #1), not an axis swap.
 *
 * Deprojects ScreenPosition through PlayerController's own camera to a world-space ray,
 * intersects that ray with Board's local Z=0 plane (Decision #1: the grid stays flat underneath
 * regardless of camera), transforms the hit into Board's local space to match
 * RTACGridToLocalOffset's own coordinate frame, then floors -- never rounds -- to recover the
 * containing tile, per this file's own corner-vs-center note above.
 *
 * Board (AGENTS.md Rule 5) holds no FRTACGrid of its own, so Grid is supplied separately by the
 * caller, which already owns the live match's FRTACMatchState. TileSize is likewise
 * caller-supplied, matching RTACGridToLocalOffset's own "caller-supplied, not hardcoded here"
 * convention -- exactly one TileSize value should exist per board and be passed to both
 * conversion directions (Failure Mode 7).
 *
 * The post-intersection resolution -- local-space transform, floor, bounds check -- is factored
 * out into RTACWorldPositionToGridPosition below, so it can be exercised directly by an
 * automation test without a live camera or LocalPlayer (neither of which that half depends on).
 * This function's own signature and behavior are unchanged by that split; it still owns the
 * deprojection and the ray/plane intersection that produce the world-space point passed onward.
 *
 * @param PlayerController  Supplies the camera to deproject through. Must be non-null.
 * @param ScreenPosition    Screen-space pixel coordinates.
 * @param Board             The board actor being clicked on; supplies world placement via its
 *                          own actor transform. Holds no simulation state (Rule 5).
 * @param Grid              The live simulation grid to validate the resolved position against.
 * @param TileSize          World units (cm) per grid step -- must match the value used to place
 *                          the board's tiles via RTACGridToLocalOffset.
 * @param OutPosition       Set only when this returns true; left unmodified on failure.
 * @return true if the deprojected ray hit the board's plane at a position inside Grid's bounds.
 */
bool RTACScreenToGridPosition(
	APlayerController* PlayerController,
	FVector2D ScreenPosition,
	const ARTACBoard& Board,
	const FRTACGrid& Grid,
	float TileSize,
	FRTACGridPosition& OutPosition);

/**
 * The pure geometry half of RTACScreenToGridPosition (AGENTS.md Rule 10): resolves a world-space
 * point already known to lie on Board's plane to the grid cell containing it.
 *
 * Factored out so this half -- InverseTransformPosition into Board's local space, floor (never
 * round, per RTACGridToLocalOffset's own doc above) to the containing tile, then bounds-check
 * against Grid -- is testable without a live camera or LocalPlayer, neither of which it depends
 * on. RTACScreenToGridPosition still owns the deprojection and the ray/plane intersection that
 * produces WorldPosition in the first place.
 *
 * @param WorldPosition  A world-space point already resolved onto Board's plane (e.g. the output
 *                       of FMath::RayPlaneIntersection against Board's Z=0 plane).
 * @param Board          Supplies the local coordinate frame via its own actor transform. Holds no
 *                       simulation state (Rule 5).
 * @param Grid           The live simulation grid to validate the resolved position against.
 * @param TileSize       World units (cm) per grid step -- must match the value used to place the
 *                       board's tiles via RTACGridToLocalOffset.
 * @param OutPosition    Set only when this returns true; left unmodified on failure.
 * @return true if WorldPosition falls inside Grid's bounds once converted to a grid cell.
 */
bool RTACWorldPositionToGridPosition(
	const FVector& WorldPosition,
	const ARTACBoard& Board,
	const FRTACGrid& Grid,
	float TileSize,
	FRTACGridPosition& OutPosition);
