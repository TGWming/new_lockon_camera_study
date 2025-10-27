#include "UIManagerComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/StructOnScope.h"
#include "UObject/UObjectIterator.h"

// Sets default values for this component's properties
UUIManagerComponent::UUIManagerComponent()
{
	// Set this component to be ticked every frame
	PrimaryComponentTick.bCanEverTick = true;

	// ==================== Initialize Default Values ====================
	
	// UI Configuration
	LockOnWidgetClass = nullptr;
	CurrentUIDisplayMode = EUIDisplayMode::SocketProjection;
	bEnableUIDebugLogs = false;
	bEnableSizeAnalysisDebugLogs = false;

	// State Management
	LockOnWidgetInstance = nullptr;
	PreviousLockOnTarget = nullptr;
	CurrentLockOnTarget = nullptr;
	CurrentUIScale = 1.0f;
	CurrentUIColor = FLinearColor::White;

	// Initialize socket projection settings with defaults from LockOnConfig
	HybridProjectionSettings = FHybridProjectionSettings();
	AdvancedCameraSettings = FAdvancedCameraSettings();

	// Initialize arrays
	TargetsWithActiveWidgets.Empty();
	WidgetComponentCache.Empty();
	EnemySizeCache.Empty();

	// Component references
	OwnerCharacter = nullptr;
	OwnerController = nullptr;
}

// Called when the game starts
void UUIManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize the UI manager
	InitializeUIManager();
}

// Called every frame
void UUIManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update owner references if needed
	UpdateOwnerReferences();

	// === 步骤1修复：添加更新频率限制 ===
	if (CurrentUIDisplayMode == EUIDisplayMode::SocketProjection && CurrentLockOnTarget)
	{
		// 安全宪法：防御性检查
		if (!IsValid(CurrentLockOnTarget))
		{
			return;
		}
		
		float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		
		// 条件1：目标改变
		bool bTargetChanged = (LastUIUpdateTarget != CurrentLockOnTarget);
		
		// 条件2：超过更新间隔
		bool bIntervalPassed = (CurrentTime - LastUIUpdateTime) >= UI_UPDATE_INTERVAL;
		
		// 只在必要时更新
		if (bTargetChanged || bIntervalPassed)
		{
			UpdateProjectionWidget(CurrentLockOnTarget);
			LastUIUpdateTarget = CurrentLockOnTarget;
			LastUIUpdateTime = CurrentTime;
		}
	}
}

// ==================== Core Public Interface ====================

void UUIManagerComponent::ShowLockOnWidget(AActor* Target)
{
	if (!IsValidTargetForUI(Target))
	{
		// ��Ƶ��־�Ż� - ���ӽ�Ƶ����
		if (bEnableUIDebugLogs)
		{
			static int32 FrameCounter = 0;
			if (++FrameCounter % 300 == 0) // ÿ5��ֻ��¼һ��
			{
				UE_LOG(LogTemp, Warning, TEXT("UIManagerComponent::ShowLockOnWidget - Invalid target: %s"), 
					Target ? *Target->GetName() : TEXT("None"));
			}
		}
		return;
	}

	// Hide all other widgets first
	HideAllLockOnWidgets();

	// Set current target
	CurrentLockOnTarget = Target;

	// Choose display method based on current mode
	switch (CurrentUIDisplayMode)
	{
		case EUIDisplayMode::SocketProjection:
			ShowSocketProjectionWidget(Target);
			break;
			
		case EUIDisplayMode::Traditional3D:
			// Traditional 3D space UI display
			{
				UActorComponent* WidgetComp = Target->GetComponentByClass(UWidgetComponent::StaticClass());
				if (WidgetComp)
				{
					UWidgetComponent* WidgetComponent = Cast<UWidgetComponent>(WidgetComp);
					if (WidgetComponent)
					{
						WidgetComponent->SetVisibility(true);
						TargetsWithActiveWidgets.AddUnique(Target);
					}
				}
			}
			break;

		case EUIDisplayMode::ScreenSpace:
			// Screen space overlay UI (����MyCharacter���߼�)
			{
				// Try to find widget class at runtime if not set
				if (!LockOnWidgetClass)
				{
					TryFindWidgetClassAtRuntime();
				}

				if (!LockOnWidgetClass)
				{
					if (bEnableUIDebugLogs)
					{
						UE_LOG(LogTemp, Error, TEXT("UIManagerComponent::ShowLockOnWidget - No LockOnWidgetClass available for ScreenSpace mode"));
					}
					return;
				}

				UpdateOwnerReferences();
				if (!OwnerController)
				{
					if (bEnableUIDebugLogs)
					{
						UE_LOG(LogTemp, Error, TEXT("UIManagerComponent::ShowLockOnWidget - No PlayerController available"));
					}
					return;
				}

				if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
				{
					LockOnWidgetInstance->RemoveFromViewport();
				}

				LockOnWidgetInstance = CreateWidget<UUserWidget>(OwnerController, LockOnWidgetClass);
				if (LockOnWidgetInstance)
				{
					LockOnWidgetInstance->AddToViewport();
					PreviousLockOnTarget = CurrentLockOnTarget;
					
					if (bEnableUIDebugLogs)
					{
						UE_LOG(LogTemp, Log, TEXT("UIManagerComponent: Created and added ScreenSpace widget to viewport"));
					}
				}
			}
			break;

		case EUIDisplayMode::SizeAdaptive:
			// Size adaptive UI mode - analyze target size and adapt UI accordingly
			{
				// Get or calculate target size category
				EEnemySizeCategory SizeCategory = EEnemySizeCategory::Unknown;
				
				if (EEnemySizeCategory* CachedSize = EnemySizeCache.Find(Target))
				{
					SizeCategory = *CachedSize;
				}
				else
				{
					SizeCategory = AnalyzeTargetSize(Target);
					EnemySizeCache.Add(Target, SizeCategory);
				}

				// Show size-adaptive widget
				ShowSizeAdaptiveWidget(Target, SizeCategory);
				return; // ShowSizeAdaptiveWidget handles the rest
			}
			break;
			
		default:
			if (bEnableUIDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("UIManagerComponent::ShowLockOnWidget - Unsupported display mode: %d"), 
					(int32)CurrentUIDisplayMode);
			}
			break;
	}

	// Broadcast event
	if (OnLockOnWidgetShown.IsBound())
	{
		OnLockOnWidgetShown.Broadcast(Target);
	}

	if (bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::ShowLockOnWidget - Target: %s, Mode: %d"), 
			*Target->GetName(), (int32)CurrentUIDisplayMode);
	}
}

