#include "SoulMathUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"

// ==================== Original MyCharacter Functions (Extracted) ====================

float USoulMathUtils::CalculateAngleToTarget(AActor* PlayerActor, AActor* Target)
{
	if (!IsValid(PlayerActor) || !IsValid(Target))
		return 180.0f;

	APlayerController* Controller = GetPlayerControllerFromActor(PlayerActor);
	if (!Controller)
		return 180.0f;

	FVector PlayerLocation = PlayerActor->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector CurrentCameraForward = Controller->GetControlRotation().Vector();
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

	// Calculate angle difference
	float DotProduct = FVector::DotProduct(CurrentCameraForward, ToTarget);
	DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	
	float AngleRadians = FMath::Acos(DotProduct);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	return AngleDegrees;
}

float USoulMathUtils::CalculateDirectionAngle(AActor* PlayerActor, AActor* Target)
{
	if (!IsValid(PlayerActor) || !IsValid(Target))
		return 0.0f;

	APlayerController* Controller = GetPlayerControllerFromActor(PlayerActor);
	if (!Controller)
		return 0.0f;

	FVector PlayerLocation = PlayerActor->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FRotator ControlRotation = Controller->GetControlRotation();
	FVector CurrentCameraForward = ControlRotation.Vector();
	FVector CurrentCameraRight = ControlRotation.RotateVector(FVector::RightVector);

	// Calculate to target vector
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

	// Calculate relative angle to camera front
	float ForwardDot = FVector::DotProduct(CurrentCameraForward, ToTarget);
	float RightDot = FVector::DotProduct(CurrentCameraRight, ToTarget);

	// Use atan2 to calculate angle, range is -180 to 180
	float AngleRadians = FMath::Atan2(RightDot, ForwardDot);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	return AngleDegrees;
}

bool USoulMathUtils::IsTargetInSectorLockZone(AActor* PlayerActor, AActor* Target, const FLockOnSettings& Settings)
{
	if (!IsValid(PlayerActor) || !IsValid(Target))
		return false;

	APlayerController* Controller = GetPlayerControllerFromActor(PlayerActor);
	if (!Controller)
		return false;

	FVector PlayerLocation = PlayerActor->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector CameraForward = Controller->GetControlRotation().Vector();
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

	// Calculate angle
	float DotProduct = FVector::DotProduct(CameraForward, ToTarget);
	DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	
	float AngleRadians = FMath::Acos(DotProduct);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	// Check if within sector lock zone
	bool bInSectorZone = AngleDegrees <= (Settings.SectorLockAngle * 0.5f);

	return bInSectorZone;
}

bool USoulMathUtils::IsTargetInEdgeDetectionZone(AActor* PlayerActor, AActor* Target, const FLockOnSettings& Settings)
{
	if (!IsValid(PlayerActor) || !IsValid(Target))
		return false;

	APlayerController* Controller = GetPlayerControllerFromActor(PlayerActor);
	if (!Controller)
		return false;

	FVector PlayerLocation = PlayerActor->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector CameraForward = Controller->GetControlRotation().Vector();
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

	// Calculate angle
	float DotProduct = FVector::DotProduct(CameraForward, ToTarget);
	DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	
	float AngleRadians = FMath::Acos(DotProduct);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	// Check if within edge detection zone
	bool bInEdgeZone = (AngleDegrees > (Settings.SectorLockAngle * 0.5f)) && 
					   (AngleDegrees <= (Settings.EdgeDetectionAngle * 0.5f));

	return bInEdgeZone;
}

void USoulMathUtils::SortCandidatesByDirection(AActor* PlayerActor, TArray<AActor*>& Targets)
{
	if (Targets.Num() <= 1 || !IsValid(PlayerActor))
		return;

	// Remove invalid pointers
	Targets.RemoveAll([](AActor* Actor) { return !IsValid(Actor); });

	if (Targets.Num() <= 1)
		return;

	// Use Lambda expression for sorting, from left to right (angle from small to large)
	Targets.Sort([PlayerActor](const AActor& A, const AActor& B) {
		if (!IsValid(&A) || !IsValid(&B)) 
			return false;
		
		float AngleA = CalculateDirectionAngle(PlayerActor, const_cast<AActor*>(&A));
		float AngleB = CalculateDirectionAngle(PlayerActor, const_cast<AActor*>(&B));
		return AngleA < AngleB;
	});
}

