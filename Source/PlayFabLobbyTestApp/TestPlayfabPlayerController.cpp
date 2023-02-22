// Fill out your copyright notice in the Description page of Project Settings.


#include "TestPlayfabPlayerController.h"

#include "CommonConstants.h"
#include "LobbyClient.h"
#include "LobbyKeeper.h"
#include "LogUtility.h"
#include "TestMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "PlayFabUtilities.h"
#include "PlayFabMultiplayerAPI.h"
#include "PlayFabServerAPI.h"
#include "Algo/Transform.h"

DEFINE_LOG_CATEGORY_STATIC(TestPlayfab, Log, All);

void ATestPlayfabPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if(!IsValid(MenuWidgetClass))
		return;
	
	MenuWidget_ = CreateWidget<UTestMenuWidget>(this, MenuWidgetClass);

	MenuWidget_->AddToViewport();
	MenuWidget_->SetVisibility(ESlateVisibility::Visible);

	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
}

void ATestPlayfabPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if(GetWorld() != nullptr)
		GetWorld()->GetTimerManager().ClearTimer(LoginExpiredTimerHandle_);
	
	if(HasMMTicket())
		StopMM();

	if(!LobbyId_.IsEmpty())
		LeaveLobby();
	
	Super::EndPlay(EndPlayReason);
}

void ATestPlayfabPlayerController::WriteLog(const FString& inText) const
{
	MenuWidget_->AddLog(FText::FromString(inText));
	UE_LOG(TestPlayfab, Log, TEXT("%s"), *inText);
}

UWorld* ATestPlayfabPlayerController::GetWorld() const
{
	if(GetOuter() == nullptr)
		return nullptr;
	return GetOuter()->GetWorld();
}

void ATestPlayfabPlayerController::LoginAsServer(const FString& InName)
{
	PlayerName_ = InName;

	//UPlayFabUtilities::setPlayFabSettings(GetTitleId(), GetDefaultSecretKey());
	UPlayFabUtilities::setPlayFabSettings(GetTitleId(), TEXT(""));
	
	ServerLogin();
}

void ATestPlayfabPlayerController::ServerLogin()
{
	PlayFab::ServerModels::FLoginWithServerCustomIdRequest Request;

	Request.CreateAccount = true;
	Request.ServerCustomId = PlayerName_;

	PlayFab::UPlayFabServerAPI::FLoginWithServerCustomIdDelegate OnSuccess;
	OnSuccess.BindUObject(this, &ATestPlayfabPlayerController::OnSuccessServerLogin);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &ATestPlayfabPlayerController::LoginError);
	
	PlayFab::UPlayFabServerAPI().LoginWithServerCustomId(Request, OnSuccess, OnError);
}

void ATestPlayfabPlayerController::OnSuccessServerLogin(const PlayFab::ServerModels::FServerLoginResult& inResult)
{
	PlayfabID_ = inResult.PlayFabId;
	SessionTicket_ = inResult.SessionTicket;
	EntityToken_ = inResult.EntityToken->EntityToken; //L"title_player_account"
	EntityTokenExpiration_ = inResult.EntityToken->TokenExpiration;
	PlayfabTitlePlayerID_ = inResult.EntityToken->Entity->Id;

	const FTimespan Timespan = EntityTokenExpiration_ - FDateTime::UtcNow();
	const float CheckSeconds = Timespan.GetTotalSeconds() - 60;
	GetWorld()->GetTimerManager().SetTimer(LoginExpiredTimerHandle_, this, &ATestPlayfabPlayerController::Login, CheckSeconds, false);
	
	LOG_MSG(TestPlayfab, *ToString());
}

void ATestPlayfabPlayerController::OnServerLoginError(const PlayFab::FPlayFabCppError& inError)
{
	LOG_VERB(TestPlayfab, Error, TEXT("error %s"), *inError.GenerateErrorReport());
}

void ATestPlayfabPlayerController::LoginAsPlayer(const FString& InName)
{
	PlayerName_ = InName;

	UPlayFabUtilities::setPlayFabSettings(GetTitleId(), TEXT(""));
	
	Login();
}

void ATestPlayfabPlayerController::Login()
{
	PlayFab::ClientModels::FLoginWithCustomIDRequest Request;
	Request.CreateAccount = true;
	Request.CustomId = PlayerName_;

	PlayFab::UPlayFabClientAPI::FLoginWithCustomIDDelegate OnSuccess;
	OnSuccess.BindUObject(this, &ATestPlayfabPlayerController::OnSuccessLogin);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &ATestPlayfabPlayerController::LoginError);
	
	PlayFab::UPlayFabClientAPI().LoginWithCustomID(Request, OnSuccess, OnError);
}

