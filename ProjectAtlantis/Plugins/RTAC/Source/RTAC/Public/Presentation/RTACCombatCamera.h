#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTACCombatCamera.generated.h"

class ARTACBoard;
class UCameraComponent;

/**
 * Phase 2 Part B's isometric-2.5D combat camera (Decision #1) -- the first camera actor to exist
 * anywhere in RTAC, and the actor whose orientation Decision #15 constrains.
 *
 * Rule 5: holds NO simulation state. It reads ARTACBoard's Rows/Columns/TileSize, which are
 * presentation configuration, and never touches FRTACGrid or FRTACMatchState.
 *
 * ORIENTATION IS NOT A FREE CHOICE. Decision #15 fixes the on-screen direction table (Row+ reads
 * higher on screen, Column+ reads further screen-right) and states that the camera is constrained
 * by that decision rather than the reverse. Decision #16 then showed which yaw range can satisfy
 * it: with Row mapped to local X and Column to local Y, any Yaw in (-90, 90) with a negative
 * Pitch works, and Yaw 0 makes the correspondence exact rather than merely correct in sign. The
 * clamps on PitchDegrees and YawDegrees below are that result, enforced -- a positive Pitch or a
 * yaw outside that range silently inverts or mirrors the table, which would make every
 * screen-click resolve to a plausible-looking but wrong tile (Failure Mode 4).
 *
 * THIS ACTOR'S TRANSFORM IS COMPUTED, NOT AUTHORED. FrameBoard() overwrites location and rotation
 * from Board's dimensions and the knobs below, on every construction pass. Dragging this actor in
 * the viewport will be undone; tune it with the properties instead. That is the point: the
 * placement derivation stays executable rather than decaying into a magic triple in a comment.
 */
UCLASS()
class ARTACCombatCamera : public AActor
{
	GENERATED_BODY()

public:
	ARTACCombatCamera();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/**
	 * Recompute this actor's transform from Board's dimensions and the framing knobs. Safe to
	 * call repeatedly; a no-op when Board is unset or its dimensions are degenerate.
	 */
	void FrameBoard();

	/**
	 * The board to frame. EditInstanceOnly, not EditAnywhere: a level-actor reference cannot be
	 * bound on a class default and must be set on the placed instance (CLAUDE.md -> UE5.8 API
	 * Gotchas -> UPROPERTY).
	 */
	UPROPERTY(EditInstanceOnly, Category = "RTAC|Camera")
	TObjectPtr<ARTACBoard> Board;

	/** Downward pitch. Must stay negative -- see the orientation note above. */
	UPROPERTY(EditAnywhere, Category = "RTAC|Camera", meta = (ClampMin = "-89.0", ClampMax = "-1.0"))
	float PitchDegrees = -40.0f;

	/** Yaw about the board. 0 makes Decision #15's table exact; the clamp is Decision #16's range. */
	UPROPERTY(EditAnywhere, Category = "RTAC|Camera", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float YawDegrees = 0.0f;

	/** Authored horizontal FOV -- but see MinAspectRatio; this is not applied as horizontal here. */
	UPROPERTY(EditAnywhere, Category = "RTAC|Camera", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float FieldOfViewDegrees = 60.0f;

	/**
	 * The narrowest viewport aspect ratio the framing is guaranteed against. This exists because
	 * FieldOfView is NOT applied as a horizontal FOV in this project: BaseEngine.ini sets
	 * AspectRatio_MaintainYFOV, so vertical FOV is held fixed and horizontal FOV shrinks as the
	 * window narrows -- cropping the board's wide axis, which is the 6-column one. Framing is
	 * therefore computed at this aspect and is safe at anything wider. Full derivation, with
	 * engine source citations, in docs/reference.md -> Camera and Projection (UE5.8).
	 */
	UPROPERTY(EditAnywhere, Category = "RTAC|Camera", meta = (ClampMin = "1.0"))
	float MinAspectRatio = 1.777778f;

	/** Slack beyond the exact fit distance. 1.0 is edge-to-edge with no breathing room. */
	UPROPERTY(EditAnywhere, Category = "RTAC|Camera", meta = (ClampMin = "1.0"))
	float FramingMargin = 1.15f;

	/**
	 * Make this the player's view target on BeginPlay. This call is the camera-swap seam: UE
	 * routes deprojection through whatever actor is the current view target, so swapping cameras
	 * is a SetViewTarget call and requires no change to RTACScreenToGridPosition or anything
	 * under Simulation/.
	 */
	UPROPERTY(EditAnywhere, Category = "RTAC|Camera")
	bool bAutoSetAsViewTarget = true;

private:
	UPROPERTY(VisibleAnywhere, Category = "RTAC|Camera")
	TObjectPtr<UCameraComponent> Camera;
};
