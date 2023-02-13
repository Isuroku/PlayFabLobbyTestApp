#pragma once

#include "CoreMinimal.h"

#ifndef __func__
#define __func__ __FUNCTION__
#endif

FString GetClassMethodName(const ANSICHAR* InName);

class FOuterLogger
{
public:
	static void AddLog(ELogVerbosity::Type InVerbosity, const TCHAR* InCategoryName, const FString& InMethodName, const TCHAR* InText);
};

#if NO_LOGGING

#define LOG_FUNC_LABEL(CategoryName) {}
#define LOG_MSG(CategoryName, TEXT) {}
#define LOG_MSGF(CategoryName, Format, ...) {}
#define LOG_VERB(CategoryName, Verbosity, Format, ...) {}

#define ENSURE(EXP, CategoryName) {}
#define ENSURE_RET(EXP, CategoryName) { if (!(EXP)) return; }
#define ENSURE_MSG_RET(EXP, CategoryName, Format, ...) { if (!(EXP)) return; }
#define ENSURE_RET_BOOL(EXP, CategoryName) { if (!(EXP)) return false; }
#define ENSURE_MSG_RET_BOOL(EXP, CategoryName, Format, ...) { if (!(EXP)) return false; }
#define ENSURE_RET_NULL(EXP, CategoryName) { if (!(EXP)) return nullptr; }
#define ENSURE_RET_STR(EXP, CategoryName) { if (!(EXP)) return TEXT(""); }
#define ENSURE_RET_0(EXP, CategoryName) { if (!(EXP)) return 0; }

#else

#define LOG_FUNC_LABEL(CategoryName) \
{ \
	FString MethodName = GetClassMethodName(__func__ ); \
	UE_LOG(CategoryName, Log, TEXT("%s"), *MethodName); \
	FOuterLogger::AddLog(ELogVerbosity::Type::Log, TEXT(#CategoryName), MethodName, nullptr);\
}

#define LOG_MSG(CategoryName, Text) \
{ \
	FString MethodName = GetClassMethodName(__func__ ); \
	UE_LOG(CategoryName, Log, TEXT("%s: %s"), *MethodName, Text); \
	FOuterLogger::AddLog(ELogVerbosity::Type::Log, TEXT(#CategoryName), MethodName, Text);\
}

#define LOG_MSGF(CategoryName, Format, ...) \
{ \
	FString MethodName = GetClassMethodName(__func__ ); \
	FString Text = FString::Printf(Format, ##__VA_ARGS__); \
	UE_LOG(CategoryName, Log, TEXT("%s: %s"), *MethodName, *Text); \
	FOuterLogger::AddLog(ELogVerbosity::Type::Log, TEXT(#CategoryName), MethodName, *Text);\
}

#define LOG_VERB(CategoryName, Verbosity, Format, ...) \
{ \
	FString MethodName = GetClassMethodName(__func__ ); \
	FString Text = FString::Printf(Format, ##__VA_ARGS__); \
	UE_LOG(CategoryName, Verbosity, TEXT("%s: %s"), *MethodName, *Text); \
	FOuterLogger::AddLog(ELogVerbosity::Type::Verbosity, TEXT(#CategoryName), MethodName, *Text);\
}

#define ENSURE(EXP, CategoryName) { \
	if (!(EXP)) \
	{ \
		FString MethodName = GetClassMethodName(__func__ ); \
		UE_LOG(CategoryName, Error, TEXT("%s %d: %s"), *MethodName, __LINE__, TEXT(#EXP)); \
		FOuterLogger::AddLog(ELogVerbosity::Type::Error, TEXT(#CategoryName), MethodName, TEXT(#EXP));\
		UE_DEBUG_BREAK(); \
	} \
}

#define ENSURE_RET(EXP, CategoryName) { \
	if (!(EXP)) \
	{ \
		FString MethodName = GetClassMethodName(__func__ ); \
		UE_LOG(CategoryName, Error, TEXT("%s %d: %s"), *MethodName, __LINE__, TEXT(#EXP)); \
		FOuterLogger::AddLog(ELogVerbosity::Type::Error, TEXT(#CategoryName), MethodName, TEXT(#EXP));\
		UE_DEBUG_BREAK(); \
		return; \
	} \
}

#define ENSURE_MSG_RET(EXP, CategoryName, Format, ...) { \
	if (!(EXP)) \
	{ \
		FString MethodName = GetClassMethodName(__func__ ); \
		FString Text = FString::Printf(Format, ##__VA_ARGS__); \
		UE_LOG(CategoryName, Error, TEXT("%s %d: %s; %s"), *MethodName, __LINE__, TEXT(#EXP), *Text); \
		FOuterLogger::AddLog(ELogVerbosity::Type::Error, TEXT(#CategoryName), MethodName, TEXT(#EXP));\
		UE_DEBUG_BREAK(); \
		return; \
	} \
}

#define ENSURE_RET_BOOL(EXP, CategoryName) { \
	if (!(EXP)) \
	{ \
		FString MethodName = GetClassMethodName(__func__ ); \
		UE_LOG(CategoryName, Error, TEXT("%s %d: %s"), *MethodName, __LINE__, TEXT(#EXP)); \
		FOuterLogger::AddLog(ELogVerbosity::Type::Error, TEXT(#CategoryName), MethodName, TEXT(#EXP));\
		UE_DEBUG_BREAK(); \
		return false; \
	} \
}

#define ENSURE_MSG_RET_BOOL(EXP, CategoryName, Format, ...) { \
	if (!(EXP)) \
	{ \
		FString MethodName = GetClassMethodName(__func__ ); \
		FString Text = FString::Printf(Format, ##__VA_ARGS__); \
		UE_LOG(CategoryName, Error, TEXT("%s %d: %s; %s"), *MethodName, __LINE__, TEXT(#EXP), *Text); \
		FOuterLogger::AddLog(ELogVerbosity::Type::Error, TEXT(#CategoryName), MethodName, TEXT(#EXP));\
		UE_DEBUG_BREAK(); \
		return false; \
	} \
}

#define ENSURE_RET_NULL(EXP, CategoryName) { \
	if (!(EXP)) \
	{ \
		FString MethodName = GetClassMethodName(__func__ ); \
		UE_LOG(CategoryName, Error, TEXT("%s %d: %s"), *MethodName, __LINE__, TEXT(#EXP)); \
		FOuterLogger::AddLog(ELogVerbosity::Type::Error, TEXT(#CategoryName), MethodName, TEXT(#EXP));\
		UE_DEBUG_BREAK(); \
		return nullptr; \
	} \
}

#define ENSURE_RET_STR(EXP, CategoryName) { \
	if (!(EXP)) \
	{ \
		FString MethodName = GetClassMethodName(__func__ ); \
		UE_LOG(CategoryName, Error, TEXT("%s %d: %s"), *MethodName, __LINE__, TEXT(#EXP)); \
		FOuterLogger::AddLog(ELogVerbosity::Type::Error, TEXT(#CategoryName), MethodName, TEXT(#EXP));\
		UE_DEBUG_BREAK(); \
		return TEXT(""); \
	} \
}

#define ENSURE_RET_0(EXP, CategoryName) { \
	if (!(EXP)) \
	{ \
		FString MethodName = GetClassMethodName(__func__ ); \
		UE_LOG(CategoryName, Error, TEXT("%s %d: %s"), *MethodName, __LINE__, TEXT(#EXP)); \
		FOuterLogger::AddLog(ELogVerbosity::Type::Error, TEXT(#CategoryName), MethodName, TEXT(#EXP));\
		UE_DEBUG_BREAK(); \
		return 0; \
	} \
}

#endif