// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoGoPowerKartsGameMode.h"
#include "GoGoPowerKartsPawn.h"
#include "GoGoPowerKartsHud.h"

AGoGoPowerKartsGameMode::AGoGoPowerKartsGameMode()
{
	DefaultPawnClass = AGoGoPowerKartsPawn::StaticClass();
	HUDClass = AGoGoPowerKartsHud::StaticClass();
}