void ATestPlayfabPlayerController::OnSuccessLogin(const PlayFab::ClientModels::FLoginResult& inResult)
{
	PlayfabID_ = inResult.PlayFabId;
	SessionTicket_ = inResult.SessionTicket;
	EntityToken_ = inResult.EntityToken->EntityToken;
	EntityTokenExpiration_ = inResult.EntityToken->TokenExpiration;
	PlayfabTitlePlayerID_ = inResult.EntityToken->Entity->Id;

	const FTimespan Timespan = EntityTokenExpiration_ - FDateTime::UtcNow();
	const float CheckSeconds = Timespan.GetTotalSeconds() - 60;
	GetWorld()->GetTimerManager().SetTimer(LoginExpiredTimerHandle_, this, &ATestPlayfabPlayerController::Login, CheckSeconds, false);
	
	LOG_MSG(TestPlayfab, *ToString());
}

void ATestPlayfabPlayerController::LoginError(const PlayFab::FPlayFabCppError& inError)
{
	LOG_VERB(TestPlayfab, Error, TEXT("error %s"), *inError.GenerateErrorReport());
}
#pragma region MM
void ATestPlayfabPlayerController::StartMM()
{
	if(!IsLogged())
	{
		WriteLog(FString(TEXT("Player must be logged!")));
		return;
	}
	PlayFab::MultiplayerModels::FEntityKey CreatorEntityKey;
	CreatorEntityKey.Id = PlayfabTitlePlayerID_;
	CreatorEntityKey.Type = CommonConstants::PlayfabTitlePlayerIDFieldName;
	
	PlayFab::MultiplayerModels::FMatchmakingPlayer Creator;
	Creator.Entity = CreatorEntityKey;

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetBoolField(TEXT("BoolField"), true);
	JsonObject->SetNumberField(TEXT("NumField"), 89);
	
	Creator.Attributes = MakeShared<PlayFab::MultiplayerModels::FMatchmakingPlayerAttributes>();
	Creator.Attributes->DataObject = JsonObject;
	
	PlayFab::MultiplayerModels::FCreateMatchmakingTicketRequest Request;
	Request.Creator = Creator;
	Request.QueueName = CommonConstants::TestQueueName;
	Request.GiveUpAfterSeconds = 120;
	Request.MembersToMatchWith = TArray<PlayFab::MultiplayerModels::FEntityKey>();
	
	PlayFab::UPlayFabMultiplayerAPI::FCreateMatchmakingTicketDelegate OnSuccess;
	OnSuccess.BindUObject(this, &ATestPlayfabPlayerController::CreatedMatchmakingTicket);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &ATestPlayfabPlayerController::CreateMatchmakingTicketError);

	bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().CreateMatchmakingTicket(Request, OnSuccess, OnError);

	FString RequestString = Request.toJSONString();
	
	WriteLog(FString::Printf(TEXT("StartMM for %s; RequestResult: %d; RequestString: %s"), *PlayerName_, RequestResult, *RequestString));
}

void ATestPlayfabPlayerController::StopMM()
{
	PlayFab::MultiplayerModels::FCancelMatchmakingTicketRequest Request;
	Request.QueueName = CommonConstants::TestQueueName;
	Request.TicketId = MMTicket_;
	
	const bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().CancelMatchmakingTicket(Request);

	MMTicket_.Reset();
	GetOuter()->GetWorld()->GetTimerManager().ClearTimer(MMTicketStatusHandle_);

	UE_LOG(LogTemp, Log, TEXT("StopMM: %s; RequestResult: %d"), *PlayerName_, RequestResult);
}

void ATestPlayfabPlayerController::CreatedMatchmakingTicket(const PlayFab::MultiplayerModels::FCreateMatchmakingTicketResult& Result)
{
	MMTicket_ = Result.TicketId;
	WriteLog(FString::Printf(TEXT("CreatedMatchmakingTicket for %s; MMTicket_: %s"), *PlayerName_, *MMTicket_));

	GetWorld()->GetTimerManager().SetTimer(MMTicketStatusHandle_, this, &ATestPlayfabPlayerController::CheckMatchmakingTicketStatus, 8, true);
}

void ATestPlayfabPlayerController::CreateMatchmakingTicketError(const PlayFab::FPlayFabCppError& inError)
{
	WriteLog(FString::Printf(TEXT("CreateMatchmakingTicketError for %s: %s"), *PlayerName_, *inError.GenerateErrorReport()));
}

