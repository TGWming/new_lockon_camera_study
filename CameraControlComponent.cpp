// Fill out your copyright notice in the Description page of Project Settings.

#include "CameraControlComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

// 控制台命令定义
static TAutoConsoleVariable<int32> CVarCameraDebugLevel(
	TEXT("Camera.DebugLevel"),
	0,
	TEXT("Camera debug visualization level (0=Off, 1=Basic, 2=Detailed, 3=Full)"),
	ECVF_Cheat
);

static TAutoConsoleVariable<float> CVarCameraInterpSpeed(
	TEXT("Camera.InterpSpeed"),
	5.0f,
	TEXT("Camera interpolation speed"),
	ECVF_Cheat
);

static TAutoConsoleVariable<bool> CVarCameraShowTargetInfo(
	TEXT("Camera.ShowTargetInfo"),
	false,
	TEXT("Show target information overlay"),
	ECVF_Cheat
);

// 控制台命令函数
static FAutoConsoleCommand CmdResetCamera(
	TEXT("Camera.Reset"),
	TEXT("Reset camera to default position"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		if (GEngine)
		{
			for (TObjectIterator<UCameraControlComponent> It; It; ++It)
			{
				if (It->GetWorld() && !It->GetWorld()->bIsTearingDown)
				{
					It->ResetCameraToDefault();
					UE_LOG(LogTemp, Warning, TEXT("Camera reset to default"));
				}
			}
		}
	})
);

static FAutoConsoleCommand CmdToggleFreeLook(
	TEXT("Camera.ToggleFreeLook"),
	TEXT("Toggle free look mode"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		for (TObjectIterator<UCameraControlComponent> It; It; ++It)
		{
			if (It->GetWorld() && !It->GetWorld()->bIsTearingDown)
			{
				It->FreeLookSettings.bEnableFreeLook = !It->FreeLookSettings.bEnableFreeLook;
				UE_LOG(LogTemp, Warning, TEXT("FreeLook: %s"), 
					It->FreeLookSettings.bEnableFreeLook ? TEXT("Enabled") : TEXT("Disabled"));
			}
		}
	})
);

// Sets default values for this component's properties
UCameraControlComponent::UCameraControlComponent()
{
	// Set this component to be ticked every frame
	PrimaryComponentTick.bCanEverTick = true;

	// ==================== 初始化状态变量 ====================
	CurrentCameraState = ECameraState::Normal;
	CurrentLockOnTarget = nullptr;
	PreviousLockOnTarget = nullptr;
	
	// 相机跟随初始化
	bShouldCameraFollowTarget = true;
	bShouldCharacterRotateToTarget = true;
	bPlayerIsMoving = false;

	// 平滑切换状态初始化
	bIsSmoothSwitching = false;
	SmoothSwitchStartTime = 0.0f;
	SmoothSwitchStartRotation = FRotator::ZeroRotator;
	SmoothSwitchTargetRotation = FRotator::ZeroRotator;
	bShouldSmoothSwitchCamera = false;
	bShouldSmoothSwitchCharacter = false;

	// 自动修正状态初始化
	bIsCameraAutoCorrection = false;
	CameraCorrectionStartTime = 0.0f;
	CameraCorrectionStartRotation = FRotator::ZeroRotator;
	CameraCorrectionTargetRotation = FRotator::ZeroRotator;
	DelayedCorrectionTarget = nullptr;

	// 重置相机状态初始化
	bIsSmoothCameraReset = false;
	SmoothResetStartTime = 0.0f;
	SmoothResetStartRotation = FRotator::ZeroRotator;
	SmoothResetTargetRotation = FRotator::ZeroRotator;

	// 高级相机距离响应初始化
	bIsAdvancedCameraAdjustment = false;
	LastAdvancedAdjustmentTime = 0.0f;
	CurrentTargetSizeCategory = EEnemySizeCategory::Unknown;
	CurrentTargetDistance = 0.0f;

	// 调试控制初始化
	bEnableCameraDebugLogs = true;
	bEnableAdvancedAdjustmentDebugLogs = false;

	// ==================== 小角度近UI切换状态初始化 ====================
	bIsMinimalChangeSwitch = false;
	MinimalChangeSwitchTime = 0.0f;
	LastPlayerMovementTime = 0.0f;

	// 相机臂动态调整初始化
	BaseArmLength = 0.0f;
	bArmLengthAdjusted = false;

	// 初始化FreeLook
	FreeLookSettings = FFreeLookSettings();
	FreeLookOffset = FRotator::ZeroRotator;
	LastFreeLookInputTime = 0.0f;
	bIsFreeLooking = false;

	// 初始化3D视口控制
	OrbitRotation = FRotator::ZeroRotator;
	bIsPreviewingCamera = false;

	// 初始化递归防护
	RecursionDepth = 0;

	// 初始化FromSoftware式相机配置
	InitializeDefaultCameraConfigs();
}

// Called when the game starts
void UCameraControlComponent::BeginPlay()
{
	Super::BeginPlay();

	// 验证拥有者是否为角色
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("CameraControlComponent: Owner is not a Character! Component requires Character owner."));
		return;
	}

	// 验证控制器
	APlayerController* PlayerController = GetOwnerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraControlComponent: No PlayerController found on owner. Some features may not work."));
	}

	// 验证组件
	USpringArmComponent* SpringArm = GetSpringArmComponent();
	UCameraComponent* Camera = GetCameraComponent();
	
	if (!SpringArm)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraControlComponent: No SpringArmComponent found on owner. Camera control may not work properly."));
	}
	
	if (!Camera)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraControlComponent: No CameraComponent found on owner. Camera control may not work properly."));
	}

	// 记录SpringArm原始长度用于动态调整
	if (SpringArm)
	{
		BaseArmLength = SpringArm->TargetArmLength;
		
		if (bEnableCameraDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("CameraControlComponent: Recorded base arm length: %.1f"), BaseArmLength);
		}
	}

	// 设置初始相机状态
	UpdateCameraState(ECameraState::Normal);

	if (bEnableCameraDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraControlComponent: Successfully initialized for %s"), 
			*OwnerCharacter->GetName());
	}
}

// Called every frame
void UCameraControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 应用控制台变量
	if (CVarCameraInterpSpeed.GetValueOnGameThread() != CameraSettings.CameraInterpSpeed)
	{
		CameraSettings.CameraInterpSpeed = CVarCameraInterpSpeed.GetValueOnGameThread();
	}
	
	// 检查Debug级别
	int32 DebugLevel = CVarCameraDebugLevel.GetValueOnGameThread();
	if (DebugLevel > 0 && bEnableCameraDebugLogs != (DebugLevel > 0))
	{
		bEnableCameraDebugLogs = DebugLevel > 0;
	}

	// 级别之间的平滑过渡和相机状态发发生变化

	// 进行摄像机自动修正
	if (bIsCameraAutoCorrection)
	{
		UpdateCameraAutoCorrection();
	}

	// 锁定状态下的摄像机更新
	if (CurrentLockOnTarget && IsValid(CurrentLockOnTarget))
	{
		// 平滑切换目标状态下的摄像机更新
		if (bIsSmoothSwitching)
		{
			UpdateSmoothTargetSwitch();
		}
		else if (!bIsCameraAutoCorrection) // 仅在不是自动修正状态下，进行锁定目标摄像机更新
		{
			UpdateLockOnCamera();
		}

		// 高级相机距离适应响应
		if (AdvancedCameraSettings.bEnableDistanceAdaptiveCamera)
		{
			UpdateAdvancedCameraAdjustment();
		}
	}

	// 每帧调试信息输出
	if (bEnableCameraDebugLogs && CurrentLockOnTarget)
	{
		static float LastDebugTime = 0.0f;
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastDebugTime > 2.0f) // 每2秒输出一次
		{
			// 输出调试信息
			// UE_LOG(LogTemp, VeryVerbose, TEXT("CameraControl: State=%s, Target=%s, Follow=%s, Rotate=%s"), 
			//	*UEnum::GetValueAsString(CurrentCameraState),
			//	*CurrentLockOnTarget->GetName(),
			//	bShouldCameraFollowTarget ? TEXT("YES") : TEXT("NO"),
			//	bShouldCharacterRotateToTarget ? TEXT("YES") : TEXT("NO"));
			LastDebugTime = CurrentTime;
		}
	}
}

