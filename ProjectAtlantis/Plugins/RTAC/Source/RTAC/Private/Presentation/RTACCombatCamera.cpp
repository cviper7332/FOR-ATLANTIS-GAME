#include "Presentation/RTACCombatCamera.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Presentation/RTACBoard.h"
#include "RTACModule.h" // LogRTAC -- Rule 9: dedicated category, never LogTemp.

ARTACCombatCamera::ARTACCombatCamera()
{
	// Placement is recomputed on construction, not per frame.
	PrimaryActorTick.bCanEverTick = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;

	// Perspective, not orthographic: Decision #1 rejected orthographic by name, wanting "a real
	// depth cue that orthographic can't provide." Note that a TRUE isometric projection is
	// orthographic, so Decision #1 uses "isometric" in the loose 3/4-view sense.
	Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
	Camera->SetFieldOfView(FieldOfViewDegrees); // read from the member -- one source of truth.
}

void ARTACCombatCamera::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	FrameBoard();
}

void ARTACCombatCamera::FrameBoard()
{
	// Silent on a null Board: OnConstruction runs constantly in-editor and would spam. BeginPlay
	// logs it once instead, where it actually matters.
	if (Board == nullptr || Camera == nullptr)
	{
		return;
	}

	const float TileSize = Board->TileSize;
	const int32 Rows = Board->Rows;
	const int32 Columns = Board->Columns;
	if (TileSize <= 0.0f || Rows < 1 || Columns < 1 || Camera->AspectRatio <= 0.0f)
	{
		return;
	}

	// Vertical half-FOV is what this project actually holds fixed (AspectRatio_MaintainYFOV in
	// BaseEngine.ini), so the horizontal angle must be derived from it at the narrowest aspect
	// we guarantee -- NOT read off FieldOfViewDegrees directly. See MinAspectRatio's doc and
	// docs/reference.md -> Camera and Projection (UE5.8).
	const float HalfFovAuthored = FMath::DegreesToRadians(FieldOfViewDegrees * 0.5f);
	const float HalfFovVertical = FMath::Atan(FMath::Tan(HalfFovAuthored) / Camera->AspectRatio);
	const float HalfFovHorizontal = FMath::Atan(FMath::Tan(HalfFovVertical) * MinAspectRatio);

	const float TanHalfH = FMath::Tan(HalfFovHorizontal);
	if (TanHalfH <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Columns run along local Y (Decision #16), so the 6-wide axis is the one framing must fit.
	const float HalfWidth = Columns * TileSize * 0.5f;
	const float Distance = (HalfWidth / TanHalfH) * FramingMargin;

	// Computed in board-local space, then composed onto the board's own transform, so a rotated
	// or moved board carries its camera with it and Decision #15's table holds relative to the
	// board's axes rather than to the world's.
	const FVector CenterLocal(Rows * TileSize * 0.5f, Columns * TileSize * 0.5f, 0.0f);
	const FRotator RotationLocal(PitchDegrees, YawDegrees, 0.0f);
	const FVector LocationLocal = CenterLocal - Distance * RotationLocal.Vector();

	SetActorLocationAndRotation(
		Board->GetActorTransform().TransformPosition(LocationLocal),
		Board->GetActorQuat() * RotationLocal.Quaternion());

	Camera->SetFieldOfView(FieldOfViewDegrees);
}

void ARTACCombatCamera::BeginPlay()
{
	Super::BeginPlay();

	if (Board == nullptr)
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("ARTACCombatCamera '%s': no Board set, camera is unframed. Board is "
				 "EditInstanceOnly -- set it on the placed instance, not the class default."),
			*GetName());
		return;
	}

	FrameBoard();

	if (!bAutoSetAsViewTarget)
	{
		return;
	}

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerController->SetViewTarget(this);
	}
	else
	{
		UE_LOG(LogRTAC, Warning,
			TEXT("ARTACCombatCamera '%s': bAutoSetAsViewTarget is set but no PlayerController "
				 "exists; view target unchanged."),
			*GetName());
	}
}
