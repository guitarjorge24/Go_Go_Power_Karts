// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/HUD.h"
#include "GoGoPowerKartsHud.generated.h"


UCLASS(config = Game)
class AGoGoPowerKartsHud : public AHUD
{
	GENERATED_BODY()

public:
	AGoGoPowerKartsHud();

	/** Font used to render the vehicle info */
	UPROPERTY()
	UFont* HUDFont;

	// Begin AHUD interface
	virtual void DrawHUD() override;
	// End AHUD interface
};
