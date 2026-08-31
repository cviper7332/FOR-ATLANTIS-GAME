// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Derives an independent, reproducible RNG stream seed from a match's master seed and a
 * stream identifier.
 *
 * One master seed per match produces N independent streams — one per consuming system — rather
 * than every system sharing a single sequence. The point is decoupling: under a shared stream,
 * changing how many entities spawn shifts what every later draw in the match receives, even in
 * systems with no logical connection to spawning, so a recorded repro seed stops reproducing
 * after any unrelated change. Rule 6's stated purpose is that a match be reproducible enough for
 * a bug report to be actionable; per-stream derivation preserves that across code changes, a
 * shared stream only preserves it for an unchanged build.
 *
 * Pure function: same inputs always yield the same output, on every platform and every run. It
 * reads nothing environmental — no time, no addresses, no global state.
 *
 * STREAM NAMES ARE A PERSISTED CONTRACT, NOT LABELS. The name is an input to the seed. Renaming
 * "EntitySpawn" to "Spawn" silently changes that stream's entire sequence and invalidates every
 * repro seed ever recorded against it. Treat a rename exactly as you would a save-format change.
 *
 * @param MasterSeed  The match's single root seed, held in FRTACRngState.
 * @param StreamName  Stable identifier for the consuming system, e.g. TEXT("EntitySpawn").
 * @return A seed suitable for FRandomStream::Initialize(int32).
 */
int32 RTACDeriveStreamSeed(int32 MasterSeed, const TCHAR* StreamName);