void UUIManagerComponent::HideLockOnWidget()
{
	HideAllLockOnWidgets();
	
	// Hide socket projection widget
	if (CurrentUIDisplayMode == EUIDisplayMode::SocketProjection)
	{
		HideSocketProjectionWidget();
	}

	// Hide screen space widget (����MyCharacter���߼�)
	if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
	{
		LockOnWidgetInstance->RemoveFromViewport();
	}

	LockOnWidgetInstance = nullptr;
	CurrentLockOnTarget = nullptr;
	PreviousLockOnTarget = nullptr;

	// Broadcast event
	if (OnLockOnWidgetHidden.IsBound())
	{
		OnLockOnWidgetHidden.Broadcast();
	}

	if (bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::HideLockOnWidget called"));
	}
}

void UUIManagerComponent::HideAllLockOnWidgets()
{
	// Hide all traditional 3D widgets
	for (AActor* Target : TargetsWithActiveWidgets)
	{
		if (IsValid(Target))
		{
			UActorComponent* WidgetComp = Target->GetComponentByClass(UWidgetComponent::StaticClass());
			if (WidgetComp)
			{
				UWidgetComponent* WidgetComponent = Cast<UWidgetComponent>(WidgetComp);
				if (WidgetComponent && WidgetComponent->IsVisible())
				{
					WidgetComponent->SetVisibility(false);
				}
			}
		}
	}

	// Clear the active widgets list
	TargetsWithActiveWidgets.Empty();

	// Hide socket projection widget
	if (CurrentUIDisplayMode == EUIDisplayMode::SocketProjection)
	{
		HideSocketProjectionWidget();
	}

	// Hide screen space widget
	if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
	{
		LockOnWidgetInstance->RemoveFromViewport();
		LockOnWidgetInstance = nullptr;
	}

	if (bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::HideAllLockOnWidgets called"));
	}
}

