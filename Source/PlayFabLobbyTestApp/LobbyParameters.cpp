#include "LobbyParameters.h"

#include "Algo/Find.h"
#include "LogUtility.h"
#include "StringBuilder.h"

DEFINE_LOG_CATEGORY_STATIC(HawkenMMStructs, Log, All);

FString FSPNames::Empty;

TArray<TripleN> FSPNames::Names_ =
{
TTuple<ESPNames, FString, FString>(ESPNames::Build,					TEXT("build"),			TEXT("string_key1")),
TTuple<ESPNames, FString, FString>(ESPNames::BuildNumber,			TEXT("build_number"),	TEXT("string_key2")),
TTuple<ESPNames, FString, FString>(ESPNames::GameVersion,			TEXT("game_version"),	TEXT("string_key3")),
TTuple<ESPNames, FString, FString>(ESPNames::Level,					TEXT("level"),			TEXT("string_key4")),
TTuple<ESPNames, FString, FString>(ESPNames::LevelWithMission,		TEXT("level_with_mission"), TEXT("string_key5")),
TTuple<ESPNames, FString, FString>(ESPNames::MissionIdent,			TEXT("mission_ident"),	TEXT("string_key6")),
TTuple<ESPNames, FString, FString>(ESPNames::ServerNeedsConfiguring,TEXT("server_needs_configuring"), TEXT("string_key7")),
TTuple<ESPNames, FString, FString>(ESPNames::ServerRegion,			TEXT("server_region"),	TEXT("string_key8")),
TTuple<ESPNames, FString, FString>(ESPNames::ServerLocation,		TEXT("server_location"),TEXT("string_key9")),
TTuple<ESPNames, FString, FString>(ESPNames::ServerName,			TEXT("server_name"),	TEXT("string_key10")),
TTuple<ESPNames, FString, FString>(ESPNames::AllocId,				TEXT("alloc_id"),		TEXT("string_key11")),
TTuple<ESPNames, FString, FString>(ESPNames::IpAddress,				TEXT("ip_address"),		TEXT("string_key12")),
TTuple<ESPNames, FString, FString>(ESPNames::Port,					TEXT("port"),			TEXT("string_key13")),
TTuple<ESPNames, FString, FString>(ESPNames::ServerOS,				TEXT("server_os"),		TEXT("string_key14")),
TTuple<ESPNames, FString, FString>(ESPNames::Environment,			TEXT("environment"),	TEXT("string_key15")),
TTuple<ESPNames, FString, FString>(ESPNames::Platform,				TEXT("platform"),		TEXT("string_key16")),
TTuple<ESPNames, FString, FString>(ESPNames::ServerLobbyLabel,		TEXT("server_lobby"),	TEXT("string_key17")),	
};

const FString& FSPNames::Normal(ESPNames InName)
{
	TripleN* Tp = Algo::FindByPredicate(Names_, [InName](const TripleN& val) { return val.Get<0>() == InName; });

	ENSURE(Tp != nullptr, HawkenMMStructs);
	if(Tp == nullptr)
		return Empty;
	
	return Tp->Get<1>();
}

const FString& FSPNames::Playfab(ESPNames InName)
{
	TripleN* Tp = Algo::FindByPredicate(Names_, [InName](const TripleN& val) { return val.Get<0>() == InName; });
	
	ENSURE(Tp != nullptr, HawkenMMStructs);
	if(Tp == nullptr)
		return Empty;
	
	return Tp->Get<2>();
}

void FLobbyParameters::AddPlayfabSearchFilters(FString& OutFilter) const
{
	for (const auto& Pair : Values_)
	{
		if(!Pair.Value.IsEmpty())
		{
			if(!OutFilter.IsEmpty())
				OutFilter.Append(TEXT(" and "));
			
			OutFilter.Appendf(TEXT("%s eq '%s'"), *FSPNames::Playfab(Pair.Key), *Pair.Value);
		}
	}
}

void FLobbyParameters::SerializeAllDataNormalName(TMap<FString, FString>& OutData) const
{
	for (const auto& Pair : Values_)
	{
		if(!Pair.Value.IsEmpty())
			OutData.Add(FSPNames::Normal(Pair.Key), Pair.Value);
	}
}

void FLobbyParameters::SerializeAllDataPrefabSearchName(TMap<FString, FString>& OutData) const
{
	for (const auto& Pair : Values_)
	{
		if(!Pair.Value.IsEmpty())
			OutData.Add(FSPNames::Playfab(Pair.Key), Pair.Value);
	}
}

FString FLobbyParameters::ToString() const
{
	FStringBuilder512 sb;
	for (const auto& Pair : Values_)
	{
		if(sb.Len() != 0)
			sb.Append(TEXT(", "));
		sb.Append(FSPNames::Normal(Pair.Key));
		sb.Append(TEXT(": "));
		sb.Append(Pair.Value);
	}
	return sb.ToString();
}