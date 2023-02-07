// Copyright Epic Games, Inc. All Rights Reserved.


#include "PlayFabLobbyTestAppGameModeBase.h"

#include "TestPlayfabPlayerController.h"

APlayFabLobbyTestAppGameModeBase::APlayFabLobbyTestAppGameModeBase()
{
	PlayerControllerClass = ATestPlayfabPlayerController::StaticClass();
}