FVector2D USoulMathUtils::ProjectSocketToScreen(const FVector& WorldLocation, APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProjectSocketToScreen: No PlayerController"));
		return FVector2D::ZeroVector;
	}

	// Project world location to screen coordinates
	FVector2D ScreenLocation;
	bool bProjected = PlayerController->ProjectWorldLocationToScreen(WorldLocation, ScreenLocation);
	
	if (bProjected)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Projection successful: World(%.1f, %.1f, %.1f) -> Screen(%.1f, %.1f)"), 
			WorldLocation.X, WorldLocation.Y, WorldLocation.Z,
			ScreenLocation.X, ScreenLocation.Y);
		return ScreenLocation;
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("Projection failed for world location (%.1f, %.1f, %.1f)"), 
			WorldLocation.X, WorldLocation.Y, WorldLocation.Z);
		return FVector2D::ZeroVector;
	}
}

// ==================== New Advanced Camera Functions ====================

EEnemySizeCategory USoulMathUtils::ClassifyEnemySize(AActor* EnemyActor, const FAdvancedCameraSettings& Settings)
{
	if (!IsValid(EnemyActor))
		return EEnemySizeCategory::Unknown;

	float BoundingHeight = GetActorBoundingHeight(EnemyActor);

	if (BoundingHeight <= Settings.SmallEnemySizeThreshold)
	{
		return EEnemySizeCategory::Small;
	}
	else if (BoundingHeight >= Settings.LargeEnemySizeThreshold)
	{
		return EEnemySizeCategory::Large;
	}
	else
	{
		return EEnemySizeCategory::Medium;
	}
}

FVector USoulMathUtils::CalculateCameraHeightAdjustment(float Distance, EEnemySizeCategory EnemySize, 
	const FCameraSettings& CameraSettings, const FAdvancedCameraSettings& AdvancedSettings)
{
	// Base offset from camera settings
	FVector BaseOffset = CameraSettings.TargetLocationOffset;
	
	// If advanced settings disabled, return base offset
	if (!AdvancedSettings.bEnableEnemySizeAdaptation)
		return BaseOffset;

	// Get size-specific offset
	FVector SizeOffset;
	switch (EnemySize)
	{
		case EEnemySizeCategory::Small:
			SizeOffset = AdvancedSettings.SmallEnemyHeightOffset;
			break;
		case EEnemySizeCategory::Medium:
			SizeOffset = AdvancedSettings.MediumEnemyHeightOffset;
			break;
		case EEnemySizeCategory::Large:
			SizeOffset = AdvancedSettings.LargeEnemyHeightOffset;
			break;
		default:
			SizeOffset = AdvancedSettings.MediumEnemyHeightOffset; // Default to medium
			break;
	}

	// Apply distance-based adjustment if enabled
	if (AdvancedSettings.bEnableDistanceAdaptiveCamera)
	{
		float DistanceMultiplier = GetDistanceBasedCameraSpeedMultiplier(Distance, AdvancedSettings);
		
		// Adjust height offset based on distance (closer = lower camera, farther = higher camera)
		if (Distance <= AdvancedSettings.CloseRangeThreshold)
		{
			SizeOffset.Z *= 0.8f; // Lower camera for close range
		}
		else if (Distance >= AdvancedSettings.FarRangeThreshold)
		{
			SizeOffset.Z *= 1.2f; // Higher camera for far range
		}
	}

	return SizeOffset;
}

