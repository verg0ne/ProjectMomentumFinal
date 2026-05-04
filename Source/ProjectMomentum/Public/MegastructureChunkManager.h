#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "DA_WorldParameters.h"
#include "MegaChunk.h"
#include "MegastructureChunkManager.generated.h"

UCLASS()
class PROJECTMOMENTUM_API AMegastructureChunkManager : public AActor
{
    GENERATED_BODY()

public:
    AMegastructureChunkManager();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Streaming", meta=(ClampMin="2", ClampMax="8"))
    int32 ChunksAheadToLoad = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Streaming", meta=(ClampMin="1", ClampMax="4"))
    int32 ChunksBehindToKeep = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Streaming", meta=(ClampMin="8", ClampMax="24"))
    int32 PoolSize = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Streaming", meta=(ClampMin="1.0", ClampMax="5.0"))
    float VelocityLookAheadSeconds = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
    TSubclassOf<AMegachunk> ChunkClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
    TSoftObjectPtr<UDA_WorldParameters> WorldParametersAsset;

    UPROPERTY(BlueprintReadOnly, Category="Config")
    UDA_WorldParameters* CachedParams = nullptr;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    TArray<AMegachunk*> ChunkPool;

    TMap<FIntVector, AMegachunk*> ActiveChunks;

    TSet<FIntVector> PendingChunks;

    UFUNCTION(BlueprintCallable)
    void WarmPool();

    AMegachunk* GetChunkFromPool();

    void ReturnChunkToPool(AMegachunk* Chunk);

    void RequestChunk(FIntVector Coord);

    bool ShouldCullChunk(AMegachunk* Chunk, FVector PlayerPos) const;

    FIntVector WorldToChunkCoord(FVector WorldPos) const;

    FVector ChunkCoordToWorldOrigin(FIntVector Coord) const;

    int32 ChunkSeedFromCoord(FIntVector Coord) const;
};
