#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "LockOnConfig.generated.h"

/**
 * Enemy size categories for adaptive camera system
 */
UENUM(BlueprintType)
enum class EEnemySizeCategory : uint8
{
	Small		UMETA(DisplayName = "Small Enemy"),
	Medium		UMETA(DisplayName = "Medium Enemy"),
	Large		UMETA(DisplayName = "Large Enemy"),
	Giant		UMETA(DisplayName = "Giant Enemy"),
	Unknown		UMETA(DisplayName = "Unknown Size")
};

/**
 * Enemy camera behavior types
 */
UENUM(BlueprintType)
enum class EEnemyCameraType : uint8
{
	Small		UMETA(DisplayName = "Small Enemy"),
	Medium		UMETA(DisplayName = "Medium Enemy"),
	Large		UMETA(DisplayName = "Large Enemy"),
	Giant		UMETA(DisplayName = "Giant Enemy"),
	Boss		UMETA(DisplayName = "Boss"),
	Unknown		UMETA(DisplayName = "Unknown")
};

/**
 * Camera type configuration for different enemy sizes
 */
USTRUCT(BlueprintType)
struct SOUL_API FCameraTypeConfig
{
	GENERATED_BODY()

	FCameraTypeConfig()
	{
		CameraDistance = 400.0f;
		HeightOffset = 0.0f;
		PitchOffset = -5.0f;
		ZBiasNear = -150.0f;
		ZBiasFar = -250.0f;
		FOVAdjustment = 0.0f;
		TrackingSpeed = 5.0f;
		SwitchSpeed = 8.0f;
	}

	/** Distance from target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (ClampMin = "200.0", ClampMax = "2000.0"))
	float CameraDistance;

	/** Height offset from target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (ClampMin = "-500.0", ClampMax = "500.0"))
	float HeightOffset;

	/** Pitch angle offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (ClampMin = "-45.0", ClampMax = "45.0"))
	float PitchOffset;

	/** Z-axis bias for near distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
	float ZBiasNear;

	/** Z-axis bias for far distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config")
	float ZBiasFar;

	/** Field of view adjustment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (ClampMin = "-20.0", ClampMax = "20.0"))
	float FOVAdjustment;

	/** Camera tracking speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float TrackingSpeed;

	/** Target switch speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float SwitchSpeed;
};

/**
 * Lock-on system configuration settings
 */
USTRUCT(BlueprintType)
struct SOUL_API FLockOnSettings
{
	GENERATED_BODY()

	FLockOnSettings()
	{
		// Default values extracted from MyCharacter.h constants
		LockOnRange = 2000.0f;
		LockOnAngle = 120.0f;
		SectorLockAngle = 60.0f;
		EdgeDetectionAngle = 90.0f;
		ExtendedLockRangeMultiplier = 1.2f;
		TargetSwitchAngleThreshold = 45.0f;
		LockCompletionThreshold = 3.0f;
		TargetSwitchCooldown = 0.5f;
		TargetSearchInterval = 0.1f;
		ThumbstickThreshold = 0.7f;
		RaycastHeightOffset = 50.0f;
	}

	/** Maximum distance for lock-on detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock-On", meta = (ClampMin = "100.0", ClampMax = "5000.0"))
	float LockOnRange;

	/** Total lock-on angle range (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock-On", meta = (ClampMin = "30.0", ClampMax = "180.0"))
	float LockOnAngle;

	/** Sector lock angle for immediate targeting (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock-On", meta = (ClampMin = "15.0", ClampMax = "90.0"))
	float SectorLockAngle;

	/** Edge detection angle for camera correction (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock-On", meta = (ClampMin = "45.0", ClampMax = "120.0"))
	float EdgeDetectionAngle;

	/** Multiplier for extended lock range to maintain lock */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock-On", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float ExtendedLockRangeMultiplier;

	/** Angle threshold for smooth target switching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock-On", meta = (ClampMin = "10.0", ClampMax = "90.0"))
	float TargetSwitchAngleThreshold;

	/** Threshold for lock completion detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock-On", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float LockCompletionThreshold;

	/** Cooldown time between target switches */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock-On", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float TargetSwitchCooldown;

	/** Interval for searching targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock-On", meta = (ClampMin = "0.05", ClampMax = "0.5"))
	float TargetSearchInterval;

	/** Thumbstick threshold for target switching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ThumbstickThreshold;

	/** Height offset for raycast detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection", meta = (ClampMin = "0.0", ClampMax = "200.0"))
	float RaycastHeightOffset;
};

/**
 * Camera system configuration settings
 */
USTRUCT(BlueprintType)
struct SOUL_API FCameraSettings
{
	GENERATED_BODY()

	FCameraSettings()
	{
		// Default values extracted from MyCharacter.h constants
		CameraInterpSpeed = 5.0f;
		CharacterRotationSpeed = 3.0f;
		TargetSwitchSmoothSpeed = 8.0f;
		CameraAutoCorrectionSpeed = 4.0f;
		CameraResetSpeed = 6.0f;
		CameraResetAngleThreshold = 2.0f;
		CameraCorrectionOffset = 30.0f;
		bEnableSmoothCameraTracking = true;
		CameraTrackingMode = 0;
		TargetLocationOffset = FVector(0.0f, 0.0f, -250.0f);
	}

