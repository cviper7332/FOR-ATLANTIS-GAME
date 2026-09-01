// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Which side a tile belongs to, per Decision #10 Ruling 3.
 *
 * EXTERNALLY ASSIGNED, NEVER COMPUTED. Ownership is data set per tile at battle setup, not a
 * value derived at runtime from Rows/Columns and a hardcoded left-half/right-half split.
 * Decision #10's own text is explicit about why: mainline battles default to a symmetric 3x3/3x3
 * split, but Liberation Mission-style battles (MMBN5) can start asymmetric — the player
 * surrounded, or one side starting with more field than the other — set by "authored rules," not
 * derivable from grid dimensions. A computed split cannot represent that; a per-tile field can.
 *
 * The authoring/assignment mechanism itself — however a battle's tiles actually get set to
 * Player/Enemy/Neutral before combat starts — is explicitly out of scope here and is not built
 * by the field's existence. This header reserves the shape only.
 *
 * Neutral is pinned to 0 so a default-constructed FRTACTile starts Neutral, matching every other
 * sentinel-style default in this tree (FRTACEntity::EntityId defaults to INDEX_NONE, not 0, for
 * the identical reason: an unassigned tile must be distinguishable from a real assignment, not
 * silently alias to Player or Enemy).
 *
 * CORRECTED, August 31, 2026: an earlier version of this comment claimed Neutral was "a real,
 * meaningful value in BN3 terms too (nobody's territory), not merely a placeholder." That claim
 * is wrong. Confirmed against BN3 source material: tiles are always exactly Player or Enemy in
 * the actual games — no neutral tiles exist as a resting gameplay state, ever. Neutral exists
 * here solely as a construction-time/unassigned-sentinel value, the same role INDEX_NONE plays
 * for FRTACEntity::EntityId — not as a value a tile is meant to hold once a battle-setup step
 * (Decision #10 Ruling 3's still-unbuilt authoring mechanism) has actually run. Every tile must
 * be assigned Player or Enemy before a match starts; Neutral surviving past that point is a sign
 * the authoring step didn't complete, not a valid game state.
 *
 * One narrow, deliberate exception: a tile CAN legitimately become Neutral mid-battle, but only
 * as the direct, authored consequence of an entity's own side changing while standing on it —
 * see combat_decisions.md → Open Questions → "Entity Allegiance Change — Mid-Battle Defection to
 * a Third Party." That is a narratively-triggered event, not an unassigned default, and this
 * correction does not apply to it.
 *
 * Not a UENUM: this is a simulation-layer type and carries no reflection (AGENTS.md Rule 5). If
 * the presentation layer or a future authoring tool needs this exposed to Blueprints or the
 * editor, that exposure belongs on the presentation side, not here.
 */
enum class ERTACTileOwner : uint8
{
	Neutral = 0,
	Player,
	Enemy
};
