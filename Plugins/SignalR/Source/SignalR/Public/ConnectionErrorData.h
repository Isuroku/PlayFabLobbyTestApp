#pragma once

#include "CoreMinimal.h"

struct SIGNALR_API FConnectionErrorData
{
    int32 Code;
    FString Status;
    FString Error;
    FString ErrorCode;
    FString ErrorMessage;

    FString ToString() const
    {
        return FString::Printf(TEXT("code:%d; status: %s; error: %s; errorCode: %s; errorMessage: %s"),
            Code, *Status, *Error, *ErrorCode, *ErrorMessage);
    }
};

DECLARE_DELEGATE_OneParam(FOnHubNegotiationCompleteFailEvent, const FConnectionErrorData& /*InErrorData*/ )
DECLARE_DELEGATE_OneParam(FOnHubNegotiationCompleteSuccessEvent, TSharedPtr<FJsonObject> /*InJsonObject*/ )
