// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyKeeper.h"

#include "CommonConstants.h"
#include "LogUtility.h"
#include "PlayFabMultiplayerAPI.h"

DEFINE_LOG_CATEGORY_STATIC(LobbyKeeper, Log, All);

void ULobbyKeeper::Start(IPlayfabDataProvider* InDataProvider, const FLobbyParameters& InSearchData, const FLobbyParameters& InServerLobbyData)
{
	Init(InDataProvider);
	CreateLobby(InSearchData, InServerLobbyData, 2);
}

void ULobbyKeeper::OnCheckLobbyHandle(const PlayFab::MultiplayerModels::FGetLobbyResult& InResult)
{
	LOG_MSGF(LobbyKeeper, TEXT("InResult: %s"), *InResult.pfLobby.LobbyId);
	Super::OnCheckLobbyHandle(InResult);
	
	SetOpenToFind(InResult.pfLobby.Members.Num() == 1);
}

void ULobbyKeeper::OnCheckLobbyErrorHandle()
{
	LOG_FUNC_LABEL(LobbyKeeper);
	Super::OnCheckLobbyErrorHandle();

	CreateLobby();
}