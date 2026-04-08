// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MomentumGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTMOMENTUM_API AMomentumGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMomentumGameMode();
protected:
	virtual void BeginPlay() override;
	
};
