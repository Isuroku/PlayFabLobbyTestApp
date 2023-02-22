// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayfabLobbyBase.h"
#include "LogUtility.h"
#include "PlayFabMultiplayerAPI.h"
#include "PlayFabServerAPI.h"
#include "PubSubRequester.h"

DEFINE_LOG_CATEGORY_STATIC(PlayfabLobbyBase, Log, All);

void UPlayfabLobbyBase::BeginDestroy()
{
	if(!LobbyId_.IsEmpty())
		LeaveLobby();
	
	UObject::BeginDestroy();
}

void UPlayfabLobbyBase::Init(IPlayfabDataProvider* InDataProvider)
{
	DataProvider_ = InDataProvider;
}

void UPlayfabLobbyBase::RegisterGame()
{
	LOG_FUNC_LABEL(PlayfabLobbyBase);

	PlayFab::ServerModels::FRegisterGameRequest Request;

	//Request.Build - only for Playfab servers

	PlayFab::UPlayFabServerAPI::FRegisterGameDelegate OnSuccess;
	OnSuccess.BindUObject(this, &UPlayfabLobbyBase::OnRegisterGame);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &UPlayfabLobbyBase::OnRegisterGameError);
	const bool RequestResult = PlayFab::UPlayFabServerAPI().RegisterGame(Request, OnSuccess, OnError);

	const FString RequestString = Request.toJSONString();
	LOG_MSGF(PlayfabLobbyBase, TEXT("RequestResult: %d; RequestString: %s"), RequestResult, *RequestString);
}

void UPlayfabLobbyBase::OnRegisterGame(const PlayFab::ServerModels::FRegisterGameResponse& InResult)
{
	LobbyId_ = InResult.LobbyId;
	//LobbyConnectionString_ = InResult.ConnectionString;

	LOG_MSGF(PlayfabLobbyBase, TEXT("LobbyId_: %d; LobbyConnectionString_: %s"), *LobbyId_, *LobbyConnectionString_);
}

void UPlayfabLobbyBase::OnRegisterGameError(const PlayFab::FPlayFabCppError& InError)
{
	LOG_VERB(PlayfabLobbyBase, Error, TEXT("%s"), *InError.GenerateErrorReport());
}

#pragma region CreateLobby
void UPlayfabLobbyBase::CreateLobby(const FLobbyParameters& InSearchData, const FLobbyParameters& InServerLobbyData, int32 InMaxPlayer)
{
	LOG_MSGF(PlayfabLobbyBase, TEXT("InSearchData: %s; InServerLobbyData: %s"), *InSearchData.ToString(), *InServerLobbyData.ToString());
	
	SearchData_ = InSearchData;
	ServerLobbyData_ = InServerLobbyData;
	MaxPlayer_ = InMaxPlayer;

	CreateLobby();
}

void UPlayfabLobbyBase::CreateLobby()
{
	LOG_FUNC_LABEL(PlayfabLobbyBase);

	ENSURE_RET(DataProvider_ != nullptr, PlayfabLobbyBase);
	
	PlayFab::MultiplayerModels::FEntityKey CreatorEntityKey;
	CreatorEntityKey.Id = DataProvider_->GetTitlePlayerID();
	CreatorEntityKey.Type = CommonConstants::PlayfabTitlePlayerIDFieldName;
	
	PlayFab::MultiplayerModels::FCreateLobbyRequest Request;

	Request.pfAccessPolicy = PlayFab::MultiplayerModels::AccessPolicy::AccessPolicyPublic;

	ServerLobbyData_.SerializeAllDataNormalName(Request.LobbyData);
	
	Request.MaxPlayers = MaxPlayer_;

	PlayFab::MultiplayerModels::FMember FirstMember;
	FirstMember.MemberEntity = MakeShared<PlayFab::MultiplayerModels::FEntityKey>(CreatorEntityKey);
	Request.Members.Add(FirstMember);
	
	Request.Owner = CreatorEntityKey;

	Request.pfOwnerMigrationPolicy =  PlayFab::MultiplayerModels::OwnerMigrationPolicy::OwnerMigrationPolicyAutomatic;

	SearchData_.SerializeAllDataPrefabSearchName(Request.SearchData);

	Request.UseConnections = true;

	PlayFab::UPlayFabMultiplayerAPI::FCreateLobbyDelegate OnSuccess;
	OnSuccess.BindUObject(this, &UPlayfabLobbyBase::OnCreateLobby);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &UPlayfabLobbyBase::OnCreateLobbyError);
		
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

	LOG_MSGF(PlayfabLobbyBase, TEXT("RequestResult: %d; RequestString: %s"), RequestResult, *RequestString);
}

