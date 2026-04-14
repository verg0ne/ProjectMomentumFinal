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

    // ── Streaming Tuning ────────────────────────────────────────

    // Number of chunks to load ahead of the player along the predicted path
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Streaming", meta=(ClampMin="2", ClampMax="8"))
    int32 ChunksAheadToLoad = 4;

    // Number of chunks to keep behind the player before culling them
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Streaming", meta=(ClampMin="1", ClampMax="4"))
    int32 ChunksBehindToKeep = 2;

    // Total pool size — must be >= ChunksAheadToLoad + ChunksBehindToKeep + 2 safety buffer
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Streaming", meta=(ClampMin="8", ClampMax="24"))
    int32 PoolSize = 12;

    // How many seconds ahead to look when predicting player position from velocity
    // Higher = earlier loads but more wasted loads if player changes direction
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Streaming", meta=(ClampMin="1.0", ClampMax="5.0"))
    float VelocityLookAheadSeconds = 2.5f;

    // ── Config ──────────────────────────────────────────────────

    // The Blueprint subclass of AMegaChunk to spawn — set this to BP_MegaChunk in editor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
    TSubclassOf<AMegachunk> ChunkClass;

    // Soft ref to the world parameter DataAsset — assign in editor, loaded at BeginPlay
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config")
    TSoftObjectPtr<UDA_WorldParameters> WorldParametersAsset;

    // ── Runtime-accessible cached params ────────────────────────

    // Loaded hard pointer — valid after BeginPlay, null before
    UPROPERTY(BlueprintReadOnly, Category="Config")
    UDA_WorldParameters* CachedParams = nullptr;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // ── Pool ────────────────────────────────────────────────────

    // All pre-spawned chunk actors. Entries are never null after WarmPool().
    TArray<AMegachunk*> ChunkPool;

    // Active chunks keyed by their 3D grid coordinate
    TMap<FIntVector, AMegachunk*> ActiveChunks;

    // Chunks currently being generated async (coord reserved, actor claimed from pool)
    TSet<FIntVector> PendingChunks;

    // ── Internal helpers ────────────────────────────────────────

    // Spawns all pool actors at BeginPlay at a far limbo position
    UFUNCTION(BlueprintCallable)
    void WarmPool();

    // Returns a free chunk from the pool, or nullptr if pool is exhausted
    AMegachunk* GetChunkFromPool();

    // Resets a chunk and marks it free in the pool
    void ReturnChunkToPool(AMegachunk* Chunk);

    // Issues an async generation request for a chunk at Coord
    void RequestChunk(FIntVector Coord);

    // Returns true if a chunk is far enough behind/beside the player to cull
    bool ShouldCullChunk(AMegachunk* Chunk, FVector PlayerPos) const;

    // Convert world position (Z-up) to 3D chunk grid coordinate
    FIntVector WorldToChunkCoord(FVector WorldPos) const;

    // Convert chunk grid coordinate to world-space origin (bottom-left-floor corner)
    FVector ChunkCoordToWorldOrigin(FIntVector Coord) const;

    // Deterministic seed from coordinate — same coord always returns same seed
    int32 ChunkSeedFromCoord(FIntVector Coord) const;
};