float USoulMathUtils::CalculateTerrainHeightInfluence(const FVector& PlayerLocation, const FVector& TargetLocation, 
	UWorld* World, const FAdvancedCameraSettings& Settings)
{
	if (!World || !Settings.bEnableTerrainHeightCompensation)
		return 0.0f;

	// Calculate height difference between player and target
	float HeightDifference = TargetLocation.Z - PlayerLocation.Z;
	float AbsHeightDiff = FMath::Abs(HeightDifference);

	// Only apply compensation if height difference is significant
	if (AbsHeightDiff < 50.0f) // Minimum threshold
		return 0.0f;

	// Clamp to maximum allowed difference
	float ClampedHeightDiff = FMath::Clamp(AbsHeightDiff, 0.0f, Settings.MaxTerrainHeightDifference);
	
	// Calculate compensation factor
	float CompensationRatio = ClampedHeightDiff / Settings.MaxTerrainHeightDifference;
	float HeightInfluence = CompensationRatio * Settings.TerrainCompensationFactor;

	// Apply sign based on height difference direction
	return HeightDifference > 0.0f ? HeightInfluence : -HeightInfluence;
}

FVector USoulMathUtils::CalculateOptimalCameraPosition(const FVector& PlayerLocation, const FVector& TargetLocation,
	EEnemySizeCategory EnemySize, const FCameraSettings& CameraSettings, 
	const FAdvancedCameraSettings& AdvancedSettings, UWorld* World)
{
	// Start with target location
	FVector OptimalPosition = TargetLocation;

	// Calculate distance
	float Distance = FVector::Dist(PlayerLocation, TargetLocation);

	// Apply size-based height adjustment
	FVector HeightAdjustment = CalculateCameraHeightAdjustment(Distance, EnemySize, CameraSettings, AdvancedSettings);
	OptimalPosition += HeightAdjustment;

	// Apply terrain height influence
	float TerrainInfluence = CalculateTerrainHeightInfluence(PlayerLocation, TargetLocation, World, AdvancedSettings);
	OptimalPosition.Z += TerrainInfluence * 100.0f; // Scale influence to reasonable offset

	// Log calculation for debugging
	UE_LOG(LogTemp, Verbose, TEXT("Optimal Camera Position: Base=(%s), HeightAdj=(%s), TerrainInf=%.1f, Final=(%s)"),
		*TargetLocation.ToString(), *HeightAdjustment.ToString(), TerrainInfluence, *OptimalPosition.ToString());

	return OptimalPosition;
}

bool USoulMathUtils::ShouldUseDirectTargetSwitching(AActor* PlayerActor, AActor* CurrentTarget, AActor* NewTarget,
	const FAdvancedCameraSettings& Settings)
{
	if (!IsValid(PlayerActor) || !IsValid(CurrentTarget) || !IsValid(NewTarget))
		return false;

	FVector PlayerLocation = PlayerActor->GetActorLocation();
	FVector CurrentTargetLocation = CurrentTarget->GetActorLocation();
	FVector NewTargetLocation = NewTarget->GetActorLocation();

	// Calculate angle between current and new target
	float AngleToNew = CalculateAngleToTarget(PlayerActor, NewTarget);
	float DistanceToNew = FVector::Dist(PlayerLocation, NewTargetLocation);

	// Use direct switching if:
	// 1. Angle is small enough
	// 2. Distance is close enough
	bool bAngleAllowsDirect = AngleToNew <= Settings.DirectSwitchAngleThreshold;
	bool bDistanceAllowsDirect = DistanceToNew <= Settings.DirectSwitchDistanceThreshold;

	return bAngleAllowsDirect && bDistanceAllowsDirect;
}

// ==================== Helper Functions ====================

