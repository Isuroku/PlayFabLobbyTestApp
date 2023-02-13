// Fill out your copyright notice in the Description page of Project Settings.


#include "TestMenuWidget.h"

#include "TestPlayfabPlayerController.h"

UTestMenuWidget* UTestMenuWidget::MenuWidgetSingleton_;

void UTestMenuWidget::NativeConstruct()
{
	MenuWidgetSingleton_ = this;
	
	Super::NativeConstruct();
}

void UTestMenuWidget::NativeDestruct()
{
	MenuWidgetSingleton_ = nullptr;
	
	Super::NativeDestruct();
}

ATestPlayfabPlayerController* UTestMenuWidget::GetCurrentPlayerController() const
{
	return Cast<ATestPlayfabPlayerController>(GetOwningPlayer());
}
