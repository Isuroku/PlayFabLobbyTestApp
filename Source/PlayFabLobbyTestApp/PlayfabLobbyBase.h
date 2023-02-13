// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PubSubRequester.h"
#include "PlayFabMultiplayerDataModels.h"
#include "PlayFabClientAPI.h"
#include "IPlayfabDataProvider.h"
#include "CommonConstants.h"
#include "PlayfabLobbyBase.generated.h"

/**
 * 
 */
UCLASS()
class PLAYFABLOBBYTESTAPP_API UPlayfabLobbyBase : public UObject
{
	GENERATED_BODY()
public:

	virtual void BeginDestroy() override;
	
	IPlayfabDataProvider* GetDataProvider() const { return DataProvider_; }

	DECLARE_EVENT_TwoParams(ULobbyKeeper, FOnLobbyCreatedEvent, const FString& /*InLobbyId*/, const FString& /*InConnectionString*/)
	FOnLobbyCreatedEvent& OnLobbyCreatedEvent() { return  OnLobbyCreatedHandle_; }
	FOnLobbyCreatedEvent& OnLobbyJoinEvent() { return  OnLobbyJoinHandle_; }

	DECLARE_EVENT_OneParam(ULobbyKeeper, FOnFindLobbiesEvent, const PlayFab::MultiplayerModels::FFindLobbiesResult& /*InResult*/)
	FOnFindLobbiesEvent& OnFindLobbiesEvent() { return  OnFindLobbiesHandle_; }

protected:
	void Init(IPlayfabDataProvider* InDataProvider);

	void CreateLobby(const FSearchData& InSearchData, const FServerLobbyData& InServerLobbyData, int32 InMaxPlayer = 2);
	void CreateLobby();

	void FindLobby(const FSearchData& InSearchData);
	virtual void FindLobby();
	virtual void OnFindLobbies(const PlayFab::MultiplayerModels::FFindLobbiesResult& InResult);

	void JoinLobby(const FString& InLobbyId, const FString& InConnectionString);
	virtual void OnJoinLobby(const PlayFab::MultiplayerModels::FJoinLobbyResult& InResult);
	virtual void OnJoinLobbyError(const PlayFab::FPlayFabCppError& InError);

	virtual void OnCheckLobbyHandle(const PlayFab::MultiplayerModels::FGetLobbyResult& InResult) {};
	virtual void OnCheckLobbyErrorHandle() {};

	void LeaveLobby();
	virtual void OnLeaveLobby(const PlayFab::MultiplayerModels::FLobbyEmptyResult& InResult);
	virtual void OnLeaveLobbyError(const PlayFab::FPlayFabCppError& InError);

	void SetOpenToFind(bool InValue);

	bool OpenToFind_ = true;
	
private:
	void OnCreateLobby(const PlayFab::MultiplayerModels::FCreateLobbyResult& InResult);
	void OnCreateLobbyError(const PlayFab::FPlayFabCppError& InError);	
	
	void OnFindLobbiesError(const PlayFab::FPlayFabCppError& InError);

	void SignalRLogin();
	void SignalRClose();
	void OnSignalRSessionOpenError(const FString& InError);
	void OnSignalRSessionOpenSuccess(const FString& InConnectionHandle, bool InReconnecting);
	void OnSignalRSessionClosed(bool InUnexpected);

	void SubscribeToLobbyChange();
	void OnSubscribeToLobby(const PlayFab::MultiplayerModels::FSubscribeToLobbyResourceResult& InResult);
	void OnSubscribeToLobbyError(const PlayFab::FPlayFabCppError& InError);

	void CheckLobby();
	void OnCheckLobby(const PlayFab::MultiplayerModels::FGetLobbyResult& InResult);
	void OnCheckLobbyError(const PlayFab::FPlayFabCppError& InError);

	IPlayfabDataProvider* DataProvider_;

	FSearchData SearchData_;
	FServerLobbyData ServerLobbyData_;
	int32 MaxPlayer_ = 2;

	FOnLobbyCreatedEvent OnLobbyCreatedHandle_;
	FOnLobbyCreatedEvent OnLobbyJoinHandle_;
	FOnFindLobbiesEvent OnFindLobbiesHandle_;

	FString LobbyId_;
	FString LobbyConnectionString_;

	TSharedPtr<FPubSubRequester> PubSubRequester_;
};