FVector USoulMathUtils::GetBestSocketLocation(AActor* TargetActor, const FSocketProjectionSettings& SocketSettings)
{
	if (!IsValid(TargetActor))
		return FVector::ZeroVector;

	// Try to get SkeletalMeshComponent from target
	USkeletalMeshComponent* SkeletalMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!SkeletalMesh)
	{
		// Fallback to actor location if no skeletal mesh
		return TargetActor->GetActorLocation() + SocketSettings.SocketOffset;
	}

	// Try primary socket first
	if (SkeletalMesh->DoesSocketExist(SocketSettings.TargetSocketName))
	{
		FVector SocketLocation = SkeletalMesh->GetSocketLocation(SocketSettings.TargetSocketName);
		return SocketLocation + SocketSettings.SocketOffset;
	}

	// Try alternative sockets if enabled
	if (SocketSettings.bEnableSocketFallback)
	{
		for (const FName& SocketName : SocketSettings.SocketSearchNames)
		{
			if (SkeletalMesh->DoesSocketExist(SocketName))
			{
				FVector SocketLocation = SkeletalMesh->GetSocketLocation(SocketName);
				UE_LOG(LogTemp, Log, TEXT("Using fallback socket '%s' for target '%s'"), 
					*SocketName.ToString(), *TargetActor->GetName());
				return SocketLocation + SocketSettings.SocketOffset;
			}
		}
	}

	// Final fallback to actor location
	UE_LOG(LogTemp, Warning, TEXT("No valid sockets found for target '%s', using actor location"), 
		*TargetActor->GetName());
	return TargetActor->GetActorLocation() + SocketSettings.SocketOffset;
}

FName USoulMathUtils::FindFirstAvailableSocket(AActor* TargetActor, const TArray<FName>& SocketNames)
{
	if (!IsValid(TargetActor) || SocketNames.Num() == 0)
		return NAME_None;

	USkeletalMeshComponent* SkeletalMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!SkeletalMesh)
		return NAME_None;

	for (const FName& SocketName : SocketNames)
	{
		if (SkeletalMesh->DoesSocketExist(SocketName))
		{
			return SocketName;
		}
	}

	return NAME_None;
}

float USoulMathUtils::GetDistanceBasedCameraSpeedMultiplier(float Distance, const FAdvancedCameraSettings& Settings)
{
	if (!Settings.bEnableDistanceAdaptiveCamera)
		return 1.0f;

	if (Distance <= Settings.CloseRangeThreshold)
	{
		return Settings.CloseRangeCameraSpeedMultiplier;
	}
	else if (Distance <= Settings.MediumRangeThreshold)
	{
		// Interpolate between close and medium
		float Alpha = (Distance - Settings.CloseRangeThreshold) / 
			(Settings.MediumRangeThreshold - Settings.CloseRangeThreshold);
		return FMath::Lerp(Settings.CloseRangeCameraSpeedMultiplier, Settings.MediumRangeCameraSpeedMultiplier, Alpha);
	}
	else if (Distance <= Settings.FarRangeThreshold)
	{
		// Interpolate between medium and far
		float Alpha = (Distance - Settings.MediumRangeThreshold) / 
			(Settings.FarRangeThreshold - Settings.MediumRangeThreshold);
		return FMath::Lerp(Settings.MediumRangeCameraSpeedMultiplier, Settings.FarRangeCameraSpeedMultiplier, Alpha);
	}
	else
	{
		return Settings.FarRangeCameraSpeedMultiplier;
	}
}

// ==================== Internal Helper Functions ====================

APlayerController* USoulMathUtils::GetPlayerControllerFromActor(AActor* PlayerActor)
{
	if (!IsValid(PlayerActor))
		return nullptr;

	if (APawn* Pawn = Cast<APawn>(PlayerActor))
	{
		return Cast<APlayerController>(Pawn->GetController());
	}

	return nullptr;
}

FVector USoulMathUtils::GetCameraForwardVector(AActor* PlayerActor)
{
	APlayerController* Controller = GetPlayerControllerFromActor(PlayerActor);
	if (Controller)
	{
		return Controller->GetControlRotation().Vector();
	}

	// Fallback to actor forward vector
	return PlayerActor ? PlayerActor->GetActorForwardVector() : FVector::ForwardVector;
}

float USoulMathUtils::GetActorBoundingHeight(AActor* Actor)
{
	if (!IsValid(Actor))
		return 0.0f;

	// Get actor's bounding box
	FVector Origin, BoxExtent;
	Actor->GetActorBounds(false, Origin, BoxExtent);

	// Return height (Z component of extent * 2)
	return BoxExtent.Z * 2.0f;
}