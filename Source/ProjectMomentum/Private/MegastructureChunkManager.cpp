#include "MegastructureChunkManager.h"
#include "Async/Async.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AMegastructureChunkManager::AMegastructureChunkManager()
{
    // Tick is enabled but we throttle it to 20hz in BeginPlay
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup    = TG_PostUpdateWork;
}

void AMegastructureChunkManager::BeginPlay()
{
    Super::BeginPlay();

    // Throttle tick — checking 20x/sec is plenty for a streaming manager
    SetActorTickInterval(0.05f);

    // Resolve the soft-reference to a hard pointer synchronously at startup.
    // This is the only synchronous load in the system — DA is tiny (<1KB).
    CachedParams = WorldParametersAsset.LoadSynchronous();

    if (!CachedParams)
    {
        UE_LOG(LogTemp, Error,
            TEXT("AMegastructureChunkManager: WorldParametersAsset is not set or failed to load! Streaming will not work."));
        return;
    }

    if (!ChunkClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("AMegastructureChunkManager: ChunkClass is not set! Assign BP_MegaChunk in the editor."));
        return;
    }

    WarmPool();

    // Immediately load the chunk the player starts in so there is no
    // blank frame at game start
    APawn* Player = GetWorld()->GetFirstPlayerController()
                        ? GetWorld()->GetFirstPlayerController()->GetPawn()
                        : nullptr;
    if (Player)
    {
        FIntVector StartCoord = WorldToChunkCoord(Player->GetActorLocation());
        RequestChunk(StartCoord);
    }
}

void AMegastructureChunkManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!CachedParams) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;
    APawn* Player = PC->GetPawn();
    if (!Player) return;

    const FVector Pos       = Player->GetActorLocation();
    const FVector Velocity  = Player->GetVelocity();

    // Project forward along current velocity to predict future position (Y + Z)
    // A swinging player moves fast — 2.5s lookahead catches the next 2-3 chunks
    const FVector Predicted = Pos + Velocity * VelocityLookAheadSeconds;

    const FIntVector CurrCoord  = WorldToChunkCoord(Pos);
    const FIntVector AheadCoord = WorldToChunkCoord(Predicted);

    // Walk N evenly-spaced steps from current to predicted position.
    // Each step maps to a chunk coord — request any we don't have yet.
    for (int32 i = 0; i <= ChunksAheadToLoad; i++)
    {
        const float t = (ChunksAheadToLoad > 0)
                      ? (float)i / (float)ChunksAheadToLoad
                      : 0.f;

        const FIntVector Target(
            FMath::RoundToInt(FMath::Lerp((float)CurrCoord.X, (float)AheadCoord.X, t)),
            FMath::RoundToInt(FMath::Lerp((float)CurrCoord.Y, (float)AheadCoord.Y, t)),
            FMath::RoundToInt(FMath::Lerp((float)CurrCoord.Z, (float)AheadCoord.Z, t))
        );

        if (!ActiveChunks.Contains(Target) && !PendingChunks.Contains(Target))
        {
            RequestChunk(Target);
        }
    }

    // Cull chunks that are now far enough away.
    // Copy keys first — can't remove from TMap while iterating it.
    TArray<FIntVector> ToRemove;
    ToRemove.Reserve(ActiveChunks.Num());

    for (auto& KV : ActiveChunks)
    {
        if (ShouldCullChunk(KV.Value, Pos))
            ToRemove.Add(KV.Key);
    }
    for (const FIntVector& Key : ToRemove)
    {
        ReturnChunkToPool(ActiveChunks[Key]);
        ActiveChunks.Remove(Key);
    }
}