// ==================== 接口函数实现 ====================

void UCameraControlComponent::SetCameraSettings(const FCameraSettings& Settings)
{
	CameraSettings = Settings;
	
	if (bEnableCameraDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("CameraControlComponent: Camera settings updated"));
		UE_LOG(LogTemp, Log, TEXT("- InterpSpeed: %.1f"), CameraSettings.CameraInterpSpeed);
		UE_LOG(LogTemp, Log, TEXT("- SmoothTracking: %s"), CameraSettings.bEnableSmoothCameraTracking ? TEXT("ON") : TEXT("OFF"));
		UE_LOG(LogTemp, Log, TEXT("- TrackingMode: %d"), CameraSettings.CameraTrackingMode);
	}
}

void UCameraControlComponent::SetAdvancedCameraSettings(const FAdvancedCameraSettings& Settings)
{
	AdvancedCameraSettings = Settings;
	
	if (bEnableAdvancedAdjustmentDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("CameraControlComponent: Advanced camera settings updated"));
		UE_LOG(LogTemp, Log, TEXT("- DistanceAdaptive: %s"), AdvancedCameraSettings.bEnableDistanceAdaptiveCamera ? TEXT("ON") : TEXT("OFF"));
		UE_LOG(LogTemp, Log, TEXT("- TerrainCompensation: %s"), AdvancedCameraSettings.bEnableTerrainHeightCompensation ? TEXT("ON") : TEXT("OFF"));
		UE_LOG(LogTemp, Log, TEXT("- EnemySizeAdaptation: %s"), AdvancedCameraSettings.bEnableEnemySizeAdaptation ? TEXT("ON") : TEXT("OFF"));
	}
}

// ==================== 状态更新和切换函数 ====================

void UCameraControlComponent::HandlePlayerInput(float TurnInput, float LookUpInput)
{
	// 检测是否需要中断自动控制
	if (ShouldInterruptAutoControl(TurnInput, LookUpInput))
	{
		// ͣ STOP_POINT

		// ͣSTOP自动修正
		if (bIsCameraAutoCorrection)
		{
			bIsCameraAutoCorrection = false;
			DelayedCorrectionTarget = nullptr;
			UpdateCameraState(ECameraState::LockedOn);
			
			if (bEnableCameraDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("Player input detected - stopping camera auto correction"));
			}
		}
		
		// ͣSTOP平滑重置
		if (bIsSmoothCameraReset)
		{
			bIsSmoothCameraReset = false;
			UpdateCameraState(ECameraState::Normal);
			
			if (bEnableCameraDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("Player input detected - stopping smooth camera reset"));
			}
		}
	}
}

void UCameraControlComponent::HandlePlayerMovement(bool bIsMoving)
{
	// === 步骤5修复：正确处理UI-Only恢复 ===
	bPlayerIsMoving = bIsMoving;

	// 安全宪法：检查锁定状态
	if (!bIsMoving || !CurrentLockOnTarget)
	{
		return;
	}

	// 平滑切换期间不中断，但允许角色开始旋转
	if (bIsSmoothSwitching)
	{
		// 允许角色开始旋转
		bShouldSmoothSwitchCharacter = true;
		return;
	}
	
	// 自动修正期间中断
	if (bIsCameraAutoCorrection)
	{
		bIsCameraAutoCorrection = false;
		DelayedCorrectionTarget = nullptr;
		UpdateCameraState(ECameraState::LockedOn);
		
		if (bEnableCameraDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("Player movement interrupted camera auto correction"));
		}
	}
	
	// === UI-Only模式恢复逻辑 ===
	if (bIsMinimalChangeSwitch)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		float ElapsedTime = CurrentTime - MinimalChangeSwitchTime;
		
		// 需要持续移动0.5秒才恢复
		if (ElapsedTime >= 0.5f)
		{
			bIsMinimalChangeSwitch = false;
			bShouldCameraFollowTarget = true;
			bShouldCharacterRotateToTarget = true;
			
			if (bEnableCameraDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("UI-Only mode ended: Moved for %.2fs"), ElapsedTime);
			}
		}
		else
		{
			// 否则保持UI-Only模式
			if (bEnableCameraDebugLogs)
			{
				UE_LOG(LogTemp, VeryVerbose, TEXT("UI-Only mode active: %.2fs elapsed, waiting for 0.5s"), ElapsedTime);
			}
			return;
		}
	}
	
	// 普通情况：立即恢复相机跟随
	if (!bShouldCameraFollowTarget)
	{
		bShouldCameraFollowTarget = true;
		bShouldCharacterRotateToTarget = true;
		
		if (bEnableCameraDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("Camera follow restored"));
		}
	}
}

// ==================== 锁定相关函数实现 ====================

void UCameraControlComponent::UpdateLockOnCamera()
{
	// === 步骤4修复：UI-Only模式下跳过相机更新 ===
	
	// 安全宪法：有效性检查
	if (!CurrentLockOnTarget || !IsValid(CurrentLockOnTarget))
	{
		return;
	}

	// 关键修复：UI-Only模式完全跳过
	if (bIsMinimalChangeSwitch)
	{
		// UI-Only模式，不更新相机
		return;
	}
	
	// 平滑切换期间由切换逻辑控制
	if (bIsSmoothSwitching)
	{
		return;
	}

	APlayerController* PlayerController = GetOwnerController();
	if (!PlayerController)
	{
		return;
	}

	// 只有在应该跟随目标时才更新相机
	if (!bShouldCameraFollowTarget)
	{
		if (bShouldCharacterRotateToTarget)
		{
			UpdateCharacterRotationToTarget();
		}
		return;
	}

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return;

	// 获取玩家位置
	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	
	// 使用新的统一目标位置计算
	FVector TargetLocation = GetOptimalLockOnPosition(CurrentLockOnTarget);

	// 使用稳定插值获取平滑位置
	if (bEnableTargetStableInterpolation)
	{
		TargetLocation = GetStableTargetLocation(CurrentLockOnTarget);
	}

	// 计算玩家朝向目标的旋转
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(PlayerLocation, TargetLocation);

	// 获取DeltaTime用于插值计算
	float DeltaTime = GetWorld()->GetDeltaSeconds();

	// 应用FreeLook偏移
	if (FreeLookSettings.bEnableFreeLook && bIsFreeLooking)
	{
		// 检查是否应该自动返回
		float TimeSinceInput = GetWorld()->GetTimeSeconds() - LastFreeLookInputTime;
		if (TimeSinceInput > FreeLookSettings.AutoReturnDelay)
		{
			// 平滑返回中心
			FreeLookOffset = FMath::RInterpTo(FreeLookOffset, FRotator::ZeroRotator, 
				DeltaTime, FreeLookSettings.ReturnToCenterSpeed);
			
			if (FreeLookOffset.IsNearlyZero(0.1f))
			{
				bIsFreeLooking = false;
			}
		}
		
		// 应用FreeLook偏移
		LookAtRotation += FreeLookOffset;
	}

	// 获取当前相机/控制器的旋转
	FRotator CurrentRotation = PlayerController->GetControlRotation();

	// 使用插值进行平滑
	FRotator NewRotation;
	
	if (CameraSettings.bEnableSmoothCameraTracking)
	{
		float Distance = CalculateDistanceToTarget(CurrentLockOnTarget);
		float SpeedMultiplier = GetCameraSpeedMultiplierForDistance(Distance);
		float AdjustedInterpSpeed = CameraSettings.CameraInterpSpeed * SpeedMultiplier;

		switch (CameraSettings.CameraTrackingMode)
		{
		case 0: // 完全跟踪
			NewRotation = FMath::RInterpTo(CurrentRotation, LookAtRotation, DeltaTime, AdjustedInterpSpeed);
			break;
		case 1: // 仅水平跟踪
			{
				FRotator HorizontalLookAt = FRotator(CurrentRotation.Pitch, LookAtRotation.Yaw, CurrentRotation.Roll);
				NewRotation = FMath::RInterpTo(CurrentRotation, HorizontalLookAt, DeltaTime, AdjustedInterpSpeed);
			}
			break;
		default:
			NewRotation = FMath::RInterpTo(CurrentRotation, LookAtRotation, DeltaTime, AdjustedInterpSpeed);
			break;
		}
	}
	else
	{
		NewRotation = LookAtRotation;
	}
	
	PlayerController->SetControlRotation(NewRotation);

	// 角色旋转
	if (bShouldCharacterRotateToTarget)
	{
		FRotator CharacterRotation = FMath::RInterpTo(OwnerCharacter->GetActorRotation(), 
			FRotator(0, LookAtRotation.Yaw, 0), DeltaTime, CHARACTER_ROTATION_SPEED);
		OwnerCharacter->SetActorRotation(CharacterRotation);
	}
}

