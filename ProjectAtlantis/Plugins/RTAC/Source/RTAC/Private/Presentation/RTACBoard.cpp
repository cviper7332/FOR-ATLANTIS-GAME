#include "Presentation/RTACBoard.h"

ARTACBoard::ARTACBoard()
{
	// Pure configuration surface (Rows/Columns) -- nothing here needs a per-frame update.
	PrimaryActorTick.bCanEverTick = false;
}
