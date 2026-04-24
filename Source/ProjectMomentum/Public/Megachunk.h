#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "PCGComponent.h"
#include "MegaChunk.generated.h"

UCLASS()
class PROJECTMOMENTUM_API AMegachunk : public AActor
{
    GENERATED_BODY()
public:
    AMegachunk();
    
    UPROPERTY(BlueprintReadWrite, Category="Chunk")
    int32 ChunkSeed = 0;

    UPROPERTY(BlueprintReadWrite, Category="Chunk")
    FVector ChunkOrigin = FVector::ZeroVector;

    bool bInUse = false;
    
    UPROPERTY(BlueprintReadWrite, Category="Chunk")
    class UDA_WorldParameters* WorldParamsRef = nullptr;

    virtual void PrepareDataAsync(int32 InSeed, FVector InOrigin);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Chunk")
    void FinalizeOnGameThread();
    virtual void FinalizeOnGameThread_Implementation();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Chunk")
    void ResetForPool();
    virtual void ResetForPool_Implementation();

    UPROPERTY(BlueprintReadOnly, Category="Anchors")
    TArray<FVector> WebAnchorPoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UHierarchicalInstancedStaticMeshComponent* HISM_Floor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UHierarchicalInstancedStaticMeshComponent* HISM_Ceiling;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UHierarchicalInstancedStaticMeshComponent* HISM_Walls;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UHierarchicalInstancedStaticMeshComponent* HISM_Ledges;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UHierarchicalInstancedStaticMeshComponent* HISM_Piers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UHierarchicalInstancedStaticMeshComponent* HISM_Bridges;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UHierarchicalInstancedStaticMeshComponent* HISM_Details;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UPCGComponent* PCG_Street;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UPCGComponent* PCG_Facades;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UPCGComponent* PCG_Bridges;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UPCGComponent* PCG_Details;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    USphereComponent* ProximityTrigger;
};