void UCameraControlComponent::StartSmoothTargetSwitch(AActor* NewTarget)
{
	// === 步骤3修复：保留DEADZONE逻辑 + 修复漂移 ===
	
	// 安全宪法：多重防御检查
	if (!NewTarget || !ValidateTarget(NewTarget))
	{
		if (bEnableCameraDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("StartSmoothTargetSwitch: Invalid target"));
		}
		return;
	}

	APlayerController* PlayerController = GetOwnerController();
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	
	// 安全宪法：组件检查
	if (!PlayerController || !OwnerCharacter)
	{
		if (bEnableCameraDebugLogs)
		{
			UE_LOG(LogTemp, Error, TEXT("StartSmoothTargetSwitch: Missing components"));
		}
		return;
	}

	// 计算角度差异
	float AngleDifference = CalculateAngleToTarget(NewTarget);
	
	// 更新目标
	PreviousLockOnTarget = CurrentLockOnTarget;
	CurrentLockOnTarget = NewTarget;
	
	// === DEADZONE 逻辑保留（UI-Only切换）===
	if (FMath::Abs(AngleDifference) <= TARGET_SWITCH_ANGLE_THRESHOLD)
	{
		// 小角度：UI-Only模式
		bIsMinimalChangeSwitch = true;
		MinimalChangeSwitchTime = GetWorld()->GetTimeSeconds();
		
		// 关键：停止相机跟随但不进入平滑切换
		bIsSmoothSwitching = false;
		bShouldCameraFollowTarget = false;
		bShouldCharacterRotateToTarget = false;
		
		UpdateCameraState(ECameraState::LockedOn);
		
		if (bEnableCameraDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("UI-Only switch: Angle=%.1f° < %.1f°"), 
				AngleDifference, TARGET_SWITCH_ANGLE_THRESHOLD);
		}
		return; // 直接返回，不移动相机
	}
	
	// === 清除 UI-Only 标志 ===
	bIsMinimalChangeSwitch = false;
	
	// 计算目标旋转
	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = GetOptimalLockOnPosition(NewTarget);
	FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(PlayerLocation, TargetLocation);
	
	// === 大角度立即切换 ===
	if (FMath::Abs(AngleDifference) > 120.0f)
	{
		PlayerController->SetControlRotation(TargetRotation);
		
		bIsSmoothSwitching = false;
		bShouldCameraFollowTarget = true;
		bShouldCharacterRotateToTarget = true;
		
		UpdateCameraState(ECameraState::LockedOn);
		
		if (bEnableCameraDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("Immediate switch: Angle=%.1f° > 120°"), AngleDifference);
		}
		return;
	}
	
	// === 中等角度平滑切换 ===
	SmoothSwitchStartTime = GetWorld()->GetTimeSeconds();
	SmoothSwitchStartRotation = PlayerController->GetControlRotation();
	SmoothSwitchTargetRotation = TargetRotation;
	
	bIsSmoothSwitching = true;
	bShouldSmoothSwitchCamera = true;
	bShouldSmoothSwitchCharacter = bPlayerIsMoving; // 只在移动时旋转角色
	bShouldCameraFollowTarget = true; // 修复漂移的关键
	
	UpdateCameraState(ECameraState::SmoothSwitching);
	
	if (bEnableCameraDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("Smooth switch: Angle=%.1f°, PlayerMoving=%s"), 
			AngleDifference, bPlayerIsMoving ? TEXT("Yes") : TEXT("No"));
	}
}

void UCameraControlComponent::UpdateSmoothTargetSwitch()
{
	if (!CurrentLockOnTarget || !IsValid(CurrentLockOnTarget))
	{
		bIsSmoothSwitching = false;
		return;
	}

	APlayerController* PlayerController = GetOwnerController();
	if (!PlayerController)
		return;

	// 计算当前时间与开始平滑切换时的时间差
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float ElapsedTime = CurrentTime - SmoothSwitchStartTime;

	// 如果超过平滑切换时间，则直接设置目标旋转，结束平滑切换
	if (ElapsedTime >= CameraSettings.CameraInterpSpeed)
	{
		PlayerController->SetControlRotation(SmoothSwitchTargetRotation);

		if (bShouldCharacterRotateToTarget)
		{
			ACharacter* OwnerCharacter = GetOwnerCharacter();
			if (OwnerCharacter)
			{
				OwnerCharacter->SetActorRotation(FRotator(0, SmoothSwitchTargetRotation.Yaw, 0));
			}
		}

		bIsSmoothSwitching = false;

		if (bEnableCameraDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("Smooth target switch completed: %s"), *CurrentLockOnTarget->GetName());
		}
	}
	else
	{
		// 按照时间插值计算当前的控制旋转
		FRotator NewRotation = FMath::Lerp(SmoothSwitchStartRotation, SmoothSwitchTargetRotation, ElapsedTime / CameraSettings.CameraInterpSpeed);
		PlayerController->SetControlRotation(NewRotation);

		if (bShouldCharacterRotateToTarget)
		{
			ACharacter* OwnerCharacter = GetOwnerCharacter();
			if (OwnerCharacter)
			{
				OwnerCharacter->SetActorRotation(FRotator(0, NewRotation.Yaw, 0));
			}
		}
	}
}

// ==================== 状态管理函数实现 ====================

void UCameraControlComponent::SetLockOnTarget(AActor* Target)
{
	// === 步骤2修复：防止重复设置 ===
	// 安全宪法：防御性检查
	if (CurrentLockOnTarget == Target)
	{
		return; // 相同目标，直接返回
	}
	
	// 记录变更
	AActor* OldTarget = CurrentLockOnTarget;
	CurrentLockOnTarget = Target;
	
	// 只在真正改变时记录日志
	if (bEnableCameraDebugLogs)
	{
		if (Target)
		{
			UE_LOG(LogTemp, Log, TEXT("CameraControlComponent: Target changed from [%s] to [%s]"), 
				OldTarget ? *OldTarget->GetName() : TEXT("None"),
				*Target->GetName());
		}
		else if (OldTarget)
		{
			UE_LOG(LogTemp, Log, TEXT("CameraControlComponent: Target cleared (was [%s])"),
				*OldTarget->GetName());
		}
	}
}

