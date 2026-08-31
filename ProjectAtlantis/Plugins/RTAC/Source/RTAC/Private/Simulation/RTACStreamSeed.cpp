// Copyright Epic Games, Inc. All Rights Reserved.

#include "Simulation/RTACStreamSeed.h"

#include "Misc/Crc.h"
#include "Templates/TypeHash.h"

int32 RTACDeriveStreamSeed(int32 MasterSeed, const TCHAR* StreamName)
{
	// FCrc::StrCrc32 is table-driven and pure, and its static_assert enforces char-width
	// consistency so equivalent strings hash identically regardless of CharType.
	const uint32 NameHash = FCrc::StrCrc32(StreamName);

	// HashCombine, NOT HashCombineFast. HashCombine's own declaration carries the guarantee
	// "This function cannot change for backward compatibility reasons"
	// (Runtime/Core/Public/Templates/TypeHash.h) — exactly the stability a persisted seed
	// derivation needs. HashCombineFast makes no such promise; TypeHash.h's own usage note
	// justifies it only "because pointers are non-persistent." Swapping to it would silently
	// reseed every stream on some future engine upgrade and invalidate every recorded repro.
	return static_cast<int32>(HashCombine(static_cast<uint32>(MasterSeed), NameHash));
}
