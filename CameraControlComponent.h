// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnConfig.h"
#include "CameraControlComponent.generated.h"

// 前向声明
class ACharacter;
class APlayerController;
class USpringArmComponent;
class UCameraComponent;
class USphereComponent;

// 相机状态枚举
UENUM(BlueprintType)
enum class ECameraState : uint8
{
	Normal				UMETA(DisplayName = "Normal"),
	LockedOn			UMETA(DisplayName = "Locked On"),
	SmoothSwitching		UMETA(DisplayName = "Smooth Switching"),
	AutoCorrection		UMETA(DisplayName = "Auto Correction"),
	SmoothReset			UMETA(DisplayName = "Smooth Reset"),
	AdvancedAdjustment	UMETA(DisplayName = "Advanced Adjustment")
};

// 相机切换完成事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraSwitchCompleted, AActor*, NewTarget);

// 相机重置完成事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCameraResetCompleted);

// 相机自动修正开始事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraCorrectionStarted, AActor*, Target);

// 高级相机调整事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAdvancedCameraAdjusted, EEnemySizeCategory, TargetSize, float, Distance, FVector, AdjustedLocation);

// FreeLook设置结构
USTRUCT(BlueprintType)
struct SOUL_API FFreeLookSettings
{
	GENERATED_BODY()
	
	/** 是否启用自由视角 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook")
	bool bEnableFreeLook;
	
	/** 水平旋转限制 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float HorizontalLimit;
	
	/** 垂直旋转限制 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float VerticalLimit;
	
	/** 返回中心速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float ReturnToCenterSpeed;
	
	/** 自动返回延迟 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float AutoReturnDelay;
	
	FFreeLookSettings()
		: bEnableFreeLook(true)
		, HorizontalLimit(90.0f)
		, VerticalLimit(45.0f)
		, ReturnToCenterSpeed(3.0f)
		, AutoReturnDelay(1.0f)
	{
	}
};

// 3D视口控制设置结构
USTRUCT(BlueprintType)
struct SOUL_API FViewport3DControl
{
	GENERATED_BODY()
	
	/** 轨道旋转半径 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3D Control", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
	float OrbitRadius;
	
	/** 轨道高度偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3D Control")
	float OrbitHeightOffset;
	
	/** 轨道旋转速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3D Control", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float OrbitSpeed;
	
	/** 启用轨道模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3D Control")
	bool bEnableOrbitMode;
	
	FViewport3DControl()
		: OrbitRadius(400.0f)
		, OrbitHeightOffset(100.0f)
		, OrbitSpeed(2.0f)
		, bEnableOrbitMode(false)
	{
	}
};

/**
 * 相机控制组件类
 * 负责处理相机跟踪、目标切换、自动修正和高级相机调整功能
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SOUL_API UCameraControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraControlComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== 事件委托 ====================
	/** 相机切换完成时触发 */
	UPROPERTY(BlueprintAssignable, Category = "Camera Control Events")
	FOnCameraSwitchCompleted OnCameraSwitchCompleted;

	/** 相机重置完成时触发 */
	UPROPERTY(BlueprintAssignable, Category = "Camera Control Events")
	FOnCameraResetCompleted OnCameraResetCompleted;

	/** 相机自动修正开始时触发 */
	UPROPERTY(BlueprintAssignable, Category = "Camera Control Events")
	FOnCameraCorrectionStarted OnCameraCorrectionStarted;

	/** 高级相机调整时触发 */
	UPROPERTY(BlueprintAssignable, Category = "Camera Control Events")
	FOnAdvancedCameraAdjusted OnCameraAdjusted;

	// ==================== 配置参数 ====================
	/** 基础相机设置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	FCameraSettings CameraSettings;

	/** 高级相机设置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Camera Settings")
	FAdvancedCameraSettings AdvancedCameraSettings;

	/** FreeLook设置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook Settings")
	FFreeLookSettings FreeLookSettings;
	
	/** 应用FreeLook输入 */
	UFUNCTION(BlueprintCallable, Category = "FreeLook")
	void ApplyFreeLookInput(float YawInput, float PitchInput);
	