void UUIManagerComponent::UpdateLockOnWidget(AActor* CurrentTarget, AActor* PreviousTarget)
{
	// Update internal state
	PreviousLockOnTarget = PreviousTarget;
	CurrentLockOnTarget = CurrentTarget;

	// ����MyCharacter.cpp�������߼�
	// Check if not locked on - ensure all UI is hidden
	if (!CurrentTarget)
	{
		HideAllLockOnWidgets();
		
		if (CurrentUIDisplayMode == EUIDisplayMode::SocketProjection)
		{
			HideSocketProjectionWidget();
		}
		
		if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
		{
			LockOnWidgetInstance->RemoveFromViewport();
			LockOnWidgetInstance = nullptr;
		}
		return;
	}

	// Check if target changed
	bool bTargetChanged = (CurrentTarget != PreviousTarget);
	
	if (bTargetChanged && IsValid(PreviousTarget))
	{
		// Hide previous target's UI
		if (CurrentUIDisplayMode == EUIDisplayMode::SocketProjection)
		{
			HideSocketProjectionWidget();
		}
		else
		{
			// Hide traditional 3D widget of previous target
			UActorComponent* PrevWidgetComp = PreviousTarget->GetComponentByClass(UWidgetComponent::StaticClass());
			if (PrevWidgetComp)
			{
				UWidgetComponent* PrevWidgetComponent = Cast<UWidgetComponent>(PrevWidgetComp);
				if (PrevWidgetComponent && PrevWidgetComponent->IsVisible())
				{
					PrevWidgetComponent->SetVisibility(false);
				}
			}
		}
	}

	// Show current target's widget
	if (IsValid(CurrentTarget))
	{
		// Handle size adaptive mode specifically
		if (CurrentUIDisplayMode == EUIDisplayMode::SizeAdaptive)
		{
			EEnemySizeCategory SizeCategory = EEnemySizeCategory::Unknown;
			
			if (EEnemySizeCategory* CachedSize = EnemySizeCache.Find(CurrentTarget))
			{
				SizeCategory = *CachedSize;
			}
			else
			{
				SizeCategory = AnalyzeTargetSize(CurrentTarget);
				EnemySizeCache.Add(CurrentTarget, SizeCategory);
			}
			
			ShowSizeAdaptiveWidget(CurrentTarget, SizeCategory);
		}
		else
		{
			ShowLockOnWidget(CurrentTarget);
		}
	}

	if (bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::UpdateLockOnWidget - Current: %s, Previous: %s"), 
			CurrentTarget ? *CurrentTarget->GetName() : TEXT("None"),
			PreviousTarget ? *PreviousTarget->GetName() : TEXT("None"));
	}
}

// ==================== Socket Projection Implementation ====================

FVector UUIManagerComponent::GetTargetProjectionLocation(AActor* Target) const
{
	if (!Target)
	{
		return FVector::ZeroVector;
	}
	
	FVector ProjectionLocation;
	
	switch (HybridProjectionSettings.ProjectionMode)
	{
	case EProjectionMode::ActorCenter:
		ProjectionLocation = Target->GetActorLocation();
		break;
		
	case EProjectionMode::BoundsCenter:
		{
			FVector Origin, BoxExtent;
			Target->GetActorBounds(false, Origin, BoxExtent);
			ProjectionLocation = Origin + FVector(0, 0, BoxExtent.Z * HybridProjectionSettings.BoundsOffsetRatio);
		}
		break;
		
	case EProjectionMode::CustomOffset:
		ProjectionLocation = Target->GetActorLocation() + HybridProjectionSettings.CustomOffset;
		break;
		
	case EProjectionMode::Hybrid:
		{
			// Hybrid mode: combine bounds center and custom offset
			FVector Origin, BoxExtent;
			Target->GetActorBounds(false, Origin, BoxExtent);
			ProjectionLocation = Origin + FVector(0, 0, BoxExtent.Z * HybridProjectionSettings.BoundsOffsetRatio);
			ProjectionLocation += HybridProjectionSettings.CustomOffset;
			
			// Size adaptive
			if (HybridProjectionSettings.bEnableSizeAdaptive)
			{
				EEnemySizeCategory SizeCategory = AnalyzeTargetSize(Target);
				FVector SizeOffset = FVector::ZeroVector;
				switch (SizeCategory)
				{
				case EEnemySizeCategory::Small:
					SizeOffset.Z = -30.0f;
					break;
				case EEnemySizeCategory::Large:
					SizeOffset.Z = 50.0f;
					break;
				default:
					break;
				}
				ProjectionLocation += SizeOffset;
			}
		}
		break;
	}
	
	// Apply multi-part offset
	if (MultiPartConfig.TargetPart != ETargetBodyPart::Center)
	{
		ETargetBodyPart ActualPart = MultiPartConfig.TargetPart;
		if (ActualPart == ETargetBodyPart::Auto)
		{
			ActualPart = GetBestTargetPart(Target);
		}
		
		const FVector* PartOffset = MultiPartConfig.PartOffsets.Find(ActualPart);
		if (PartOffset)
		{
			ProjectionLocation += *PartOffset;
		}
	}
	
	return ProjectionLocation;
}

bool UUIManagerComponent::HasValidSocket(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	// Check if target has the specified socket - for backwards compatibility
	// Note: In hybrid projection mode, this is less critical but kept for fallback support
	USkeletalMeshComponent* SkeletalMesh = Target->FindComponentByClass<USkeletalMeshComponent>();
	if (SkeletalMesh)
	{
		// Use a common socket name for checking
		FName SocketName = TEXT("Spine2Socket");
		bool bSocketExists = SkeletalMesh->DoesSocketExist(SocketName);
		return bSocketExists;
	}

	return false;
}

