// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TestMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class PLAYFABLOBBYTESTAPP_API UTestMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	void SetGameInstance(class ULocalTestInstance* InGameInstance);

	UFUNCTION(BlueprintImplementableEvent)
	void AddLog(const FText& InText);

	UFUNCTION(BlueprintCallable)
	ATestPlayfabPlayerController* GetCurrentPlayerController() const;

	UFUNCTION(BlueprintImplementableEvent)
	void SetLobbyId(const FString& InValue);

	UFUNCTION(BlueprintImplementableEvent)
	void SetFoundLobbyIds(const TArray<FString>& InLobbyNames);

private:

};