	/** 重置FreeLook */
	UFUNCTION(BlueprintCallable, Category = "FreeLook")
	void ResetFreeLook();

	// ==================== 调试控制 ====================
	/** 是否启用相机调试日志 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableCameraDebugLogs;

	/** 是否启用高级相机调整调试日志 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableAdvancedAdjustmentDebugLogs;

protected:
	// ==================== 内部状态变量 ====================
	/** 当前相机状态 */
	UPROPERTY()
	ECameraState CurrentCameraState;

	/** 当前锁定目标 */
	UPROPERTY()
	AActor* CurrentLockOnTarget;

	/** 上一个锁定目标（用于切换逻辑） */
	UPROPERTY()
	AActor* PreviousLockOnTarget;

	/** 是否应让相机跟随目标 */
	UPROPERTY()
	bool bShouldCameraFollowTarget;

	/** 是否应让角色转向目标 */
	UPROPERTY()
	bool bShouldCharacterRotateToTarget;

	/** 玩家是否正在移动 */
	UPROPERTY()
	bool bPlayerIsMoving;

	// ==================== 小角度近UI切换状态 ====================
	/** 是否正在进行小角度近距UI切换 */
	bool bIsMinimalChangeSwitch = false;

	/** 小角度近距UI切换开始时间 */
	float MinimalChangeSwitchTime = 0.0f;

	// ==================== 记录玩家移动时机 ====================
	/** 玩家上一次移动时间 */
	float LastPlayerMovementTime = 0.0f;

	// ==================== 平滑切换相关变量 ====================
	/** 是否正在进行平滑切换 */
	UPROPERTY()
	bool bIsSmoothSwitching;

	/** 平滑切换开始时间 */
	float SmoothSwitchStartTime;

	/** 平滑切换开始旋转 */
	FRotator SmoothSwitchStartRotation;

	/** 平滑切换目标旋转 */
	FRotator SmoothSwitchTargetRotation;

	/** 相机是否需要平滑切换 */
	bool bShouldSmoothSwitchCamera;

	/** 角色是否需要平滑切换 */
	bool bShouldSmoothSwitchCharacter;

	// ==================== 自动修正相关变量 ====================
	/** 是否正在进行相机自动修正 */
	UPROPERTY()
	bool bIsCameraAutoCorrection;

	/** 修正开始时间 */
	float CameraCorrectionStartTime;

	/** 修正开始的相机旋转 */
	FRotator CameraCorrectionStartRotation;

	/** 修正目标相机旋转 */
	FRotator CameraCorrectionTargetRotation;

	/** 延迟修正的目标物体 */
	UPROPERTY()
	AActor* DelayedCorrectionTarget;

	// ==================== 平滑重置相关变量 ====================
	/** 是否正在进行平滑相机重置 */
	bool bIsSmoothCameraReset;

	/** 平滑重置开始时间 */
	float SmoothResetStartTime;

	/** 平滑重置开始的相机旋转 */
	FRotator SmoothResetStartRotation;

	/** 平滑重置目标相机旋转 */
	FRotator SmoothResetTargetRotation;

	// ==================== 高级相机距离响应相关变量 ====================
	/** 是否正在进行高级相机调整 */
	bool bIsAdvancedCameraAdjustment;

	/** 上次高级调整时间 */
	float LastAdvancedAdjustmentTime;

	/** 当前目标尺寸 */
	EEnemySizeCategory CurrentTargetSizeCategory;

	/** 当前目标距离 */
	float CurrentTargetDistance;

	/** 高级相机调整间隔 */
	static constexpr float ADVANCED_ADJUSTMENT_INTERVAL = 0.1f;

	// 记录相机臂原始长度，用来恢复到静态配置
	float BaseArmLength = 0.0f;

	// 是否已调整相机臂（避免多次重复动态调整）
	bool bArmLengthAdjusted = false;

	// ==================== FreeLook状态 ====================
protected:
	/** FreeLook状态 */
	FRotator FreeLookOffset;
	float LastFreeLookInputTime;
	bool bIsFreeLooking;

