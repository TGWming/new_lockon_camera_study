// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TargetDetectionComponent.h"
#include "CameraControlComponent.h"
#include "UIManagerComponent.h"
#include "PoiseComponent.h"
#include "StaminaComponent.h"
#include "DodgeComponent.h"
#include "ExecutionComponent.h"
#include "CameraPresetComponent.h"
#include "CameraDebugComponent.h"
#include "LockOnConfig.h"
#include "CameraSetupConfig.h"
#include "MyCharacter.generated.h"

// 前置声明
class USpringArmComponent;
class UCameraComponent;
class USphereComponent;
class UInputComponent;
class UUserWidget;
class UWidgetComponent;
class UCanvasPanelSlot;  // 用于Socket投射系统
class UTargetDetectionComponent; // 目标检测组件
class UCameraControlComponent; // 相机控制组件
class UCameraPresetComponent; // 相机预设组件

UCLASS()
class SOUL_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// ==================== 组件声明 ====================
	/** 目标检测组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTargetDetectionComponent* TargetDetectionComponent;

	/** 相机控制组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraControlComponent* CameraControlComponent;

	/** UI管理组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UUIManagerComponent* UIManagerComponent;

	/** 相机预设组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraPresetComponent* CameraPresetComponent;

	/** 相机调试组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraDebugComponent* CameraDebugComponent;

	/** Soul游戏系统组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soul Components")
	UPoiseComponent* PoiseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soul Components")
	UStaminaComponent* StaminaComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soul Components")
	UDodgeComponent* DodgeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soul Components")
	UExecutionComponent* ExecutionComponent;

	// ==================== 锁定检测组件 ====================
	/** 锁定检测球体组件 - 用于检测可锁定目标 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lock On System", meta = (DisplayName = "Lock-On Detection Sphere"))
	class USphereComponent* LockOnDetectionSphere;

	// ==================== 相机配置参数（可在编辑器调整） ====================
	/** 相机初始配置 - 设计师可在编辑器中自由调整 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Configuration", meta = (DisplayName = "Camera Setup Config"))
	FCameraSetupConfig CameraSetupConfig;
	
	/** 是否在BeginPlay时应用相机配置（false则使用蓝图设置的值） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Configuration", meta = (DisplayName = "Apply Config On BeginPlay"))
	bool bApplyCameraConfigOnBeginPlay = true;
	
	/** 不同敌人体型的相机配置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Configuration|Size Adaptation")
	TMap<EEnemySizeCategory, FCameraSetupConfig> SizeBasedCameraConfigs;

	// ==================== 常量定义 ====================
	// 目标查找频率（秒）
	static constexpr float TARGET_SEARCH_INTERVAL = 0.2f;
	
	// 角色旋转速度
	static constexpr float CHARACTER_ROTATION_SPEED = 10.0f;
	
	// 射线检测高度偏移
	static constexpr float RAYCAST_HEIGHT_OFFSET = 50.0f;
	
	// 摇杆切换目标的阈值
	static constexpr float THUMBSTICK_THRESHOLD = 0.5f; // 降低阈值，更容易触发

	// 锁定切换角度阈值（度）- 小于此角度时不旋转相机和角色
	static constexpr float TARGET_SWITCH_ANGLE_THRESHOLD = 20.0f;
	
	// 锁定切换时的平滑插值速度
	static constexpr float TARGET_SWITCH_SMOOTH_SPEED = 3.0f;

	// 新增常量：减少魔法数字
	static constexpr float UI_HEIGHT_OFFSET = 80.0f;
	static constexpr float LOCK_COMPLETION_THRESHOLD = 5.0f;
	static constexpr float SMOOTH_SWITCH_ANGLE_THRESHOLD = 2.0f;
	static constexpr float EXTENDED_LOCK_RANGE_MULTIPLIER = 1.5f;

	// ==================== 扇形锁定区域常量 ====================
	// 扇形锁定区域：前方2/3区域的角度范围（更准确的前方区域）
	static constexpr float SECTOR_LOCK_ANGLE = 100.0f; // 扇形锁定范围（±50度）
	
	// 边缘锁定检测角度：用于检测屏幕边缘的目标
	static constexpr float EDGE_DETECTION_ANGLE = 120.0f; // 边缘检测范围（±120度）
	
	// 相机自动修正速度（当锁定边缘目标时）
	static constexpr float CAMERA_AUTO_CORRECTION_SPEED = 7.0f;
	
	// 边缘锁定时相机修正的目标角度偏移（减少修正幅度）
	static constexpr float CAMERA_CORRECTION_OFFSET = 150.0f; // 修正150度让目标进入主锁定区域

	// ==================== 相机重置常量 ====================
	// 相机重置平滑插值速度
	static constexpr float CAMERA_RESET_SPEED = 7.0f; // 控制相机重置的平滑速度
	
	// 相机重置角度阈值（小于此角度时停止插值）
	static constexpr float CAMERA_RESET_ANGLE_THRESHOLD = 1.0f;

	// ==================== 相机组件 ====================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	// ==================== 锁定相机控制参数 ====================
	// 可调节的相机插值速度（与蓝图一致的参数）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn Camera", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float CameraInterpSpeed = 5.0f;
	
	// 是否启用平滑相机跟踪
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn Camera")
	bool bEnableSmoothCameraTracking = true;
	
	// 相机跟踪模式：0=完全跟踪，1=仅水平跟踪，2=自定义
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn Camera", meta = (ClampMin = "0", ClampMax = "2"))
	int32 CameraTrackingMode = 0;

	// 新增：目标位置偏移（对应蓝图中的Vector减法）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn Camera")
	FVector TargetLocationOffset = FVector(0.0f, 0.0f, -450.0f);

	// ==================== 锁定与索敌相关 ====================
	// 锁定状态
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
	bool bIsLockedOn;

	// 当前锁定的目标
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
	AActor* CurrentLockOnTarget;

	// 可锁定目标列表
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LockOn")
	TArray<AActor*> LockOnCandidates;

	// 锁定范围
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "100.0", ClampMax = "3000.0"))
	float LockOnRange;

	// 锁定角度（视野范围）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "30.0", ClampMax = "180.0"))
	float LockOnAngle;

	// 目标切换状态
	bool bJustSwitchedTarget = false;
	float TargetSwitchCooldown = 0.5f;
	float LastTargetSwitchTime = 0.0f;

	// 新增：锁定切换相关状态
	bool bIsSmoothSwitching = false;			// 是否正在进行平滑切换
	float SmoothSwitchStartTime = 0.0f;		// 平滑切换开始时间
	FRotator SmoothSwitchStartRotation = FRotator::ZeroRotator;	// 平滑切换起始旋转
	FRotator SmoothSwitchTargetRotation = FRotator::ZeroRotator;// 平滑切换目标旋转
	bool bShouldSmoothSwitchCamera = false;		// 相机是否需要平滑切换
	bool bShouldSmoothSwitchCharacter = false;	// 角色是否需要平滑切换

	// 新增：相机跟随控制
	bool bShouldCameraFollowTarget = true;		// 相机是否应该跟随目标
	bool bShouldCharacterRotateToTarget = true; // 角色身体是否应该转向目标
	bool bPlayerIsMoving = false;				// 玩家是否在移动

	// ==================== 扇形锁定相关状态 ====================
	// 相机自动修正状态
	bool bIsCameraAutoCorrection = false;		// 是否正在进行相机自动修正
	float CameraCorrectionStartTime = 0.0f;	// 修正开始时间
	FRotator CameraCorrectionStartRotation = FRotator::ZeroRotator; // 修正起始相机旋转
	FRotator CameraCorrectionTargetRotation = FRotator::ZeroRotator; // 修正目标相机旋转

	// 延迟修正的目标引用
	UPROPERTY()
	AActor* DelayedCorrectionTarget = nullptr;

	// ==================== 相机重置相关状态 ====================
	// 是否正在进行平滑相机重置
	bool bIsSmoothCameraReset = false;
	
	// 平滑重置开始时间
	float SmoothResetStartTime = 0.0f;
	
	// 平滑重置起始相机旋转
	FRotator SmoothResetStartRotation = FRotator::ZeroRotator;
	
	// 平滑重置目标相机旋转
	FRotator SmoothResetTargetRotation = FRotator::ZeroRotator;

	// ==================== 调试控制 ====================
	// 是否启用相机调试日志
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableCameraDebugLogs = false;
	
	// 是否启用锁定调试日志
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableLockOnDebugLogs = false;

	// ==================== UMG相关声明 ====================
	// 锁定UI Widget类引用（在蓝图中设置）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> LockOnWidgetClass;

	// 当前显示的锁定UI Widget实例
	UPROPERTY()
	UUserWidget* LockOnWidgetInstance;

	// 新增：追踪上一个锁定目标，用于UI管理
	UPROPERTY()
	AActor* PreviousLockOnTarget = nullptr;

	// ==================== Socket投射系统声明 ====================  // 👈 在这里添加新的声明
	// Socket投射相关
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn Socket")
	FName TargetSocketName = TEXT("Spine2Socket");

	// 投射相关参数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn Socket", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float ProjectionScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn Socket")
	FVector SocketOffset = FVector(0.0f, 0.0f, 50.0f);

	// 是否启用Socket投射（用于逐步迁移）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn Socket")
	bool bUseSocketProjection = true;

	// Socket投射相关函数声明
	UFUNCTION(BlueprintCallable, Category = "LockOn Socket")
	FVector GetTargetSocketWorldLocation(AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category = "LockOn Socket")
	bool HasValidSocket(AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category = "LockOn Socket")
	FVector2D ProjectSocketToScreen(const FVector& SocketWorldLocation) const;

	// Socket投射UMG相关函数声明
	UFUNCTION(BlueprintCallable, Category = "LockOn Socket")
	void ShowSocketProjectionWidget();
	
	UFUNCTION(BlueprintCallable, Category = "LockOn Socket")
	void UpdateSocketProjectionWidget();
	
	UFUNCTION(BlueprintCallable, Category = "LockOn Socket")
	void HideSocketProjectionWidget();


	// ==================== 移动参数 ====================
	// 普通移动速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "100.0", ClampMax = "1000.0"))
	float NormalWalkSpeed;

	// 锁定状态下的移动速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "50.0", ClampMax = "500.0"))
	float LockedWalkSpeed;

	// 当前移动输入值（用于动画）
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float ForwardInputValue;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float RightInputValue;

	// ==================== 输入状态跟踪 ====================
	// 右摇杆输入状态跟踪
	bool bRightStickLeftPressed = false;
	bool bRightStickRightPressed = false;
	float LastRightStickX = 0.0f;

	// ==================== 性能优化 ====================
	// 上次查找目标的时间
	float LastFindTargetsTime = 0.0f;

	// ==================== 移动函数 ====================
	void MoveForward(float Value);
	void MoveRight(float Value);
	void StartJump();
	void StopJump();

	// 相机控制
	void Turn(float Rate);
	void LookUp(float Rate);

	// ==================== 新的输入处理函数 ====================
	// 右摇杆水平输入处理（用于切换目标）
	void HandleRightStickX(float Value);
	
	// 锁定按钮处理
	void HandleLockOnButton();

	// 调试函数
	void DebugInputTest();

	// 新增：验证UMG Widget设置的调试函数
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugWidgetSetup();

	// ==================== 步骤5：运行时调试命令 ====================
	/** 设置运行时调试控制台命令 */
	void SetupDebugCommands();

	// ==================== UMG相关函数声明 ====================
	// 显示锁定UI
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowLockOnWidget();

	// 隐藏锁定UI
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideLockOnWidget();

	// 隐藏所有目标的锁定UI
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideAllLockOnWidgets();

	// 更新锁定UI位置和状态
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateLockOnWidget();

	// ==================== 锁定系统方法 ====================
	// 切换锁定
	void ToggleLockOn();

	// 查找可锁定目标
	void FindLockOnCandidates();

	// 新增：检查球体内是否有候选目标
	bool HasCandidatesInSphere();
	
	// 新增：执行简单的相机重置
	void PerformSimpleCameraReset();
	
	// 新增：开始锁定目标
	void StartLockOn(AActor* Target);
	
	// 新增：取消锁定
	void CancelLockOn();
	
	// 新增：相机重置到指定旋转
	void StartCameraReset(const FRotator& TargetRotation);

	// 新增：开始平滑相机重置
	void StartSmoothCameraReset();
	
	// 新增：更新平滑相机重置
	void UpdateSmoothCameraReset();

	// 切换锁定目标（左右切换）
	void SwitchLockOnTargetLeft();
	void SwitchLockOnTargetRight();
	
	// 切换目标时更新角色朝向（但不更新相机）
	void UpdateCharacterRotationToTarget();

	// 新增：计算到目标的角度差异
	float CalculateAngleToTarget(AActor* Target) const;
	
	// 新增：计算目标相对于玩家的方向角度（-180到180度，左负右正）
	float CalculateDirectionAngle(AActor* Target) const;
	
	// 新增：按方向角度排序候选目标（从左到右）
	void SortCandidatesByDirection(TArray<AActor*>& Targets);

	// 新增：开始平滑切换到新目标
	void StartSmoothTargetSwitch(AActor* NewTarget);
	
	// 新增：更新平滑切换状态
	void UpdateSmoothTargetSwitch();

	// ==================== 扇形锁定系统方法 ====================
	// 检查目标是否在扇形锁定区域内
	bool IsTargetInSectorLockZone(AActor* Target) const;
	
	// 检查目标是否在边缘检测区域内
	bool IsTargetInEdgeDetectionZone(AActor* Target) const;
	
	// 开始相机自动修正
	void StartCameraAutoCorrection(AActor* Target);
	
	// 更新相机自动修正
	void UpdateCameraAutoCorrection();
	
	// 延迟相机修正函数
	UFUNCTION()
	void DelayedCameraCorrection();
	
	// 新增：恢复相机跟随状态函数
	UFUNCTION()
	void RestoreCameraFollowState();
	
	// 获取扇形区域内的最佳锁定目标
	AActor* GetBestSectorLockTarget();
	
	// 从目标列表中获取最佳目标（通用评分函数）
	AActor* GetBestTargetFromList(const TArray<AActor*>& TargetList);

	// 获取最佳锁定目标
	AActor* GetBestLockOnTarget();

	// 检查目标是否有效
	bool IsValidLockOnTarget(AActor* Target);

	// 检查目标是否仍然可以保持锁定（更宽松的条件）"
	bool IsTargetStillLockable(AActor* Target);

	// 更新锁定状态
	void UpdateLockOnTarget();

	// 锁定时的相机更新（优化后的核心函数）
	void UpdateLockOnCamera();

	// 绘制锁定光标UI（仅在开发版本中启用）
	void DrawLockOnCursor();
	
	// 重置相机（保持兼容性）
	void ResetCamera();

	// 新增：尝试获取扇形区域内的锁定目标
	AActor* TryGetSectorLockTarget();
	
	// 新增：尝试获取需要相机修正的目标
	AActor* TryGetCameraCorrectionTarget();
	
	// 新增：开始针对特定目标的相机修正
	void StartCameraCorrectionForTarget(AActor* Target);

	// ==================== 新增：敌人尺寸分析接口 ====================
	/** 获取目标的尺寸分类 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Size Analysis")
	EEnemySizeCategory GetTargetSizeCategory(AActor* Target);

	/** 根据尺寸分类获取目标列表 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Size Analysis")
	TArray<AActor*> GetTargetsBySize(EEnemySizeCategory SizeCategory);

	/** 获取指定尺寸分类中最近的目标 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Size Analysis")
	AActor* GetNearestTargetBySize(EEnemySizeCategory SizeCategory);

	/** 获取所有尺寸分类统计信息 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Size Analysis")
	TMap<EEnemySizeCategory, int32> GetSizeCategoryStatistics();

	/** 调试函数：显示目标尺寸分析信息 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugDisplayTargetSizes();

	/** 系统集成测试函数 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void TestCameraSystem();

private:
	// ==================== 相机组件查找辅助函数 ====================
	/** 查找CameraBoom组件（多策略容错） */
	USpringArmComponent* FindCameraBoomComponent();
	
	/** 查找FollowCamera组件（多策略容错） */
	UCameraComponent* FindFollowCameraComponent();
	
	/** 验证并缓存相机组件 */
	void ValidateAndCacheComponents();
	
	// ==================== 调试辅助函数 ====================
	/** 输出所有组件信息（用于调试） */
	void LogAllComponents();
	
	/** 输出相机状态信息 */
	void LogCameraState(const FString& Context);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ==================== 公共接口 ====================
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	bool IsLockedOn() const { return bIsLockedOn; }

	UFUNCTION(BlueprintCallable, Category = "LockOn")
	AActor* GetLockOnTarget() const { return CurrentLockOnTarget; }

	// ==================== 相机配置动态调整接口 ====================
	/** 应用指定的相机配置 */
	UFUNCTION(BlueprintCallable, Category = "Camera Configuration")
	void ApplyCameraConfig(const FCameraSetupConfig& NewConfig);
	
	/** 根据敌人体型应用相机配置 */
	UFUNCTION(BlueprintCallable, Category = "Camera Configuration")
	void ApplyCameraConfigForEnemySize(EEnemySizeCategory SizeCategory);
	
	/** 重置相机到默认配置 */
	UFUNCTION(BlueprintCallable, Category = "Camera Configuration")
	void ResetCameraToDefaultConfig();
	
	/** 获取当前相机配置 */
	UFUNCTION(BlueprintPure, Category = "Camera Configuration")
	FCameraSetupConfig GetCurrentCameraConfig() const;

	// ==================== 编辑器实时更新接口 ====================
#if WITH_EDITOR
	/** 编辑器中属性更改时调用 - 实时更新相机配置 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	/** 在编辑器中手动应用相机配置 */
	UFUNCTION(CallInEditor, Category = "Camera Configuration", meta = (DisplayName = "Apply Camera Config Now"))
	void PreviewCameraConfigInEditor();
	
	/** 重置相机配置到默认值 */
	UFUNCTION(CallInEditor, Category = "Camera Configuration", meta = (DisplayName = "Reset to Defaults"))
	void ResetCameraConfigInEditor();
	
	/** 编辑器中组件变换改变时调用 */
	virtual void PostEditMove(bool bFinished) override;
	
	/** 从组件同步配置值 */
	UFUNCTION(CallInEditor, Category = "Camera Configuration", meta = (DisplayName = "Sync Config From Camera"))
	void SyncCameraConfigFromComponents();
#endif
};