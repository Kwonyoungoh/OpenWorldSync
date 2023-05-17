// Copyright Epic Games, Inc. All Rights Reserved.

#include "OpenWorldSyncGameMode.h"
#include "OpenWorldSyncCharacter.h"
#include "UObject/ConstructorHelpers.h"

AOpenWorldSyncGameMode::AOpenWorldSyncGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