void AMegastructureChunkManager::WarmPool()
{
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.bNoFail = true;

    // Far behind and below the world — invisible, not affecting any overlap queries
    const FVector  LimboPos(0.f, -999999.f, -999999.f);
    const FRotator LimboRot = FRotator::ZeroRotator;

    ChunkPool.Reserve(PoolSize);

    for (int32 i = 0; i < PoolSize; i++)
    {
        AMegachunk* C = GetWorld()->SpawnActor<AMegachunk>(
            ChunkClass, LimboPos, LimboRot, Params);

        if (ensure(C))
        {
            C->SetActorHiddenInGame(true);
            C->bInUse        = false;
            // Pass the DA reference immediately — no BP loop needed at BeginPlay
            C->WorldParamsRef = CachedParams;
            ChunkPool.Add(C);
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("MegaChunk pool warmed: %d actors ready"), ChunkPool.Num());
}

AMegachunk* AMegastructureChunkManager::GetChunkFromPool()
{
    for (AMegachunk* C : ChunkPool)
    {
        if (C && !C->bInUse)
        {
            C->bInUse = true;
            return C;
        }
    }
    // Pool exhausted — this is a configuration error, not a runtime error.
    // Raise PoolSize or reduce ChunksAheadToLoad.
    UE_LOG(LogTemp, Warning,
        TEXT("AMegastructureChunkManager: Pool exhausted! Raise PoolSize (currently %d)."),
        PoolSize);
    return nullptr;
}

void AMegastructureChunkManager::ReturnChunkToPool(AMegachunk* Chunk)
{
    if (!Chunk) return;
    // ResetForPool clears all HISM instances, destroys any spawned structures,
    // and moves the actor to limbo. It is a BlueprintNativeEvent so the
    // BP_MegaChunk override runs first, then calls Super.
    Chunk->ResetForPool();
}

void AMegastructureChunkManager::RequestChunk(FIntVector Coord)
{
    PendingChunks.Add(Coord);

    AMegachunk* Chunk = GetChunkFromPool();
    if (!Chunk)
    {
        PendingChunks.Remove(Coord); // cancel reservation
        return;
    }

    // Pre-compute the deterministic values for this coord
    // These are pure math — safe to compute here on the game thread
    const int32   Seed   = ChunkSeedFromCoord(Coord);
    const FVector Origin = ChunkCoordToWorldOrigin(Coord);

    // Capture everything by value — no raw pointers to UObjects in async lambdas
    // (UObjects can be GC'd while the lambda is queued).
    // Chunk* is safe here because pool actors are never GC'd while the pool owns them.
    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
    [this, Chunk, Coord, Seed, Origin]()
    {
        // --- BACKGROUND THREAD ---
        // PrepareDataAsync does only plain C++ math (no UObjects).
        // It pre-rolls any values that can be computed without the engine.
        Chunk->PrepareDataAsync(Seed, Origin);

        // Return to game thread to do anything that touches UObjects / engine
        AsyncTask(ENamedThreads::GameThread,
        [this, Chunk, Coord]()
        {
            // --- GAME THREAD ---
            // Guard against the chunk having been reclaimed while async was in-flight
            // (e.g. player teleported away and cull ran before this completed)
            if (!Chunk->bInUse)
            {
                PendingChunks.Remove(Coord);
                return;
            }

            // Finalize triggers PCG generation, places special structures etc.
            Chunk->FinalizeOnGameThread();

            ActiveChunks.Add(Coord, Chunk);
            PendingChunks.Remove(Coord);
        });
    });
}

// ── Helpers ──────────────────────────────────────────────────────────────────

FIntVector AMegastructureChunkManager::WorldToChunkCoord(FVector WorldPos) const
{
    // UE5 is Z-up.
    // Y = primary travel / street axis.
    // X = corridor width axis.
    // Z = vertical layer axis.
    // All three produce independent chunk indices.
    const float L = CachedParams->ChunkLength;
    return FIntVector(
        FMath::FloorToInt(WorldPos.X / L),
        FMath::FloorToInt(WorldPos.Y / L),
        FMath::FloorToInt(WorldPos.Z / L)
    );
}

FVector AMegastructureChunkManager::ChunkCoordToWorldOrigin(FIntVector Coord) const
{
    // Origin is the world position of the chunk's (0,0,0) local corner —
    // i.e. the bottom-left-floor point of the chunk volume.
    const float L = CachedParams->ChunkLength;
    return FVector(
        (float)Coord.X * L,
        (float)Coord.Y * L,
        (float)Coord.Z * L   // Z = vertical layer, never set to 0 unless layer 0
    );
}

bool AMegastructureChunkManager::ShouldCullChunk(AMegachunk* Chunk, FVector PlayerPos) const
{
    const float L    = CachedParams->ChunkLength;
    const float Dist = FVector::Dist(Chunk->ChunkOrigin, PlayerPos);

    // Cull radius = (behind + ahead + 2 safety buffer) chunk lengths.
    // The +2 prevents popping when the player hovers near a chunk boundary.
    const float CullRadius = (float)(ChunksBehindToKeep + ChunksAheadToLoad + 2) * L;
    return Dist > CullRadius;
}

int32 AMegastructureChunkManager::ChunkSeedFromCoord(FIntVector C) const
{
    // Knuth multiplicative hash with XOR mixing across all three axes.
    // Produces well-distributed seeds — different in X, Y, and Z layers.
    uint32 H = (uint32)C.X * 2654435761u
              ^ (uint32)C.Y * 805459861u
              ^ (uint32)C.Z * 1234567891u;
    // Mask to positive int32 range — Blueprint Random Stream requires positive seed
    return (int32)(H & 0x7FFFFFFF);
}