void ATestPlayfabPlayerController::CheckMatchmakingTicketStatus()
{
	WriteLog(FString::Printf(TEXT("CheckMatchmakingTicketStatus for %s"), *PlayerName_));
	
	PlayFab::MultiplayerModels::FGetMatchmakingTicketRequest Request;
	Request.QueueName = CommonConstants::TestQueueName;
	Request.TicketId = MMTicket_;

	PlayFab::UPlayFabMultiplayerAPI::FGetMatchmakingTicketDelegate OnSuccess;
	OnSuccess.BindUObject(this, &ATestPlayfabPlayerController::CheckedMatchmakingTicketStatus);
	PlayFab::FPlayFabErrorDelegate OnError; 
	OnError.BindUObject(this, &ATestPlayfabPlayerController::CheckMatchmakingTicketStatusError);
	PlayFab::UPlayFabMultiplayerAPI().GetMatchmakingTicket(Request, OnSuccess, OnError);
}

void ATestPlayfabPlayerController::CheckedMatchmakingTicketStatus(const PlayFab::MultiplayerModels::FGetMatchmakingTicketResult& Result)
{
	WriteLog(FString::Printf(TEXT("CheckedMatchmakingTicketStatus for %s; Result.Status: %s"), *PlayerName_, *Result.Status));

	//WaitingForMatch Canceled Matched
	if(Result.Status.Equals("Matched", ESearchCase::IgnoreCase))
	{
		GetOuter()->GetWorld()->GetTimerManager().ClearTimer(MMTicketStatusHandle_);
		
		PlayFab::MultiplayerModels::FGetMatchRequest Request;
		Request.MatchId = Result.MatchId;
		Request.QueueName = CommonConstants::TestQueueName;

		PlayFab::UPlayFabMultiplayerAPI::FGetMatchDelegate OnSuccess;
		OnSuccess.BindUObject(this, &ATestPlayfabPlayerController::GotMatch);
		PlayFab::FPlayFabErrorDelegate OnError;
		OnError.BindUObject(this, &ATestPlayfabPlayerController::GetMatchError);
		PlayFab::UPlayFabMultiplayerAPI().GetMatch(Request,OnSuccess, OnError);
	}
	else if(Result.Status.Equals("Canceled", ESearchCase::IgnoreCase))
	{
		GetOuter()->GetWorld()->GetTimerManager().ClearTimer(MMTicketStatusHandle_);
		MMTicket_.Reset();
	}
}

void ATestPlayfabPlayerController::CheckMatchmakingTicketStatusError(const PlayFab::FPlayFabCppError& inError)
{
	WriteLog(FString::Printf(TEXT("CheckMatchmakingTicketStatusError for %s: %s"), *PlayerName_, *inError.GenerateErrorReport()));
}

void ATestPlayfabPlayerController::GotMatch(const PlayFab::MultiplayerModels::FGetMatchResult& InResult)
{
#pragma region example	
	//GotMatch for Client1:
	//{
	//	"ArrangementString":"1:b9f3d434-2641-409b-afb8-61bd2cbba29e|664901|title_player_account!44FA187E9DF2D27D|1675245520|mv1:6Kw5CYCnKb1M/+5iVeGzDsIJY81mVgT51dmH8kcvyP8=",
	//	"MatchId":"b9f3d434-2641-409b-afb8-61bd2cbba29e",
	//	"Members":
	//	[
	//		{
	//			"Entity":
	//			{
	//				"Id":"318C67ECC3CF4C2",
	//				"Type":"title_player_account"
	//			}
	//		},
	//		{
	//			"Entity":
	//			{
	//				"Id":"44FA187E9DF2D27D",
	//				"Type":"title_player_account"
	//			}
	//		}
	//	]
	//}
#pragma endregion example		
	WriteLog(FString::Printf(TEXT("GotMatch for %s: %s"), *PlayerName_, *InResult.toJSONString()));
}

void ATestPlayfabPlayerController::GetMatchError(const PlayFab::FPlayFabCppError& inError)
{
	WriteLog(FString::Printf(TEXT("GetMatchError for %s: %s"), *PlayerName_, *inError.GenerateErrorReport()));
}
#pragma endregion MM

void ATestPlayfabPlayerController::StartCreateLobby(const TArray<FString>& InSearchParams)
{
	if(!IsLogged())
	{
		WriteLog(FString(TEXT("Player must be logged!")));
		return;
	}

	LobbiesSearchParams_ = InSearchParams;

	SignalRLogin();
}

void ATestPlayfabPlayerController::SignalRLogin()
{
	PubSubRequester_ = MakeShareable(new FPubSubRequester());
	PubSubRequester_->OnOpenSessionError().AddUObject(this, &ATestPlayfabPlayerController::OnSignalRSessionOpenError);
	PubSubRequester_->OnOpenSessionSuccess().AddUObject(this, &ATestPlayfabPlayerController::OnSignalRSessionOpenSuccess);
	PubSubRequester_->OnCloseSession().AddUObject(this, &ATestPlayfabPlayerController::OnSignalRSessionClosed);
	PubSubRequester_->Start(TitleID_, EntityToken_);
}