void UPlayfabLobbyBase::OnCreateLobby(const PlayFab::MultiplayerModels::FCreateLobbyResult& InResult)
{
	//{
	//	"ConnectionString":"cv1:475c0e14-9679-4936-88d8-3252a9f190ce|664901|kv1|cv1:dmsot0x5G5NIinIYU/ssRaP4QY1zGCtxkz775D8jlLk=",
	//	"LobbyId":"475c0e14-9679-4936-88d8-3252a9f190ce"
	//}
	LobbyId_ = InResult.LobbyId;
	LobbyConnectionString_ = InResult.ConnectionString;

	LOG_MSGF(PlayfabLobbyBase, TEXT("LobbyId_: %d; LobbyConnectionString_: %s"), *LobbyId_, *LobbyConnectionString_);

	OnLobbyCreatedHandle_.Broadcast(LobbyId_, LobbyConnectionString_);

	SignalRLogin();
}

void UPlayfabLobbyBase::OnCreateLobbyError(const PlayFab::FPlayFabCppError& InError)
{
	//The searchData Key parameter is not valid: Invalid key 'SearchData_Region' provided. - must be: string_key1
	//The list of members is empty
	LOG_VERB(PlayfabLobbyBase, Error, TEXT("%s"), *InError.GenerateErrorReport());
}
#pragma endregion CreateLobby

#pragma region FindLobby
void UPlayfabLobbyBase::FindLobby(const FLobbyParameters& InSearchData)
{
	LOG_MSGF(PlayfabLobbyBase, TEXT("InSearchData: %s"), *InSearchData.ToString());
	SearchData_ = InSearchData;

	FindLobby();
}
void UPlayfabLobbyBase::FindLobby()
{
	LOG_FUNC_LABEL(PlayfabLobbyBase);

	PlayFab::MultiplayerModels::FFindLobbiesRequest Request;

	Request.Filter = TEXT("lobby/membershipLock eq 'Unlocked' and lobby/memberCount eq 1");
	SearchData_.AddPlayfabSearchFilters(Request.Filter);
	
	Request.OrderBy = TEXT("lobby/memberCount asc");

	Request.Pagination = MakeShareable(new PlayFab::MultiplayerModels::FPaginationRequest());
	Request.Pagination->PageSizeRequested = 20;

	PlayFab::UPlayFabMultiplayerAPI::FFindLobbiesDelegate OnSuccess;
	OnSuccess.BindUObject(this, &UPlayfabLobbyBase::OnFindLobbies);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &UPlayfabLobbyBase::OnFindLobbiesError);

	const bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().FindLobbies(Request, OnSuccess, OnError);

	const FString RequestString = Request.toJSONString();

	LOG_MSGF(PlayfabLobbyBase, TEXT("RequestResult: %d; RequestString: %s"), RequestResult, *RequestString);
}

