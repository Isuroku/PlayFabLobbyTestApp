#include "PubSubRequester.h"

#include "IHubConnection.h"
#include "SignalRSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(PubSubRequester, Log, All);

FPubSubRequester::FPubSubRequester()
{
}

FString FPubSubRequester::CreateTraceParent()
{
	return FString();
}

void FPubSubRequester::Start(const FString& InTitleId, const FString& InEntityToken)
{
	const FString Url = FString::Printf(TEXT("https://%s.playfabapi.com/pubsub"), *InTitleId);

	UE_LOG(PubSubRequester, Log, TEXT("SignalRLogin: Url: %s; EntityToken_: %s"),  *Url, *InEntityToken);

	//Url: https://A2545.playfabapi.com/pubsub; EntityToken_: NHx4SkhPYmNicmNEeUJobDNvdUhRZG8vdDZtajQnlOa0JHSnE3Z0xHa0prPXx7ImkiOiIyMDIzLTAyLTAzVDExOjE2OjQ2LjY3ODIxN...
	
	Hub_ = GEngine->GetEngineSubsystem<USignalRSubsystem>()->CreateHubConnection(Url);
	Hub_->AddNegotiateHeader(TEXT("X-EntityToken"), InEntityToken);
	Hub_->AddNegotiateHeader(TEXT("ContentType"), TEXT("application/json"));

	Hub_->OnNegotiationCompleteFail()->BindLambda([Self = TWeakPtr<FPubSubRequester>(AsShared())](const FConnectionErrorData& InErrorData)
	{
		if (const TSharedPtr<FPubSubRequester> SharedSelf = Self.Pin())
			if(SharedSelf.IsValid())
			{
				const FString ErrorText = InErrorData.ToString();
				UE_LOG(LogTemp, Error, TEXT("OnNegotiationCompleteFail: %s"), *ErrorText);
				SharedSelf->OpenSessionErrorHandle_.Broadcast(ErrorText);
			}
	});

	EntityToken_ = InEntityToken;

	Hub_->OnNegotiationCompleteSuccess()->BindRaw(this, &FPubSubRequester::OnSignalRNegotiateResult);
	
	Hub_->OnConnected().AddRaw(this, &FPubSubRequester::OnHubConnected);
	Hub_->OnConnectionError().AddRaw(this, &FPubSubRequester::OnHubConnectionError);
	Hub_->Start();
}

void FPubSubRequester::OnSignalRNegotiateResult(TSharedPtr<FJsonObject> InJsonObject, bool InReconnecting)
{
	UE_LOG(LogTemp, Log, TEXT("OnNegotiationCompleteSuccess"));

	const FString UrlStr = TEXT("url");
	const FString AccessTokenStr = TEXT("accessToken");
	const FString AuthorizationStr = TEXT("Authorization");

	if(!InJsonObject->Values.Contains(UrlStr) || !InJsonObject->Values.Contains(AccessTokenStr))
		return;

	const FString RedirectUrl = InJsonObject->Values[UrlStr]->AsString();
	const FString AccessToken = InJsonObject->Values[AccessTokenStr]->AsString();

	UE_LOG(PubSubRequester, Log, TEXT("RedirectUrl: %s; AccessToken: %s"), *RedirectUrl, *AccessToken);
	//https://pubsub-signalr-prod-westus3-001.service.signalr.net/client/?hub=pubsubhub;

	Hub_->ClearWebSocketHeader();
	Hub_->AddWebSocketHeader(AuthorizationStr, AccessToken);
	Hub_->AddWebSocketHeader(TEXT("X-EntityToken"), EntityToken_);
	Hub_->AddWebSocketHeader(TEXT("ContentType"), TEXT("application/json"));
	
	Hub_->StartWebSocket(RedirectUrl);
}

