// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Simulation/RTACGridPosition.h"
#include "Simulation/RTACSurfaceModifier.h"

/**
 * One tile of the combat grid.
 *
 * Simulation-layer type (AGENTS.md Rule 5): plain struct, UE Core value types only, no
 * UObject/AActor ownership, no UPROPERTY, no reflection. Presentation reads this and renders
 * it; this never reaches back the other way.
 */
struct FRTACTile
{
	/** Grid position of this tile, rows x columns per Decision #5. */
	FRTACGridPosition Position;

	/**
	 * Entity currently occupying this tile, or INDEX_NONE when empty.
	 *
	 * A plain integer ID, never a pointer of any kind. Rule 5 (Addendum #2) and Decision #6
	 * bar UObject* / AActor* ownership from simulation state; the reasons that survive scrutiny
	 * are test cost, Rule 6 determinism, and future serialization — all of which an integer
	 * handle preserves and a pointer destroys. Resolving an ID to whatever actor renders it
	 * is a presentation-layer concern.
	 *
	 * INDEX_NONE is UE Core's standard sentinel (-1), defined in
	 * Runtime/Core/Public/Misc/CoreMiscDefines.h — verified against 5.8 source, not assumed.
	 */
	int32 OccupantEntityId = INDEX_NONE;

	/** Surface modifier on this tile. Only None exists at Phase 1 — see ERTACSurfaceModifier. */
	ERTACSurfaceModifier SurfaceModifier = ERTACSurfaceModifier::None;

	/**
	 * Discrete elevation level of this tile.
	 *
	 * ---------------------------------------------------------------------------------
	 * INTENTIONALLY INERT AT PHASE 1 — THIS IS NOT DEAD CODE. DO NOT DELETE.
	 *
	 * No logic anywhere reads this field yet, and that is correct. Decision #3 defers
	 * elevation's *mechanical direction* to Phase 6, but not its existence as a tile
	 * property — the decision's own text already calls elevation "a tile property
	 * independent of those BN3-style modifiers."
	 *
	 * The slot is reserved now because Rule 8's "Concrete risk here" note warns that if the
	 * core loop's tile model is built without it, elevation can only arrive later as a nested
	 * branch inside existing logic — which is Rule 8 being violated in advance. Phase 1's
	 * Definition of Done requires exactly this: "an elevation slot that exists but is
	 * mechanically inert."
	 *
	 * DOMAIN (Rule 10): this is a discrete LEVEL, not a world-space height in centimetres.
	 * Any conversion to world height is a presentation-side lookup from this value.
	 * ---------------------------------------------------------------------------------
	 */
	int32 Elevation = 0;

	/**
	 * True when some entity occupies this tile.
	 *
	 * Kept as an accessor because it encodes the INDEX_NONE sentinel convention, which is not
	 * evident from the field alone. Row/Column need no equivalent — Position.Row and
	 * Position.Column already say what they are.
	 */
	bool IsOccupied() const { return OccupantEntityId != INDEX_NONE; }
};