	// ==================== 3D视口控制状态 ====================
	/** 轨道角度 */
	FRotator OrbitRotation;
	
	/** 是否正在预览 */
	bool bIsPreviewingCamera;

	// ==================== 目标位置稳定插值系统 ====================
	/** 是否启用目标位置稳定插值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	bool bEnableTargetStableInterpolation = true;

	/** 缓存的目标位置（用于平滑插值） */
	FVector CachedTargetLocation = FVector::ZeroVector;

	/** 上一帧的目标Actor（用于检测目标切换） */
	UPROPERTY()
	AActor* LastFrameTarget = nullptr;

	// ==================== 递归防护系统 ====================
	/** 递归调用深度计数器 */
	mutable int32 RecursionDepth;
	
	/** 最大递归深度限制 */
	static constexpr int32 MAX_RECURSION_DEPTH = 3;
	
	/** 递归防护辅助类 */
	struct FRecursionGuard
	{
		int32& DepthCounter;
		bool bValid;
		
		FRecursionGuard(int32& InDepthCounter, int32 MaxDepth)
			: DepthCounter(InDepthCounter)
			, bValid(DepthCounter < MaxDepth)
		{
			if (bValid)
			{
				DepthCounter++;
			}
		}
		
		~FRecursionGuard()
		{
			if (bValid)
			{
				DepthCounter--;
			}
		}
		
		bool IsValid() const { return bValid; }
	};

	// ==================== 性能优化常量 ====================
	/** 目标切换角度阈值 */
	static constexpr float TARGET_SWITCH_ANGLE_THRESHOLD = 20.0f;

	/** 平滑切换速度 */
	static constexpr float TARGET_SWITCH_SMOOTH_SPEED = 3.0f;

	/** 锁定完成阈值 */
	static constexpr float LOCK_COMPLETION_THRESHOLD = 5.0f;

	/** 相机自动修正速度 */
	static constexpr float CAMERA_AUTO_CORRECTION_SPEED = 7.0f;

	/** 相机修正偏移角度 */
	static constexpr float CAMERA_CORRECTION_OFFSET = 150.0f;

	/** 相机重置速度 */
	static constexpr float CAMERA_RESET_SPEED = 7.0f;

	/** 相机重置角度阈值 */
	static constexpr float CAMERA_RESET_ANGLE_THRESHOLD = 1.0f;

	/** 角色旋转速度 */
	static constexpr float CHARACTER_ROTATION_SPEED = 10.0f;

public:
	// ==================== 主要接口函数 ====================
	
	/** 设置相机设置 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	void SetCameraSettings(const FCameraSettings& Settings);

	/** 设置高级相机设置 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	void SetAdvancedCameraSettings(const FAdvancedCameraSettings& Settings);

	/** 获取当前相机状态 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	ECameraState GetCurrentCameraState() const { return CurrentCameraState; }

	/** 获取当前锁定目标 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	AActor* GetCurrentLockOnTarget() const { return CurrentLockOnTarget; }

	// ==================== 3D视口控制 ====================
	
	/** 3D控制设置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3D Viewport Control", meta = (DisplayName = "3D Control Settings"))
	FViewport3DControl Viewport3DControl;
	
	/** 设置轨道模式 */
	UFUNCTION(BlueprintCallable, Category = "3D Control")
	void SetOrbitMode(bool bEnable);
	
	/** 更新轨道位置 */
	UFUNCTION(BlueprintCallable, Category = "3D Control")
	void UpdateOrbitPosition(float DeltaYaw, float DeltaPitch);

#if WITH_EDITOR
	/** 编辑器中预览相机位置 */
	UFUNCTION(CallInEditor, Category = "3D Control", meta = (DisplayName = "Preview Camera Position"))
	void PreviewCameraPosition();
	
	/** 重置相机预览 */
	UFUNCTION(CallInEditor, Category = "3D Control", meta = (DisplayName = "Reset Camera Preview"))
	void ResetCameraPreview();
	
	/** 编辑器中调整轨道角度 - 新增 */
	UFUNCTION(CallInEditor, Category = "3D Control", meta = (DisplayName = "Rotate Preview Left"))
	void RotatePreviewLeft();
	
