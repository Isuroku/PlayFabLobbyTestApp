#pragma once
#include "CoreMinimal.h"

enum class ESPNames : uint8
{
	Build = 0,
	BuildNumber,
	GameVersion,
	Level,
	LevelWithMission,
	MissionIdent,
	ServerNeedsConfiguring,
	ServerRegion,
	ServerLocation,
	ServerName,
	AllocId,
	IpAddress,
	Port,
	ServerOS,
	Environment,
	Platform,
	ServerLobbyLabel,
};

using TripleN = TTuple<ESPNames, FString, FString>;

class FSPNames
{
public:
	static const FString& Normal(ESPNames InName);
	static const FString& Playfab(ESPNames InName);
private:
	static TArray<TripleN> Names_;
	static FString Empty;
}; 

class FLobbyParameters
{
public:
	virtual ~FLobbyParameters() = default;
	
	void AddPlayfabSearchFilters(FString& OutFilter) const;

	void SerializeAllDataNormalName(TMap<FString, FString>& OutData) const;
	void SerializeAllDataPrefabSearchName(TMap<FString, FString>& OutData) const;

	FString& operator[](ESPNames Key) { return Values_.FindOrAdd(Key); }
	
	FString ToString() const;

protected:
	TMap<ESPNames, FString> Values_;
};