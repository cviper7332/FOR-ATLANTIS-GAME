#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/RTACGrid.h"
#include "RTACBoard.generated.h"

/**
 * Presentation-layer wrapper exposing the combat board's dimensions to the level author, per
 * Decision #8's outstanding half: "the presentation-layer wrapper exposing editable
 * Rows/Columns in the Details panel."
 *
 * Rule 5 boundary, stated explicitly: this actor holds NO simulation state. It does not own an
 * FRTACGrid or FRTACMatchState, and it does not call FRTACGrid::Init() itself. It is only the
 * editor-facing configuration surface -- whoever owns and initialises the match's
 * FRTACMatchState reads Rows/Columns from here and passes them to Grid.Init(Rows, Columns),
 * exactly as RTACMatchState.h's own doc comment already describes that call shape. That owner
 * (a game mode, a subsystem, or a dedicated match controller) is a separate, larger
 * presentation-architecture question this actor deliberately does not answer.
 *
 * Defaults are read FROM FRTACGrid::DefaultRows/DefaultColumns rather than restating 3 and 6 as
 * independent literals -- RTACGrid.h's own header comment asks for exactly this, so the 3x6
 * default (Decision #8) keeps one source of truth (Failure Mode 7).
 */
UCLASS()
class ARTACBoard : public AActor
{
	GENERATED_BODY()

public:
	ARTACBoard();

	/**
	 * Row count for the combat grid. Defaults to FRTACGrid::DefaultRows (3), per Decision #8.
	 * Clamped to >= 1 in the Details panel, matching FRTACGrid::Init()'s documented precondition.
	 */
	UPROPERTY(EditAnywhere, Category = "RTAC|Grid", meta = (ClampMin = "1"))
	int32 Rows = FRTACGrid::DefaultRows;

	/**
	 * Column count for the combat grid. Defaults to FRTACGrid::DefaultColumns (6), per
	 * Decision #8. Clamped to >= 1, matching FRTACGrid::Init()'s documented precondition.
	 */
	UPROPERTY(EditAnywhere, Category = "RTAC|Grid", meta = (ClampMin = "1"))
	int32 Columns = FRTACGrid::DefaultColumns;
};
