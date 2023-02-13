// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PubSubRequester.h"
#include "PlayFabMultiplayerDataModels.h"
#include "PlayFabClientAPI.h"
#include "IPlayfabDataProvider.h"
#include "PlayfabLobbyBase.h"
#include "LobbyKeeper.generated.h"

/**
 * 
 */
UCLASS()
class PLAYFABLOBBYTESTAPP_API ULobbyKeeper : public UPlayfabLobbyBase
{
	GENERATED_BODY()
public:

	void Start(IPlayfabDataProvider* InDataProvider, const FSearchData& InSearchData, const FServerLobbyData& InServerLobbyData);

protected:
	virtual void OnCheckLobbyHandle(const PlayFab::MultiplayerModels::FGetLobbyResult& InResult) override;
	virtual void OnCheckLobbyErrorHandle() override;

	
private:

};