void ATestPlayfabPlayerController::OnSignalRSessionOpenError(const FString& InError)
{
	LOG_VERB(TestPlayfab, Error, TEXT("InError %s"), *InError);
}

void ATestPlayfabPlayerController::OnSignalRSessionOpenSuccess(const FString& InConnectionHandle, bool InReconnecting)
{
	LOG_VERB(TestPlayfab, Error, TEXT("InConnectionHandle %s; InReconnecting: %d"), *InConnectionHandle, InReconnecting);
	
	if(!InReconnecting)
		CreateLobby();
}

void ATestPlayfabPlayerController::OnSignalRSessionClosed(bool InUnexpected)
{
	WriteLog( FString::Printf(TEXT("OnSignalRSessionClosed, InUnexpected: %d"), InUnexpected));
	if(InUnexpected && !LobbyId_.IsEmpty())
		GetLobby(LobbyId_);
}

void ATestPlayfabPlayerController::CreateLobby()
{
	LOG_FUNC_LABEL(TestPlayfab);
	
	PlayFab::MultiplayerModels::FEntityKey CreatorEntityKey;
	CreatorEntityKey.Id = PlayfabTitlePlayerID_;
	CreatorEntityKey.Type = CommonConstants::PlayfabTitlePlayerIDFieldName;
	
	PlayFab::MultiplayerModels::FCreateLobbyRequest Request;

	Request.pfAccessPolicy = PlayFab::MultiplayerModels::AccessPolicy::AccessPolicyPublic;
	//Request.CustomTags.Add(TEXT("CustomTag_BuildNumber"), TEXT("9.8.2"));
	Request.LobbyData.Add(TEXT("LobbyData_Temp"), TEXT("LobbyData_Value"));
	Request.MaxPlayers = 2;

	PlayFab::MultiplayerModels::FMember FirstMember;
	FirstMember.MemberEntity = MakeShared<PlayFab::MultiplayerModels::FEntityKey>(CreatorEntityKey);
	Request.Members.Add(FirstMember);
	
	Request.Owner = CreatorEntityKey;

	Request.pfOwnerMigrationPolicy =  PlayFab::MultiplayerModels::OwnerMigrationPolicy::OwnerMigrationPolicyAutomatic;

	for(int32 i = 0; i < LobbiesSearchParams_.Num(); i++)
	{
		if(!LobbiesSearchParams_[i].IsEmpty())
			Request.SearchData.Add( FString::Printf(TEXT("string_key%d"), i+1), LobbiesSearchParams_[i]);
	}

	Request.UseConnections = true;

	PlayFab::UPlayFabMultiplayerAPI::FCreateLobbyDelegate OnSuccess;
	OnSuccess.BindUObject(this, &ATestPlayfabPlayerController::OnCreateLobby);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &ATestPlayfabPlayerController::OnCreateLobbyError);
		
	const bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().CreateLobby(Request, OnSuccess, OnError);

	const FString RequestString = Request.toJSONString();
#pragma region example

	//{
	//	"AccessPolicy":"Public",
	//	"CustomTags":
	//	{
	//		"CustomTag_BuildNumber":"9.8.2"
	//	},
	//	"LobbyData":
	//	{
	//		"LobbyData_Temp":"LobbyData_Value"
	//	},
	//	"MaxPlayers":2,
	//	"Members":
	//	[
	//		{
	//			"MemberEntity":
	//			{
	//				"Id":"44FA187E9DF2D27D",
	//				"Type":"title_player_account"
	//			}
	//		}
	//	],
	//	"Owner":
	//	{
	//		"Id":"44FA187E9DF2D27D",
	//		"Type":"title_player_account"
	//	},
	//	"OwnerMigrationPolicy":"Automatic",
	//	"SearchData":
	//	{
	//		"string_key1":"na_region"
	//	},
	//	"UseConnections":true
	//}
#pragma endregion example	

	LOG_MSGF(TestPlayfab, TEXT("CreateLobby for %s; RequestResult: %d; RequestString: %s"), *PlayerName_, RequestResult, *RequestString);
}

void ATestPlayfabPlayerController::OnCreateLobby(const PlayFab::MultiplayerModels::FCreateLobbyResult& InResult)
{
	//{
	//	"ConnectionString":"cv1:475c0e14-9679-4936-88d8-3252a9f190ce|664901|kv1|cv1:dmsot0x5G5NIinIYU/ssRaP4QY1zGCtxkz775D8jlLk=",
	//	"LobbyId":"475c0e14-9679-4936-88d8-3252a9f190ce"
	//}
	LobbyId_ = InResult.LobbyId;
	LobbyConnectionString_ = InResult.ConnectionString;

	OnLobbyCreatedOrJoin(LobbyId_, LobbyConnectionString_);
	
	WriteLog(FString::Printf(TEXT("OnCreateLobby for %s: %s"), *PlayerName_, *InResult.toJSONString()));

	SubscribeToLobbyChange();
}

