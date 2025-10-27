#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "LockOnConfig.h"
#include "SoulMathUtils.generated.h"

/**
 * Math utilities for the Soul lock-on system
 * Extracted from MyCharacter for better organization and reusability
 */
UCLASS(BlueprintType)
class SOUL_API USoulMathUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ==================== Original MyCharacter Functions (Extracted) ====================

	/**
	 * Calculate the angle between player's camera direction and target
	 * @param PlayerActor The player character
	 * @param Target The target actor
	 * @return Angle in degrees (0-180)
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Lock-On")
	static float CalculateAngleToTarget(AActor* PlayerActor, AActor* Target);

	/**
	 * Calculate the directional angle of target relative to player's camera
	 * @param PlayerActor The player character
	 * @param Target The target actor
	 * @return Angle in degrees (-180 to 180, positive = right, negative = left)
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Lock-On")
	static float CalculateDirectionAngle(AActor* PlayerActor, AActor* Target);

	/**
	 * Check if target is within the sector lock zone
	 * @param PlayerActor The player character
	 * @param Target The target actor
	 * @param Settings Lock-on configuration settings
	 * @return True if target is in sector zone
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Lock-On")
	static bool IsTargetInSectorLockZone(AActor* PlayerActor, AActor* Target, const FLockOnSettings& Settings);

	/**
	 * Check if target is within the edge detection zone
	 * @param PlayerActor The player character
	 * @param Target The target actor
	 * @param Settings Lock-on configuration settings
	 * @return True if target is in edge detection zone
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Lock-On")
	static bool IsTargetInEdgeDetectionZone(AActor* PlayerActor, AActor* Target, const FLockOnSettings& Settings);

	/**
	 * Sort an array of targets by their directional angle (left to right)
	 * @param PlayerActor The player character
	 * @param Targets Array of target actors to sort
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Lock-On")
	static void SortCandidatesByDirection(AActor* PlayerActor, UPARAM(ref) TArray<AActor*>& Targets);

	/**
	 * Project a world location to screen coordinates
	 * @param WorldLocation The world position to project
	 * @param PlayerController The player controller for projection
	 * @return Screen coordinates (Vector2D)
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Projection")
	static FVector2D ProjectSocketToScreen(const FVector& WorldLocation, APlayerController* PlayerController);

	// ==================== New Advanced Camera Functions ====================

	/**
	 * Classify enemy size based on bounding box
	 * @param EnemyActor The enemy actor to classify
	 * @param Settings Advanced camera settings for thresholds
	 * @return Enemy size category
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Advanced Camera")
	static EEnemySizeCategory ClassifyEnemySize(AActor* EnemyActor, const FAdvancedCameraSettings& Settings);

	/**
	 * Calculate camera height adjustment based on distance and enemy size
	 * @param Distance Distance to target
	 * @param EnemySize Enemy size category
	 * @param CameraSettings Camera configuration
	 * @param AdvancedSettings Advanced camera settings
	 * @return Height adjustment vector
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Advanced Camera")
	static FVector CalculateCameraHeightAdjustment(float Distance, EEnemySizeCategory EnemySize, 
		const FCameraSettings& CameraSettings, const FAdvancedCameraSettings& AdvancedSettings);

	/**
	 * Calculate terrain height influence on camera positioning
	 * @param PlayerLocation Player's world location
	 * @param TargetLocation Target's world location
	 * @param World World context for raycasting
	 * @param Settings Advanced camera settings
	 * @return Height adjustment factor
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Advanced Camera")
	static float CalculateTerrainHeightInfluence(const FVector& PlayerLocation, const FVector& TargetLocation, 
		UWorld* World, const FAdvancedCameraSettings& Settings);

	/**
	 * Calculate optimal camera position with all factors considered
	 * @param PlayerLocation Player's world location
	 * @param TargetLocation Target's world location
	 * @param EnemySize Enemy size category
	 * @param CameraSettings Base camera settings
	 * @param AdvancedSettings Advanced camera settings
	 * @param World World context for calculations
	 * @return Optimal camera target location
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Advanced Camera")
	static FVector CalculateOptimalCameraPosition(const FVector& PlayerLocation, const FVector& TargetLocation,
		EEnemySizeCategory EnemySize, const FCameraSettings& CameraSettings, 
		const FAdvancedCameraSettings& AdvancedSettings, UWorld* World);

	/**
	 * Determine if direct target switching should be used (vs smooth switching)
	 * @param PlayerActor The player character
	 * @param CurrentTarget Current lock-on target
	 * @param NewTarget New potential target
	 * @param Settings Advanced camera settings
	 * @return True if direct switching should be used
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Advanced Camera")
	static bool ShouldUseDirectTargetSwitching(AActor* PlayerActor, AActor* CurrentTarget, AActor* NewTarget,
		const FAdvancedCameraSettings& Settings);

	// ==================== Helper Functions ====================

	/**
	 * Get the best socket location from a target actor
	 * @param TargetActor The target actor
	 * @param SocketSettings Socket projection settings
	 * @return World location of the best available socket
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Socket")
	static FVector GetBestSocketLocation(AActor* TargetActor, const FSocketProjectionSettings& SocketSettings);

	/**
	 * Check if an actor has any of the specified sockets
	 * @param TargetActor The target actor
	 * @param SocketNames Array of socket names to check
	 * @return Name of the first found socket, or None if not found
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Socket")
	static FName FindFirstAvailableSocket(AActor* TargetActor, const TArray<FName>& SocketNames);

	/**
	 * Get distance-based camera speed multiplier
	 * @param Distance Distance to target
	 * @param Settings Advanced camera settings
	 * @return Speed multiplier
	 */
	UFUNCTION(BlueprintCallable, Category = "Soul Math Utils|Advanced Camera")
	static float GetDistanceBasedCameraSpeedMultiplier(float Distance, const FAdvancedCameraSettings& Settings);

private:
	// ==================== Internal Helper Functions ====================

	/**
	 * Get player controller from actor
	 */
	static APlayerController* GetPlayerControllerFromActor(AActor* PlayerActor);

	/**
	 * Get camera forward vector from player
	 */
	static FVector GetCameraForwardVector(AActor* PlayerActor);

	/**
	 * Calculate actor bounding box height
	 */
	static float GetActorBoundingHeight(AActor* Actor);
};