#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "LockOnConfig.h"
#include "UIManagerComponent.generated.h"

// Forward declarations
class AActor;
class UUserWidget;
class UWidgetComponent;
class ACharacter;
class APlayerController;

/** Target body part enumeration */
UENUM(BlueprintType)
enum class ETargetBodyPart : uint8
{
	Center      UMETA(DisplayName = "Center"),
	Head        UMETA(DisplayName = "Head"),
	Chest       UMETA(DisplayName = "Chest"),
	Feet        UMETA(DisplayName = "Feet"),
	Auto        UMETA(DisplayName = "Auto Select")
};

/** Multi-part configuration */
USTRUCT(BlueprintType)
struct FMultiPartConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multi-Part")
	ETargetBodyPart TargetPart;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multi-Part")
	TMap<ETargetBodyPart, FVector> PartOffsets;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multi-Part")
	bool bEnableSmartSelection;
	
	FMultiPartConfig()
	{
		TargetPart = ETargetBodyPart::Auto;
		bEnableSmartSelection = true;
		PartOffsets.Add(ETargetBodyPart::Head, FVector(0, 0, 80.0f));
		PartOffsets.Add(ETargetBodyPart::Chest, FVector(0, 0, 0));
		PartOffsets.Add(ETargetBodyPart::Feet, FVector(0, 0, -80.0f));
	}
};

/**
 * Hybrid projection mode enumeration
 */
UENUM(BlueprintType)
enum class EProjectionMode : uint8
{
	ActorCenter     UMETA(DisplayName = "Actor Center"),
	BoundsCenter    UMETA(DisplayName = "Bounds Center"),
	CustomOffset    UMETA(DisplayName = "Custom Offset"),
	Hybrid          UMETA(DisplayName = "Hybrid Mode")
};

/**
 * Hybrid projection settings
 */
USTRUCT(BlueprintType)
struct FHybridProjectionSettings
{
	GENERATED_BODY()
	
	/** Projection mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	EProjectionMode ProjectionMode;
	
	/** Custom offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	FVector CustomOffset;
	
	/** Bounds offset ratio */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BoundsOffsetRatio;
	
	/** Enable size adaptive */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	bool bEnableSizeAdaptive;
	
	FHybridProjectionSettings()
	{
		ProjectionMode = EProjectionMode::Hybrid;
		CustomOffset = FVector(0, 0, 50.0f);
		BoundsOffsetRatio = 0.6f;
		bEnableSizeAdaptive = true;
	}
};

/**
 * UI display mode enumeration for lock-on system
 */
UENUM(BlueprintType)
enum class EUIDisplayMode : uint8
{
	Traditional3D		UMETA(DisplayName = "Traditional 3D World Space"),
	SocketProjection	UMETA(DisplayName = "Socket Projection to Screen"),
	ScreenSpace			UMETA(DisplayName = "Screen Space Overlay"),
	SizeAdaptive		UMETA(DisplayName = "Size Adaptive Mode")
};

/**
 * Event delegates for UI state changes
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockOnWidgetShown, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLockOnWidgetHidden);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSocketProjectionUpdated, AActor*, Target, FVector2D, ScreenPosition);

/**
 * UI Manager Component for Lock-On Target System
 * 
 * This component manages all UI-related functionality for the lock-on system,
 * including widget display, socket projection, and screen space management.
 * Designed to be modular and reusable across different character types.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SOUL_API UUIManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Default constructor
	 */
	UUIManagerComponent();

protected:
	/**
	 * Called when the game starts
	 */
	virtual void BeginPlay() override;