void ATestPlayfabPlayerController::OnLobbyCreatedOrJoin(const FString& InLobbyId, const FString& InConnectionString)
{
	MenuWidget_->SetLobbyId(InLobbyId);
}

void ATestPlayfabPlayerController::SubscribeToLobbyChange()
{
	WriteLog(FString::Printf(TEXT("SubscribeToLobbyChange for %s"), *PlayerName_));
	
	PlayFab::MultiplayerModels::FSubscribeToLobbyResourceRequest Request;

	Request.Type = PlayFab::MultiplayerModels::SubscriptionTypeLobbyChange;
	Request.pfEntityKey.Id = PlayfabTitlePlayerID_;
	Request.pfEntityKey.Type = CommonConstants::PlayfabTitlePlayerIDFieldName;
	Request.ResourceId = LobbyId_;
	Request.SubscriptionVersion = 1;

	if(PubSubRequester_.IsValid() && !PubSubRequester_->GetConnectionHandleString().IsEmpty())
		Request.PubSubConnectionHandle = PubSubRequester_->GetConnectionHandleString();
	else
		WriteLog(FString::Printf(TEXT("OnCreateLobby Error %s: PubSubRequester_ invalid!"), *PlayerName_));

	PlayFab::UPlayFabMultiplayerAPI::FSubscribeToLobbyResourceDelegate OnSuccess;
	OnSuccess.BindUObject(this, &ATestPlayfabPlayerController::OnSubscribeToLobby);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &ATestPlayfabPlayerController::OnSubscribeToLobbyError);

	//This field is required: SubscribeToLobbyResourceRequest::PubSubConnectionHandle, PlayFab calls may not work if it remains empty.
	const bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().SubscribeToLobbyResource(Request, OnSuccess, OnError);

	const FString RequestString = Request.toJSONString();
	//{
	//	"EntityKey":
	//	{
	//		"Id":"44FA187E9DF2D27D",
	//		"Type":"title_player_account"
	//	},
	//	"PubSubConnectionHandle":"cv1:ab110cea-a5eb-48bc-9ec1-a82f4c0889af|664901|kv1|cv1:dXyhpl6LutFFvou9lGKswqTCxfpc25sKjORQgOILncs=",
	//	"ResourceId":"ab110cea-a5eb-48bc-9ec1-a82f4c0889af",
	//	"SubscriptionVersion":0,
	//	"Type":"LobbyChange"
	//}
	WriteLog(FString::Printf(TEXT("OnCreateLobby for %s; RequestResult: %d; RequestString: %s"), *PlayerName_, RequestResult, *RequestString));	
}

void ATestPlayfabPlayerController::OnSubscribeToLobby(const PlayFab::MultiplayerModels::FSubscribeToLobbyResourceResult& InResult)
{
	//OnSubscribeToLobby for Client1: {"Topic":"1~lobby~LobbyChange~e425335a-0432-47be-8c12-ff0274ad4552"}
	WriteLog(FString::Printf(TEXT("OnSubscribeToLobby for %s: %s"), *PlayerName_, *InResult.toJSONString()));
}

void ATestPlayfabPlayerController::OnSubscribeToLobbyError(const PlayFab::FPlayFabCppError& InError)
{
	//Invalid input parameters - PubSubConnectionHandle: The PubSubConnectionHandle field is required.
	//The incoming request subscription version does not match lobby subscription version
	WriteLog(FString::Printf(TEXT("OnSubscribeToLobbyError for %s: %s"), *PlayerName_, *InError.GenerateErrorReport()));
}

void ATestPlayfabPlayerController::OnCreateLobbyError(const PlayFab::FPlayFabCppError& InError)
{
	//The searchData Key parameter is not valid: Invalid key 'SearchData_Region' provided. - must be: string_key1
	//The list of members is empty
	WriteLog(FString::Printf(TEXT("OnCreateLobbyError for %s: %s"), *PlayerName_, *InError.GenerateErrorReport()));
}

void ATestPlayfabPlayerController::GetLobby(const FString& InLobbyId)
{
	PlayFab::MultiplayerModels::FGetLobbyRequest Request;

	Request.LobbyId = InLobbyId;

	PlayFab::UPlayFabMultiplayerAPI::FGetLobbyDelegate OnSuccess;
	OnSuccess.BindUObject(this, &ATestPlayfabPlayerController::OnGetLobby);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &ATestPlayfabPlayerController::OnGetLobbyError);
		
	const bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().GetLobby(Request, OnSuccess, OnError);

	const FString RequestString = Request.toJSONString();

	WriteLog(FString::Printf(TEXT("GetLobby for %s; RequestResult: %d; RequestString: %s"), *PlayerName_, RequestResult, *RequestString));
}

