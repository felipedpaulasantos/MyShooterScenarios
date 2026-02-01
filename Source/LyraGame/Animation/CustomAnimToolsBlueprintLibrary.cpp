// Copyright (c) 2026

#include "Animation/CustomAnimToolsBlueprintLibrary.h"

#include "Math/UnrealMathUtility.h"

float UCustomAnimToolsBlueprintLibrary::ComputeAnimPlayRateFromMovementSpeed(
	float MovementSpeed,
	float ReferenceSpeedForMinRate,
	float MaxSpeedForMaxRate,
	float MinPlayRate,
	float MaxPlayRate)
{
	return UpdateAnimRateOnMovementSpeed(MovementSpeed, ReferenceSpeedForMinRate, MaxSpeedForMaxRate, MinPlayRate, MaxPlayRate);
}

float UCustomAnimToolsBlueprintLibrary::NormalizeMovementSpeedForAnimRate(
	float MovementSpeed,
	float ReferenceSpeedForMinRate,
	float MaxSpeedForMaxRate)
{
	// Thread-safe / AnimGraph-safe: purely math, no UObject access.

	if (!FMath::IsFinite(MovementSpeed))
	{
		MovementSpeed = 0.f;
	}

	MovementSpeed = FMath::Max(0.f, MovementSpeed);

	if (!FMath::IsFinite(ReferenceSpeedForMinRate) || ReferenceSpeedForMinRate < 0.f)
	{
		ReferenceSpeedForMinRate = 0.f;
	}

	if (!FMath::IsFinite(MaxSpeedForMaxRate) || MaxSpeedForMaxRate < 0.f)
	{
		MaxSpeedForMaxRate = 0.f;
	}

	if (MaxSpeedForMaxRate < ReferenceSpeedForMinRate)
	{
		Swap(MaxSpeedForMaxRate, ReferenceSpeedForMinRate);
	}

	// Degenerate case: no interpolation range.
	if (FMath::IsNearlyEqual(MaxSpeedForMaxRate, ReferenceSpeedForMinRate))
	{
		return (MovementSpeed >= MaxSpeedForMaxRate) ? 1.f : 0.f;
	}

	if (MovementSpeed <= ReferenceSpeedForMinRate)
	{
		return 0.f;
	}

	if (MovementSpeed >= MaxSpeedForMaxRate)
	{
		return 1.f;
	}

	const float Alpha = (MovementSpeed - ReferenceSpeedForMinRate) / (MaxSpeedForMaxRate - ReferenceSpeedForMinRate);
	return FMath::Clamp(Alpha, 0.f, 1.f);
}

float UCustomAnimToolsBlueprintLibrary::UpdateAnimRateOnMovementSpeed(
	float MovementSpeed,
	float ReferenceSpeedForMinRate,
	float MaxSpeedForMaxRate,
	float MinPlayRate,
	float MaxPlayRate)
{
	// Thread-safe / AnimGraph-safe: purely math, no UObject access.

	if (!FMath::IsFinite(MovementSpeed))
	{
		MovementSpeed = 0.f;
	}

	MovementSpeed = FMath::Max(0.f, MovementSpeed);

	if (!FMath::IsFinite(ReferenceSpeedForMinRate) || ReferenceSpeedForMinRate < 0.f)
	{
		ReferenceSpeedForMinRate = 0.f;
	}

	if (!FMath::IsFinite(MaxSpeedForMaxRate) || MaxSpeedForMaxRate < 0.f)
	{
		MaxSpeedForMaxRate = 0.f;
	}

	if (!FMath::IsFinite(MinPlayRate))
	{
		MinPlayRate = 0.f;
	}

	if (!FMath::IsFinite(MaxPlayRate))
	{
		MaxPlayRate = MinPlayRate;
	}

	// Ensure ordering to avoid inverted ranges.
	if (MaxPlayRate < MinPlayRate)
	{
		Swap(MaxPlayRate, MinPlayRate);
	}

	if (MaxSpeedForMaxRate < ReferenceSpeedForMinRate)
	{
		Swap(MaxSpeedForMaxRate, ReferenceSpeedForMinRate);
	}

	// Degenerate case: no interpolation range.
	if (FMath::IsNearlyEqual(MaxSpeedForMaxRate, ReferenceSpeedForMinRate))
	{
		return (MovementSpeed >= MaxSpeedForMaxRate) ? MaxPlayRate : MinPlayRate;
	}

	if (MovementSpeed <= ReferenceSpeedForMinRate)
	{
		return MinPlayRate;
	}

	if (MovementSpeed >= MaxSpeedForMaxRate)
	{
		return MaxPlayRate;
	}

	const float Alpha = (MovementSpeed - ReferenceSpeedForMinRate) / (MaxSpeedForMaxRate - ReferenceSpeedForMinRate);
	return FMath::Lerp(MinPlayRate, MaxPlayRate, FMath::Clamp(Alpha, 0.f, 1.f));
}
