// Fill out your copyright notice in the Description page of Project Settings.


#include "MomentumGameMode.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Actor.h"

AMomentumGameMode::AMomentumGameMode()
{
	//static ConstructorHelpers:: FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/All/Content/Blueprints/BP_PlayerCharacter"));
	//DefaultPawnClass = PlayerPawnClassFinder.Class;
}

void AMomentumGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("Game has started"));
}


