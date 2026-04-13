#include "MegaChunk.h"
#include "Components/SphereComponent.h"
#include "PCGComponent.h"
#include "DA_WorldParameters.h"

AMegachunk::AMegachunk()
{
    PrimaryActorTick.bCanEverTick = false; // Chunk never ticks

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>("Root");
    SetRootComponent(Root);

    // ── HISM factory ─────────────────────────────────────────────────────────────
    // Each HISM gets a ComponentTag matching its name so PCG_DetailScatter
    // can filter surfaces by tag ("HISM_Walls", "HISM_Ceiling") when sampling.
    auto MakeHISM = [this](const FName& InName) -> UHierarchicalInstancedStaticMeshComponent*
    {
        auto* C = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(InName);
        C->SetupAttachment(GetRootComponent());
        C->NumCustomDataFloats = 0;
        C->bUseAsOccluder = true;
        // Tag matches the variable name — used by PCG surface sampler to filter components
        C->ComponentTags.Add(InName);
        return C;
    };

    HISM_Floor   = MakeHISM("HISM_Floor");
    HISM_Ceiling = MakeHISM("HISM_Ceiling");
    HISM_Walls   = MakeHISM("HISM_Walls");
    HISM_Ledges  = MakeHISM("HISM_Ledges");
    HISM_Piers   = MakeHISM("HISM_Piers");
    HISM_Bridges = MakeHISM("HISM_Bridges");
    HISM_Details = MakeHISM("HISM_Details");
    // Details have no collision — decorative mesh only
    HISM_Details->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HISM_Details->SetGenerateOverlapEvents(false);

    // ── PCG components ───────────────────────────────────────────────────────────
    auto MakePCG = [this](const char* Name) -> UPCGComponent*
    {
        auto* C = CreateDefaultSubobject<UPCGComponent>(Name);
        C->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
        return C;
    };

    PCG_Street  = MakePCG("PCG_Street");
    PCG_Facades = MakePCG("PCG_Facades");
    PCG_Bridges = MakePCG("PCG_Bridges");
    PCG_Details = MakePCG("PCG_Details");

    // ── Proximity trigger for detail scatter ─────────────────────────────────────
    // Created here in C++ constructor — NOT dynamically at runtime via AddComponentByClass.
    // Blueprint configures its radius and binds overlap events.
    // Default radius 1500cm; FinalizeOnGameThread overrides from WorldParamsRef.
    ProximityTrigger = CreateDefaultSubobject<USphereComponent>("ProximityTrigger");
    ProximityTrigger->SetupAttachment(GetRootComponent());
    ProximityTrigger->SetSphereRadius(1500.f);
    ProximityTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProximityTrigger->SetCollisionObjectType(ECC_WorldDynamic);
    ProximityTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    ProximityTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    ProximityTrigger->SetGenerateOverlapEvents(true);
}

// PrepareDataAsync — runs on background thread.
// Base implementation stores seed + origin. All PCG/mesh work happens in
// FinalizeOnGameThread_Implementation (or its Blueprint override).
// Override this in a C++ subclass only if you have pure-math pre-computation to add.
void AMegachunk::PrepareDataAsync(int32 InSeed, FVector InOrigin)
{
    // Safe: ChunkSeed and ChunkOrigin are plain value types, not UObjects.
    // Writing them here is safe because the game-thread callback that reads them
    // only runs after this function returns.
    ChunkSeed   = InSeed;
    ChunkOrigin = InOrigin;
}

void AMegachunk::FinalizeOnGameThread_Implementation()
{
    // Base C++ implementation is empty — BP_MegaChunk overrides this entirely.
    // If BP_MegaChunk calls "Add call to parent function" via the Super node,
    // this is what runs. Safe to leave empty.
}

void AMegachunk::ResetForPool_Implementation()
{
    // Clear all HISM instances — fast bulk-clear, no per-instance deallocation
    HISM_Floor  ->ClearInstances();
    HISM_Ceiling->ClearInstances();
    HISM_Walls  ->ClearInstances();
    HISM_Ledges ->ClearInstances();
    HISM_Piers  ->ClearInstances();
    HISM_Bridges->ClearInstances();
    HISM_Details->ClearInstances();

    WebAnchorPoints.Reset();
    bInUse = false;

    // Move to limbo — must match WarmPool's spawn position.
    // Using large negative Z to guarantee it never overlaps with any game content
    // regardless of how large the world is vertically.
    SetActorLocation(FVector(0.f, 0.f, -9999999.f));
    SetActorHiddenInGame(true);
}