void ATestPlayfabPlayerController::OnGetLobby(const PlayFab::MultiplayerModels::FGetLobbyResult& InResult)
{
	LobbyId_ = InResult.pfLobby.LobbyId;
	LobbyConnectionString_= InResult.pfLobby.ConnectionString;
	WriteLog(FString::Printf(TEXT("OnGetLobby for %s: %s"), *PlayerName_, *InResult.toJSONString()));
#pragma region example
	//	"Lobby":
	//	{
	//		"AccessPolicy":"Public",
	//		"ChangeNumber":2,
	//		"ConnectionString":"cv1:d12d946e-5d71-41aa-9965-3553fb1b7341|664901|kv1|cv1:ZkomGupvGPh4oYbFuFBmMOK6IXftKxufWR4TIfTSDhY=",
	//		"LobbyData":
	//		{
	//			"LobbyData_Temp":"LobbyData_Value"
	//		},
	//		"LobbyId":"d12d946e-5d71-41aa-9965-3553fb1b7341",
	//		"MaxPlayers":2,
	//		"Members":
	//		[
	//			{
	//				"MemberEntity":
	//				{
	//					"Id":"8061383BF1175413","Type":"title_player_account"
	//				},
	//				"PubSubConnectionHandle":"1.Y2VudHJhbC11c35wdWJzdWItc2lnbmFsci1wcm9kLXdlc3R1czMtMDAxfjU2eGlzek9mb1N1UFZHTk1VZW94d0E4N2EyODEwMzE="
	//			},
	//			{
	//				"MemberEntity":
	//				{
	//					"Id":"44FA187E9DF2D27D",
	//					"Type":"title_player_account"
	//				}
	//			}
	//		],
	//		"MembershipLock":"Unlocked",
	//		"Owner":
	//		{
	//			"Id":"8061383BF1175413",
	//			"Type":"title_player_account"
	//		},
	//		"OwnerMigrationPolicy":"Automatic",
	//		"SearchData":
	//		{
	//			"string_key2":"na-region",
	//			"string_key1":"Ver 9.8.1"
	//			},
	//			"UseConnections":true
	//		}
	//	}
#pragma endregion example	
}

void ATestPlayfabPlayerController::OnGetLobbyError(const PlayFab::FPlayFabCppError& InError)
{
	WriteLog(FString::Printf(TEXT("OnGetLobbyError for %s: %s"), *PlayerName_, *InError.GenerateErrorReport()));
}

void ATestPlayfabPlayerController::FindLobby(const TArray<FString>& InSearchParams)
{
	if(!IsLogged())
	{
		WriteLog(FString(TEXT("Player must be logged!")));
		return;
	}

	PlayFab::MultiplayerModels::FFindLobbiesRequest Request;

	Request.Filter = TEXT("lobby/membershipLock eq 'Unlocked'");
	for(int32 i = 0; i < InSearchParams.Num(); i++)
	{
		if(InSearchParams[i].IsEmpty())
			continue;
	
		if(!Request.Filter.IsEmpty())
			Request.Filter.Append(TEXT(" and "));
		
		Request.Filter.Appendf(TEXT("string_key%d eq '%s'"), i+1, *InSearchParams[i]);
	}
	
	Request.OrderBy = TEXT("lobby/memberCount desc");

	Request.Pagination = MakeShareable(new PlayFab::MultiplayerModels::FPaginationRequest());
	Request.Pagination->PageSizeRequested = 20;

	PlayFab::UPlayFabMultiplayerAPI::FFindLobbiesDelegate OnSuccess;
	OnSuccess.BindUObject(this, &ATestPlayfabPlayerController::OnFindLobbies);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &ATestPlayfabPlayerController::OnFindLobbiesError);

	const bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().FindLobbies(Request, OnSuccess, OnError);

	const FString RequestString = Request.toJSONString();

	WriteLog(FString::Printf(TEXT("FindLobby for %s; RequestResult: %d; RequestString: %s"), *PlayerName_, RequestResult, *RequestString));
}