void UPlayfabLobbyBase::OnFindLobbies(const PlayFab::MultiplayerModels::FFindLobbiesResult& InResult)
{
	for (const PlayFab::MultiplayerModels::FLobbySummary& LobbySummary : InResult.Lobbies)
	{
		LOG_MSGF(PlayfabLobbyBase, TEXT("Lobby: %s"), *LobbySummary.LobbyId);
	}

	OnFindLobbiesHandle_.Broadcast(InResult);

// 	{"Lobbies":[
// {"ConnectionString":"cv1:8bae3b23-7655-4c12-bd32-07e86050b419|664901|kv1|cv1:xVD99e3/h1C0xF8rTX2KX/zdN+hT+lk/f/TTsWSUtfg=",
// "CurrentPlayers":1,
// "LobbyId":"8bae3b23-7655-4c12-bd32-07e86050b419",
// "MaxPlayers":2,
// "MembershipLock":"Unlocked",
// "Owner":{"Id":"8061383BF1175413","Type":"title_player_account"},
// "SearchData":{
// "string_key30":"server_search_flag",
// "string_key5":"eu-central",
// "string_key2":"36.9.6",
// "string_key1":"92535",
// "string_key4":"eu-central",
// "string_key3":"World_Map_01_Persistent_Level"}}],
// "Pagination":{"TotalMatchedLobbyCount":1}}
}

void UPlayfabLobbyBase::OnFindLobbiesError(const PlayFab::FPlayFabCppError& InError)
{
	LOG_MSGF(PlayfabLobbyBase, TEXT("InError: %s"), *InError.GenerateErrorReport());
}
#pragma endregion FindLobby

#pragma region JoinLobby
void UPlayfabLobbyBase::JoinLobby(const FString& InLobbyId, const FString& InConnectionString)
{
	LOG_MSGF(PlayfabLobbyBase, TEXT("LobbyId: %s"), *InLobbyId);

	PlayFab::MultiplayerModels::FJoinLobbyRequest Request;
	
	Request.ConnectionString = InConnectionString;
	LobbyConnectionString_ = InConnectionString;

	Request.MemberEntity = MakeShareable(new PlayFab::MultiplayerModels::FEntityKey());
	Request.MemberEntity->Id = DataProvider_->GetTitlePlayerID();
	Request.MemberEntity->Type = CommonConstants::PlayfabTitlePlayerIDFieldName;

	PlayFab::UPlayFabMultiplayerAPI::FJoinLobbyDelegate OnSuccess;
	OnSuccess.BindUObject(this, &UPlayfabLobbyBase::OnJoinLobby);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &UPlayfabLobbyBase::OnJoinLobbyError);
	
	const bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().JoinLobby(Request, OnSuccess, OnError);

	const FString RequestString = Request.toJSONString();

	LOG_MSGF(PlayfabLobbyBase, TEXT("RequestResult: %d; RequestString: %s"), RequestResult, *RequestString);
}

void UPlayfabLobbyBase::OnJoinLobby(const PlayFab::MultiplayerModels::FJoinLobbyResult& InResult)
{
	LOG_MSGF(PlayfabLobbyBase, TEXT("InResult %s"), *InResult.LobbyId);
	
	LobbyId_ = InResult.LobbyId;

	OnLobbyJoinHandle_.Broadcast(LobbyId_, LobbyConnectionString_);

	CheckLobby();
}

void UPlayfabLobbyBase::OnJoinLobbyError(const PlayFab::FPlayFabCppError& InError)
{
	LOG_VERB(PlayfabLobbyBase, Error, TEXT("InError: %s; ErrorCode: %d"), *InError.GenerateErrorReport(), InError.ErrorCode);

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
			CheckLobby();
		}
	}
}
#pragma endregion JoinLobby

#pragma region SignalR
void UPlayfabLobbyBase::SignalRLogin()
{
	LOG_FUNC_LABEL(PlayfabLobbyBase);
	ENSURE_RET(DataProvider_ != nullptr, PlayfabLobbyBase);
	
	PubSubRequester_ = MakeShareable(new FPubSubRequester());
	PubSubRequester_->OnOpenSessionError().AddUObject(this, &UPlayfabLobbyBase::OnSignalRSessionOpenError);
	PubSubRequester_->OnOpenSessionSuccess().AddUObject(this, &UPlayfabLobbyBase::OnSignalRSessionOpenSuccess);
	PubSubRequester_->OnCloseSession().AddUObject(this, &UPlayfabLobbyBase::OnSignalRSessionClosed);
	PubSubRequester_->Start(DataProvider_->GetTitleId(), DataProvider_->GetEntityToken());
}