void UCameraControlComponent::ClearLockOnTarget()
{
	if (bEnableCameraDebugLogs && CurrentLockOnTarget)
	{
		UE_LOG(LogTemp, Log, TEXT("CameraControlComponent: Clearing lock-on target: %s"), *CurrentLockOnTarget->GetName());
	}
	
	PreviousLockOnTarget = CurrentLockOnTarget;
	CurrentLockOnTarget = nullptr;
	
	bIsSmoothSwitching = false;
	bShouldSmoothSwitchCamera = false;
	bShouldSmoothSwitchCharacter = false;
	bIsCameraAutoCorrection = false;
	bIsSmoothCameraReset = false;
	DelayedCorrectionTarget = nullptr;
	
	bShouldCameraFollowTarget = true;
	bShouldCharacterRotateToTarget = true;
	
	UpdateCameraState(ECameraState::Normal);
	
	if (bEnableCameraDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("CameraControlComponent: Lock-on target cleared, camera state reset to Normal"));
	}
}

void UCameraControlComponent::ResetCameraToDefault()
{
	// 清除锁定目标
	ClearLockOnTarget();
	
	// 重置FreeLook
	ResetFreeLook();
	
	// 重置相机到角色前方
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	APlayerController* PlayerController = GetOwnerController();
	
	if (OwnerCharacter && PlayerController)
	{
		FRotator CharacterRotation = OwnerCharacter->GetActorRotation();
		PlayerController->SetControlRotation(CharacterRotation);
	}
	
	// 重置相机臂长度
	USpringArmComponent* SpringArm = GetSpringArmComponent();
	if (SpringArm && BaseArmLength > 0.0f)
	{
		SpringArm->TargetArmLength = BaseArmLength;
		bArmLengthAdjusted = false;
	}
	
	if (bEnableCameraDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("Camera reset to default state"));
	}
}

// ==================== 从MyCharacter迁移的函数实现（保持向前兼容） ====================

void UCameraControlComponent::UpdateCharacterRotationToTarget()
{
	if (!CurrentLockOnTarget || !IsValid(CurrentLockOnTarget))
		return;
	
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return;
	
	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = GetOptimalLockOnPosition(CurrentLockOnTarget);
	
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(PlayerLocation, TargetLocation);
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	
	FRotator CharacterRotation = FMath::RInterpTo(OwnerCharacter->GetActorRotation(), 
		FRotator(0, LookAtRotation.Yaw, 0), DeltaTime, CHARACTER_ROTATION_SPEED);
	OwnerCharacter->SetActorRotation(CharacterRotation);
}

void UCameraControlComponent::StartSmoothCameraReset()
{
	APlayerController* PlayerController = GetOwnerController();
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!PlayerController || !OwnerCharacter)
		return;
	
	bIsSmoothCameraReset = true;
	SmoothResetStartTime = GetWorld()->GetTimeSeconds();
	SmoothResetStartRotation = PlayerController->GetControlRotation();
	SmoothResetTargetRotation = OwnerCharacter->GetActorRotation();
	
	UpdateCameraState(ECameraState::SmoothReset);
	
	if (bEnableCameraDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("StartSmoothCameraReset: Started smooth camera reset"));
	}
}

void UCameraControlComponent::UpdateSmoothCameraReset()
{
	if (!bIsSmoothCameraReset)
		return;
	
	APlayerController* PlayerController = GetOwnerController();
	if (!PlayerController)
		return;
	
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	FRotator CurrentRotation = PlayerController->GetControlRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, SmoothResetTargetRotation, 
		DeltaTime, CAMERA_RESET_SPEED);
	
	PlayerController->SetControlRotation(NewRotation);
	
	// 检查是否完成
	if (IsInterpolationComplete(NewRotation, SmoothResetTargetRotation, CAMERA_RESET_ANGLE_THRESHOLD))
	{
		bIsSmoothCameraReset = false;
		UpdateCameraState(ECameraState::Normal);
		OnCameraResetCompleted.Broadcast();
		
		if (bEnableCameraDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("UpdateSmoothCameraReset: Smooth camera reset completed"));
		}
	}
}

void UCameraControlComponent::StartCameraReset(const FRotator& TargetRotation)
{
	APlayerController* PlayerController = GetOwnerController();
	if (!PlayerController)
		return;
	
	bIsSmoothCameraReset = true;
	SmoothResetStartTime = GetWorld()->GetTimeSeconds();
	SmoothResetStartRotation = PlayerController->GetControlRotation();
	SmoothResetTargetRotation = TargetRotation;
	
	UpdateCameraState(ECameraState::SmoothReset);
	
	if (bEnableCameraDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("StartCameraReset: Started camera reset to target rotation"));
	}
}

void UCameraControlComponent::PerformSimpleCameraReset()
{
	APlayerController* PlayerController = GetOwnerController();
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!PlayerController || !OwnerCharacter)
		return;
	
	FRotator CharacterRotation = OwnerCharacter->GetActorRotation();
	PlayerController->SetControlRotation(CharacterRotation);
	
	UpdateCameraState(ECameraState::Normal);
	OnCameraResetCompleted.Broadcast();
	
	if (bEnableCameraDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("PerformSimpleCameraReset: Camera reset completed"));
	}
}

void UCameraControlComponent::StartCameraAutoCorrection(AActor* Target)
{
	if (!Target || !ValidateTarget(Target))
		return;
	
	APlayerController* PlayerController = GetOwnerController();
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!PlayerController || !OwnerCharacter)
		return;
	
	bIsCameraAutoCorrection = true;
	CameraCorrectionStartTime = GetWorld()->GetTimeSeconds();
	CameraCorrectionStartRotation = PlayerController->GetControlRotation();
	
	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FRotator DirectionToTarget = UKismetMathLibrary::FindLookAtRotation(PlayerLocation, TargetLocation);
	
	CameraCorrectionTargetRotation = DirectionToTarget;
	
	UpdateCameraState(ECameraState::AutoCorrection);
	OnCameraCorrectionStarted.Broadcast(Target);
	
	if (bEnableCameraDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("StartCameraAutoCorrection: Started auto correction to target: %s"), *Target->GetName());
	}
}

void UCameraControlComponent::UpdateCameraAutoCorrection()
{
	if (!bIsCameraAutoCorrection)
		return;
	
	APlayerController* PlayerController = GetOwnerController();
	if (!PlayerController)
		return;
	
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	FRotator CurrentRotation = PlayerController->GetControlRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, CameraCorrectionTargetRotation, 
		DeltaTime, CAMERA_AUTO_CORRECTION_SPEED);
	
	PlayerController->SetControlRotation(NewRotation);
	
	// 检查是否完成
	if (IsInterpolationComplete(NewRotation, CameraCorrectionTargetRotation, LOCK_COMPLETION_THRESHOLD))
	{
		bIsCameraAutoCorrection = false;
		UpdateCameraState(ECameraState::LockedOn);
		
		if (bEnableCameraDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("UpdateCameraAutoCorrection: Auto correction completed"));
		}
	}
}

void UCameraControlComponent::StartCameraCorrectionForTarget(AActor* Target)
{
	if (!Target)
		return;
	
	DelayedCorrectionTarget = Target;
	StartCameraAutoCorrection(Target);
}

