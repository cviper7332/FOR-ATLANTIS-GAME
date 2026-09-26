#include "Presentation/RTACBoard.h"

#include "Components/SceneComponent.h"

ARTACBoard::ARTACBoard()
{
	// Pure configuration surface (Rows/Columns) -- nothing here needs a per-frame update.
	PrimaryActorTick.bCanEverTick = false;

	// Without a root component this actor's transform is permanently FTransform::Identity
	// (Actor.h:1561), so it cannot be placed anywhere but the world origin, and every downstream
	// Board.GetActorTransform() -- the conversion functions, ARTACCombatCamera's framing -- is
	// silently operating on identity. Confirmed live before this fix: get_root_component returned
	// null, and a placement request at Z=2000 no-opped to (0,0,0).
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}
