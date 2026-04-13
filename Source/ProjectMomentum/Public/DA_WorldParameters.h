#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "DA_WorldParameters.generated.h"

// One entry in the special structure registry.
// Add new structure types here — no code changes needed elsewhere.
USTRUCT(BlueprintType)
struct FStructureEntry
{
    GENERATED_BODY()

    // Soft class pointer — loads only when the structure is actually picked
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftClassPtr<AActor> StructureClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1", ClampMax="10.0"))
    float SpawnWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bGrantsPoints = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="bGrantsPoints"))
    int32 PointValue = 100;
};

UCLASS(BlueprintType)
class PROJECTMOMENTUM_API UDA_WorldParameters : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // ── STREET SHAPE ─────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Street Shape")
    float CorridorWidthMin  = 700.f;   // X axis span, cm

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Street Shape")
    float CorridorWidthMax  = 2200.f;

    // Height is Z-axis. Min 1400 = 14m ceiling above floor
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Street Shape")
    float CorridorHeightMin = 1400.f;  // Z axis height, cm

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Street Shape")
    float CorridorHeightMax = 4000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Street Shape")
    float ChunkLength = 10000.f;      // Y axis, cm (100m per chunk)

    // ── FACADES ──────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Facades")
    float LedgeSpacingMin  = 350.f;   // cm, along Z axis on wall

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Facades")
    float LedgeSpacingMax  = 700.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Facades")
    float BuildingDepthMin = 400.f;   // cm, along X axis (into void)

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Facades")
    float BuildingDepthMax = 2800.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Facades", meta=(ClampMin="0.0",ClampMax="1.0"))
    float ProtrusionChance = 0.35f;

    // ── BRIDGES ──────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bridges")
    int32 BridgeCountMin = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bridges")
    int32 BridgeCountMax = 6;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bridges")
    float BridgeSpanMax  = 1600.f;  // max X span of a bridge

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bridges")
    float PierSpacing    = 600.f;   // cm between pier columns along X

    // ── SPECIAL STRUCTURES ───────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special Structures", meta=(ClampMin="0.0",ClampMax="1.0"))
    float SpecialStructureChance = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Special Structures")
    TArray<FStructureEntry> SpecialStructureTypes;

    // ── DETAILS ──────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Details")
    float DetailSpawnRadius = 1500.f;

    // ── MESH KITS ─────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh Kits")
    TArray<TSoftObjectPtr<UStaticMesh>> WallMeshKit;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh Kits")
    TArray<TSoftObjectPtr<UStaticMesh>> GreebleMeshKit;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh Kits")
    TSoftObjectPtr<UStaticMesh> FloorPlateMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh Kits")
    TSoftObjectPtr<UStaticMesh> CeilingPlateMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh Kits")
    TSoftObjectPtr<UStaticMesh> LedgePieceMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh Kits")
    TSoftObjectPtr<UStaticMesh> PierColumnMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh Kits")
    TSoftObjectPtr<UStaticMesh> BridgeDeckMesh;
};