FVector2D UUIManagerComponent::ProjectToScreen(const FVector& WorldLocation) const
{
	const_cast<UUIManagerComponent*>(this)->UpdateOwnerReferences();

	if (!OwnerController)
	{
		return FVector2D::ZeroVector;
	}

	FVector2D ScreenLocation;
	bool bProjected = OwnerController->ProjectWorldLocationToScreen(WorldLocation, ScreenLocation);
	
	if (!bProjected)
	{
		// Fallback check: Is the target at screen edge?
		FVector2D ViewportSize;
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->GetViewportSize(ViewportSize);
			
			// Extend screen range to include edge targets
			const float EdgeExtension = 100.0f;
			if (ScreenLocation.X < -EdgeExtension || ScreenLocation.X > ViewportSize.X + EdgeExtension ||
				ScreenLocation.Y < -EdgeExtension || ScreenLocation.Y > ViewportSize.Y + EdgeExtension)
			{
				return FVector2D::ZeroVector;
			}
			
			// Clamp to screen edge
			ScreenLocation.X = FMath::Clamp(ScreenLocation.X, 0.0f, ViewportSize.X);
			ScreenLocation.Y = FMath::Clamp(ScreenLocation.Y, 0.0f, ViewportSize.Y);
		}
		else
		{
			return FVector2D::ZeroVector;
		}
	}
	
	return ScreenLocation;
}

FVector2D UUIManagerComponent::ProjectSocketToScreen(const FVector& SocketWorldLocation) const
{
	// Update owner references
	const_cast<UUIManagerComponent*>(this)->UpdateOwnerReferences();

	// Get player controller
	if (!OwnerController)
	{
		// ��Ƶ��־�Ż� - ԭʼ���뱣����ع��
		// if (bEnableUIDebugLogs)
		// {
		//	UE_LOG(LogTemp, Warning, TEXT("UIManagerComponent::ProjectSocketToScreen: No PlayerController"));
		// }
		return FVector2D::ZeroVector;
	}

	// Project world location to screen coordinates
	FVector2D ScreenLocation;
	bool bProjected = OwnerController->ProjectWorldLocationToScreen(SocketWorldLocation, ScreenLocation);
	
	if (bProjected)
	{
		// ��Ƶ��־�Ż� - ԭʼ���뱣����ع��
		// if (bEnableUIDebugLogs)
		// {
		//	UE_LOG(LogTemp, Verbose, TEXT("UIManagerComponent: Projection successful: World(%.1f, %.1f, %.1f) -> Screen(%.1f, %.1f)"), 
		//		SocketWorldLocation.X, SocketWorldLocation.Y, SocketWorldLocation.Z,
		//		ScreenLocation.X, ScreenLocation.Y);
		// }
		return ScreenLocation;
	}
	else
	{
		// ��Ƶ��־�Ż� - ԭʼ���뱣����ع��
		// if (bEnableUIDebugLogs)
		// {
		//	UE_LOG(LogTemp, Verbose, TEXT("UIManagerComponent: Projection failed for world location (%.1f, %.1f, %.1f)"), 
		//		SocketWorldLocation.X, SocketWorldLocation.Y, SocketWorldLocation.Z);
		// }
		return FVector2D::ZeroVector;
	}
}

void UUIManagerComponent::ShowSocketProjectionWidget(AActor* Target)
{
	if (!Target)
	{
		if (bEnableUIDebugLogs)
		{
			UE_LOG(LogTemp, Error, TEXT("UIManagerComponent::ShowSocketProjectionWidget: No target specified"));
		}
		return;
	}

	// Update owner references
	UpdateOwnerReferences();

	// If LockOnWidgetClass is not set, try to find it at runtime (����MyCharacter���߼�)
	if (!LockOnWidgetClass)
	{
		if (bEnableUIDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("UIManagerComponent::ShowSocketProjectionWidget: LockOnWidgetClass is not set! Attempting runtime search..."));
		}
		
		if (!TryFindWidgetClassAtRuntime())
		{
			if (bEnableUIDebugLogs)
			{
				UE_LOG(LogTemp, Error, TEXT("UIManagerComponent: Could not find any Widget class at runtime!"));
				UE_LOG(LogTemp, Error, TEXT("Please ensure:"));
				UE_LOG(LogTemp, Error, TEXT("1. One of these Widgets is compiled and valid"));
				UE_LOG(LogTemp, Error, TEXT("2. LockOnWidgetClass is set in Blueprint to one of these Widgets"));
				UE_LOG(LogTemp, Error, TEXT("3. Widget contains UpdateLockOnPostition event"));
			}
			return;
		}
	}

	if (!OwnerController)
	{
		if (bEnableUIDebugLogs)
		{
			UE_LOG(LogTemp, Error, TEXT("UIManagerComponent::ShowSocketProjectionWidget: No valid PlayerController"));
		}
		return;
	}

	// If already have instance and in viewport, remove it first (����MyCharacter���߼�)
	if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
	{
		if (bEnableUIDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("UIManagerComponent: Removing existing UMG widget instance"));
		}
		LockOnWidgetInstance->RemoveFromViewport();
	}

	// Create new Widget instance (����MyCharacter���߼�)
	LockOnWidgetInstance = CreateWidget<UUserWidget>(OwnerController, LockOnWidgetClass);
	if (LockOnWidgetInstance)
	{
		if (bEnableUIDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("UIManagerComponent: Successfully created UMG widget instance: %s"), 
				*LockOnWidgetInstance->GetClass()->GetName());
		}
		
		LockOnWidgetInstance->AddToViewport();
		if (bEnableUIDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("UIManagerComponent: UMG widget added to viewport"));
		}
		
		// Immediately update position
		UpdateProjectionWidget(Target);
		
		PreviousLockOnTarget = CurrentLockOnTarget;
		CurrentLockOnTarget = Target;
		
		if (bEnableUIDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("UIManagerComponent: Hybrid projection widget created and shown for target: %s"), 
				*Target->GetName());
			UE_LOG(LogTemp, Warning, TEXT("UIManagerComponent: Using projection mode: %d"), 
				(int32)HybridProjectionSettings.ProjectionMode);
		}
	}
	else
	{
		if (bEnableUIDebugLogs)
		{
			UE_LOG(LogTemp, Error, TEXT("UIManagerComponent: Failed to create UMG widget instance! LockOnWidgetClass: %s"), 
				LockOnWidgetClass ? *LockOnWidgetClass->GetName() : TEXT("NULL"));
		}
	}
}

