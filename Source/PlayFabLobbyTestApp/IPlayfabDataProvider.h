#pragma once
#include "Containers/UnrealString.h"

class IPlayfabDataProvider
{
public:
	virtual ~IPlayfabDataProvider() = default;

	virtual const TCHAR* GetTitleId() const = 0;
	virtual const FString& GetTitlePlayerID() const = 0;
	virtual const FString& GetEntityToken() const = 0;
	virtual UWorld* GetWorld() const = 0;
};
