#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "GameFramework/SpringArmComponent.h"
#include "PlayerCharacter.generated.h"

// Forward declarations
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class PROJECTMOMENTUM_API APlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    APlayerCharacter();
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsFlipping = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* Camera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spring Arm")
    USpringArmComponent* SpringArm;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
	
protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputAction* MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputAction* JumpAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputAction* LookAction;
};