void UUIManagerComponent::UpdateProjectionWidget(AActor* Target)
{
	if (!Target || !LockOnWidgetInstance || !LockOnWidgetInstance->IsInViewport())
		return;

	// Get target projection location using hybrid projection system
	FVector ProjectionLocation = GetTargetProjectionLocation(Target);
	
	// Project to screen coordinates
	FVector2D ScreenPosition = ProjectToScreen(ProjectionLocation);
	
	// Fallback mechanism: If projection failed, try other methods
	if (ScreenPosition == FVector2D::ZeroVector)
	{
		// Try using Actor center
		ProjectionLocation = Target->GetActorLocation();
		ScreenPosition = ProjectToScreen(ProjectionLocation);
		
		if (ScreenPosition == FVector2D::ZeroVector)
		{
			// Final fallback: Hide UI
			if (LockOnWidgetInstance->IsVisible())
			{
				LockOnWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
				
				if (bEnableUIDebugLogs)
				{
					UE_LOG(LogTemp, Warning, TEXT("UIManager: Projection failed, hiding widget"));
				}
			}
			return;
		}
	}

	// Try to call your UMG event function UpdateLockOnPostition (keeping original spelling)
	UFunction* UpdateFunction = LockOnWidgetInstance->GetClass()->FindFunctionByName(FName(TEXT("UpdateLockOnPostition")));
	
	if (UpdateFunction)
	{
		// Check function parameter count
		if (UpdateFunction->NumParms >= 1)
		{
			// Create parameter structure
			uint8* Params = static_cast<uint8*>(FMemory_Alloca(UpdateFunction->ParmsSize));
			FMemory::Memzero(Params, UpdateFunction->ParmsSize);
			
			// Find FVector2D parameter
			bool bFoundParam = false;
			for (TFieldIterator<FProperty> It(UpdateFunction); It; ++It)
			{
				FProperty* Property = *It;
				if (Property->HasAnyPropertyFlags(CPF_Parm) && !Property->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					// Check if it's FVector2D type
					if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
					{
						if (StructProp->Struct && StructProp->Struct->GetFName() == FName("Vector2D"))
						{
							// Set parameter value
							FVector2D* ParamPtr = reinterpret_cast<FVector2D*>(Params + Property->GetOffset_ForInternal());
							*ParamPtr = ScreenPosition;
							bFoundParam = true;
							break;
						}
					}
				}
			}
			
			if (bFoundParam)
			{
				// Call function
				LockOnWidgetInstance->ProcessEvent(UpdateFunction, Params);
			}
			else
			{
				if (bEnableUIDebugLogs)
				{
					UE_LOG(LogTemp, Error, TEXT("UIManagerComponent: Could not find Vector2D parameter in UMG function UpdateLockOnPostition"));
				}
			}
		}
		else
		{
			if (bEnableUIDebugLogs)
			{
				UE_LOG(LogTemp, Error, TEXT("UIManagerComponent: UMG function UpdateLockOnPostition has no parameters"));
			}
		}
	}
	else
	{
		if (bEnableUIDebugLogs)
		{
			UE_LOG(LogTemp, Error, TEXT("UIManagerComponent: Could not find UMG update function UpdateLockOnPostition in widget class"));
		}
	}
	
	// Ensure Widget is visible
	if (!LockOnWidgetInstance->IsVisible())
	{
		LockOnWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	}
	
	// Broadcast event
	if (OnSocketProjectionUpdated.IsBound())
	{
		OnSocketProjectionUpdated.Broadcast(Target, ScreenPosition);
	}
}

void UUIManagerComponent::UpdateSocketProjectionWidget(AActor* Target)
{
	// Redirect to new function name for backward compatibility
	UpdateProjectionWidget(Target);
}

