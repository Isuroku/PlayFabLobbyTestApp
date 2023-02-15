// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PlayfabLobbyBase.h"
#include "Engine/EngineTypes.h"
#include "LobbyClient.generated.h"

// UENUM()
// enum class ELobbyClientStates : uint8
// {
// 	Created = 0,
// 	FindLobby,
// 	WaitJoining,
// 	CheckLobby,
// 	FailState,
// };

/**
 * 
 */
UCLASS()
class PLAYFABLOBBYTESTAPP_API ULobbyClient : public UPlayfabLobbyBase
{
	GENERATED_BODY()
public:
	virtual void BeginDestroy() override;
	
	void Start(IPlayfabDataProvider* InDataProvider, const FLobbyParameters& InSearchData);

	DECLARE_EVENT(ULobbyKeeper, FOnFailFindLobbiesEvent)
	FOnFailFindLobbiesEvent& OnFailFindLobbiesEvent() { return  OnFailFindLobbiesHandle_; }

protected:
	virtual void FindLobby() override;
	virtual void OnFindLobbies(const PlayFab::MultiplayerModels::FFindLobbiesResult& InResult) override;
	virtual void OnCheckLobbyHandle(const PlayFab::MultiplayerModels::FGetLobbyResult& InResult) override;
	virtual void OnCheckLobbyErrorHandle() override;
	virtual void OnLeaveLobby(const PlayFab::MultiplayerModels::FLobbyEmptyResult& InResult) override;

	virtual void OnJoinLobbyError(const PlayFab::FPlayFabCppError& InError) override;
	
private:
	
	FOnFailFindLobbiesEvent OnFailFindLobbiesHandle_;
	
	bool bReFindLobby = true;

	static float FindTime;
	FTimerHandle FindTimer_;

	static uint8 MaxFindLobbyCounter_;
	uint8 FindLobbyCounter_ = 0;
};