	UFUNCTION(CallInEditor, Category = "3D Control", meta = (DisplayName = "Rotate Preview Right"))
	void RotatePreviewRight();
	
	UFUNCTION(CallInEditor, Category = "3D Control", meta = (DisplayName = "Rotate Preview Up"))
	void RotatePreviewUp();
	
	UFUNCTION(CallInEditor, Category = "3D Control", meta = (DisplayName = "Rotate Preview Down"))
	void RotatePreviewDown();
#endif

	// ==================== 新的目标位置计算系统 ====================
	/** 获取目标的优化锁定位置 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	FVector GetOptimalLockOnPosition(AActor* Target) const;
	
	/** 计算基于体型的目标偏移 */
	UFUNCTION(BlueprintPure, Category = "Camera Control")
	FVector CalculateSizeBasedOffset(AActor* Target, EEnemySizeCategory SizeCategory) const;
	
	/** 获取目标的边界盒中心 */
	UFUNCTION(BlueprintPure, Category = "Camera Control")
	FVector GetTargetBoundsCenter(AActor* Target) const;

	/** 获取稳定的目标位置（使用插值缓存） */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	FVector GetStableTargetLocation(AActor* Target);

	// ==================== 输入处理接口 ====================
	
	/** 处理玩家输入控制 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	void HandlePlayerInput(float TurnInput, float LookUpInput);

	/** 处理玩家移动状态 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	void HandlePlayerMovement(bool bIsMoving);

	// ==================== 从MyCharacter迁移的函数（保持向后兼容） ====================
	
	/** 更新锁定相机 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void UpdateLockOnCamera();

	/** 更新角色朝向目标的旋转 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void UpdateCharacterRotationToTarget();

	/** 开始平滑目标切换 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void StartSmoothTargetSwitch(AActor* NewTarget);

	/** 更新平滑目标切换 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void UpdateSmoothTargetSwitch();

	/** 开始平滑相机重置 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void StartSmoothCameraReset();

	/** 更新平滑相机重置 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void UpdateSmoothCameraReset();

	/** 开始相机重置 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void StartCameraReset(const FRotator& TargetRotation);

	/** 执行简单的相机重置 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void PerformSimpleCameraReset();

	/** 开始相机自动修正 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void StartCameraAutoCorrection(AActor* Target);

	/** 更新相机自动修正 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void UpdateCameraAutoCorrection();

	/** 开始针对特定目标的相机修正 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void StartCameraCorrectionForTarget(AActor* Target);

	/** 延迟相机修正 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
void DelayedCameraCorrection();

	/** 恢复相机跟随状态 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	void RestoreCameraFollowState();

	// ==================== 新增的高级相机距离响应功能 ====================
	
	/** 更新高级相机调整 */
	UFUNCTION(BlueprintCallable, Category = "Advanced Camera")
	void UpdateAdvancedCameraAdjustment();

	/** 计算高级目标位置 */
	UFUNCTION(BlueprintCallable, Category = "Advanced Camera")
	FVector CalculateAdvancedTargetLocation(AActor* Target, EEnemySizeCategory SizeCategory, float Distance) const;

	// ==================== 状态管理函数 ====================
	
	/** 设置锁定目标 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	void SetLockOnTarget(AActor* Target);

	/** 清除锁定目标 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	void ClearLockOnTarget();

	/** 重置相机到默认位置 */
	UFUNCTION(BlueprintCallable, Category = "Camera Control")
	void ResetCameraToDefault();

protected:
	// ==================== 内部辅助函数 ====================
	
	/** 获取拥有者角色 */
	ACharacter* GetOwnerCharacter() const;

	/** 获取拥有者的控制器 */
	APlayerController* GetOwnerController() const;

	/** 获取弹簧臂组件 */
	USpringArmComponent* GetSpringArmComponent() const;

	/** 获取相机组件 */
	UCameraComponent* GetCameraComponent() const;

	/** 【新增】编辑器安全的弹簧臂组件获取 */
	USpringArmComponent* GetSpringArmComponentSafe() const;