public:
	/**
	 * Called every frame
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== Core Public Interface ====================

	/**
	 * Show lock-on widget for specified target
	 * @param Target - The actor to show UI for
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Manager")
	void ShowLockOnWidget(AActor* Target);

	/**
	 * Hide the current lock-on widget
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Manager")
	void HideLockOnWidget();

	/**
	 * Hide all lock-on widgets from all targets
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Manager")
	void HideAllLockOnWidgets();

	/**
	 * Update lock-on widget state when target changes
	 * @param CurrentTarget - The new target to focus on
	 * @param PreviousTarget - The previous target to clean up
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Manager")
	void UpdateLockOnWidget(AActor* CurrentTarget, AActor* PreviousTarget);

	// ==================== Socket Projection Interface ====================

	/**
	 * Get target projection location with hybrid mode support
	 * @param Target - The target actor
	 * @return World location based on selected projection mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Hybrid Projection")
	FVector GetTargetProjectionLocation(AActor* Target) const;

	/**
	 * Check if target has a valid socket
	 * @param Target - The target actor
	 * @return True if target has the specified socket
	 */
	UFUNCTION(BlueprintCallable, Category = "Socket Projection")
	bool HasValidSocket(AActor* Target) const;

	/**
	 * Project socket world location to screen coordinates
	 * @param SocketWorldLocation - World location to project
	 * @return Screen coordinates or zero vector if projection failed
	 */
	UFUNCTION(BlueprintCallable, Category = "Socket Projection")
	FVector2D ProjectSocketToScreen(const FVector& SocketWorldLocation) const;

	/**
	 * Project world location to screen with fallback mechanism
	 * @param WorldLocation - World location to project
	 * @return Screen coordinates or zero vector if projection failed completely
	 */
	UFUNCTION(BlueprintCallable, Category = "Socket Projection")
	FVector2D ProjectToScreen(const FVector& WorldLocation) const;

	/**
	 * Show socket projection widget for current target
	 */
	UFUNCTION(BlueprintCallable, Category = "Socket Projection")
	void ShowSocketProjectionWidget(AActor* Target);

	/**
	 * Update projection widget position with fallback mechanism
	 */
	UFUNCTION(BlueprintCallable, Category = "Socket Projection")
	void UpdateProjectionWidget(AActor* Target);

	/**
	 * Update socket projection widget position (backward compatibility wrapper)
	 */
	UFUNCTION(BlueprintCallable, Category = "Socket Projection")
	void UpdateSocketProjectionWidget(AActor* Target);

	/**
	 * Hide socket projection widget
	 */
	UFUNCTION(BlueprintCallable, Category = "Socket Projection")
	void HideSocketProjectionWidget();

	// ==================== Size Adaptive UI Interface ====================

	/**
	 * Show size adaptive widget for specified target with size-based adjustments
	 * @param Target - The actor to show UI for
	 * @param SizeCategory - The size category of the target
	 */
	UFUNCTION(BlueprintCallable, Category = "Size Adaptive UI")
	void ShowSizeAdaptiveWidget(AActor* Target, EEnemySizeCategory SizeCategory);

	/**
	 * Update widget for enemy size-specific properties
	 * @param Target - The target actor
	 * @param SizeCategory - The size category to adapt to
	 */
	UFUNCTION(BlueprintCallable, Category = "Size Adaptive UI")
	void UpdateWidgetForEnemySize(AActor* Target, EEnemySizeCategory SizeCategory);

	/**
	 * Get UI scale factor based on enemy size
	 * @param SizeCategory - The enemy size category
	 * @return Scale factor for UI elements
	 */
	UFUNCTION(BlueprintPure, Category = "Size Adaptive UI")
	float GetUIScaleForEnemySize(EEnemySizeCategory SizeCategory) const;

	/**
	 * Get UI color based on enemy size
	 * @param SizeCategory - The enemy size category
	 * @return Color for UI elements
	 */
	UFUNCTION(BlueprintPure, Category = "Size Adaptive UI")
	FLinearColor GetUIColorForEnemySize(EEnemySizeCategory SizeCategory) const;

	// ==================== Debug Interface ====================

	/**
	 * Debug widget setup and configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugWidgetSetup();

	/**
	 * Log current widget state for debugging
	 * @param Context - Context string for the log
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void LogWidgetState(const FString& Context);

	/**
	 * Validate widget class configuration
	 * @return True if widget class is properly configured
	 */
	UFUNCTION(BlueprintPure, Category = "Debug")
	bool ValidateWidgetClass() const;

	// ==================== Configuration Accessors ====================

	/**
	 * Get current UI display mode
	 */
	UFUNCTION(BlueprintPure, Category = "UI Manager")
	EUIDisplayMode GetUIDisplayMode() const { return CurrentUIDisplayMode; }

	/**
	 * Set UI display mode
	 * @param NewMode - The new display mode to use
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Manager")
	void SetUIDisplayMode(EUIDisplayMode NewMode);

	/**
	 * Get hybrid projection settings
	 */
	UFUNCTION(BlueprintPure, Category = "UI Manager")
	const FHybridProjectionSettings& GetHybridProjectionSettings() const { return HybridProjectionSettings; }

	/**
	 * Set hybrid projection settings
	 * @param NewSettings - The new hybrid projection settings
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Manager")
	void SetHybridProjectionSettings(const FHybridProjectionSettings& NewSettings);

	/**
	 * Get advanced camera settings
	 */
	UFUNCTION(BlueprintPure, Category = "UI Manager")
	const FAdvancedCameraSettings& GetAdvancedCameraSettings() const { return AdvancedCameraSettings; }

	/**
	 * Set advanced camera settings
	 * @param NewSettings - The new advanced camera settings
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Manager")
	void SetAdvancedCameraSettings(const FAdvancedCameraSettings& NewSettings);

	/**
	 * Switch target body part
	 * @param NewPart - The new body part to target
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Manager")
	void SwitchTargetPart(ETargetBodyPart NewPart);
	
	/**
	 * Get best target body part based on automatic selection
	 * @param Target - The target actor
	 * @return The optimal body part to target
	 */
	UFUNCTION(BlueprintPure, Category = "UI Manager")
	ETargetBodyPart GetBestTargetPart(AActor* Target) const;

	// ==================== Event Delegates ====================

	/** Event fired when lock-on widget is shown */
	UPROPERTY(BlueprintAssignable, Category = "UI Manager Events")
	FOnLockOnWidgetShown OnLockOnWidgetShown;

	/** Event fired when lock-on widget is hidden */
	UPROPERTY(BlueprintAssignable, Category = "UI Manager Events")
	FOnLockOnWidgetHidden OnLockOnWidgetHidden;

	/** Event fired when socket projection is updated */
	UPROPERTY(BlueprintAssignable, Category = "UI Manager Events")
	FOnSocketProjectionUpdated OnSocketProjectionUpdated;

	// ==================== Configuration Properties ====================

	/** Widget class to use for lock-on UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Configuration")
	TSubclassOf<UUserWidget> LockOnWidgetClass;

	/** Current UI display mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Configuration")
	EUIDisplayMode CurrentUIDisplayMode;

	/** Hybrid projection configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Configuration")
	FHybridProjectionSettings HybridProjectionSettings;

	/** Advanced camera settings for size adaptation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Configuration")
	FAdvancedCameraSettings AdvancedCameraSettings;

	/** Multi-part configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Configuration")
	FMultiPartConfig MultiPartConfig;

	// ==================== Size Adaptive Configuration ====================

	/** UI scale factors for different enemy sizes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Size Adaptive UI", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float SmallEnemyUIScale = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Size Adaptive UI", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float MediumEnemyUIScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Size Adaptive UI", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float LargeEnemyUIScale = 1.2f;

	/** UI colors for different enemy sizes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Size Adaptive UI")
	FLinearColor SmallEnemyUIColor = FLinearColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Size Adaptive UI")
	FLinearColor MediumEnemyUIColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Size Adaptive UI")
	FLinearColor LargeEnemyUIColor = FLinearColor::Red;

	/** Enable size-based UI adaptations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Size Adaptive UI")
	bool bEnableSizeAdaptiveUI = true;

	/** Enable debug logging for UI operations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableUIDebugLogs;

	/** Enable debug logging for size analysis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableSizeAnalysisDebugLogs = false;

protected:
	// ==================== Internal State Management ====================

	// === 新增：防止重复更新的状态跟踪（步骤1）===
	AActor* LastUIUpdateTarget = nullptr;
	float LastUIUpdateTime = 0.0f;
	static constexpr float UI_UPDATE_INTERVAL = 0.033f; // 30 FPS

	/** Current lock-on widget instance */
	UPROPERTY()
	UUserWidget* LockOnWidgetInstance;

	/** Previous target for state cleanup */
	UPROPERTY()
	AActor* PreviousLockOnTarget;

	/** Current target being displayed */
	UPROPERTY()
	AActor* CurrentLockOnTarget;

	/** List of targets with active widgets */
	UPROPERTY()
	TArray<AActor*> TargetsWithActiveWidgets;

	/** Cache of widget components found on targets */
	UPROPERTY()
	TMap<AActor*, UWidgetComponent*> WidgetComponentCache;

	/** Cache of enemy size categories for performance optimization */
	UPROPERTY()
	TMap<AActor*, EEnemySizeCategory> EnemySizeCache;

	/** Current UI scale being applied */
	float CurrentUIScale = 1.0f;

	/** Current UI color being applied */
	FLinearColor CurrentUIColor = FLinearColor::White;

	// ==================== Component References ====================

	/** Cached reference to owner character */
	UPROPERTY()
	ACharacter* OwnerCharacter;

	/** Cached reference to player controller */
	UPROPERTY()
	APlayerController* OwnerController;

	// ==================== Internal Helper Functions ====================

	/**
	 * Initialize UI manager component
	 */
	void InitializeUIManager();

	/**
	 * Cleanup UI resources
	 */
	void CleanupUIResources();

	/**
	 * Get player controller reference
	 */
	APlayerController* GetPlayerController() const;

	/**
	 * Validate target for UI display
	 */
	bool IsValidTargetForUI(AActor* Target) const;

	/**
	 * Clear widget component cache
	 */
	void ClearWidgetComponentCache();

	/**
	 * Update owner references (Character and Controller)
	 */
	void UpdateOwnerReferences();

	// ==================== Size Analysis Helper Functions ====================

	/**
	 * Try to find widget class at runtime if not set
	 * @return True if widget class was found and set
	 */
	bool TryFindWidgetClassAtRuntime();

	/**
	 * Calculate target bounding box size for size classification
	 * @param Target - The target actor
	 * @return Size value for classification
	 */
	float CalculateTargetBoundingBoxSize(AActor* Target) const;

	/**
	 * Analyze target size and return appropriate category
	 * @param Target - The target actor
	 * @return Size category of the target
	 */
	EEnemySizeCategory AnalyzeTargetSize(AActor* Target) const;

	/**
	 * Update cached size category for target
	 * @param Target - The target actor
	 */
	void UpdateTargetSizeCategory(AActor* Target);

	/**
	 * Clean up invalid entries from size cache
	 */
	void CleanupSizeCache();
};