void UCameraControlComponent::DelayedCameraCorrection()
{
	if (DelayedCorrectionTarget && IsValid(DelayedCorrectionTarget))
	{
		StartCameraAutoCorrection(DelayedCorrectionTarget);
		DelayedCorrectionTarget = nullptr;
	}
}

void UCameraControlComponent::RestoreCameraFollowState()
{
	// 恢复相机跟随状态
	bShouldCameraFollowTarget = true;
	bShouldCharacterRotateToTarget = true;
	
	// 清除延迟修正目标
	DelayedCorrectionTarget = nullptr;
	
	// 停止自动修正
	if (bIsCameraAutoCorrection)
	{
		bIsCameraAutoCorrection = false;
	}
	
	// 停止平滑切换
	if (bIsSmoothSwitching)
	{
		bIsSmoothSwitching = false;
		bShouldSmoothSwitchCamera = false;
		bShouldSmoothSwitchCharacter = false;
	}
	
	if (bEnableCameraDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("Camera follow state restored"));
	}
}

// ==================== FreeLook函数实现 ====================

void UCameraControlComponent::ApplyFreeLookInput(float YawInput, float PitchInput)
{
	if (!FreeLookSettings.bEnableFreeLook || !CurrentLockOnTarget)
		return;
	
	// 更新FreeLook偏移
	FreeLookOffset.Yaw = FMath::Clamp(FreeLookOffset.Yaw + YawInput, -FreeLookSettings.HorizontalLimit, FreeLookSettings.HorizontalLimit);
	FreeLookOffset.Pitch = FMath::Clamp(FreeLookOffset.Pitch + PitchInput, -FreeLookSettings.VerticalLimit, FreeLookSettings.VerticalLimit);
	
	// 记录最后输入时间
	if (FMath::Abs(YawInput) > 0.01f || FMath::Abs(PitchInput) > 0.01f)
	{
		LastFreeLookInputTime = GetWorld()->GetTimeSeconds();
		bIsFreeLooking = true;
	}
}

void UCameraControlComponent::ResetFreeLook()
{
	FreeLookOffset = FRotator::ZeroRotator;
	bIsFreeLooking = false;
}

void UCameraControlComponent::UpdateAdvancedCameraAdjustment()
{
	if (!CurrentLockOnTarget || !IsValid(CurrentLockOnTarget))
		return;
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastAdvancedAdjustmentTime < ADVANCED_ADJUSTMENT_INTERVAL)
		return;
	
	LastAdvancedAdjustmentTime = CurrentTime;
	
	EEnemySizeCategory SizeCategory = GetTargetSizeCategoryV2(CurrentLockOnTarget);
	float Distance = CalculateDistanceToTarget(CurrentLockOnTarget);
	
	FVector AdjustedLocation = CalculateAdvancedTargetLocation(CurrentLockOnTarget, SizeCategory, Distance);
	
	OnCameraAdjusted.Broadcast(SizeCategory, Distance, AdjustedLocation);
	
	if (bEnableAdvancedAdjustmentDebugLogs)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Advanced Camera Adjustment: Size=%s, Distance=%.1f"), 
			*UEnum::GetValueAsString(SizeCategory), Distance);
	}
}

FVector UCameraControlComponent::CalculateAdvancedTargetLocation(AActor* Target, EEnemySizeCategory SizeCategory, float Distance) const
{
	if (!Target)
		return FVector::ZeroVector;
	
	FVector BaseLocation = GetTargetBoundsCenter(Target);
	FVector SizeOffset = CalculateSizeBasedOffset(Target, SizeCategory);
	FVector TerrainCompensation = FVector::ZeroVector;
	
	if (AdvancedCameraSettings.bEnableTerrainHeightCompensation)
	{
		TerrainCompensation = ApplyTerrainHeightCompensation(BaseLocation, Target);
	}
	
	return BaseLocation + SizeOffset + TerrainCompensation;
}

// ==================== 辅助函数实现 ====================

ACharacter* UCameraControlComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

APlayerController* UCameraControlComponent::GetOwnerController() const
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (OwnerCharacter)
	{
		return Cast<APlayerController>(OwnerCharacter->GetController());
	}
	return nullptr;
}

USpringArmComponent* UCameraControlComponent::GetSpringArmComponent() const
{
	// 首先尝试新的安全获取方法
	if (USpringArmComponent* SpringArm = GetSpringArmComponentSafe())
	{
		return SpringArm;
	}
	
	// 保留原有逻辑作为后备
	if (AActor* Owner = GetOwner())
	{
		if (ACharacter* Character = Cast<ACharacter>(Owner))
		{
			if (UPrimitiveComponent* RootComp = Character->GetCapsuleComponent())
			{
				TArray<USceneComponent*> Children;
				RootComp->GetChildrenComponents(false, Children);
				for (USceneComponent* Child : Children)
				{
					if (USpringArmComponent* SpringArm = Cast<USpringArmComponent>(Child))
					{
						return SpringArm;
					}
				}
			}
		}
	}
	return nullptr;
}

UCameraComponent* UCameraControlComponent::GetCameraComponent() const
{
	// 首先尝试新的安全获取方法
	if (UCameraComponent* Camera = GetCameraComponentSafe())
	{
		return Camera;
	}
	
	// 保留原有逻辑作为后备
	if (USpringArmComponent* SpringArm = GetSpringArmComponent())
	{
		TArray<USceneComponent*> Children;
		SpringArm->GetChildrenComponents(false, Children);
		for (USceneComponent* Child : Children)
		{
			if (UCameraComponent* Camera = Cast<UCameraComponent>(Child))
			{
				return Camera;
			}
		}
	}
	return nullptr;
}

float UCameraControlComponent::CalculateAngleToTarget(AActor* Target) const
{
	if (!Target)
		return 0.0f;
	
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return 0.0f;
	
	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector PlayerForward = OwnerCharacter->GetActorForwardVector();
	FVector DirectionToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();
	
	float DotProduct = FVector::DotProduct(PlayerForward, DirectionToTarget);
	return FMath::Acos(DotProduct) * (180.0f / PI);
}

float UCameraControlComponent::CalculateDirectionAngle(AActor* Target) const
{
	if (!Target)
		return 0.0f;
	
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return 0.0f;
	
	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector DirectionToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();
	
	FRotator PlayerRotation = OwnerCharacter->GetActorRotation();
	FRotator DirectionRotation = DirectionToTarget.Rotation();
	
	float AngleDiff = DirectionRotation.Yaw - PlayerRotation.Yaw;
	return NormalizeAngleDifference(AngleDiff);
}

EEnemySizeCategory UCameraControlComponent::GetTargetSizeCategory(AActor* Target) const
{
	return GetTargetSizeCategoryV2(Target);
}

EEnemySizeCategory UCameraControlComponent::GetTargetSizeCategoryV2(AActor* Target) const
{
	if (!Target)
		return EEnemySizeCategory::Unknown;
	
	// 获取Actor的边界盒
	FVector Origin, BoxExtent;
	Target->GetActorBounds(false, Origin, BoxExtent);
	
	float Height = BoxExtent.Z * 2.0f;
	float Volume = BoxExtent.X * BoxExtent.Y * BoxExtent.Z * 8.0f;
	
	// 基于体积和高度的分类
	if (Height > 500.0f || Volume > 1000000.0f)
		return EEnemySizeCategory::Giant;
	else if (Height > 300.0f || Volume > 300000.0f)
		return EEnemySizeCategory::Large;
	else if (Height > 150.0f || Volume > 100000.0f)
		return EEnemySizeCategory::Medium;
	else
		return EEnemySizeCategory::Small;
}

float UCameraControlComponent::CalculateDistanceToTarget(AActor* Target) const
{
	if (!Target)
		return 0.0f;
	
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return 0.0f;
	
	return FVector::Dist(OwnerCharacter->GetActorLocation(), Target->GetActorLocation());
}

