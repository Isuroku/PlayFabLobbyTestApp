#pragma once
#include "HAL/Platform.h"

namespace CommonConstants
{
	constexpr TCHAR* PlayfabTitlePlayerIDFieldName = TEXT("title_player_account");
	constexpr TCHAR* PlayfabPlayerIDFieldName = TEXT("master_player_account");
	constexpr TCHAR* TestQueueName = TEXT("test_queue_name");

	constexpr TCHAR* ServerSearchFlag = TEXT("server_search_flag");
	
	constexpr TCHAR* IP_Address = TEXT("ip_address");
	constexpr TCHAR* Port = TEXT("port");

	constexpr TCHAR* PFSK_BuildNumber = TEXT("string_key1");
	constexpr TCHAR* PFSK_GameVersion = TEXT("string_key2");
	constexpr TCHAR* PFSK_Level = TEXT("string_key3");
	constexpr TCHAR* PFSK_ServerNeedsConfiguring = TEXT("string_key4");
	constexpr TCHAR* PFSK_ServerRegion = TEXT("string_key5");

	constexpr TCHAR* PFSK_ServerName = TEXT("server_name");
	constexpr TCHAR* PFSK_AllocId = TEXT("alloc_id");
	constexpr TCHAR* PFSK_ServerSearchFlag = TEXT("string_key30");
}

struct FLobbyData
{
	virtual ~FLobbyData() = default;
	
	virtual void SaveToMap(TMap<FString, FString>& OutSearchData) const = 0;
	
	FString ToString() const
	{
		TMap<FString, FString> Data;
		SaveToMap(Data);
		FString Res;
		for (const auto& Pair : Data)
		{
			if(!Res.IsEmpty())
				Res.Append(TEXT(", "));
			Res.Appendf(TEXT("%s: %s"), *Pair.Key, *Pair.Value);
		}
		return Res;
	}
};

struct FSearchData: public FLobbyData
{
	FString BuildNumber;
	FString GameVersion;
	FString Level;
	FString ServerNeedsConfiguring;
	FString ServerRegion;
	FString ServerName;
	FString AllocId;

	virtual void SaveToMap(TMap<FString, FString>& OutSearchData) const override
	{
		OutSearchData.Add(CommonConstants::PFSK_ServerSearchFlag, CommonConstants::ServerSearchFlag);
		if(!BuildNumber.IsEmpty())
			OutSearchData.Add(CommonConstants::PFSK_BuildNumber, BuildNumber);
		if(!GameVersion.IsEmpty())
			OutSearchData.Add(CommonConstants::PFSK_GameVersion, GameVersion);
		if(!Level.IsEmpty())
			OutSearchData.Add(CommonConstants::PFSK_Level, Level);
		if(!ServerNeedsConfiguring.IsEmpty())
			OutSearchData.Add(CommonConstants::PFSK_ServerNeedsConfiguring, ServerNeedsConfiguring);
		if(!ServerRegion.IsEmpty())
			OutSearchData.Add(CommonConstants::PFSK_ServerRegion, ServerRegion);
		if(!ServerName.IsEmpty())
			OutSearchData.Add(CommonConstants::PFSK_ServerName, ServerName);
		if(!AllocId.IsEmpty())
			OutSearchData.Add(CommonConstants::PFSK_AllocId, AllocId);
	}

	void AddPlayfabSearchFilters(FString& OutFilter) const
	{
		TMap<FString, FString> Data;
		SaveToMap(Data);
		for (const auto& Pair : Data)
		{
			if(!OutFilter.IsEmpty())
				OutFilter.Append(TEXT(" and "));
			
			OutFilter.Appendf(TEXT("%s eq '%s'"), *Pair.Key, *Pair.Value);
		}
	}
};

struct FServerLobbyData: public FLobbyData
{
	FString IpAddress;
	FString Port;

	virtual void SaveToMap(TMap<FString, FString>& OutSearchData) const override
	{
		if(!IpAddress.IsEmpty())
			OutSearchData.Add(CommonConstants::IP_Address, IpAddress);
		if(!Port.IsEmpty())
			OutSearchData.Add(CommonConstants::Port, Port);
	}
};