void ATestPlayfabPlayerController::OnFindLobbies(const PlayFab::MultiplayerModels::FFindLobbiesResult& InResult)
{
	WriteLog(FString::Printf(TEXT("OnFindLobbies for %s: %s"), *PlayerName_, *InResult.toJSONString()));

	FoundLobbies_.Reset();
	FoundLobbies_.Append(InResult.Lobbies);

	TArray<FString> LobbyNames;
	Algo::Transform(FoundLobbies_, LobbyNames, [](const PlayFab::MultiplayerModels::FLobbySummary& LobbySummary) { return LobbySummary.LobbyId; });

	MenuWidget_->SetFoundLobbyIds(LobbyNames);
}

void ATestPlayfabPlayerController::OnFindLobbiesError(const PlayFab::FPlayFabCppError& InError)
{
	WriteLog(FString::Printf(TEXT("OnFindLobbiesError for %s: %s"), *PlayerName_, *InError.GenerateErrorReport()));
}

void ATestPlayfabPlayerController::JoinLobby(const FString& InLobbyId)
{
	WriteLog(FString::Printf(TEXT("JoinLobby for %s: %s"), *PlayerName_, *InLobbyId));

	const PlayFab::MultiplayerModels::FLobbySummary* pLobbySummary = Algo::FindByPredicate(FoundLobbies_, [&InLobbyId] (const PlayFab::MultiplayerModels::FLobbySummary& InLobbySummary)
		{ return InLobbySummary.LobbyId == InLobbyId; });
	
	if(pLobbySummary == nullptr)
	{
		WriteLog(FString::Printf(TEXT("JoinLobby Error for %s: Can't find Lobby Id %s"), *PlayerName_, *InLobbyId));
		return;
	}
	
	PlayFab::MultiplayerModels::FJoinLobbyRequest Request;
	
	Request.ConnectionString = pLobbySummary->ConnectionString;
	LobbyConnectionString_ = pLobbySummary->ConnectionString;

	Request.MemberEntity = MakeShareable(new PlayFab::MultiplayerModels::FEntityKey());
	Request.MemberEntity->Id = PlayfabTitlePlayerID_;
	Request.MemberEntity->Type = CommonConstants::PlayfabTitlePlayerIDFieldName;

	PlayFab::UPlayFabMultiplayerAPI::FJoinLobbyDelegate OnSuccess;
	OnSuccess.BindUObject(this, &ATestPlayfabPlayerController::OnJoinLobby);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &ATestPlayfabPlayerController::OnJoinLobbyError);
	
	const bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().JoinLobby(Request, OnSuccess, OnError);

	const FString RequestString = Request.toJSONString();

	WriteLog(FString::Printf(TEXT("GetLobby for %s; RequestResult: %d; RequestString: %s"), *PlayerName_, RequestResult, *RequestString));
}

void ATestPlayfabPlayerController::OnJoinLobby(const PlayFab::MultiplayerModels::FJoinLobbyResult& InResult)
{
	LobbyId_ = InResult.LobbyId;

	OnLobbyCreatedOrJoin(LobbyId_, LobbyConnectionString_);
	
	WriteLog(FString::Printf(TEXT("OnJoinLobby for %s: %s"), *PlayerName_, *InResult.toJSONString()));
}

void ATestPlayfabPlayerController::OnJoinLobbyError(const PlayFab::FPlayFabCppError& InError)
{
	WriteLog(FString::Printf(TEXT("OnJoinLobbyError for %s: %s"), *PlayerName_, *InError.GenerateErrorReport()));

	// LobbyBadRequest	13007
	// LobbyDoesNotExist	13000
	// LobbyMemberCannotRejoin	13004
	// LobbyNotJoinable	13003
	// LobbyPlayerAlreadyJoined	13002
	// LobbyPlayerMaxLobbyLimitExceeded	13008
	// LobbyRateLimitExceeded	13001
	if(InError.ErrorCode == 13002)
	{
		TArray<FString> Values;
		InError.ErrorDetails.MultiFind(TEXT("LobbyId"), Values);
		if(Values.Num() == 1)
		{
			LobbyId_ = Values[0];
			GetLobby(LobbyId_);
		}
	}
}

void ATestPlayfabPlayerController::LeaveLobby()
{
	WriteLog(FString::Printf(TEXT("LeaveLobby for %s"), *PlayerName_));

	if(LobbyId_.IsEmpty())
	{
		WriteLog(FString::Printf(TEXT("LeaveLobby Error for %s: Lobby Id is empty"), *PlayerName_));
		return;
	}

	if(PubSubRequester_.IsValid())
		PubSubRequester_->Stop();
	PubSubRequester_.Reset();
	
	PlayFab::MultiplayerModels::FLeaveLobbyRequest Request;
	
	Request.LobbyId = LobbyId_;

	Request.MemberEntity = MakeShareable(new PlayFab::MultiplayerModels::FEntityKey());
	Request.MemberEntity->Id = PlayfabTitlePlayerID_;
	Request.MemberEntity->Type = CommonConstants::PlayfabTitlePlayerIDFieldName;

	PlayFab::UPlayFabMultiplayerAPI::FLeaveLobbyDelegate OnSuccess;
	OnSuccess.BindUObject(this, &ATestPlayfabPlayerController::OnLeaveLobby);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &ATestPlayfabPlayerController::OnLeaveLobbyError);
	
	const bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().LeaveLobby(Request, OnSuccess, OnError);

	const FString RequestString = Request.toJSONString();

	WriteLog(FString::Printf(TEXT("GetLobby for %s; RequestResult: %d; RequestString: %s"), *PlayerName_, RequestResult, *RequestString));
}