	/** Speed of camera interpolation when following target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float CameraInterpSpeed;

	/** Speed of character rotation towards target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float CharacterRotationSpeed;

	/** Speed for smooth target switching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float TargetSwitchSmoothSpeed;

	/** Speed for automatic camera correction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0", ClampMax = "15.0"))
	float CameraAutoCorrectionSpeed;

	/** Speed for camera reset animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0", ClampMax = "15.0"))
	float CameraResetSpeed;

	/** Angle threshold for camera reset completion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float CameraResetAngleThreshold;

	/** Offset angle for camera correction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "10.0", ClampMax = "60.0"))
	float CameraCorrectionOffset;

	/** Enable smooth camera tracking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bEnableSmoothCameraTracking;

	/** Camera tracking mode (0=Full, 1=Horizontal Only, 2=Custom) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", ClampMax = "2"))
	int32 CameraTrackingMode;

	/** Offset applied to target location for camera calculations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector TargetLocationOffset;
};

/**
 * Advanced camera system settings for distance-adaptive behavior
 */
USTRUCT(BlueprintType)
struct SOUL_API FAdvancedCameraSettings
{
	GENERATED_BODY()

	FAdvancedCameraSettings()
	{
		// Default adaptive camera settings
		bEnableDistanceAdaptiveCamera = true;
		bEnableTerrainHeightCompensation = true;
		bEnableEnemySizeAdaptation = true;
		
		// Distance thresholds
		CloseRangeThreshold = 500.0f;
		MediumRangeThreshold = 1000.0f;
		FarRangeThreshold = 1500.0f;
		
		// Camera adjustment factors
		CloseRangeCameraSpeedMultiplier = 1.5f;
		MediumRangeCameraSpeedMultiplier = 1.0f;
		FarRangeCameraSpeedMultiplier = 0.7f;
		
		// Enemy size adjustments
		SmallEnemyHeightOffset = FVector(0.0f, 0.0f, -100.0f);
		MediumEnemyHeightOffset = FVector(0.0f, 0.0f, -200.0f);
		LargeEnemyHeightOffset = FVector(0.0f, 0.0f, -350.0f);
		
		// Terrain compensation
		MaxTerrainHeightDifference = 300.0f;
		TerrainCompensationFactor = 0.6f;
		
		// Size classification thresholds
		SmallEnemySizeThreshold = 150.0f;
		LargeEnemySizeThreshold = 400.0f;
		
		// Direct switching thresholds
		DirectSwitchAngleThreshold = 25.0f;
		DirectSwitchDistanceThreshold = 800.0f;
	}

	/** Enable distance-adaptive camera behavior */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Camera")
	bool bEnableDistanceAdaptiveCamera;

	/** Enable terrain height compensation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Camera")
	bool bEnableTerrainHeightCompensation;

	/** Enable enemy size-based camera adaptation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Camera")
	bool bEnableEnemySizeAdaptation;

	// Distance thresholds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Adaptation", meta = (ClampMin = "100.0", ClampMax = "1000.0"))
	float CloseRangeThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Adaptation", meta = (ClampMin = "500.0", ClampMax = "1500.0"))
	float MediumRangeThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Adaptation", meta = (ClampMin = "1000.0", ClampMax = "2500.0"))
	float FarRangeThreshold;

	// Camera speed multipliers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Adaptation", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float CloseRangeCameraSpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Adaptation", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float MediumRangeCameraSpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Adaptation", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float FarRangeCameraSpeedMultiplier;

	// Enemy size-based height offsets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Size Adaptation")
	FVector SmallEnemyHeightOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Size Adaptation")
	FVector MediumEnemyHeightOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Size Adaptation")
	FVector LargeEnemyHeightOffset;

	// Terrain compensation settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Compensation", meta = (ClampMin = "100.0", ClampMax = "1000.0"))
	float MaxTerrainHeightDifference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Compensation", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TerrainCompensationFactor;

	// Size classification thresholds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Size Classification", meta = (ClampMin = "50.0", ClampMax = "300.0"))
	float SmallEnemySizeThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Size Classification", meta = (ClampMin = "200.0", ClampMax = "800.0"))
	float LargeEnemySizeThreshold;

	// Direct switching settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Direct Switching", meta = (ClampMin = "10.0", ClampMax = "45.0"))
	float DirectSwitchAngleThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Direct Switching", meta = (ClampMin = "300.0", ClampMax = "1500.0"))
	float DirectSwitchDistanceThreshold;
};

/**
 * Socket projection system configuration
 */
USTRUCT(BlueprintType)
struct SOUL_API FSocketProjectionSettings
{
	GENERATED_BODY()

	FSocketProjectionSettings()
	{
		// Default values extracted from MyCharacter.h
		bUseSocketProjection = true;
		TargetSocketName = TEXT("Spine2Socket");
		SocketOffset = FVector(0.0f, 0.0f, 50.0f);
		ProjectionScale = 1.0f;
		bEnableSocketFallback = true;
		SocketSearchNames = { TEXT("Spine2Socket"), TEXT("SpineSocket"), TEXT("HeadSocket"), TEXT("ChestSocket") };
	}

	/** Enable socket-based projection system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket Projection")
	bool bUseSocketProjection;

	/** Primary socket name to target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket Projection")
	FName TargetSocketName;

	/** Offset applied to socket location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket Projection")
	FVector SocketOffset;

	/** Scale factor for projection calculations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket Projection", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ProjectionScale;

	/** Enable fallback to actor location if socket not found */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket Projection")
	bool bEnableSocketFallback;

	/** List of socket names to search for (in priority order) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket Projection")
	TArray<FName> SocketSearchNames;

};