float UCameraControlComponent::GetCameraSpeedMultiplierForDistance(float Distance) const
{
	// 距离越远，插值速度越快
	if (Distance > 1500.0f)
		return 2.0f;
	else if (Distance > 800.0f)
		return 1.5f;
	else
		return 1.0f;
}

FVector UCameraControlComponent::GetHeightOffsetForEnemySize(EEnemySizeCategory SizeCategory) const
{
	switch (SizeCategory)
	{
	case EEnemySizeCategory::Giant:
		return FVector(0, 0, 200.0f);
	case EEnemySizeCategory::Large:
		return FVector(0, 0, 100.0f);
	case EEnemySizeCategory::Medium:
		return FVector(0, 0, 50.0f);
	case EEnemySizeCategory::Small:
		return FVector(0, 0, 0.0f);
	default:
		return FVector::ZeroVector;
	}
}

void UCameraControlComponent::UpdateCameraState(ECameraState NewState)
{
	if (CurrentCameraState != NewState)
	{
		CurrentCameraState = NewState;
		
		if (bEnableCameraDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("Camera state changed to: %s"), 
				*UEnum::GetValueAsString(NewState));
		}
	}
}

bool UCameraControlComponent::ShouldInterruptAutoControl(float TurnInput, float LookUpInput) const
{
	const float InputThreshold = 0.1f;
	return FMath::Abs(TurnInput) > InputThreshold || FMath::Abs(LookUpInput) > InputThreshold;
}

int32 UCameraControlComponent::GetSizeCategoryLevel(EEnemySizeCategory Category) const
{
	switch (Category)
	{
	case EEnemySizeCategory::Small: return 0;
	case EEnemySizeCategory::Medium: return 1;
	case EEnemySizeCategory::Large: return 2;
	case EEnemySizeCategory::Giant: return 3;
	default: return -1;
	}
}

bool UCameraControlComponent::ShouldUseCameraMovementForSizeChange(EEnemySizeCategory CurrentSize, EEnemySizeCategory NewSize) const
{
	int32 CurrentLevel = GetSizeCategoryLevel(CurrentSize);
	int32 NewLevel = GetSizeCategoryLevel(NewSize);
	return FMath::Abs(NewLevel - CurrentLevel) <= 1;
}

float UCameraControlComponent::GetActorBoundingHeight(AActor* Actor) const
{
	if (!Actor)
		return 0.0f;
	
	FVector Origin, BoxExtent;
	Actor->GetActorBounds(false, Origin, BoxExtent);
	return BoxExtent.Z * 2.0f;
}

bool UCameraControlComponent::IsPlayerBehindTarget(AActor* Target) const
{
	if (!Target)
		return false;
	
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return false;
	
	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector TargetForward = Target->GetActorForwardVector();
	FVector DirectionToPlayer = (PlayerLocation - TargetLocation).GetSafeNormal();
	
	float DotProduct = FVector::DotProduct(TargetForward, DirectionToPlayer);
	return DotProduct < 0.0f;
}

bool UCameraControlComponent::IsOwnerOccludingSocket(const FVector& SocketWorldLocation) const
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	UCameraComponent* Camera = GetCameraComponent();
	if (!OwnerCharacter || !Camera)
		return false;
	
	FVector CameraLocation = Camera->GetComponentLocation();
	FVector DirectionToSocket = (SocketWorldLocation - CameraLocation).GetSafeNormal();
	float DistanceToSocket = FVector::Dist(CameraLocation, SocketWorldLocation);
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CameraLocation,
		SocketWorldLocation,
		ECC_Visibility,
		QueryParams
	);
	
	return bHit && HitResult.Distance < DistanceToSocket * 0.5f;
}

void UCameraControlComponent::AdjustSpringArmForSizeDistance(EEnemySizeCategory SizeCategory, float Distance, float BoundingHeight)
{
	USpringArmComponent* SpringArm = GetSpringArmComponent();
	if (!SpringArm)
		return;
	
	float TargetArmLength = BaseArmLength;
	
	// 根据尺寸调整
	switch (SizeCategory)
	{
	case EEnemySizeCategory::Giant:
		TargetArmLength *= 1.5f;
		break;
	case EEnemySizeCategory::Large:
		TargetArmLength *= 1.2f;
		break;
	case EEnemySizeCategory::Medium:
		TargetArmLength *= 1.0f;
		break;
	case EEnemySizeCategory::Small:
		TargetArmLength *= 0.9f;
		break;
	default:
		break;
	}
	
	// 根据距离调整
	if (Distance < 300.0f)
		TargetArmLength *= 0.8f;
	else if (Distance > 1000.0f)
		TargetArmLength *= 1.2f;
	
	SpringArm->TargetArmLength = TargetArmLength;
	bArmLengthAdjusted = true;
}

// ==================== 私有辅助函数实现 ====================

void UCameraControlComponent::PerformCameraInterpolation(const FRotator& TargetRotation, float InterpSpeed)
{
	APlayerController* PlayerController = GetOwnerController();
	if (!PlayerController)
		return;
	
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	FRotator CurrentRotation = PlayerController->GetControlRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, InterpSpeed);
	PlayerController->SetControlRotation(NewRotation);
}

void UCameraControlComponent::PerformCharacterRotationInterpolation(const FRotator& TargetRotation, float InterpSpeed)
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return;
	
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, InterpSpeed);
	OwnerCharacter->SetActorRotation(NewRotation);
}

bool UCameraControlComponent::IsInterpolationComplete(const FRotator& Current, const FRotator& Target, float Threshold) const
{
	return (Current - Target).IsNearlyZero(Threshold);
}

float UCameraControlComponent::NormalizeAngleDifference(float AngleDiff) const
{
	while (AngleDiff > 180.0f)
		AngleDiff -= 360.0f;
	while (AngleDiff < -180.0f)
		AngleDiff += 360.0f;
	return AngleDiff;
}

float UCameraControlComponent::CalculateDistanceWeight(float Distance, float MaxDistance) const
{
	return FMath::Clamp(1.0f - (Distance / MaxDistance), 0.0f, 1.0f);
}

FVector UCameraControlComponent::ApplyTerrainHeightCompensation(const FVector& BaseLocation, AActor* Target) const
{
	if (!Target)
		return FVector::ZeroVector;
	
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return FVector::ZeroVector;
	
	float PlayerZ = OwnerCharacter->GetActorLocation().Z;
	float TargetZ = Target->GetActorLocation().Z;
	float HeightDiff = TargetZ - PlayerZ;
	
	// 如果目标更高，稍微降低锁定点
	if (HeightDiff > 100.0f)
		return FVector(0, 0, -HeightDiff * 0.3f);
	else if (HeightDiff < -100.0f)
		return FVector(0, 0, -HeightDiff * 0.2f);
	
	return FVector::ZeroVector;
}

bool UCameraControlComponent::ValidateTarget(AActor* Target) const
{
	return Target != nullptr && IsValid(Target) && !Target->IsPendingKill();
}