void UPlayfabLobbyBase::SignalRClose()
{
	if(!PubSubRequester_.IsValid())
		return;

	LOG_FUNC_LABEL(PlayfabLobbyBase);

	PubSubRequester_->OnOpenSessionError().Clear();
	PubSubRequester_->OnOpenSessionSuccess().Clear();
	PubSubRequester_->OnCloseSession().Clear();
	PubSubRequester_->Stop();
	
	PubSubRequester_.Reset();	
}

void UPlayfabLobbyBase::OnSignalRSessionOpenError(const FString& InError)
{
	LOG_VERB(PlayfabLobbyBase, Error, TEXT("InError %s"), *InError);
}

void UPlayfabLobbyBase::OnSignalRSessionOpenSuccess(const FString& InConnectionHandle, bool InReconnecting)
{
	LOG_MSGF(PlayfabLobbyBase, TEXT("InConnectionHandle %s; InReconnecting: %d"), *InConnectionHandle, InReconnecting);

	SubscribeToLobbyChange();
}

void UPlayfabLobbyBase::OnSignalRSessionClosed(bool InUnexpected)
{
	LOG_MSGF(PlayfabLobbyBase, TEXT("InUnexpected: %d"), InUnexpected);
	if(!InUnexpected)
		return;

	SignalRClose();

	CheckLobby();
}

void UPlayfabLobbyBase::SubscribeToLobbyChange()
{
	LOG_FUNC_LABEL(PlayfabLobbyBase)
	ENSURE_RET(DataProvider_ != nullptr, PlayfabLobbyBase);
	
	PlayFab::MultiplayerModels::FSubscribeToLobbyResourceRequest Request;

	Request.Type = PlayFab::MultiplayerModels::SubscriptionTypeLobbyChange;
	Request.pfEntityKey.Id = DataProvider_->GetTitlePlayerID();
	Request.pfEntityKey.Type = CommonConstants::PlayfabTitlePlayerIDFieldName;
	Request.ResourceId = LobbyId_;
	Request.SubscriptionVersion = 1;

	if(PubSubRequester_.IsValid() && !PubSubRequester_->GetConnectionHandleString().IsEmpty())
		Request.PubSubConnectionHandle = PubSubRequester_->GetConnectionHandleString();
	else
	{
		LOG_VERB(PlayfabLobbyBase, Error, TEXT("PubSubRequester_ invalid!"), 0);
	}

	PlayFab::UPlayFabMultiplayerAPI::FSubscribeToLobbyResourceDelegate OnSuccess;
	OnSuccess.BindUObject(this, &UPlayfabLobbyBase::OnSubscribeToLobby);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &UPlayfabLobbyBase::OnSubscribeToLobbyError);

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
	LOG_MSGF(PlayfabLobbyBase, TEXT("RequestResult: %d; RequestString: %s"), RequestResult, *RequestString);
}

void UPlayfabLobbyBase::OnSubscribeToLobby(const PlayFab::MultiplayerModels::FSubscribeToLobbyResourceResult& InResult)
{
	//OnSubscribeToLobby for Client1: {"Topic":"1~lobby~LobbyChange~e425335a-0432-47be-8c12-ff0274ad4552"}
	LOG_MSGF(PlayfabLobbyBase, TEXT("InResult: %s"), *InResult.toJSONString());
}

void UPlayfabLobbyBase::OnSubscribeToLobbyError(const PlayFab::FPlayFabCppError& InError)
{
	//Invalid input parameters - PubSubConnectionHandle: The PubSubConnectionHandle field is required.
	//The incoming request subscription version does not match lobby subscription version
	LOG_VERB(PlayfabLobbyBase, Error, TEXT("InError %s"), *InError.GenerateErrorReport());
}
#pragma endregion SignalR

#pragma region CheckLobby
void UPlayfabLobbyBase::CheckLobby()
{
	ENSURE_RET(!LobbyId_.IsEmpty(), PlayfabLobbyBase);
	
	PlayFab::MultiplayerModels::FGetLobbyRequest Request;

	Request.LobbyId = LobbyId_;

	PlayFab::UPlayFabMultiplayerAPI::FGetLobbyDelegate OnSuccess;
	OnSuccess.BindUObject(this, &UPlayfabLobbyBase::OnCheckLobby);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &UPlayfabLobbyBase::OnCheckLobbyError);
		
	const bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().GetLobby(Request, OnSuccess, OnError);

	const FString RequestString = Request.toJSONString();

	LOG_MSGF(PlayfabLobbyBase, TEXT("RequestResult: %d; RequestString: %s"), RequestResult, *RequestString);
}