void UUIManagerComponent::HideSocketProjectionWidget()
{
	if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
	{
		LockOnWidgetInstance->RemoveFromViewport();
		if (bEnableUIDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("UIManagerComponent: Socket projection widget hidden"));
		}
	}
	
	LockOnWidgetInstance = nullptr;
}

// ==================== Configuration Accessors ====================

void UUIManagerComponent::SetUIDisplayMode(EUIDisplayMode NewMode)
{
	if (CurrentUIDisplayMode != NewMode)
	{
		// Hide current widgets before switching
		HideAllLockOnWidgets();
		
		CurrentUIDisplayMode = NewMode;
		
		if (bEnableUIDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::SetUIDisplayMode - New mode: %d"), (int32)NewMode);
		}
	}
}

void UUIManagerComponent::SetHybridProjectionSettings(const FHybridProjectionSettings& NewSettings)
{
	HybridProjectionSettings = NewSettings;
	
	if (bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::SetHybridProjectionSettings - Mode: %d"), 
			(int32)NewSettings.ProjectionMode);
	}
}

void UUIManagerComponent::SetAdvancedCameraSettings(const FAdvancedCameraSettings& NewSettings)
{
	AdvancedCameraSettings = NewSettings;
	
	if (bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::SetAdvancedCameraSettings - Size adaptation enabled: %s"), 
			NewSettings.bEnableEnemySizeAdaptation ? TEXT("YES") : TEXT("NO"));
	}
}

// ==================== Multi-Part Support Implementation ====================

void UUIManagerComponent::SwitchTargetPart(ETargetBodyPart NewPart)
{
	MultiPartConfig.TargetPart = NewPart;
	
	// Update UI position
	if (CurrentLockOnTarget)
	{
		UpdateLockOnWidget(CurrentLockOnTarget, PreviousLockOnTarget);
	}
	
	UE_LOG(LogTemp, Log, TEXT("Switched target part to: %s"), *UEnum::GetValueAsString(NewPart));
}

ETargetBodyPart UUIManagerComponent::GetBestTargetPart(AActor* Target) const
{
	if (!Target || !MultiPartConfig.bEnableSmartSelection)
		return ETargetBodyPart::Center;
	
	// Based on distance, select the best body part
	const_cast<UUIManagerComponent*>(this)->UpdateOwnerReferences();
	if (OwnerCharacter)
	{
		float PlayerHeight = OwnerCharacter->GetActorLocation().Z;
		float TargetHeight = Target->GetActorLocation().Z;
		float HeightDiff = TargetHeight - PlayerHeight;
		
		if (HeightDiff > 200.0f)
			return ETargetBodyPart::Feet;  // Target above, aim for feet
		else if (HeightDiff < -200.0f)
			return ETargetBodyPart::Head;   // Target below, aim for head
	}
	
	return ETargetBodyPart::Chest;  // Default aim for chest
}

// ==================== Size Analysis Helper Functions ====================

void UUIManagerComponent::ShowSizeAdaptiveWidget(AActor* Target, EEnemySizeCategory SizeCategory)
{
	if (!Target)
		return;
	
	// Update widget for enemy size
	UpdateWidgetForEnemySize(Target, SizeCategory);
	
	// Show widget using appropriate display mode
	if (CurrentUIDisplayMode == EUIDisplayMode::SocketProjection)
	{
		ShowSocketProjectionWidget(Target);
	}
	else if (CurrentUIDisplayMode == EUIDisplayMode::ScreenSpace)
	{
		// Show screen space widget with size-based adjustments
		UpdateOwnerReferences();
		if (!OwnerController || !LockOnWidgetClass)
		{
			if (bEnableUIDebugLogs)
			{
				UE_LOG(LogTemp, Error, TEXT("UIManagerComponent::ShowSizeAdaptiveWidget - Missing requirements"));
			}
			return;
		}
		
		if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
		{
			LockOnWidgetInstance->RemoveFromViewport();
		}
		
		LockOnWidgetInstance = CreateWidget<UUserWidget>(OwnerController, LockOnWidgetClass);
		if (LockOnWidgetInstance)
		{
			LockOnWidgetInstance->AddToViewport();
			
			if (bEnableUIDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("UIManagerComponent: Created size adaptive widget for %s, Size: %s"), 
					*Target->GetName(), *UEnum::GetValueAsString(SizeCategory));
			}
		}
	}
	
	TargetsWithActiveWidgets.AddUnique(Target);
}

void UUIManagerComponent::UpdateWidgetForEnemySize(AActor* Target, EEnemySizeCategory SizeCategory)
{
	if (!Target)
		return;
	
	// Update current UI scale and color based on size
	CurrentUIScale = GetUIScaleForEnemySize(SizeCategory);
	CurrentUIColor = GetUIColorForEnemySize(SizeCategory);
	
	// Cache the size category
	EnemySizeCache.Add(Target, SizeCategory);
	
	if (bEnableSizeAnalysisDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::UpdateWidgetForEnemySize - Target: %s, Size: %s, Scale: %.2f"), 
			*Target->GetName(), *UEnum::GetValueAsString(SizeCategory), CurrentUIScale);
	}
}

