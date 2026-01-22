#include "PCG/PCGSetPointBoundsFromMesh.h"

#include "Engine/StaticMesh.h"

#include "PCGContext.h"
#include "PCGData.h"
#include "PCGPointData.h"

#define LOCTEXT_NAMESPACE "PCGSetPointBoundsFromMesh"

FName UPCGSetPointBoundsFromMeshSettings::GetMainOutputLabel() const
{
	static const FName OutLabel = TEXT("Out");
	return OutLabel;
}

TArray<FPCGPinProperties> UPCGSetPointBoundsFromMeshSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> Pins;
	Pins.Emplace(PCGPinConstants::DefaultInputLabel, EPCGDataType::Point);
	return Pins;
}

TArray<FPCGPinProperties> UPCGSetPointBoundsFromMeshSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> Pins;
	Pins.Emplace(GetMainOutputLabel(), EPCGDataType::Point);
	return Pins;
}

FPCGElementPtr UPCGSetPointBoundsFromMeshSettings::CreateElement() const
{
	return MakeShared<FPCGSetPointBoundsFromMeshElement>();
}

bool FPCGSetPointBoundsFromMeshElement::ExecuteInternal(FPCGContext* Context) const
{
	check(Context);

	const UPCGSetPointBoundsFromMeshSettings* Settings = Context->GetInputSettings<UPCGSetPointBoundsFromMeshSettings>();
	if (!Settings || !Settings->ReferenceMesh)
	{
		// No reference mesh => passthrough.
		Context->OutputData = Context->InputData;
		return true;
	}

	const FBoxSphereBounds MeshBounds = Settings->ReferenceMesh->GetBounds();
	const FVector ExtentsUU = MeshBounds.BoxExtent; // half size in UU

	const TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);
	for (const FPCGTaggedData& InTagged : Inputs)
	{
		const UPCGPointData* InPointData = Cast<UPCGPointData>(InTagged.Data);
		if (!InPointData)
		{
			continue;
		}

		UPCGPointData* OutPointData = NewObject<UPCGPointData>();
		OutPointData->InitializeFromData(InPointData);

		TArray<FPCGPoint>& OutPoints = OutPointData->GetMutablePoints();
		OutPoints = InPointData->GetPoints();

		for (FPCGPoint& Pt : OutPoints)
		{
			if (Settings->bWriteBounds)
			{
				// Makes point dimensions match the reference mesh (Width/Length/Height).
				Pt.BoundsMin = -ExtentsUU;
				Pt.BoundsMax = ExtentsUU;
			}

			if (Settings->bForceUnitScale)
			{
				Pt.Transform.SetScale3D(FVector::OneVector);
			}
		}

		FPCGTaggedData& OutTagged = Context->OutputData.TaggedData.Emplace_GetRef();
		OutTagged.Data = OutPointData;
		OutTagged.Tags = InTagged.Tags;
		OutTagged.Pin = Settings->GetMainOutputLabel();
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