void ATestPlayfabPlayerController::OnLeaveLobby(const PlayFab::MultiplayerModels::FLobbyEmptyResult& InResult)
{
	LobbyId_ = FString();
	LobbyConnectionString_ = FString();
	MenuWidget_->SetLobbyId(LobbyId_);
	
	WriteLog(FString::Printf(TEXT("OnJoinLobby for %s: %s"), *PlayerName_, *InResult.toJSONString()));
}

void ATestPlayfabPlayerController::OnLeaveLobbyError(const PlayFab::FPlayFabCppError& InError)
{
	WriteLog(FString::Printf(TEXT("OnJoinLobbyError for %s: %s"), *PlayerName_, *InError.GenerateErrorReport()));
}


void ATestPlayfabPlayerController::CreateLobbyKeeper()
{
	LOG_FUNC_LABEL(TestPlayfab);
	ENSURE_RET(!PlayfabTitlePlayerID_.IsEmpty(), TestPlayfab);
	ENSURE_RET(!IsValid(LobbyClient_), TestPlayfab);
	
	LobbyKeeper_ = NewObject<ULobbyKeeper>(this);
	
	LobbyKeeper_->OnLobbyCreatedEvent().AddUObject(this, &ATestPlayfabPlayerController::OnLobbyCreatedOrJoin);

	FLobbyParameters SearchData;
	SearchData[ESPNames::ServerLobbyLabel] = TEXT("1");
	SearchData[ESPNames::BuildNumber] = TEXT("92535");
	SearchData[ESPNames::Level] = TEXT("World_Map_01_Persistent_Level");
	SearchData[ESPNames::GameVersion] = TEXT("36.9.6");
	SearchData[ESPNames::ServerRegion] = TEXT("eu-central");
	SearchData[ESPNames::ServerNeedsConfiguring] = TEXT("eu-central");
	
	FLobbyParameters ServerLobbyData;
	ServerLobbyData[ESPNames::IpAddress] = TEXT("127.0.0.1");
	ServerLobbyData[ESPNames::Port] = TEXT("7777");
	
	LobbyKeeper_->Start(this, SearchData, ServerLobbyData);
}

void ATestPlayfabPlayerController::DeleteLobbyKeeper()
{
	LOG_FUNC_LABEL(TestPlayfab);

	if(IsValid(LobbyKeeper_))
		LobbyKeeper_->ConditionalBeginDestroy();
	LobbyKeeper_ = nullptr;
}

void ATestPlayfabPlayerController::CreateLobbyClient()
{
	LOG_FUNC_LABEL(TestPlayfab);
	ENSURE_RET(!PlayfabTitlePlayerID_.IsEmpty(), TestPlayfab);
	ENSURE_RET(!IsValid(LobbyKeeper_), TestPlayfab);
	LobbyClient_ = NewObject<ULobbyClient>(this);
	
	LobbyClient_->OnFindLobbiesEvent().AddUObject(this, &ATestPlayfabPlayerController::OnFindLobbies);
	LobbyClient_->OnLobbyJoinEvent().AddUObject(this, &ATestPlayfabPlayerController::OnLobbyCreatedOrJoin);

	FLobbyParameters SearchData;
	SearchData[ESPNames::ServerLobbyLabel] = TEXT("1");
	SearchData[ESPNames::BuildNumber] = TEXT("92535");
	SearchData[ESPNames::Level] = TEXT("World_Map_01_Persistent_Level");
	SearchData[ESPNames::GameVersion] = TEXT("36.9.6");
	SearchData[ESPNames::ServerRegion] = TEXT("eu-central");
	SearchData[ESPNames::ServerNeedsConfiguring] = TEXT("eu-central");
	
	LobbyClient_->Start(this, SearchData);
}

void ATestPlayfabPlayerController::DeleteLobbyClient()
{
	LOG_FUNC_LABEL(TestPlayfab);

	if(IsValid(LobbyClient_))
	{
		LobbyClient_->OnFindLobbiesEvent().Clear();
		LobbyClient_->ConditionalBeginDestroy();
	}
	LobbyClient_ = nullptr;
}