float UUIManagerComponent::GetUIScaleForEnemySize(EEnemySizeCategory SizeCategory) const
{
	switch (SizeCategory)
	{
	case EEnemySizeCategory::Small:
		return SmallEnemyUIScale;
	case EEnemySizeCategory::Medium:
		return MediumEnemyUIScale;
	case EEnemySizeCategory::Large:
	case EEnemySizeCategory::Giant:
		return LargeEnemyUIScale;
	default:
		return MediumEnemyUIScale;
	}
}

FLinearColor UUIManagerComponent::GetUIColorForEnemySize(EEnemySizeCategory SizeCategory) const
{
	switch (SizeCategory)
	{
	case EEnemySizeCategory::Small:
		return SmallEnemyUIColor;
	case EEnemySizeCategory::Medium:
		return MediumEnemyUIColor;
	case EEnemySizeCategory::Large:
	case EEnemySizeCategory::Giant:
		return LargeEnemyUIColor;
	default:
		return MediumEnemyUIColor;
	}
}

void UUIManagerComponent::DebugWidgetSetup()
{
	UE_LOG(LogTemp, Warning, TEXT("===== UIManagerComponent Debug Widget Setup ====="));
	UE_LOG(LogTemp, Warning, TEXT("LockOnWidgetClass: %s"), 
		LockOnWidgetClass ? *LockOnWidgetClass->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("Current UI Display Mode: %d"), (int32)CurrentUIDisplayMode);
	UE_LOG(LogTemp, Warning, TEXT("Current Lock-On Target: %s"), 
		CurrentLockOnTarget ? *CurrentLockOnTarget->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("Widget Instance: %s"), 
		LockOnWidgetInstance ? TEXT("Valid") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("Widget In Viewport: %s"), 
		(LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport()) ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Warning, TEXT("Owner Character: %s"), 
		OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("Owner Controller: %s"), 
		OwnerController ? *OwnerController->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("Active Widgets Count: %d"), TargetsWithActiveWidgets.Num());
	UE_LOG(LogTemp, Warning, TEXT("Size Cache Count: %d"), EnemySizeCache.Num());
	UE_LOG(LogTemp, Warning, TEXT("Current UI Scale: %.2f"), CurrentUIScale);
	UE_LOG(LogTemp, Warning, TEXT("=============================================="));
}

void UUIManagerComponent::LogWidgetState(const FString& Context)
{
	if (bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent [%s] - Widget: %s, Target: %s, Mode: %d"), 
			*Context,
			LockOnWidgetInstance ? TEXT("Valid") : TEXT("NULL"),
			CurrentLockOnTarget ? *CurrentLockOnTarget->GetName() : TEXT("NULL"),
			(int32)CurrentUIDisplayMode);
	}
}

bool UUIManagerComponent::ValidateWidgetClass() const
{
	bool bIsValid = (LockOnWidgetClass != nullptr);
	
	if (!bIsValid && bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Error, TEXT("UIManagerComponent::ValidateWidgetClass - LockOnWidgetClass is NULL!"));
	}
	
	return bIsValid;
}

// ==================== Internal Helper Functions ====================

void UUIManagerComponent::InitializeUIManager()
{
	// Update owner references
	UpdateOwnerReferences();
	
	// Clear all widget arrays
	TargetsWithActiveWidgets.Empty();
	WidgetComponentCache.Empty();
	EnemySizeCache.Empty();
	
	// Log initialization
	if (bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::InitializeUIManager - Component initialized"));
		DebugWidgetSetup();
	}
}

void UUIManagerComponent::CleanupUIResources()
{
	// Hide all widgets
	HideAllLockOnWidgets();
	
	// Clear widget instance
	if (LockOnWidgetInstance)
	{
		if (LockOnWidgetInstance->IsInViewport())
		{
			LockOnWidgetInstance->RemoveFromViewport();
		}
		LockOnWidgetInstance = nullptr;
	}
	
	// Clear all caches
	TargetsWithActiveWidgets.Empty();
	WidgetComponentCache.Empty();
	EnemySizeCache.Empty();
	
	CurrentLockOnTarget = nullptr;
	PreviousLockOnTarget = nullptr;
	
	if (bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::CleanupUIResources - All UI resources cleaned up"));
	}
}

APlayerController* UUIManagerComponent::GetPlayerController() const
{
	if (OwnerController)
	{
		return OwnerController;
	}
	
	if (OwnerCharacter)
	{
		return Cast<APlayerController>(OwnerCharacter->GetController());
	}
	
	AActor* Owner = GetOwner();
	if (Owner)
	{
		if (ACharacter* Character = Cast<ACharacter>(Owner))
		{
			return Cast<APlayerController>(Character->GetController());
		}
	}
	
	return nullptr;
}

