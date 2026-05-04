#include "MegaChunk.h"
#include "Components/SphereComponent.h"
#include "PCGComponent.h"
#include "DA_WorldParameters.h"

AMegachunk::AMegachunk()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>("Root");
    SetRootComponent(Root);

    auto MakeHISM = [this](const FName& InName) -> UHierarchicalInstancedStaticMeshComponent*
    {
        auto* C = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(InName);
        C->SetupAttachment(GetRootComponent());
        C->NumCustomDataFloats = 0;
        C->bUseAsOccluder = true;
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
    HISM_Details->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HISM_Details->SetGenerateOverlapEvents(false);

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
    ProximityTrigger = CreateDefaultSubobject<USphereComponent>("ProximityTrigger");
    ProximityTrigger->SetupAttachment(GetRootComponent());
    ProximityTrigger->SetSphereRadius(1500.f);
    ProximityTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProximityTrigger->SetCollisionObjectType(ECC_WorldDynamic);
    ProximityTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    ProximityTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    ProximityTrigger->SetGenerateOverlapEvents(true);
}

void AMegachunk::PrepareDataAsync(int32 InSeed, FVector InOrigin)
{

    ChunkSeed   = InSeed;
    ChunkOrigin = InOrigin;
}

void AMegachunk::FinalizeOnGameThread_Implementation()
{

}

void AMegachunk::ResetForPool_Implementation()
{
    HISM_Floor  ->ClearInstances();
    HISM_Ceiling->ClearInstances();
    HISM_Walls  ->ClearInstances();
    HISM_Ledges ->ClearInstances();
    HISM_Piers  ->ClearInstances();
    HISM_Bridges->ClearInstances();
    HISM_Details->ClearInstances();

    WebAnchorPoints.Reset();
    bInUse = false;
    
    SetActorLocation(FVector(0.f, 0.f, -9999999.f));
    SetActorHiddenInGame(true);
}