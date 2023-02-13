#include "LogUtility.h"

#include "TestMenuWidget.h"
#include "Engine/GameInstance.h"

FString ConvertFirst127CharStrToFString(const ANSICHAR* inCharStr)
{
	const TCHAR* p = ANSI_TO_TCHAR(inCharStr);
	return FString(p);
}

FString GetClassMethodName(const ANSICHAR* InName)
{
	const ANSICHAR* p = TCString<ANSICHAR>::Strchr(InName, ':');
	if (p == nullptr || *(p + 1) != ':')
		p = InName;
	else
		p = p + 2;

	return ConvertFirst127CharStrToFString(p);
}

const TCHAR* LogVerbosityToString(ELogVerbosity::Type Verbosity)
{
	if (Verbosity == ELogVerbosity::Type::Error)
		return TEXT("Error");
	if (Verbosity == ELogVerbosity::Type::Warning)
		return TEXT("Warning");
	return TEXT("Log");
}

void FOuterLogger::AddLog(ELogVerbosity::Type InVerbosity, const TCHAR* InCategoryName, const FString& InMethodName, const TCHAR* InText)
{
	UTestMenuWidget*  MenuWidget = UTestMenuWidget::GetMenuWidgetSingleton();
	if(MenuWidget == nullptr)
		return;;

	FString Text;
	if(InText != nullptr)
		Text = FString::Printf(TEXT("[%s] %s::%s: %s"), LogVerbosityToString(InVerbosity), InCategoryName, *InMethodName, InText);
	else
		Text = FString::Printf(TEXT("[%s] %s::%s"), LogVerbosityToString(InVerbosity), InCategoryName, *InMethodName);
	MenuWidget->AddLog(FText::FromString(Text));
}