void FPubSubRequester::OnHubConnected(bool InReconnecting)
{
	UE_LOG(PubSubRequester, Log, TEXT("Hub connected! InReconnecting: %d"), InReconnecting);

	TMap<FString, FSignalRValue> Arguments;
	//PlayFab SignalR doesn't request a value of traceParent. 
	//const FString TraceParentValue = FString::Printf(TEXT("00-7678fd69ae13e45fce1338289bcf482-%s-00"), *PlayfabTitlePlayerID_.ToLower());
	Arguments.Emplace(TEXT("traceParent"), FSignalRValue(CreateTraceParent()));
	Arguments.Emplace(TEXT("oldConnectionHandle"), FSignalRValue(ConnectionHandle_));
	
	Hub_->Invoke(TEXT("StartOrRecoverSession"), Arguments).BindLambda([Self = TWeakPtr<FPubSubRequester>(AsShared()), InReconnecting](const FSignalRInvokeResult& Result)
	{
		if (TSharedPtr<FPubSubRequester> SharedSelf = Self.Pin())
			if(SharedSelf.IsValid())
			{
				if (Result.HasError())
				{
					UE_LOG(LogTemp, Error, TEXT("StartOrRecoverSessionFail: %s"), *Result.GetErrorMessage());
					SharedSelf->OpenSessionErrorHandle_.Broadcast(Result.GetErrorMessage());
				}
				else
				{
					FString Error;
					if(Result.IsObject())
					{
						const FString NewConnectionHandleKey = TEXT("newConnectionHandle");
						
						const TMap<FString, FSignalRValue>& Params = Result.AsObject();
						if(Params.Contains(NewConnectionHandleKey))
						{
							SharedSelf->ConnectionHandle_ = Params[NewConnectionHandleKey].AsString();
							UE_LOG(LogTemp, Log, TEXT("StartOrRecoverSessionSuccess: %s"), *SharedSelf->ConnectionHandle_);
							
							SharedSelf->OpenSessionSuccessHandle_.Broadcast(SharedSelf->ConnectionHandle_, InReconnecting);
						}
						else
						{
							Error = FString::Printf(TEXT("StartOrRecoverSessionFail: Can't find %s field!"), *NewConnectionHandleKey);								
						}
					}
					else
						Error = TEXT("StartOrRecoverSessionFail: Result isn't object!");

					if(!Error.IsEmpty())
					{
						UE_LOG(LogTemp, Error, TEXT("StartOrRecoverSessionFail: %s"), *Error);
						SharedSelf->OpenSessionErrorHandle_.Broadcast(Error);
					}
					//		"newConnectionHandle":"1.Y2VudHJhbC11c35wdWJzdWItc2lnbmFsci1wcm9kLXdlc3R1czMtMDAxfml0MW15a3BnUU5tUzVrMWU3dDg2S1FlMjE0MDRmNzE=",
					//		"status":"success",
					//		"traceId":"187e5c306b4fad8409fd78a0a393d657"

					//UE_LOG(LogTemp, Error, TEXT("StartOrRecoverSessionSuccess: %s"), *Result.AsString());
				}
			}
	});
}

void FPubSubRequester::OnHubConnectionError(const FString& InError)
{
	//Num=147 "{"code":401,"status":"Unauthorized","error":"NotAuthenticated","errorCode":1074,"errorMessage":"This API method does not allow anonymous callers."}"
	UE_LOG(PubSubRequester, Error, TEXT("OnHubConnectionError: %s"), *InError);
	OpenSessionErrorHandle_.Broadcast(InError);
}

void FPubSubRequester::Stop()
{
	UE_LOG(PubSubRequester, Log, TEXT("FPubSubRequester::Close()"));

	TMap<FString, FSignalRValue> Arguments;
	Arguments.Emplace(TEXT("traceParent"), FSignalRValue(CreateTraceParent()));
	Hub_->Invoke(TEXT("EndSessionRequest"), Arguments).BindLambda([Self = TWeakPtr<FPubSubRequester>(AsShared())](const FSignalRInvokeResult& Result)
	{
		if (TSharedPtr<FPubSubRequester> SharedSelf = Self.Pin())
			if(SharedSelf.IsValid())
			{
				UE_LOG(PubSubRequester, Log, TEXT("Session Closed"));
			}
	});

	if(Hub_.IsValid())
		Hub_->Stop();
	Hub_.Reset();
}