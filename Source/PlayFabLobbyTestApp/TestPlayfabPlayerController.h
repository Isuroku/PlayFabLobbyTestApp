// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IPlayfabDataProvider.h"
#include "GameFramework/PlayerController.h"
#include "TestMenuWidget.h"
#include "PubSubRequester.h"
#include "PlayFabMultiplayerDataModels.h"
#include "PlayFabClientAPI.h"
#include "PlayFabServerDataModels.h"
#include "TestPlayfabPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class PLAYFABLOBBYTESTAPP_API ATestPlayfabPlayerController : public APlayerController, public IPlayfabDataProvider
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UTestMenuWidget> MenuWidgetClass;

	virtual const TCHAR* GetTitleId() const override { return TitleID_; }
	virtual const FString& GetTitlePlayerID() const override { return PlayfabTitlePlayerID_; }
	virtual const FString& GetEntityToken() const override { return EntityToken_; }
	const TCHAR* GetDefaultSecretKey() const { return DefaultSecretKey_; }

	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintCallable)
	void LoginAsPlayer(const FString& InName);

	UFUNCTION(BlueprintCallable)
	void LoginAsServer(const FString& InName);

	FString ToString() const { return FString::Printf(TEXT("PlayerName: %s; PlayfabID: %s; PlayfabTitlePlayerID_: %s, EntityTokenExpiration_: %s; EntityToken_: %s"),
		*PlayerName_, *PlayfabID_, *PlayfabTitlePlayerID_, *EntityTokenExpiration_.ToIso8601(), *EntityToken_); }

	bool IsLogged() const { return !PlayfabTitlePlayerID_.IsEmpty(); }

	bool HasMMTicket() const { return !MMTicket_.IsEmpty(); }

	UFUNCTION(BlueprintCallable)
	void StartMM();

	UFUNCTION(BlueprintCallable)
	void StopMM();

	UFUNCTION(BlueprintCallable)
	void StartCreateLobby(const TArray<FString>& InSearchParams);

	UFUNCTION(BlueprintCallable)
	void GetLobby(const FString& InLobbyId);

	UFUNCTION(BlueprintCallable)
	void FindLobby(const TArray<FString>& InSearchParams);

	UFUNCTION(BlueprintCallable)
	void JoinLobby(const FString& InLobbyId);

	UFUNCTION(BlueprintCallable)
	void LeaveLobby();

	UFUNCTION(BlueprintCallable)
	void CreateLobbyKeeper();

	UFUNCTION(BlueprintCallable)
	void DeleteLobbyKeeper();

	UFUNCTION(BlueprintCallable)
	void CreateLobbyClient();

	UFUNCTION(BlueprintCallable)
	void DeleteLobbyClient();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void WriteLog(const FString& inText) const;

	void ServerLogin();
	void OnSuccessServerLogin(const PlayFab::ServerModels::FServerLoginResult& inResult);
	void OnServerLoginError(const PlayFab::FPlayFabCppError& inError);
	
	void Login();
	void OnSuccessLogin(const PlayFab::ClientModels::FLoginResult& inResult);
	void LoginError(const PlayFab::FPlayFabCppError& inError);

	void CreatedMatchmakingTicket(const PlayFab::MultiplayerModels::FCreateMatchmakingTicketResult& Result);
	void CreateMatchmakingTicketError(const PlayFab::FPlayFabCppError& Result);

	void CheckMatchmakingTicketStatus();
	void CheckedMatchmakingTicketStatus(const PlayFab::MultiplayerModels::FGetMatchmakingTicketResult& Result);
	void CheckMatchmakingTicketStatusError(const PlayFab::FPlayFabCppError& Result);
	void GotMatch(const PlayFab::MultiplayerModels::FGetMatchResult& Result);
	void GetMatchError(const PlayFab::FPlayFabCppError& InError);

	void OnGetLobby(const PlayFab::MultiplayerModels::FGetLobbyResult& InResult);
	void OnGetLobbyError(const PlayFab::FPlayFabCppError& InError);

	void CreateLobby();
	void OnCreateLobby(const PlayFab::MultiplayerModels::FCreateLobbyResult& InResult);
	void OnCreateLobbyError(const PlayFab::FPlayFabCppError& InError);
	void OnLobbyCreatedOrJoin(const FString& InLobbyId, const FString& InConnectionString);

	void OnFindLobbies(const PlayFab::MultiplayerModels::FFindLobbiesResult& InResult);
	void OnFindLobbiesError(const PlayFab::FPlayFabCppError& InError);

	void SubscribeToLobbyChange();
	void OnSubscribeToLobby(const PlayFab::MultiplayerModels::FSubscribeToLobbyResourceResult& InResult);
	void OnSubscribeToLobbyError(const PlayFab::FPlayFabCppError& InError);

	void SignalRLogin();
	void OnSignalRSessionOpenError(const FString& InError);
	void OnSignalRSessionOpenSuccess(const FString& InConnectionHandle, bool InReconnecting);
	void OnSignalRSessionClosed(bool InUnexpected);

	void OnJoinLobby(const PlayFab::MultiplayerModels::FJoinLobbyResult& InResult);
	void OnJoinLobbyError(const PlayFab::FPlayFabCppError& InError);

	void OnLeaveLobby(const PlayFab::MultiplayerModels::FLobbyEmptyResult& InResult);
	void OnLeaveLobbyError(const PlayFab::FPlayFabCppError& InError);
	
	const TCHAR* TitleID_ = TEXT("A2545");
	const TCHAR* DefaultSecretKey_ = TEXT("73E68RPHN5MDM3TPXFJ17YKWM5A9XRJBP8JZBOSRC93973ENOT");

	class UTestMenuWidget* MenuWidget_;

	FString PlayerName_;
	FString PlayfabID_;
	FString PlayfabTitlePlayerID_; //L"title_player_account"
	FString SessionTicket_;
	FString EntityToken_;
	FDateTime EntityTokenExpiration_;
	FString MMTicket_;

	FTimerHandle MMTicketStatusHandle_;
	FTimerHandle LoginExpiredTimerHandle_;
	
	FTimerHandle LobbiesStatusHandle_;

	FString LobbyId_;
	FString LobbyConnectionString_;
	
	TArray<FString> LobbiesSearchParams_;

	TSharedPtr<FPubSubRequester> PubSubRequester_;

	TArray<PlayFab::MultiplayerModels::FLobbySummary> FoundLobbies_;

	UPROPERTY()
	class ULobbyKeeper* LobbyKeeper_;

	UPROPERTY()
	class ULobbyClient* LobbyClient_;
};