void UCameraControlComponent::InitializeDefaultCameraConfigs()
{
	// 初始化默认的相机类型配置
	FCameraTypeConfig SmallConfig;
	SmallConfig.HeightOffset = 0.0f;
	SmallConfig.CameraDistance = 400.0f;
	SmallConfig.TrackingSpeed = 5.0f;
	CameraTypeConfigs.Add(EEnemyCameraType::Small, SmallConfig);
	
	FCameraTypeConfig MediumConfig;
	MediumConfig.HeightOffset = 50.0f;
	MediumConfig.CameraDistance = 500.0f;
	MediumConfig.TrackingSpeed = 5.5f;
	CameraTypeConfigs.Add(EEnemyCameraType::Medium, MediumConfig);
	
	FCameraTypeConfig LargeConfig;
	LargeConfig.HeightOffset = 100.0f;
	LargeConfig.CameraDistance = 600.0f;
	LargeConfig.TrackingSpeed = 6.0f;
	CameraTypeConfigs.Add(EEnemyCameraType::Large, LargeConfig);
	
	FCameraTypeConfig GiantConfig;
	GiantConfig.HeightOffset = 200.0f;
	GiantConfig.CameraDistance = 800.0f;
	GiantConfig.TrackingSpeed = 6.5f;
	CameraTypeConfigs.Add(EEnemyCameraType::Giant, GiantConfig);
}

EEnemyCameraType UCameraControlComponent::GetEnemyCameraTypeFromActor(AActor* Target) const
{
	if (!Target)
		return EEnemyCameraType::Medium;
	
	EEnemySizeCategory SizeCategory = GetTargetSizeCategoryV2(Target);
	
	switch (SizeCategory)
	{
	case EEnemySizeCategory::Small: return EEnemyCameraType::Small;
	case EEnemySizeCategory::Medium: return EEnemyCameraType::Medium;
	case EEnemySizeCategory::Large: return EEnemyCameraType::Large;
	case EEnemySizeCategory::Giant: return EEnemyCameraType::Giant;
	default: return EEnemyCameraType::Medium;
	}
}

FVector UCameraControlComponent::PreCalculateFinalTargetLocation(AActor* Target) const
{
	if (!Target)
		return FVector::ZeroVector;
	
	return GetOptimalLockOnPosition(Target);
}

void UCameraControlComponent::UpdateCachedTargetLocation(AActor* Target, float DeltaTime)
{
	if (!Target)
		return;
	
	// 检测目标切换
	if (LastFrameTarget != Target)
	{
		CachedTargetLocation = Target->GetActorLocation();
		LastFrameTarget = Target;
		return;
	}
	
	// 平滑插值更新缓存位置
	FVector TargetLocation = Target->GetActorLocation();
	float InterpSpeed = 10.0f;
	CachedTargetLocation = FMath::VInterpTo(CachedTargetLocation, TargetLocation, DeltaTime, InterpSpeed);
}

FVector UCameraControlComponent::GetOptimalLockOnPosition(AActor* Target) const
{
	if (!Target)
		return FVector::ZeroVector;
	
	EEnemySizeCategory SizeCategory = GetTargetSizeCategoryV2(Target);
	FVector BaseLocation = GetTargetBoundsCenter(Target);
	FVector SizeOffset = CalculateSizeBasedOffset(Target, SizeCategory);
	
	// === 修复点：添加 TargetLocationOffset ===
	// 原代码：return BaseLocation + SizeOffset;
	// 新代码：加入配置的偏移
	FVector FinalLocation = BaseLocation + SizeOffset + CameraSettings.TargetLocationOffset;
	
	// 验证日志（用于确认修复生效）
	static int32 LogCounter = 0;
	if (LogCounter++ % 60 == 0) // 每60帧记录一次，避免日志爆炸
	{
		UE_LOG(LogTemp, Warning, TEXT("GetOptimalLockOnPosition: Applied TargetLocationOffset = %s"), 
			*CameraSettings.TargetLocationOffset.ToString());
	}
	
	// 如果启用了高级设置，应用额外的偏移
	if (AdvancedCameraSettings.bEnableEnemySizeAdaptation)
	{
		FVector AdditionalOffset = FVector::ZeroVector;
		switch (SizeCategory)
		{
		case EEnemySizeCategory::Small:
			AdditionalOffset = AdvancedCameraSettings.SmallEnemyHeightOffset;
			break;
		case EEnemySizeCategory::Medium:
			AdditionalOffset = AdvancedCameraSettings.MediumEnemyHeightOffset;
			break;
		case EEnemySizeCategory::Large:
			AdditionalOffset = AdvancedCameraSettings.LargeEnemyHeightOffset;
			break;
		default:
			break;
		}
		FinalLocation += AdditionalOffset;
		
		// 验证日志
		if (LogCounter % 60 == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("GetOptimalLockOnPosition: Applied size offset for %s"), 
				*UEnum::GetValueAsString(SizeCategory));
		}
	}
	
	return FinalLocation;
}

FVector UCameraControlComponent::CalculateSizeBasedOffset(AActor* Target, EEnemySizeCategory SizeCategory) const
{
	return GetHeightOffsetForEnemySize(SizeCategory);
}

FVector UCameraControlComponent::GetTargetBoundsCenter(AActor* Target) const
{
	if (!Target)
		return FVector::ZeroVector;
	
	FVector Origin, BoxExtent;
	Target->GetActorBounds(false, Origin, BoxExtent);
	return Origin;
}

FVector UCameraControlComponent::GetStableTargetLocation(AActor* Target)
{
	if (!Target)
		return FVector::ZeroVector;
	
	float DeltaTime = GetWorld()->GetDeltaSeconds();
	UpdateCachedTargetLocation(Target, DeltaTime);
	
	return CachedTargetLocation;
}

// ==================== 3D视口控制函数实现 ====================

void UCameraControlComponent::SetOrbitMode(bool bEnable)
{
	Viewport3DControl.bEnableOrbitMode = bEnable;
	
	if (bEnable)
	{
		// 初始化轨道位置
		if (CurrentLockOnTarget)
		{
			ACharacter* OwnerCharacter = GetOwnerCharacter();
			if (OwnerCharacter)
			{
				FVector ToTarget = CurrentLockOnTarget->GetActorLocation() - OwnerCharacter->GetActorLocation();
				OrbitRotation = ToTarget.Rotation();
			}
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("Orbit mode %s"), bEnable ? TEXT("enabled") : TEXT("disabled"));
}

void UCameraControlComponent::UpdateOrbitPosition(float DeltaYaw, float DeltaPitch)
{
	if (!Viewport3DControl.bEnableOrbitMode || !CurrentLockOnTarget)
		return;
	
	// 更新轨道角度
	OrbitRotation.Yaw += DeltaYaw * Viewport3DControl.OrbitSpeed;
	OrbitRotation.Pitch = FMath::Clamp(OrbitRotation.Pitch + DeltaPitch * Viewport3DControl.OrbitSpeed, -85.0f, 85.0f);
	
	// 计算新的相机位置
	FVector TargetLocation = GetOptimalLockOnPosition(CurrentLockOnTarget);
	FVector OrbitOffset = OrbitRotation.Vector() * (-Viewport3DControl.OrbitRadius);
	OrbitOffset.Z += Viewport3DControl.OrbitHeightOffset;
	
	FVector NewCameraLocation = TargetLocation + OrbitOffset;
	
	// 应用到相机
	APlayerController* PlayerController = GetOwnerController();
	if (PlayerController)
	{
		FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(NewCameraLocation, TargetLocation);
		PlayerController->SetControlRotation(LookAtRotation);
	}
}

#if WITH_EDITOR
void UCameraControlComponent::PreviewCameraPosition()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("PreviewCameraPosition: No owner actor"));
		return;
	}
	
	// 使用新的安全获取方法
	USpringArmComponent* SpringArm = GetSpringArmComponentSafe();
	UCameraComponent* Camera = GetCameraComponentSafe();
	
	if (!SpringArm)
	{
		UE_LOG(LogTemp, Error, TEXT("PreviewCameraPosition: Could not find SpringArmComponent"));
		
		// 调试信息：列出所有组件
		TArray<UActorComponent*> Components = Owner->GetComponents().Array();
		UE_LOG(LogTemp, Warning, TEXT("Total components on %s: %d"), *Owner->GetName(), Components.Num());
		for (int32 i = 0; i < FMath::Min(5, Components.Num()); i++) // 只显示前5个
		{
			if (Components[i])
			{
				UE_LOG(LogTemp, Warning, TEXT("  [%d] %s (Class: %s)"), 
					i,
					*Components[i]->GetName(), 
					*Components[i]->GetClass()->GetName());
			}
		}
		return;
	}
	
	// 相机不是必需的，只记录警告
	if (!Camera)
	{
		UE_LOG(LogTemp, Warning, TEXT("PreviewCameraPosition: Camera not found (non-critical)"));
	}
	
	// 标记预览状态
	bIsPreviewingCamera = true;
	
	// 保存当前值
	FVector OriginalOffset = SpringArm->TargetOffset;
	float OriginalLength = SpringArm->TargetArmLength;
	FRotator OriginalRotation = SpringArm->GetRelativeRotation();
	
	// 应用预览设置
	FVector OrbitOffset;
	float Yaw = FMath::DegreesToRadians(OrbitRotation.Yaw);
	float Pitch = FMath::DegreesToRadians(OrbitRotation.Pitch);
	
	OrbitOffset.X = Viewport3DControl.OrbitRadius * FMath::Cos(Pitch) * FMath::Cos(Yaw);
	OrbitOffset.Y = Viewport3DControl.OrbitRadius * FMath::Cos(Pitch) * FMath::Sin(Yaw);
	OrbitOffset.Z = Viewport3DControl.OrbitRadius * FMath::Sin(Pitch) + Viewport3DControl.OrbitHeightOffset;
	
	SpringArm->TargetOffset = OrbitOffset;
	SpringArm->TargetArmLength = Viewport3DControl.OrbitRadius;
	
	// 强制刷新
	SpringArm->MarkRenderStateDirty();
	
	UE_LOG(LogTemp, Display, TEXT("✓ Camera Preview Applied - Radius: %.1f, Yaw: %.1f, Pitch: %.1f"), 
		Viewport3DControl.OrbitRadius, OrbitRotation.Yaw, OrbitRotation.Pitch);
}

