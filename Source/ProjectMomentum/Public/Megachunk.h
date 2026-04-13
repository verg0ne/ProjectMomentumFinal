#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "PCGComponent.h"  // UE 5.7: no subdirectory prefix
#include "MegaChunk.generated.h"

UCLASS()
class PROJECTMOMENTUM_API AMegachunk : public AActor
{
    GENERATED_BODY()
public:
    AMegachunk();

    // ── Set by manager before async step — need UPROPERTY so Blueprint can read them ──

    // Deterministic seed for this chunk. Blueprint reads this to seed ChunkRandomStream.
    UPROPERTY(BlueprintReadWrite, Category="Chunk")
    int32 ChunkSeed = 0;

    // World-space bottom-left-floor corner of this chunk (Z-up). Blueprint reads for SetActorLocation.
    UPROPERTY(BlueprintReadWrite, Category="Chunk")
    FVector ChunkOrigin = FVector::ZeroVector;

    // Pool state flag — manager reads/writes this. Not exposed to BP to prevent accidental misuse.
    bool bInUse = false;

    // WorldParams reference — Blueprint reads to access DA values and pass to PCG
    UPROPERTY(BlueprintReadWrite, Category="Chunk")
    class UDA_WorldParameters* WorldParamsRef = nullptr;

    // ── Called off game thread — ONLY plain C++ math, absolutely no UObjects ──
    // Base implementation just stores seed/origin. Override in C++ child if needed.
    virtual void PrepareDataAsync(int32 InSeed, FVector InOrigin);

    // ── Called on game thread — override in BP_MegaChunk via BlueprintNativeEvent ──
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Chunk")
    void FinalizeOnGameThread();
    virtual void FinalizeOnGameThread_Implementation();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Chunk")
    void ResetForPool();
    virtual void ResetForPool_Implementation();

    // ── Web anchor points — filled after PCG completes, queried by swing system ──
    UPROPERTY(BlueprintReadOnly, Category="Anchors")
    TArray<FVector> WebAnchorPoints;

    // ── HISM components ──────────────────────────────────────────────────────────
    // BlueprintReadOnly: Blueprint can GET the reference to call methods on it
    // (Set Collision Profile Name, etc.) but cannot reassign the pointer itself.
    // VisibleAnywhere: shows in Details panel for debugging.
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

    // ── PCG components — same access pattern as HISM ──
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UPCGComponent* PCG_Street;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UPCGComponent* PCG_Facades;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UPCGComponent* PCG_Bridges;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UPCGComponent* PCG_Details;

    // ── Detail proximity trigger ──────────────────────────────────────────────────
    // Created in C++ constructor — never dynamically at runtime.
    // Blueprint binds overlap events to it, but the component itself lives here.
    // Radius is set in FinalizeOnGameThread from WorldParamsRef.DetailSpawnRadius.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    USphereComponent* ProximityTrigger;
};