	/** 【新增】编辑器安全的相机组件获取 */
	UCameraComponent* GetCameraComponentSafe() const;

	/** 计算到目标的角度差异 */
	float CalculateAngleToTarget(AActor* Target) const;

	/** 计算目标相对于玩家的方向角度 */
	float CalculateDirectionAngle(AActor* Target) const;

	/** 获取目标的尺寸类别 */
	EEnemySizeCategory GetTargetSizeCategory(AActor* Target) const;

	/** FromSoftware式敌人体型检测（新版本） */
	EEnemySizeCategory GetTargetSizeCategoryV2(AActor* Target) const;

	/** 计算到目标的距离 */
	float CalculateDistanceToTarget(AActor* Target) const;

	/** 根据距离获取相机速度倍数 */
	float GetCameraSpeedMultiplierForDistance(float Distance) const;

	/** 根据敌人尺寸获取高度偏移 */
	FVector GetHeightOffsetForEnemySize(EEnemySizeCategory SizeCategory) const;

	/** 更新相机状态 */
	void UpdateCameraState(ECameraState NewState);

	/** 判断玩家输入是否需要中断自动控制 */
	bool ShouldInterruptAutoControl(float TurnInput, float LookUpInput) const;

	// ==================== 小角度近UI切换辅助函数 ====================
	
	/** 获取尺寸类别等级 */
	int32 GetSizeCategoryLevel(EEnemySizeCategory Category) const;

	/** 判断是否应该使用相机移动处理尺寸变化 */
	bool ShouldUseCameraMovementForSizeChange(EEnemySizeCategory CurrentSize, EEnemySizeCategory NewSize) const;

	// ==================== 相机臂长度调整辅助函数 ====================
	
	/** 获取Actor包围盒高度（Z方向*2） */
	float GetActorBoundingHeight(AActor* Actor) const;

	/** 玩家是否处于目标身后 */
	bool IsPlayerBehindTarget(AActor* Target) const;

	/** 相机到Socket是否被玩家自身遮挡 */
	bool IsOwnerOccludingSocket(const FVector& SocketWorldLocation) const;

	/** 按体型与距离微调相机臂长度 */
	void AdjustSpringArmForSizeDistance(EEnemySizeCategory SizeCategory, float Distance, float BoundingHeight);

private:
	// ==================== 私有辅助函数 ====================
	
	/** 内部函数：执行相机插值 */
	void PerformCameraInterpolation(const FRotator& TargetRotation, float InterpSpeed);

	/** 内部函数：执行角色旋转插值 */
	void PerformCharacterRotationInterpolation(const FRotator& TargetRotation, float InterpSpeed);

	/** 内部函数：检查插值是否完成 */
	bool IsInterpolationComplete(const FRotator& Current, const FRotator& Target, float Threshold = 2.0f) const;

	/** 内部函数：标准化角度差异 */
	float NormalizeAngleDifference(float AngleDiff) const;

	/** 内部函数：计算距离权重 */
	float CalculateDistanceWeight(float Distance, float MaxDistance) const;

	/** 内部函数：应用地形高度补偿 */
	FVector ApplyTerrainHeightCompensation(const FVector& BaseLocation, AActor* Target) const;

	/** 内部函数：验证目标有效性 */
	bool ValidateTarget(AActor* Target) const;

	/** FromSoftware式体型配置映射 */
	UPROPERTY(EditAnywhere, Category = "Enemy Size Config")
	TMap<EEnemyCameraType, FCameraTypeConfig> CameraTypeConfigs;

	/** 初始化默认配置 */
	void InitializeDefaultCameraConfigs();

	/** 从Actor获取体型类型 */
	EEnemyCameraType GetEnemyCameraTypeFromActor(AActor* Target) const;

	/** 预计算目标的最终位置（用于平滑切换优化） */
	FVector PreCalculateFinalTargetLocation(AActor* Target) const;

	/** 更新缓存的目标位置（用于稳定插值） */
	void UpdateCachedTargetLocation(AActor* Target, float DeltaTime);
};