void UCameraControlComponent::ResetCameraPreview()
{
	bIsPreviewingCamera = false;
	OrbitRotation = FRotator::ZeroRotator;
	
	// 使用安全的组件获取方法
	USpringArmComponent* SpringArm = GetSpringArmComponentSafe();
	if (SpringArm && BaseArmLength > 0.0f)
	{
		SpringArm->TargetArmLength = BaseArmLength;
		SpringArm->bUsePawnControlRotation = true;
		
		// 恢复到角色前方
		ACharacter* OwnerCharacter = GetOwnerCharacter();
		if (OwnerCharacter)
		{
			SpringArm->SetWorldRotation(OwnerCharacter->GetActorRotation());
		}
		
		// 标记组件需要更新
		SpringArm->MarkRenderStateDirty();
	}
	
	// 重置相机控制器旋转
	APlayerController* PlayerController = GetOwnerController();
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (PlayerController && OwnerCharacter)
	{
		// 恢复到角色朝向
		PlayerController->SetControlRotation(OwnerCharacter->GetActorRotation());
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Camera preview reset - Arm Length restored to: %.1f"), BaseArmLength);
}

void UCameraControlComponent::RotatePreviewLeft()
{
	if (!bIsPreviewingCamera)
	{
		// 如果没有激活预览，先激活
		PreviewCameraPosition();
		return;
	}
	
	OrbitRotation.Yaw -= 15.0f;
	PreviewCameraPosition(); // 重新应用预览
}

void UCameraControlComponent::RotatePreviewRight()
{
	if (!bIsPreviewingCamera)
	{
		PreviewCameraPosition();
		return;
	}
	
	OrbitRotation.Yaw += 15.0f;
	PreviewCameraPosition();
}

void UCameraControlComponent::RotatePreviewUp()
{
	if (!bIsPreviewingCamera)
	{
		PreviewCameraPosition();
		return;
	}
	
	OrbitRotation.Pitch = FMath::Clamp(OrbitRotation.Pitch - 10.0f, -85.0f, 85.0f);
	PreviewCameraPosition();
}

void UCameraControlComponent::RotatePreviewDown()
{
	if (!bIsPreviewingCamera)
	{
		PreviewCameraPosition();
		return;
	}
	
	OrbitRotation.Pitch = FMath::Clamp(OrbitRotation.Pitch + 10.0f, -85.0f, 85.0f);
	PreviewCameraPosition();
}
#endif

// ==================== 编辑器安全组件获取函数 ====================

USpringArmComponent* UCameraControlComponent::GetSpringArmComponentSafe() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	
	// 方法1：直接通过类型查找（运行时优先）
	USpringArmComponent* SpringArm = Owner->FindComponentByClass<USpringArmComponent>();
	if (SpringArm)
	{
		return SpringArm;
	}
	
	// 方法2：通过常见名称查找（编辑器友好）
	TArray<FString> CommonNames = {
		TEXT("CameraBoom"),
		TEXT("SpringArm"),
		TEXT("CameraArm"),
		TEXT("Boom")
	};
	
	for (const FString& Name : CommonNames)
	{
		USpringArmComponent* FoundComponent = Cast<USpringArmComponent>(
			Owner->GetDefaultSubobjectByName(FName(*Name)));
		
		if (FoundComponent)
		{
			UE_LOG(LogTemp, Log, TEXT("GetSpringArmComponentSafe: Found SpringArm by name '%s'"), *Name);
			return FoundComponent;
		}
	}
	
	// 方法3：遍历所有组件查找（最后手段）
	TArray<UActorComponent*> Components;
	Owner->GetComponents(USpringArmComponent::StaticClass(), Components);
	
	if (Components.Num() > 0)
	{
		USpringArmComponent* Found = Cast<USpringArmComponent>(Components[0]);
		if (Found)
		{
			UE_LOG(LogTemp, Log, TEXT("GetSpringArmComponentSafe: Found SpringArm by component iteration"));
			return Found;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("GetSpringArmComponentSafe: No SpringArmComponent found on '%s'"), 
		*Owner->GetName());
	return nullptr;
}

UCameraComponent* UCameraControlComponent::GetCameraComponentSafe() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	
	// 方法1：直接通过类型查找（运行时优先）
	UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>();
	if (Camera)
	{
		return Camera;
	}
	
	// 方法2：通过常见名称查找（编辑器友好）
	TArray<FString> CommonNames = {
		TEXT("FollowCamera"),
		TEXT("Camera"),
		TEXT("PlayerCamera"),
		TEXT("ThirdPersonCamera")
	};
	
	for (const FString& Name : CommonNames)
	{
		UCameraComponent* FoundComponent = Cast<UCameraComponent>(
			Owner->GetDefaultSubobjectByName(FName(*Name)));
		
		if (FoundComponent)
		{
			UE_LOG(LogTemp, Log, TEXT("GetCameraComponentSafe: Found Camera by name '%s'"), *Name);
			return FoundComponent;
		}
	}
	
	// 方法3：遍历所有组件查找（最后手段）
	TArray<UActorComponent*> Components;
	Owner->GetComponents(UCameraComponent::StaticClass(), Components);
	
	if (Components.Num() > 0)
	{
		UCameraComponent* Found = Cast<UCameraComponent>(Components[0]);
		if (Found)
		{
			UE_LOG(LogTemp, Log, TEXT("GetCameraComponentSafe: Found Camera by component iteration"));
			return Found;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("GetCameraComponentSafe: No CameraComponent found on '%s'"), 
		*Owner->GetName());
	return nullptr;
}