void UPlayfabLobbyBase::OnCheckLobby(const PlayFab::MultiplayerModels::FGetLobbyResult& InResult)
{
	LOG_MSGF(PlayfabLobbyBase, TEXT("InResult: %s"), *InResult.pfLobby.LobbyId);
	ENSURE(LobbyId_ == InResult.pfLobby.LobbyId, PlayfabLobbyBase);
	
	LobbyId_ = InResult.pfLobby.LobbyId;
	LobbyConnectionString_= InResult.pfLobby.ConnectionString;

	OnCheckLobbyHandle(InResult);

	if(OpenToFind_ && !PubSubRequester_.IsValid())
	{
		SignalRLogin();
	}

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

void UPlayfabLobbyBase::OnCheckLobbyError(const PlayFab::FPlayFabCppError& InError)
{
	LOG_MSGF(PlayfabLobbyBase, TEXT("InResult: %s"), *InError.GenerateErrorReport());
	
	LobbyId_ = FString();
	LobbyConnectionString_= FString();

	SignalRClose();

	OnCheckLobbyErrorHandle();
}
#pragma endregion CheckLobby

#pragma region LeaveLobby
void UPlayfabLobbyBase::LeaveLobby()
{
	LOG_FUNC_LABEL(PlayfabLobbyBase);
	ENSURE_RET(!LobbyId_.IsEmpty(), PlayfabLobbyBase);
	ENSURE_RET(DataProvider_ != nullptr, PlayfabLobbyBase);

	SignalRClose();
	
	PlayFab::MultiplayerModels::FLeaveLobbyRequest Request;
	
	Request.LobbyId = LobbyId_;

	Request.MemberEntity = MakeShareable(new PlayFab::MultiplayerModels::FEntityKey());
	Request.MemberEntity->Id = DataProvider_->GetTitlePlayerID();
	Request.MemberEntity->Type = CommonConstants::PlayfabTitlePlayerIDFieldName;

	PlayFab::UPlayFabMultiplayerAPI::FLeaveLobbyDelegate OnSuccess;
	OnSuccess.BindUObject(this, &UPlayfabLobbyBase::OnLeaveLobby);
	PlayFab::FPlayFabErrorDelegate OnError;
	OnError.BindUObject(this, &UPlayfabLobbyBase::OnLeaveLobbyError);
	
	const bool RequestResult = PlayFab::UPlayFabMultiplayerAPI().LeaveLobby(Request, OnSuccess, OnError);

	const FString RequestString = Request.toJSONString();

	LOG_MSGF(PlayfabLobbyBase, TEXT("RequestResult: %d; RequestString: %s"), RequestResult, *RequestString);
}

void UPlayfabLobbyBase::OnLeaveLobby(const PlayFab::MultiplayerModels::FLobbyEmptyResult& InResult)
{
	LOG_MSGF(PlayfabLobbyBase, TEXT("InResult %s"), *InResult.toJSONString());

	LobbyId_ = FString();
	LobbyConnectionString_ = FString();
}

void UPlayfabLobbyBase::OnLeaveLobbyError(const PlayFab::FPlayFabCppError& InError)
{
	LOG_VERB(PlayfabLobbyBase, Error, TEXT("%s"), *InError.GenerateErrorReport());
}
#pragma endregion LeaveLobby

void UPlayfabLobbyBase::SetOpenToFind(bool InValue)
{
	if(OpenToFind_ == InValue)
		return;
	
	LOG_MSGF(PlayfabLobbyBase, TEXT("InValue: %d"), InValue);

	OpenToFind_ = InValue;
	if(OpenToFind_)
	{
		SignalRLogin();
	}
	else
	{
		SignalRClose();
	}
}

