// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Surface modifier occupying a tile, in the MMBN3 tradition (ice, grass, lava, steel,
 * poison, cracked, broken panels).
 *
 * MOSTLY UNPOPULATED AT PHASE 1, WITH ONE NARROW EXCEPTION. The full modifier list (ice,
 * grass, lava, steel, poison, cracked) and its resolution effects remain Phase 3 scope per
 * docs/PHASES.md ("Attacks, HP/Damage & Tile Modifiers") — this header reserves the slot so
 * Phase 3 adds the rest additively, with no structural change to FRTACTile and no new field
 * threaded through existing code.
 *
 * `Broken` is the one exception, added ahead of Phase 3 by Decision #10's August 31, 2026
 * addendum ("Broken value scoped exception"). Movement-legality checking (Decision #10
 * Ruling 4's "not broken" clause) is Phase 1's own stated concern, and had nothing real to
 * compare against without it. Nothing else in the eventual full list is added by that
 * addendum, and Phase 3's scope over the rest of the list is otherwise unchanged.
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
	None = 0,
	Broken
};
