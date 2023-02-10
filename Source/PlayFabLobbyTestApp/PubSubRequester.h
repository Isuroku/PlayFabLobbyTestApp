#pragma once

class IHubConnection;

class FPubSubRequester: public TSharedFromThis<FPubSubRequester>
{
public:
	FPubSubRequester();

	void Start(const FString& InTitleId, const FString& InEntityToken);

	void Stop();

	const FString& GetConnectionHandleString() const { return ConnectionHandle_; }
	
	DECLARE_EVENT_OneParam(FPubSubRequester, FOnOpenSessionErrorEvent, const FString& /* InError */);
	FOnOpenSessionErrorEvent& OnOpenSessionError() { return OpenSessionErrorHandle_; }

	DECLARE_EVENT_TwoParams(FPubSubRequester, FOnOpenSessionSuccessEvent, const FString& /* InConnectionHandle */, bool /*InReconnecting*/);
	FOnOpenSessionSuccessEvent& OnOpenSessionSuccess() { return OpenSessionSuccessHandle_; }

	DECLARE_EVENT_OneParam(FPubSubRequester, FOnCloseSessionEvent, bool /*InUnexpected*/);
	FOnCloseSessionEvent& OnCloseSession() { return CloseSessionHandle_; }

private:
	FOnOpenSessionErrorEvent OpenSessionErrorHandle_;
	FOnOpenSessionSuccessEvent OpenSessionSuccessHandle_;
	FOnCloseSessionEvent CloseSessionHandle_;

	void OnSignalRNegotiateResult(TSharedPtr<FJsonObject> InJsonObject, bool InReconnecting);
	void OnHubConnected(bool InReconnecting);
	void OnHubConnectionError(const FString& InError);
	void OnHubClosed(bool InUnexpected);

	static FString CreateTraceParent();

	TSharedPtr<IHubConnection> Hub_;

	FString EntityToken_;
	FString ConnectionHandle_;
};
