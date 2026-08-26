// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTACModule.h"

#define LOCTEXT_NAMESPACE "FRTACModule"

DEFINE_LOG_CATEGORY(LogRTAC);

void FRTACModule::StartupModule()
{
	UE_LOG(LogRTAC, Log, TEXT("RTAC module loaded."));
}

void FRTACModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRTACModule, RTAC)
