#include "MegastructureChunkManager.h"
#include "Async/Async.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AMegastructureChunkManager::AMegastructureChunkManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup    = TG_PostUpdateWork;
}

void AMegastructureChunkManager::BeginPlay()
{
    Super::BeginPlay();

    SetActorTickInterval(0.05f);
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

    const FVector Predicted = Pos + Velocity * VelocityLookAheadSeconds;

    const FIntVector CurrCoord  = WorldToChunkCoord(Pos);
    const FIntVector AheadCoord = WorldToChunkCoord(Predicted);

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
    UE_LOG(LogTemp, Warning,
        TEXT("AMegastructureChunkManager: Pool exhausted! Raise PoolSize (currently %d)."),
        PoolSize);
    return nullptr;
}

void AMegastructureChunkManager::ReturnChunkToPool(AMegachunk* Chunk)
{
    if (!Chunk) return;
    Chunk->ResetForPool();
}

void AMegastructureChunkManager::RequestChunk(FIntVector Coord)
{
    PendingChunks.Add(Coord);

    AMegachunk* Chunk = GetChunkFromPool();
    if (!Chunk)
    {
        PendingChunks.Remove(Coord);
        return;
    }

    const int32   Seed   = ChunkSeedFromCoord(Coord);
    const FVector Origin = ChunkCoordToWorldOrigin(Coord);

    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
    [this, Chunk, Coord, Seed, Origin]()
    {

        Chunk->PrepareDataAsync(Seed, Origin);

        AsyncTask(ENamedThreads::GameThread,
        [this, Chunk, Coord]()
        {

            if (!Chunk->bInUse)
            {
                PendingChunks.Remove(Coord);
                return;
            }

            Chunk->FinalizeOnGameThread();

            ActiveChunks.Add(Coord, Chunk);
            PendingChunks.Remove(Coord);
        });
    });
}
FIntVector AMegastructureChunkManager::WorldToChunkCoord(FVector WorldPos) const
{
    const float L = CachedParams->ChunkLength;
    return FIntVector(
        FMath::FloorToInt(WorldPos.X / L),
        FMath::FloorToInt(WorldPos.Y / L),
        FMath::FloorToInt(WorldPos.Z / L)
    );
}

FVector AMegastructureChunkManager::ChunkCoordToWorldOrigin(FIntVector Coord) const
{
    const float L = CachedParams->ChunkLength;
    return FVector(
        (float)Coord.X * L,
        (float)Coord.Y * L,
        (float)Coord.Z * L
    );
}

bool AMegastructureChunkManager::ShouldCullChunk(AMegachunk* Chunk, FVector PlayerPos) const
{
    const float L    = CachedParams->ChunkLength;
    const float Dist = FVector::Dist(Chunk->ChunkOrigin, PlayerPos);
    const float CullRadius = (float)(ChunksBehindToKeep + ChunksAheadToLoad + 2) * L;
    return Dist > CullRadius;
}

int32 AMegastructureChunkManager::ChunkSeedFromCoord(FIntVector C) const
{

    uint32 H = (uint32)C.X * 2654435761u
              ^ (uint32)C.Y * 805459861u
              ^ (uint32)C.Z * 1234567891u;
    return (int32)(H & 0x7FFFFFFF);
}