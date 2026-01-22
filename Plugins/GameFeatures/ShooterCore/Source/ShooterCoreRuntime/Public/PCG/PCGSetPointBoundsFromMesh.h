#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGSetPointBoundsFromMesh.generated.h"

class UStaticMesh;

/**
 * PCG node that overwrites incoming point bounds using the bounds of a reference Static Mesh.
 *
 * Input:  Point Data
 * Params: ReferenceMesh
 * Output: Point Data (same points, with BoundsMin/BoundsMax set to ReferenceMesh dimensions)
 */
UCLASS(BlueprintType, ClassGroup=(PCG))
class SHOOTERCORERUNTIME_API UPCGSetPointBoundsFromMeshSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	// UPCGSettings
	virtual FName GetMainOutputLabel() const override;
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;

	/** Mesh used as the exact size reference (Width/Length/Height) in Unreal Units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PCG|Bounds")
	TObjectPtr<UStaticMesh> ReferenceMesh = nullptr;

	/** If true, overwrites BoundsMin/BoundsMax on every point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PCG|Bounds")
	bool bWriteBounds = true;

	/** If true, forces point Transform scale to 1,1,1 (useful for grid-tiling workflows). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PCG|Bounds")
	bool bForceUnitScale = true;
};

class FPCGSetPointBoundsFromMeshElement : public FSimplePCGElement
{
public:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
