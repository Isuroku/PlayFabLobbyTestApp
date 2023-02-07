// Fill out your copyright notice in the Description page of Project Settings.


#include "TestMenuWidget.h"

#include "TestPlayfabPlayerController.h"

 ATestPlayfabPlayerController* UTestMenuWidget::GetCurrentPlayerController() const
 {
 	return Cast<ATestPlayfabPlayerController>(GetOwningPlayer());
 }