bool UUIManagerComponent::IsValidTargetForUI(AActor* Target) const
{
	return Target != nullptr && IsValid(Target) && !Target->IsPendingKill();
}

void UUIManagerComponent::ClearWidgetComponentCache()
{
	WidgetComponentCache.Empty();
	
	if (bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::ClearWidgetComponentCache - Cache cleared"));
	}
}

void UUIManagerComponent::UpdateOwnerReferences()
{
	// Update owner character if not set
	if (!OwnerCharacter)
	{
		AActor* Owner = GetOwner();
		if (Owner)
		{
			OwnerCharacter = Cast<ACharacter>(Owner);
		}
	}
	
	// Update owner controller if not set
	if (!OwnerController)
	{
		if (OwnerCharacter)
		{
			OwnerController = Cast<APlayerController>(OwnerCharacter->GetController());
		}
	}
}

bool UUIManagerComponent::TryFindWidgetClassAtRuntime()
{
	// Try to find widget class from project content
	// This is a fallback for when widget class is not set in blueprint
	
	if (LockOnWidgetClass)
	{
		return true; // Already have a valid class
	}
	
	// Try to find common widget class names
	TArray<FString> CommonWidgetNames = {
		TEXT("/Game/UI/LockOnWidget.LockOnWidget_C"),
		TEXT("/Game/Blueprints/UI/LockOnWidget.LockOnWidget_C"),
		TEXT("/Game/UI/Widgets/LockOnWidget.LockOnWidget_C")
	};
	
	for (const FString& WidgetPath : CommonWidgetNames)
	{
		UClass* FoundClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, *WidgetPath);
		if (FoundClass)
		{
			LockOnWidgetClass = FoundClass;
			if (bEnableUIDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("UIManagerComponent: Found widget class at runtime: %s"), *WidgetPath);
			}
			return true;
		}
	}
	
	if (bEnableUIDebugLogs)
	{
		UE_LOG(LogTemp, Error, TEXT("UIManagerComponent: Could not find widget class at runtime!"));
	}
	
	return false;
}

float UUIManagerComponent::CalculateTargetBoundingBoxSize(AActor* Target) const
{
	if (!Target)
		return 0.0f;
	
	FVector Origin, BoxExtent;
	Target->GetActorBounds(false, Origin, BoxExtent);
	
	// Return the height as the primary size metric
	return BoxExtent.Z * 2.0f;
}

EEnemySizeCategory UUIManagerComponent::AnalyzeTargetSize(AActor* Target) const
{
	if (!Target)
		return EEnemySizeCategory::Unknown;
	
	// Get bounding box
	FVector Origin, BoxExtent;
	Target->GetActorBounds(false, Origin, BoxExtent);
	
	float Height = BoxExtent.Z * 2.0f;
	float Volume = BoxExtent.X * BoxExtent.Y * BoxExtent.Z * 8.0f;
	
	// Classify based on height and volume
	if (Height > 500.0f || Volume > 1000000.0f)
		return EEnemySizeCategory::Giant;
	else if (Height > 300.0f || Volume > 300000.0f)
		return EEnemySizeCategory::Large;
	else if (Height > 150.0f || Volume > 100000.0f)
		return EEnemySizeCategory::Medium;
	else
		return EEnemySizeCategory::Small;
}

void UUIManagerComponent::UpdateTargetSizeCategory(AActor* Target)
{
	if (!Target)
		return;
	
	EEnemySizeCategory NewCategory = AnalyzeTargetSize(Target);
	
	// Update cache
	EEnemySizeCategory* ExistingCategory = EnemySizeCache.Find(Target);
	if (ExistingCategory)
	{
		if (*ExistingCategory != NewCategory)
		{
			*ExistingCategory = NewCategory;
			
			if (bEnableSizeAnalysisDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("UIManagerComponent: Target size category updated: %s -> %s"), 
					*Target->GetName(), *UEnum::GetValueAsString(NewCategory));
			}
		}
	}
	else
	{
		EnemySizeCache.Add(Target, NewCategory);
	}
}

void UUIManagerComponent::CleanupSizeCache()
{
	// Remove invalid entries from size cache
	TArray<AActor*> InvalidTargets;
	
	for (const TPair<AActor*, EEnemySizeCategory>& Pair : EnemySizeCache)
	{
		if (!IsValid(Pair.Key))
		{
			InvalidTargets.Add(Pair.Key);
		}
	}
	
	for (AActor* InvalidTarget : InvalidTargets)
	{
		EnemySizeCache.Remove(InvalidTarget);
	}
	
	if (bEnableUIDebugLogs && InvalidTargets.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("UIManagerComponent::CleanupSizeCache - Removed %d invalid entries"), 
			InvalidTargets.Num());
	}
}