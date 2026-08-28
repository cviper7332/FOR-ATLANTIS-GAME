// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Surface modifier occupying a tile, in the MMBN3 tradition (ice, grass, lava, steel,
 * poison, cracked, broken panels).
 *
 * DELIBERATELY UNPOPULATED AT PHASE 1. Only the None default exists here. The actual
 * modifier list and its resolution effects are Phase 3 scope per docs/PHASES.md
 * ("Attacks, HP/Damage & Tile Modifiers") — this header reserves the slot so Phase 3 adds
 * enumerators additively, with no structural change to FRTACTile and no new field threaded
 * through existing code.
 *
 * None is pinned to 0 so a default-constructed FRTACTile is unmodified, and so future
 * additions stay strictly append-only.
 *
 * Not a UENUM: this is a simulation-layer type and carries no reflection (AGENTS.md Rule 5).
 * If the presentation layer later needs this exposed to Blueprints, that exposure belongs on
 * the presentation side, not here.
 */
enum class ERTACSurfaceModifier : uint8
{
	None = 0
};
