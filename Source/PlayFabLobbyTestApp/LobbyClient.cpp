// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyClient.h"

#include "CommonConstants.h"
#include "LogUtility.h"
#include "PlayFabMultiplayerAPI.h"
#include "TimerManager.h"
#include "Algo/IndexOf.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LobbyClient, Log, All);

float ULobbyClient::FindTime = 5;
uint8 ULobbyClient::MaxFindLobbyCounter_ = 3;

void ULobbyClient::BeginDestroy()
{
	bReFindLobby = false;
	if(GetDataProvider() != nullptr && GetDataProvider()->GetWorld() != nullptr)
	{
		GetDataProvider()->GetWorld()->GetTimerManager().ClearTimer(FindTimer_);
	}
	
	Super::BeginDestroy();
}

void ULobbyClient::Start(IPlayfabDataProvider* InDataProvider, const FLobbyParameters& InSearchData)
{
	LOG_FUNC_LABEL(LobbyClient);
	Init(InDataProvider);
	OpenToFind_ = false;
	FindLobbyCounter_ = 0;
	Super::FindLobby(InSearchData);
}

void ULobbyClient::FindLobby()
{
	FindLobbyCounter_++;
	LOG_MSGF(LobbyClient, TEXT("FindLobbyCounter: %d"), FindLobbyCounter_);

	if(FindLobbyCounter_ <= MaxFindLobbyCounter_)
		Super::FindLobby();
	else
	{
		LOG_MSG(LobbyClient, TEXT("Max find lobby attampt"));
		OnFailFindLobbiesHandle_.Broadcast();
	}
}

void ULobbyClient::OnFindLobbies(const PlayFab::MultiplayerModels::FFindLobbiesResult& InResult)
{
	LOG_FUNC_LABEL(LobbyClient);
	
	Super::OnFindLobbies(InResult);

	if(InResult.Lobbies.Num() > 0)
	{
		JoinLobby(InResult.Lobbies[0].LobbyId, InResult.Lobbies[0].ConnectionString);		
	}
	else
	{
		LOG_MSG(LobbyClient, TEXT("Start timer for FindLobby"));		
		FTimerManager& TimerManager = GetDataProvider()->GetWorld()->GetTimerManager();
		TimerManager.SetTimer(FindTimer_, this, &ULobbyClient::FindLobby, FindTime, false);
	}
}

void ULobbyClient::OnJoinLobbyError(const PlayFab::FPlayFabCppError& InError)
{
	Super::OnJoinLobbyError(InError);

	LOG_FUNC_LABEL(LobbyClient);
	// LobbyBadRequest	13007
	// LobbyDoesNotExist	13000
	// LobbyMemberCannotRejoin	13004
	// LobbyNotJoinable	13003
	// LobbyPlayerAlreadyJoined	13002
	// LobbyPlayerMaxLobbyLimitExceeded	13008
	// LobbyRateLimitExceeded	13001
	
	if(InError.ErrorCode == 13000 || //LobbyDoesNotExist
		InError.ErrorCode == 13004 || //LobbyMemberCannotRejoin
		InError.ErrorCode == 13003 || //LobbyNotJoinable
		InError.ErrorCode == 13008 //LobbyPlayerMaxLobbyLimitExceeded
		)
	{
		FindLobby();
	}
}

void ULobbyClient::OnCheckLobbyHandle(const PlayFab::MultiplayerModels::FGetLobbyResult& InResult)
{
	LOG_MSGF(LobbyClient, TEXT("InResult: %s"), *InResult.pfLobby.LobbyId);
	Super::OnCheckLobbyHandle(InResult);

	for (const auto& Data : InResult.pfLobby.LobbyData)
	{
		LOG_MSGF(LobbyClient, TEXT("%s: %s"), *Data.Key, *Data.Value);
	}

	for (const auto& Member : InResult.pfLobby.Members)
	{
		LOG_MSGF(LobbyClient, TEXT("Member: %s"), *Member.MemberEntity->Id);
	}

	// FString TitlePlayerId = GetDataProvider()->GetTitlePlayerID();
	// const int32 index = Algo::IndexOfByPredicate(InResult.pfLobby.Members, [TitlePlayerId](const PlayFab::MultiplayerModels::FMember& Member)
	// 	 { return Member.MemberEntity->Id == TitlePlayerId; });

	if(InResult.pfLobby.Members.Num() > 2)
	{
		LeaveLobby();
	}
}

void ULobbyClient::OnCheckLobbyErrorHandle()
{
	LOG_FUNC_LABEL(LobbyClient);
	
	FindLobby();
}

void ULobbyClient::OnLeaveLobby(const PlayFab::MultiplayerModels::FLobbyEmptyResult& InResult)
{
	LOG_FUNC_LABEL(LobbyClient);
	Super::OnLeaveLobby(InResult);
	if(bReFindLobby)
		FindLobby();
}


// void ULobbyClient::OnCheckLobby(const PlayFab::MultiplayerModels::FGetLobbyResult& InResult)
// {
// 	LOG_MSGF(LobbyClient, TEXT("InResult: %s"), *InResult.toJSONString());
// 	ENSURE(LobbyId_ == InResult.pfLobby.LobbyId, LobbyClient);
// 	
// 	LobbyId_ = InResult.pfLobby.LobbyId;
// 	LobbyConnectionString_= InResult.pfLobby.ConnectionString;
//
// 	FString TitlePlayerId = DataProvider_->GetTitlePlayerID();
// 	const int32 index = Algo::IndexOfByPredicate(InResult.pfLobby.Members, [TitlePlayerId](const PlayFab::MultiplayerModels::FMember& Member)
// 		 { return Member.MemberEntity->Id == TitlePlayerId; });
//
// 	if(index > 1)
// 	{
// 		State_ = ELobbyClientStates::FindLobby;
// 		LOG_MSGF(LobbyClient, TEXT("Set new state: %s"), *StaticEnum<ELobbyClientStates>()->GetValueAsString(State_));
// 		LeaveLobby();
// 	}
// 	else
// 	{
// 		FTimerManager& TimerManager = GetWorld()->GetTimerManager();
// 		TimerManager.SetTimer(UpdateTimer_, this, &ULobbyClient::CheckLobby, LobbyCheckTime, false);
//
// 		FString OwnerId = InResult.pfLobby.Owner->Id;
// 		const int32 index_owner = Algo::IndexOfByPredicate(InResult.pfLobby.Members,
// 			[OwnerId](const PlayFab::MultiplayerModels::FMember& Member){ return Member.MemberEntity->Id == OwnerId; });
//
// 		SubscribeToLobbyChange(InResult.pfLobby.Members[index_owner].PubSubConnectionHandle);
// 	}
// }