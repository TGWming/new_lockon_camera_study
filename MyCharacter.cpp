// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacter.h"
#include "Components/InputComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Engine.h"
#include "UObject/StructOnScope.h"

// Sets default values
AMyCharacter::AMyCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// ==================== 创建目标检测组件 ====================
	TargetDetectionComponent = CreateDefaultSubobject<UTargetDetectionComponent>(TEXT("TargetDetectionComponent"));

	// ==================== 创建相机控制组件 ====================
	CameraControlComponent = CreateDefaultSubobject<UCameraControlComponent>(TEXT("CameraControlComponent"));

	// ==================== 创建UI管理组件 ====================
	UIManagerComponent = CreateDefaultSubobject<UUIManagerComponent>(TEXT("UIManagerComponent"));

	// ==================== 创建相机预设组件 ====================
	CameraPresetComponent = CreateDefaultSubobject<UCameraPresetComponent>(TEXT("CameraPresetComponent"));

	// ==================== 创建相机调试组件 ====================
	CameraDebugComponent = CreateDefaultSubobject<UCameraDebugComponent>(TEXT("CameraDebugComponent"));

	// ==================== 创建Soul游戏系统组件 ====================
	PoiseComponent = CreateDefaultSubobject<UPoiseComponent>(TEXT("PoiseComponent"));
	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));
	DodgeComponent = CreateDefaultSubobject<UDodgeComponent>(TEXT("DodgeComponent"));
	ExecutionComponent = CreateDefaultSubobject<UExecutionComponent>(TEXT("ExecutionComponent"));

	// ==================== 创建锁定检测球体组件 ====================
	// 原始代码注释保留：此组件用于检测可锁定的敌人
	LockOnDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("LockOnDetectionSphere"));
	LockOnDetectionSphere->SetupAttachment(RootComponent);
	LockOnDetectionSphere->InitSphereRadius(LockOnRange);  // 使用配置的锁定范围
	// 设置碰撞通道
	LockOnDetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LockOnDetectionSphere->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	LockOnDetectionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	LockOnDetectionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	// 编辑器可视化设置
	LockOnDetectionSphere->SetHiddenInGame(true);  // 游戏中隐藏
	LockOnDetectionSphere->ShapeColor = FColor::Green;  // 编辑器中显示为绿色

	// ==================== ��ʼ������ ====================
	bIsLockedOn = false;
	CurrentLockOnTarget = nullptr;
	LockOnRange = 2000.0f;  // ���ӵ�20�ף����ӽ��ڼ�3������
	LockOnAngle = 120.0f;   // ������Ұ�Ƕȵ���60�ȣ�����������
	NormalWalkSpeed = 600.0f;
	LockedWalkSpeed = 600.0f;
	ForwardInputValue = 0.0f;
	RightInputValue = 0.0f;
	LastFindTargetsTime = 0.0f;
	
	// ==================== ����������������Ʋ�����ʼ�� ====================
	CameraInterpSpeed = 5.0f;           // Ĭ�ϲ�ֵ�ٶ�
	bEnableSmoothCameraTracking = true; // Ĭ������ƽ������
	CameraTrackingMode = 0;             // Ĭ����ȫ����ģʽ
	
	// Ŀ��λ��ƫ�Ƴ�ʼ������Ӧ��ͼ�е�Vector������
	TargetLocationOffset = FVector(0.0f, 0.0f, -250.0f);

	// ==================== ���Կ��Ƴ�ʼ�� ====================
	bEnableCameraDebugLogs = false;     // Ĭ�Ϲر��������
	bEnableLockOnDebugLogs = false;     // Ĭ�Ϲر���������
	
	// ����״̬��ʼ��
	bRightStickLeftPressed = false;
	bRightStickRightPressed = false;
	LastRightStickX = 0.0f;
	
	// Ŀ���л�״̬��ʼ��
	bJustSwitchedTarget = false;
	TargetSwitchCooldown = 0.5f;
	LastTargetSwitchTime = 0.0f;

	// �����л�ƽ��������ʼ��
	bIsSmoothSwitching = false;
	SmoothSwitchStartTime = 0.0f;
	SmoothSwitchStartRotation = FRotator::ZeroRotator;
	SmoothSwitchTargetRotation = FRotator::ZeroRotator;
	bShouldSmoothSwitchCamera = false;
	bShouldSmoothSwitchCharacter = false;

	// ���������Ƴ�ʼ��
	bShouldCameraFollowTarget = true;
	bShouldCharacterRotateToTarget = true; // ��ɫ����ت����Ƴ�ʼ��
	bPlayerIsMoving = false;

	// ��������ϵͳ��ʼ��
	bIsCameraAutoCorrection = false;
	CameraCorrectionStartTime = 0.0f;
	CameraCorrectionStartRotation = FRotator::ZeroRotator;
	CameraCorrectionTargetRotation = FRotator::ZeroRotator;
	DelayedCorrectionTarget = nullptr;

	// UMG��س�ʼ��
	LockOnWidgetClass = nullptr;
	LockOnWidgetInstance = nullptr;
	PreviousLockOnTarget = nullptr;

	// ������ó�ʼ��
	bIsSmoothCameraReset = false;
	SmoothResetStartTime = 0.0f;
	SmoothResetStartRotation = FRotator::ZeroRotator;
	SmoothResetTargetRotation = FRotator::ZeroRotator;

	// ==================== SocketͶ��ϵͳ��ʼ�� ====================
	TargetSocketName = TEXT("Spine2Socket");
	ProjectionScale = 1.0f;
	SocketOffset = FVector(0.0f, 0.0f, 50.0f);
	bUseSocketProjection = true; // Ĭ������SocketͶ��

	// ==================== 创建相机组件 ====================
	// 原始代码注释保留：
	// CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	// CameraBoom->SetupAttachment(RootComponent);
	// CameraBoom->TargetArmLength = 450.0f; // 相机距离
	// CameraBoom->bUsePawnControlRotation = true; // 跟随控制器旋转

	// 修改后：使用配置结构体的默认值
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// 使用配置中的值而非硬编码
	CameraBoom->TargetArmLength = CameraSetupConfig.ArmLength;
	CameraBoom->bUsePawnControlRotation = CameraSetupConfig.bUsePawnControlRotation;
	CameraBoom->SetRelativeRotation(CameraSetupConfig.InitialRotation);
	// 新增：相机延迟设置
	CameraBoom->bEnableCameraLag = CameraSetupConfig.bEnableCameraLag;
	CameraBoom->CameraLagSpeed = CameraSetupConfig.CameraLagSpeed;

	// 原始代码注释保留：
	// FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	// FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// FollowCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

	// 修改后：使用配置中的插座偏移
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->SetRelativeLocation(CameraSetupConfig.SocketOffset);
	FollowCamera->bUsePawnControlRotation = false; // �����������ت

	// ==================== ........................ ====================
	// ���ý�ɫ�ƶ����� ====================
	// ���ý�ɫ�����������ت
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// ���ý�ɫ�ƶ����
	GetCharacterMovement()->bOrientRotationToMovement = true; // ��ɫ�����ƶ�����
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ��ت�ٶ�
	GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// ==================== 步骤1：强制验证相机系统 ====================
	UE_LOG(LogTemp, Warning, TEXT("=== CHARACTER BEGIN PLAY ==="));
	
	// 验证并缓存组件
	ValidateAndCacheComponents();
	
	// ==================== 步骤2：如果找到组件，应用初始配置 ====================
	if (CameraBoom && FollowCamera)
	{
		// 设置初始移动速度
		GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
		
		// ==================== 应用相机配置（新增） ====================
		// 只有在明确要求应用配置时才覆盖蓝图设置
		if (bApplyCameraConfigOnBeginPlay)
		{
			// 记录变更前状态
			float OldArmLength = CameraBoom->TargetArmLength;
			FRotator OldRotation = CameraBoom->GetRelativeRotation();
			
			// 应用相机臂设置
			CameraBoom->TargetArmLength = CameraSetupConfig.ArmLength;
			CameraBoom->SetRelativeRotation(CameraSetupConfig.InitialRotation);
			CameraBoom->bUsePawnControlRotation = CameraSetupConfig.bUsePawnControlRotation;
			CameraBoom->bEnableCameraLag = CameraSetupConfig.bEnableCameraLag;
			CameraBoom->CameraLagSpeed = CameraSetupConfig.CameraLagSpeed;
			
			// 应用相机设置
			FollowCamera->SetRelativeLocation(CameraSetupConfig.SocketOffset);
			
			// 验证应用结果
			float ActualArmLength = CameraBoom->TargetArmLength;
			if (FMath::IsNearlyEqual(ActualArmLength, CameraSetupConfig.ArmLength, 0.1f))
			{
				UE_LOG(LogTemp, Warning, TEXT("✓✓✓ CONFIG APPLIED SUCCESSFULLY ✓✓✓"));
				UE_LOG(LogTemp, Warning, TEXT("  ArmLength: %.1f -> %.1f"), OldArmLength, ActualArmLength);
				UE_LOG(LogTemp, Warning, TEXT("  Rotation: (%.1f,%.1f,%.1f) -> (%.1f,%.1f,%.1f)"),
					OldRotation.Pitch, OldRotation.Yaw, OldRotation.Roll,
					CameraSetupConfig.InitialRotation.Pitch, CameraSetupConfig.InitialRotation.Yaw, CameraSetupConfig.InitialRotation.Roll);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("✗✗✗ CONFIG VERIFICATION FAILED ✗✗✗"));
				UE_LOG(LogTemp, Error, TEXT("  Expected: %.1f, Actual: %.1f"), 
					CameraSetupConfig.ArmLength, ActualArmLength);
			}
			
			// 记录基础臂长供后续动态调整使用
			if (CameraControlComponent)
			{
				// 传递基础配置给相机控制组件
				FCameraSettings CameraSetup;
				CameraSetup.CameraInterpSpeed = CameraInterpSpeed;
				CameraSetup.bEnableSmoothCameraTracking = bEnableSmoothCameraTracking;
				CameraSetup.CameraTrackingMode = CameraTrackingMode;
				CameraSetup.TargetLocationOffset = TargetLocationOffset;
				CameraControlComponent->SetCameraSettings(CameraSetup);
			}
			
			// 记录最终状态
			LogCameraState(TEXT("BeginPlay Initial"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MyCharacter: Using Blueprint camera settings (bApplyCameraConfigOnBeginPlay=false)"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BeginPlay FAILED - Camera components not initialized properly"));
		return;
	}
	
	// ==================== 步骤3：设置运行时调试命令 ====================
	SetupDebugCommands();
	
	// 初始化不同体型的相机配置（如果没有在编辑器中设置）
	if (SizeBasedCameraConfigs.Num() == 0)
	{
		// 小型敌人配置
		FCameraSetupConfig SmallConfig = CameraSetupConfig;
		SmallConfig.ArmLength *= 0.8f;
		SmallConfig.LockOnHeightOffset = FVector(0, 0, 0);
		SizeBasedCameraConfigs.Add(EEnemySizeCategory::Small, SmallConfig);
		
		// 中型敌人配置
		FCameraSetupConfig MediumConfig = CameraSetupConfig;
		MediumConfig.ArmLength *= 1.0f;
		MediumConfig.LockOnHeightOffset = FVector(0, 0, 30);
		SizeBasedCameraConfigs.Add(EEnemySizeCategory::Medium, MediumConfig);
		
		// 大型敌人配置
		FCameraSetupConfig LargeConfig = CameraSetupConfig;
		LargeConfig.ArmLength *= 1.3f;
		LargeConfig.InitialRotation.Pitch = -25.0f;
		LargeConfig.LockOnHeightOffset = FVector(0, 0, 60);
		SizeBasedCameraConfigs.Add(EEnemySizeCategory::Large, LargeConfig);
		
		// 巨型敌人配置
		FCameraSetupConfig GiantConfig = CameraSetupConfig;
		GiantConfig.ArmLength *= 1.6f;
		GiantConfig.InitialRotation.Pitch = -30.0f;
		GiantConfig.LockOnHeightOffset = FVector(0, 0, 100);
		SizeBasedCameraConfigs.Add(EEnemySizeCategory::Giant, GiantConfig);
		
		UE_LOG(LogTemp, Warning, TEXT("MyCharacter: Initialized default size-based camera configs"));
	}
	
	// 确保球体组件正确初始化
	if (LockOnDetectionSphere)
	{
		LockOnDetectionSphere->SetSphereRadius(LockOnRange);
		// 确保编辑器中可见，游戏中隐藏
		#if WITH_EDITOR
			LockOnDetectionSphere->SetHiddenInGame(false);  // 编辑器中显示
		#else
			LockOnDetectionSphere->SetHiddenInGame(true);   // 游戏中隐藏
		#endif
		
		UE_LOG(LogTemp, Log, TEXT("LockOnDetectionSphere initialized with radius: %.1f"), LockOnRange);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("LockOnDetectionSphere is null in BeginPlay!"));
	}

	// ==================== 配置目标检测组件 ====================
	if (TargetDetectionComponent)
	{
		// ���ü����������
		TargetDetectionComponent->SetLockOnDetectionSphere(LockOnDetectionSphere);
		
		// ������������
		TargetDetectionComponent->LockOnSettings.LockOnRange = LockOnRange;
		TargetDetectionComponent->LockOnSettings.LockOnAngle = LockOnAngle;
		
		// �����������
		TargetDetectionComponent->CameraSettings.CameraInterpSpeed = CameraInterpSpeed;
		TargetDetectionComponent->CameraSettings.bEnableSmoothCameraTracking = bEnableSmoothCameraTracking;
		TargetDetectionComponent->CameraSettings.CameraTrackingMode = CameraTrackingMode;
		TargetDetectionComponent->CameraSettings.TargetLocationOffset = TargetLocationOffset;
		
		// ����SocketͶ�����
		TargetDetectionComponent->SocketProjectionSettings.bUseSocketProjection = bUseSocketProjection;
		TargetDetectionComponent->SocketProjectionSettings.TargetSocketName = TargetSocketName;
		TargetDetectionComponent->SocketProjectionSettings.SocketOffset = SocketOffset;
		TargetDetectionComponent->SocketProjectionSettings.ProjectionScale = ProjectionScale;
		
		// ���õ�������
		TargetDetectionComponent->bEnableTargetDetectionDebugLogs = bEnableLockOnDebugLogs;
		TargetDetectionComponent->bEnableSizeAnalysisDebugLogs = bEnableLockOnDebugLogs;
		
		UE_LOG(LogTemp, Warning, TEXT("MyCharacter: TargetDetectionComponent configured successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter: TargetDetectionComponent is null!"));
	}

	// ==================== ��ʼ���������������� ====================
	if (CameraControlComponent)
	{
		// �����������
		FCameraSettings CameraSetup;
		CameraSetup.CameraInterpSpeed = CameraInterpSpeed;
		CameraSetup.bEnableSmoothCameraTracking = bEnableSmoothCameraTracking;
		CameraSetup.CameraTrackingMode = CameraTrackingMode;
		CameraSetup.TargetLocationOffset = TargetLocationOffset;
		CameraControlComponent->SetCameraSettings(CameraSetup);
		
		// ���ø߼��������
		FAdvancedCameraSettings AdvancedSetup;
		// ʹ��CameraControlComponent�ж���ĳ���
		CameraControlComponent->SetAdvancedCameraSettings(AdvancedSetup);
		
		UE_LOG(LogTemp, Warning, TEXT("MyCharacter: CameraControlComponent configured successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter: CameraControlComponent is null!"));
	}

	// ==================== ��ʼ��UI����������� ====================
	if (UIManagerComponent)
	{
		UIManagerComponent->LockOnWidgetClass = LockOnWidgetClass;
		
		// Configure Hybrid Projection Settings (replacing Socket Projection)
		FHybridProjectionSettings HybridSettings;
		HybridSettings.ProjectionMode = EProjectionMode::Hybrid;
		HybridSettings.CustomOffset = SocketOffset;
		HybridSettings.BoundsOffsetRatio = 0.6f;
		HybridSettings.bEnableSizeAdaptive = true;
		UIManagerComponent->SetHybridProjectionSettings(HybridSettings);
		
		UE_LOG(LogTemp, Warning, TEXT("MyCharacter: UIManagerComponent configured successfully with hybrid projection"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter: UIManagerComponent is null!"));
	}

	// ==================== 验证所有核心组件实际已经挂载，功能检查） ====================
	if (PoiseComponent)
	{
		UE_LOG(LogTemp, Log, TEXT("PoiseComponent initialized successfully"));
	}

	if (StaminaComponent)
	{
		UE_LOG(LogTemp, Log, TEXT("StaminaComponent initialized successfully"));
	}

	if (DodgeComponent)
	{
		UE_LOG(LogTemp, Log, TEXT("DodgeComponent initialized successfully"));
	}

	if (ExecutionComponent)
	{
		UE_LOG(LogTemp, Log, TEXT("ExecutionComponent initialized successfully"));
	}

	// ==================== 初始化相机预设组件 ====================
	if (CameraPresetComponent)
	{
		// 应用默认预设
		CameraPresetComponent->SwitchToPreset(0);
		
		UE_LOG(LogTemp, Warning, TEXT("MyCharacter: CameraPresetComponent configured successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter: CameraPresetComponent is null!"));
	}

	// ==================== 配置Debug组件 ====================
	if (CameraDebugComponent)
	{
#if WITH_EDITOR
		// 编辑器中默认开启Debug
		CameraDebugComponent->bEnableDebugVisualization = true;
		CameraDebugComponent->VisualizationMode = EDebugVisualizationMode::TargetInfo;
#else
		// 发布版本默认关闭
		CameraDebugComponent->bEnableDebugVisualization = false;
#endif
		
		UE_LOG(LogTemp, Warning, TEXT("MyCharacter: CameraDebugComponent configured"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter: CameraDebugComponent is null!"));
	}
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ����ƽ��������ã����ȼ���ߣ���Ϊ�Ƿ�����״̬��
	if (bIsSmoothCameraReset)
	{
		UpdateSmoothCameraReset();
		return; // �����ڼ䲻ִ����������߼�
	}

	// ��������Զ����������ȼ���ߣ�
	if (bIsCameraAutoCorrection)
	{
		UpdateCameraAutoCorrection();
	}

	// ��������״̬
	if (bIsLockedOn)
	{
		UpdateLockOnTarget();
		
		// ����ƽ���l�״̬�����ȼ�������ͨ������£�
		if (bIsSmoothSwitching)
		{
			UpdateSmoothTargetSwitch();
		}
		else if (!bIsCameraAutoCorrection) // ֻ�ڷ��Զ�����״̬�²�ִ����ͨ�������
		{
			// ����������������߼�
			UpdateLockOnCamera();
		}
		
		// ����UMG����UI������SocketͶ�䣩
		UpdateLockOnWidget();

		// ���������Ϣ���ɿ����ƣ�- ���ӽ�Ƶ����
		if (bEnableCameraDebugLogs && CurrentLockOnTarget)
		{
			static float LastCameraDebugLogTime = 0.0f;
			float CurrentTime = GetWorld()->GetTimeSeconds();
			if (CurrentTime - LastCameraDebugLogTime > 0.5f) // ÿ0.5���¼һ��
			{
				UE_LOG(LogTemp, VeryVerbose, TEXT("LockOn Active: Target=%s, CameraFollow=%s, CharacterRotate=%s"), 
					*CurrentLockOnTarget->GetName(),
					bShouldCameraFollowTarget ? TEXT("YES") : TEXT("NO"),
					bShouldCharacterRotateToTarget ? TEXT("YES") : TEXT("NO"));
				LastCameraDebugLogTime = CurrentTime;
			}
		}
	}

	// ���ڲ��Թ�����Ŀ�꣨����Ƶ�����Ż����ܣ�
	// ��ƽ���l��ڼ���ͣĿ����������ֹ��ͻ
	if (!bIsSmoothSwitching)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastFindTargetsTime > TARGET_SEARCH_INTERVAL)
		{
			FindLockOnCandidates();
			LastFindTargetsTime = CurrentTime;

			// ����������Ϣ���ɿ��ƣ�- ���ӽ�Ƶ����
			if (bEnableLockOnDebugLogs)
			{
				static float LastTargetSearchLogTime = 0.0f;
				if (CurrentTime - LastTargetSearchLogTime > 1.0f) // ÿ1���¼һ��
				{
					UE_LOG(LogTemp, Verbose, TEXT("Target search completed: Found %d candidates"), LockOnCandidates.Num());
					LastTargetSearchLogTime = CurrentTime;
				}
			}
		}
	}
	// === 新增：参数验证调试（在函数末尾添加）===
	#if WITH_EDITOR  // 仅在编辑器中执行
	if (bEnableCameraDebugLogs && bIsLockedOn)
	{
		static float LastParamDebugTime = 0.0f;
		float CurrentTime = GetWorld()->GetTimeSeconds();
		
		// 每秒输出一次参数状态
		if (CurrentTime - LastParamDebugTime > 1.0f)
		{
			if (CameraBoom && FollowCamera)
			{
				UE_LOG(LogTemp, Warning, TEXT("=== CAMERA PARAMS DEBUG ==="));
				UE_LOG(LogTemp, Warning, TEXT("1. ArmLength: %.1f (Config: %.1f)"), 
					CameraBoom->TargetArmLength, CameraSetupConfig.ArmLength);
				UE_LOG(LogTemp, Warning, TEXT("2. InitialRotation: %s (Config: %s)"), 
					*CameraBoom->GetRelativeRotation().ToString(), 
					*CameraSetupConfig.InitialRotation.ToString());
				UE_LOG(LogTemp, Warning, TEXT("3. SocketOffset: %s (Config: %s)"), 
					*FollowCamera->GetRelativeLocation().ToString(), 
					*CameraSetupConfig.SocketOffset.ToString());
				UE_LOG(LogTemp, Warning, TEXT("4. LockOnHeightOffset: %s (Applied: %s)"), 
					*CameraSetupConfig.LockOnHeightOffset.ToString(),
					bIsLockedOn ? TEXT("YES") : TEXT("NO"));
				
				// 验证 CameraControlComponent 的设置
				if (CameraControlComponent)
				{
					UE_LOG(LogTemp, Warning, TEXT("5. TargetLocationOffset in Component: %s"), 
						*CameraControlComponent->CameraSettings.TargetLocationOffset.ToString());
				}
				
				UE_LOG(LogTemp, Warning, TEXT("==========================="));
				LastParamDebugTime = CurrentTime;
			}
		}
	}
	#endif
}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (!PlayerInputComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerInputComponent is null!"));
		return;
	}

	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// ���ƶ�����
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyCharacter::MoveRight);

	// ���������
	PlayerInputComponent->BindAxis("Turn", this, &AMyCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMyCharacter::LookUp);

	// ���� Ծ����
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMyCharacter::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMyCharacter::StopJump);

	// ==================== ��������� ====================
	// �������� - ֧����خاذٔۥٶȣ̂خم́ـ
	PlayerInputComponent->BindAction("LockOn", IE_Pressed, this, &AMyCharacter::HandleLockOnButton);
	
	// ��ƙ��ˮƽ���루�����ل��ح Gep̳
	PlayerInputComponent->BindAxis("RightStickX", this, &AMyCharacter::HandleRightStickX);
	
	// �������Ҽ�ͷ�ل�Ŀ��
	PlayerInputComponent->BindAction("SwitchTargetLeft", IE_Pressed, this, &AMyCharacter::SwitchLockOnTargetLeft);
	PlayerInputComponent->BindAction("SwitchTargetRight", IE_Pressed, this, &AMyCharacter::SwitchLockOnTargetRight);

	// ==================== ��������� ====================
	// ���ӵ��԰�key (F��) ����������
	PlayerInputComponent->BindAction("DebugInput", IE_Pressed, this, &AMyCharacter::DebugInputTest);
	
	// ����Ŀ��ߴ�������԰��� (G��)
	PlayerInputComponent->BindAction("DebugTargetSizes", IE_Pressed, this, &AMyCharacter::DebugDisplayTargetSizes);
	
	// ��ӡ��ǰ��״̬
	UE_LOG(LogTemp, Warning, TEXT("Input bindings set up successfully"));
	UE_LOG(LogTemp, Warning, TEXT("Available debug commands:"));
	UE_LOG(LogTemp, Warning, TEXT("- DebugInput (F): General debug info"));
	UE_LOG(LogTemp, Warning, TEXT("- DebugTargetSizes (G): Enemy size analysis"));
}

// ==================== �ƶ�����ʵ�� ====================
void AMyCharacter::MoveForward(float Value)
{
	ForwardInputValue = Value;

	// �ȴ��������������ƶ�����
	if (CameraControlComponent)
	{
		bool bIsMoving = (FMath::Abs(Value) > 0.1f);
		CameraControlComponent->HandlePlayerMovement(bIsMoving);
	}

	// �������Ƿ����ƶ���������������߼���
	bPlayerIsMoving = (FMath::Abs(Value) > 0.1f || FMath::Abs(RightInputValue) > 0.1f);

	// �����ҿ�ʼ�ƶ��Ҵ�������״̬������������������ت��
	if (bPlayerIsMoving && bIsLockedOn)
	{
		// ������ڽ���ƽ���l�������ͣSTOP���ָ���������
		if (bIsSmoothSwitching)
		{
			bIsSmoothSwitching = false;
			bShouldSmoothSwitchCamera = false;
			bShouldSmoothSwitchCharacter = false;
			UE_LOG(LogTemp, Warning, TEXT("Player movement interrupted smooth target switch"));
		}
		
		// ������ڽ�������Զ�����������ͣSTOP
		if (bIsCameraAutoCorrection)
		{
			bIsCameraAutoCorrection = false;
			DelayedCorrectionTarget = nullptr;
			UE_LOG(LogTemp, Warning, TEXT("Player movement interrupted camera auto correction"));
		}
		
		if (!bShouldCameraFollowTarget)
		{
			bShouldCameraFollowTarget = true;
			bShouldCharacterRotateToTarget = true; // ͬʱ��������ت��
			UE_LOG(LogTemp, Warning, TEXT("Player started moving - enabling camera follow and character rotation"));
		}
	}

	if (Value != 0.0f && Controller)
	{
		if (bIsLockedOn)
		{
			// ����״̬�£�����ڽ�ɫ�����ƶ�
			const FVector Direction = GetActorForwardVector();
			AddMovementInput(Direction, Value);
		}
		else
		{
			// ����״̬�£��������������ƶ�
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);
			const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			AddMovementInput(Direction, Value);
		}
	}
}

void AMyCharacter::MoveRight(float Value)
{
	RightInputValue = Value;

	// �ȴ��������������ƶ�����
	if (CameraControlComponent)
	{
		bool bIsMoving = (FMath::Abs(Value) > 0.1f);
		CameraControlComponent->HandlePlayerMovement(bIsMoving);
	}

	// �������Ƿ����ƶ���������������߼���
	bPlayerIsMoving = (FMath::Abs(Value) > 0.1f || FMath::Abs(ForwardInputValue) > 0.1f);

	// �����ҿ�ʼ�ƶ��Ҵ�������״̬������������������ت��
	if (bPlayerIsMoving && bIsLockedOn)
	{
		// ������ڽ���ƽ���l�������ͣSTOP���ָ���������
		if (bIsSmoothSwitching)
		{
			bIsSmoothSwitching = false;
			bShouldSmoothSwitchCamera = false;
			bShouldSmoothSwitchCharacter = false;
			UE_LOG(LogTemp, Warning, TEXT("Player movement interrupted smooth target switch"));
		}
		
		// �����ڽ�������Զ�����������ͣSTOP
		if (bIsCameraAutoCorrection)
		{
			bIsCameraAutoCorrection = false;
			DelayedCorrectionTarget = nullptr;
			UE_LOG(LogTemp, Warning, TEXT("Player movement interrupted camera auto correction"));
		}
		
		if (!bShouldCameraFollowTarget)
		{
			bShouldCameraFollowTarget = true;
			bShouldCharacterRotateToTarget = true; // ͬʱ��������ت��
			UE_LOG(LogTemp, Warning, TEXT("Player started moving - enabling camera follow and character rotation"));
		}
	}

	if (Value != 0.0f && Controller)
	{
		if (bIsLockedOn)
		{
			// ����״̬�£�����ڽ�ɫ�����ƶ�
			const FVector Direction = GetActorRightVector();
			AddMovementInput(Direction, Value);
		}
		else
		{
			// ����״̬�£��������������ƶ�
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);
			const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
			AddMovementInput(Direction, Value);
		}
	}
}

void AMyCharacter::StartJump()
{
	Jump();
}

void AMyCharacter::StopJump()
{
	StopJumping();
}

// ==================== ������� ====================
void AMyCharacter::Turn(float Rate)
{
	// �ȴ�����������������
	if (CameraControlComponent)
	{
		CameraControlComponent->HandlePlayerInput(Rate, 0.0f);
	}

	// ����״̬����ȫ��ֹ��ҵ�ˮƽ�������
	if (bIsLockedOn)
	{
		// ������«̳��ɣ� ֻ��������ıص�
		if (FMath::Abs(Rate) > 0.1f)
		{
			// ��������������ͼ��ͣ STOP 慢速转向时相机保持
			if (bIsCameraAutoCorrection)
			{
				bIsCameraAutoCorrection = false;
				DelayedCorrectionTarget = nullptr;
				if (bEnableCameraDebugLogs)
				{
					UE_LOG(LogTemp, Log, TEXT("Player camera input detected - stopping auto correction"));
				}
			}
			
			if (bIsSmoothCameraReset)
			{
				bIsSmoothCameraReset = false;
				if (bEnableCameraDebugLogs)
				{
					UE_LOG(LogTemp, Log, TEXT("Player camera input detected - stopping smooth camera reset"));
				}
			}
		}
		return; // ����״̬��ֱ�L��
	}

	// ������״̬�µ��������
	if (FMath::Abs(Rate) > 0.1f)
	{
		// ��������������ͼ��ͣ STOP 慢速转向时相机保持
		if (bIsCameraAutoCorrection)
		{
			bIsCameraAutoCorrection = false;
			DelayedCorrectionTarget = nullptr;
			if (bEnableCameraDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("Player camera input detected - stopping auto correction"));
			}
		}
		
		if (bIsSmoothCameraReset)
		{
			bIsSmoothCameraReset = false;
			if (bEnableCameraDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("Player camera input detected - stopping smooth camera reset"));
			}
		}
	}
	
	// ������״̬��������ў������
	AddControllerYawInput(Rate);
}

void AMyCharacter::LookUp(float Rate)
{
	// �ȴ�����������������
	if (CameraControlComponent)
	{
		CameraControlComponent->HandlePlayerInput(0.0f, Rate);
	}

	// ����״̬����ȫ��ֹ��ҵĴ�ֱ�������
	if (bIsLockedOn)
	{
		// ������«̳��ɣ� ֻ��������ıص�
		if (FMath::Abs(Rate) > 0.1f)
		{
			// ��������������ͼ��ͣ STOP 慢速转向时相机保持
			if (bIsCameraAutoCorrection)
			{
				bIsCameraAutoCorrection = false;
				DelayedCorrectionTarget = nullptr;
				if (bEnableCameraDebugLogs)
				{
					UE_LOG(LogTemp, Log, TEXT("Player camera input detected - stopping auto correction"));
				}
			}
			
			if (bIsSmoothCameraReset)
			{
				bIsSmoothCameraReset = false;
				if (bEnableCameraDebugLogs)
				{
					UE_LOG(LogTemp, Log, TEXT("Player camera input detected - stopping smooth camera reset"));
				}
			}
		}
		return; // ����״̬��ֱ�L��
	}

	// ������״̬�µ��������
	if (FMath::Abs(Rate) > 0.1f)
	{
		// ��������������ͼ��ͣ STOP 慢速转向时相机保持
		if (bIsCameraAutoCorrection)
		{
			bIsCameraAutoCorrection = false;
			DelayedCorrectionTarget = nullptr;
			if (bEnableCameraDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("Player camera input detected - stopping auto correction"));
			}
		}
		
		if (bIsSmoothCameraReset)
		{
			bIsSmoothCameraReset = false;
			if (bEnableCameraDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("Player camera input detected - stopping smooth camera reset"));
			}
		}
	}
	
	// ������״̬��������在哪里����
	AddControllerPitchInput(Rate);
}

// ==================== ����ϵͳʵ�� ====================
void AMyCharacter::ToggleLockOn()
{
	if (bEnableLockOnDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("ToggleLockOn called - Current state: %s"), 
			bIsLockedOn ? TEXT("LOCKED") : TEXT("UNLOCKED"));
	}
	
	if (bIsLockedOn)
	{
		// �����UI�������� - �ڵ€�CancelLockOn֮ǿ����������UI
		if (bEnableLockOnDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("Cancelling lock-on..."));
		}
		
		HideAllLockOnWidgets();
		if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport()) 
		{
			LockOnWidgetInstance->RemoveFromViewport();
			LockOnWidgetInstance = nullptr;
		}
		
		// ȡ���������߼����ֲ���
		CancelLockOn();
	}
	else
	{
		if (bEnableLockOnDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("Attempting to start lock-on..."));
		}
		
		FindLockOnCandidates();
		
		if (bEnableLockOnDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("Found %d lock-on candidates"), LockOnCandidates.Num());
		}
		
		// ֻ������������ڵ�Ŀ��
		AActor* SectorTarget = TryGetSectorLockTarget();
		
		if (SectorTarget)
		{
			if (bEnableLockOnDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("Found sector target: %s"), *SectorTarget->GetName());
			}
			// ���1������������������Χ�� �� ��������
			StartLockOn(SectorTarget);
		}
		else
		{
			if (bEnableLockOnDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("No valid sector target found, performing camera reset"));
			}
			// ���2�������Ƿ��е��ˣ���ִ�о�ͷ����
			PerformSimpleCameraReset();
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("=== ToggleLockOn complete ==="));
}

// ������ִ�m򵥵�������ã��޸�Ϊƽ���汾��
void AMyCharacter::PerformSimpleCameraReset()
{
	if (CameraControlComponent)
	{
		CameraControlComponent->PerformSimpleCameraReset();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::PerformSimpleCameraReset: CameraControlComponent is null!"));
	}
}

// ��������ʼƽ���������
void AMyCharacter::StartSmoothCameraReset()
{
	if (CameraControlComponent)
	{
		CameraControlComponent->StartSmoothCameraReset();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::StartSmoothCameraReset: CameraControlComponent is null!"));
	}
}

// ���������ƽ���������
void AMyCharacter::UpdateSmoothCameraReset()
{
	if (CameraControlComponent)
	{
		CameraControlComponent->UpdateSmoothCameraReset();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::UpdateSmoothCameraReset: CameraControlComponent is null!"));
	}
}

// ==================== 开始锁定目标
void AMyCharacter::StartLockOn(AActor* Target)
{
	if (!Target)
		return;
		
	bIsLockedOn = true;
	CurrentLockOnTarget = Target;
	
	// ==================== 应用体型相关的相机配置（新增） ====================
	EEnemySizeCategory TargetSize = GetTargetSizeCategory(Target);
	if (SizeBasedCameraConfigs.Contains(TargetSize))
	{
		// 应用对应体型的相机配置
		ApplyCameraConfigForEnemySize(TargetSize);
		UE_LOG(LogTemp, Warning, TEXT("StartLockOn: Applied camera config for %s enemy"), 
			*UEnum::GetValueAsString(TargetSize));
	}
	else
	{
		// 使用默认锁定调整
		if (CameraBoom)
		{
			CameraBoom->TargetArmLength = CameraSetupConfig.ArmLength * CameraSetupConfig.LockOnArmLengthMultiplier;
		}
	}
	
	// 设置锁定状态下的移动速度
	GetCharacterMovement()->MaxWalkSpeed = LockedWalkSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	
	// 显示锁定UI
	ShowLockOnWidget();
	
	UE_LOG(LogTemp, Warning, TEXT("Started lock-on with target: %s"), *Target->GetName());
}

// ������ȡ������
void AMyCharacter::CancelLockOn()
{
	if (!bIsLockedOn)
		return;
	
	// 强制隐藏当前目标和所有候选目标的UI（最高优先级）
	HideAllLockOnWidgets();
	
	// 额外保障，清理屏幕空间UI也清理
	if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
	{
		LockOnWidgetInstance->RemoveFromViewport();
		LockOnWidgetInstance = nullptr;
	}
	
	// ==================== 重置相机配置到默认（新增） ====================
	ResetCameraToDefaultConfig();
	
	// 重置锁定状态
	bIsLockedOn = false;
	PreviousLockOnTarget = CurrentLockOnTarget;
	CurrentLockOnTarget = nullptr;
	
	// 重置相机跟随状态
	bShouldCameraFollowTarget = true;
	bShouldCharacterRotateToTarget = true;
	
	// 停止任何正在进行的平滑切换或自动修正
	bIsSmoothSwitching = false;
	bShouldSmoothSwitchCamera = false;
	bShouldSmoothSwitchCharacter = false;
	bIsCameraAutoCorrection = false;
	DelayedCorrectionTarget = nullptr;
	
	// **关键修复**：实际执行相机重置，让相机脱离对敌人的锁定
	if (CameraControlComponent)
	{
		// 清除相机控制组件中的锁定目标
		CameraControlComponent->ClearLockOnTarget();
		
		// 执行相机重置，让相机回到正常跟随状态
		CameraControlComponent->StartSmoothCameraReset();
		
		if (bEnableLockOnDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("CancelLockOn: Camera reset initiated via CameraControlComponent"));
		}
	}
	else
	{
		// 备用方案：如果相机控制组件不可用，使用传统重置方法
		PerformSimpleCameraReset();
		
		if (bEnableLockOnDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("CancelLockOn: Using fallback camera reset method"));
		}
	}
	
	// 恢复正常移动设置
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
	
	UE_LOG(LogTemp, Warning, TEXT("Lock-on cancelled with camera reset"));
}

// �������������
void AMyCharacter::ResetCamera()
{
	// �����������ֻ�ǵ��ü����ã����ּ�����
	PerformSimpleCameraReset();
}

// ��Ϊ�������
void AMyCharacter::StartCameraReset(const FRotator& TargetRotation)
{
	if (CameraControlComponent)
	{
		CameraControlComponent->StartCameraReset(TargetRotation);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::StartCameraReset: CameraControlComponent is null!"));
	}
}

// ==================== ���º��� ====================
void AMyCharacter::FindLockOnCandidates()
{
	if (TargetDetectionComponent)
	{
		TargetDetectionComponent->FindLockOnCandidates();
		LockOnCandidates = TargetDetectionComponent->GetLockOnCandidates();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::FindLockOnCandidates: TargetDetectionComponent is null!"));
		LockOnCandidates.Empty();
	}
}

bool AMyCharacter::IsValidLockOnTarget(AActor* Target)
{
	if (TargetDetectionComponent)
	{
		return TargetDetectionComponent->IsValidLockOnTarget(Target);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::IsValidLockOnTarget: TargetDetectionComponent is null!"));
		return false;
	}
}

bool AMyCharacter::IsTargetInSectorLockZone(AActor* Target) const
{
	if (!Target || !Controller)
		return false;

	FVector PlayerLocation = GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector CameraForward = Controller->GetControlRotation().Vector();
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

	// ����Ƕ�
	float DotProduct = FVector::DotProduct(CameraForward, ToTarget);
	DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	
	float AngleRadians = FMath::Acos(DotProduct);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	// ����Ƿ�����������������
	bool bInSectorZone = AngleDegrees <= (SECTOR_LOCK_ANGLE * 0.5f);

	return bInSectorZone;
}

bool AMyCharacter::IsTargetInEdgeDetectionZone(AActor* Target) const
{
	if (!Target || !Controller)
		return false;

	FVector PlayerLocation = GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector CameraForward = Controller->GetControlRotation().Vector();
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

	// ����Ƕ�
	float DotProduct = FVector::DotProduct(CameraForward, ToTarget);
	DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	
	float AngleRadians = FMath::Acos(DotProduct);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	// ����Ƿ��ڱ�Ե���������
	bool bInEdgeZone = (AngleDegrees > (SECTOR_LOCK_ANGLE * 0.5f)) && 
					   (AngleDegrees <= (EDGE_DETECTION_ANGLE * 0.5f));

	return bInEdgeZone;
}

AActor* AMyCharacter::GetBestTargetFromList(const TArray<AActor*>& TargetList)
{
	if (TargetDetectionComponent)
	{
		return TargetDetectionComponent->GetBestTargetFromList(TargetList);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::GetBestTargetFromList: TargetDetectionComponent is null!"));
		return nullptr;
	}
}

AActor* AMyCharacter::GetBestLockOnTarget()
{
	return GetBestSectorLockTarget();
}

AActor* AMyCharacter::GetBestSectorLockTarget()
{
	if (TargetDetectionComponent)
	{
		return TargetDetectionComponent->GetBestSectorLockTarget();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::GetBestSectorLockTarget: TargetDetectionComponent is null!"));
		return nullptr;
	}
}

void AMyCharacter::UpdateLockOnTarget()
{
	if (!CurrentLockOnTarget)
	{
		bIsLockedOn = false;
		// ǿ������UI
		HideAllLockOnWidgets();
		if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
		{
			LockOnWidgetInstance->RemoveFromViewport();
			LockOnWidgetInstance = nullptr;
		}
		return;
	}

	// ��鵱ǰ����Ŀ���Ƿ���Ȼ��Ч
	if (!IsTargetStillLockable(CurrentLockOnTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("Current lock-on target is no longer lockable - cancelling lock-on"));
		
		// ǿ����������UI��ʹ�ø�ǿ��������
		HideAllLockOnWidgets();
		if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
		{
			LockOnWidgetInstance->RemoveFromViewport();
			LockOnWidgetInstance = nullptr;
		}
		
		// Ȼ��ȡ������ش̴�
		bIsLockedOn = false;
		PreviousLockOnTarget = CurrentLockOnTarget;
		CurrentLockOnTarget = nullptr;
		bShouldCameraFollowTarget = true;
		bShouldCharacterRotateToTarget = true;
		
		// ͣStopping any ongoing smooth switching or auto-correction
		bIsSmoothSwitching = false;
		bShouldSmoothSwitchCamera = false;
		bShouldSmoothSwitchCharacter = false;
		bIsCameraAutoCorrection = false;
		DelayedCorrectionTarget = nullptr;
		
		// �ָ������ƶ�����
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
	}
}

bool AMyCharacter::IsTargetStillLockable(AActor* Target)
{
	if (TargetDetectionComponent)
	{
		return TargetDetectionComponent->IsTargetStillLockable(Target);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::IsTargetStillLockable: TargetDetectionComponent is null!"));
		return false;
	}
}

void AMyCharacter::UpdateLockOnCamera()
{
	if (CameraControlComponent)
	{
		CameraControlComponent->SetLockOnTarget(CurrentLockOnTarget);
		CameraControlComponent->UpdateLockOnCamera();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::UpdateLockOnCamera: CameraControlComponent is null!"));
	}
}

void AMyCharacter::UpdateCharacterRotationToTarget()
{
	if (CameraControlComponent && CurrentLockOnTarget)
	{
		CameraControlComponent->SetLockOnTarget(CurrentLockOnTarget);
		CameraControlComponent->UpdateCharacterRotationToTarget();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::UpdateCharacterRotationToTarget: CameraControlComponent or CurrentLockOnTarget is null!"));
	}
}

float AMyCharacter::CalculateAngleToTarget(AActor* Target) const
{
	if (!IsValid(Target) || !Controller)
		return 180.0f;

	FVector PlayerLocation = GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector CurrentCameraForward = Controller->GetControlRotation().Vector();
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

	// ����ǶȲ���
	float DotProduct = FVector::DotProduct(CurrentCameraForward, ToTarget);
	DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	
	float AngleRadians = FMath::Acos(DotProduct);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	return AngleDegrees;
}

float AMyCharacter::CalculateDirectionAngle(AActor* Target) const
{
	if (!IsValid(Target) || !Controller)
		return 0.0f;

	FVector PlayerLocation = GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FRotator ControlRotation = Controller->GetControlRotation();
	FVector CurrentCameraForward = ControlRotation.Vector();
	FVector CurrentCameraRight = ControlRotation.RotateVector(FVector::RightVector);

	// ���㵽Ŀ�������
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

	// ������������ǰ����ĽǶ�
	float ForwardDot = FVector::DotProduct(CurrentCameraForward, ToTarget);
	float RightDot = FVector::DotProduct(CurrentCameraRight, ToTarget);

	// ʹ��atan2����Ƕȣ���Χ��-180��180
	float AngleRadians = FMath::Atan2(RightDot, ForwardDot);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	return AngleDegrees;
}

void AMyCharacter::SortCandidatesByDirection(TArray<AActor*>& Targets)
{
	if (TargetDetectionComponent)
	{
		TargetDetectionComponent->SortCandidatesByDirection(Targets);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::SortCandidatesByDirection: TargetDetectionComponent is null!"));
	}
}

void AMyCharacter::StartSmoothTargetSwitch(AActor* NewTarget)
{
	if (CameraControlComponent && NewTarget)
	{
		CameraControlComponent->StartSmoothTargetSwitch(NewTarget);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::StartSmoothTargetSwitch: CameraControlComponent or NewTarget is null!"));
	}
}

void AMyCharacter::UpdateSmoothTargetSwitch()
{
	if (CameraControlComponent)
	{
		CameraControlComponent->UpdateSmoothTargetSwitch();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::UpdateSmoothTargetSwitch: CameraControlComponent is null!"));
	}
}

// ==================== ���봦������ ====================
void AMyCharacter::HandleRightStickX(float Value)
{
	// ֻ������״̬�´�����ق���ل�Ŀ��
	if (!bIsLockedOn)
		return;

	// ���ق�˴�����λ���ƶ�������
	bRightStickLeftPressed = (Value < -THUMBSTICK_THRESHOLD);
	bRightStickRightPressed = (Value > THUMBSTICK_THRESHOLD);

	// ��ⰴ���¼�����δ���µ����£�
	if (bRightStickLeftPressed)
	{
		SwitchLockOnTargetLeft();
	}
	else if (bRightStickRightPressed)
	{
		SwitchLockOnTargetRight();
	}

	// ����״̬
	LastRightStickX = Value;
}

void AMyCharacter::HandleLockOnButton()
{
	ToggleLockOn();
}

void AMyCharacter::SwitchLockOnTargetLeft()
{
	if (!bIsLockedOn || LockOnCandidates.Num() <= 1)
		return;

	// �л�ǰ��ǿ����������UI��ȷ��û������
	HideAllLockOnWidgets();

	// ֻ�ڱ�Ҫʱˢ�º�ѡĿ���б�
	if (LockOnCandidates.Num() == 0)
	{
		FindLockOnCandidates();
	}

	int32 CurrentIndex = LockOnCandidates.Find(CurrentLockOnTarget);
	if (CurrentIndex != INDEX_NONE && CurrentIndex > 0)
	{
		int32 NewIndex = CurrentIndex - 1;
		AActor* NewTarget = LockOnCandidates[NewIndex];
		
		if (IsValidLockOnTarget(NewTarget))
		{
			PreviousLockOnTarget = CurrentLockOnTarget;
			CurrentLockOnTarget = NewTarget;
			StartSmoothTargetSwitch(NewTarget);
			ShowLockOnWidget();
		}
	}
}

void AMyCharacter::SwitchLockOnTargetRight()
{
	if (!bIsLockedOn || LockOnCandidates.Num() <= 1)
		return;

	// �л�ǰ��ǿ����������UI��ȷ��û������
	HideAllLockOnWidgets();

	// ֻ�ڱ�Ҫʱˢ�º�ѡĿ���б�
	if (LockOnCandidates.Num() == 0)
	{
		FindLockOnCandidates();
	}

	int32 CurrentIndex = LockOnCandidates.Find(CurrentLockOnTarget);
	if (CurrentIndex != INDEX_NONE && CurrentIndex < LockOnCandidates.Num() - 1)
	{
		int32 NewIndex = CurrentIndex + 1;
		AActor* NewTarget = LockOnCandidates[NewIndex];
		
		if (IsValidLockOnTarget(NewTarget))
		{
			PreviousLockOnTarget = CurrentLockOnTarget;
			CurrentLockOnTarget = NewTarget;
			StartSmoothTargetSwitch(NewTarget);
			ShowLockOnWidget();
		}
	}
}

void AMyCharacter::DebugInputTest()
{
	UE_LOG(LogTemp, Warning, TEXT("=== DEBUG INPUT TEST ==="));
	UE_LOG(LogTemp, Warning, TEXT("IsLockedOn: %s"), bIsLockedOn ? TEXT("True") : TEXT("False"));
	UE_LOG(LogTemp, Warning, TEXT("Current Target: %s"), CurrentLockOnTarget ? *CurrentLockOnTarget->GetName() : TEXT("None"));
	UE_LOG(LogTemp, Warning, TEXT("Available Targets: %d"), LockOnCandidates.Num());
	UE_LOG(LogTemp, Warning, TEXT("========================"));
}

void AMyCharacter::DebugWidgetSetup()
{
	UE_LOG(LogTemp, Warning, TEXT("=== WIDGET SETUP DEBUG ==="));
	UE_LOG(LogTemp, Warning, TEXT("LockOnWidgetClass is set: %s"), LockOnWidgetClass ? TEXT("YES") : TEXT("NO"));
	
	if (LockOnWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("LockOnWidgetClass name: %s"), *LockOnWidgetClass->GetName());
		UE_LOG(LogTemp, Warning, TEXT("LockOnWidgetClass path: %s"), *LockOnWidgetClass->GetPathName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("LockOnWidgetClass is NULL! Attempting to find available Widgets..."));
		
		// ���Բ��ҿ��õ�Widget��
		TArray<FString> PossibleWidgetPaths = {
			TEXT("/Game/Levels/Widget_LockOnIcon.Widget_LockOnIcon_C"),
			TEXT("/Game/LockOnTS/Widgets/UI_LockOnWidget.UI_LockOnWidget_C"),
			TEXT("Widget_LockOnIcon_C"),
			TEXT("UI_LockOnWidget_C")
		};
		
		UE_LOG(LogTemp, Warning, TEXT("Searching for Widget classes:"));
		for (const FString& WidgetPath : PossibleWidgetPaths)
		{
			UClass* FoundWidgetClass = LoadObject<UClass>(nullptr, *WidgetPath);
			if (!FoundWidgetClass)
			{
				FoundWidgetClass = FindObject<UClass>(ANY_PACKAGE, *WidgetPath);
			}
			
			if (FoundWidgetClass && FoundWidgetClass->IsChildOf(UUserWidget::StaticClass()))
			{
				UE_LOG(LogTemp, Warning, TEXT("? FOUND: %s"), *WidgetPath);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("? NOT FOUND: %s"), *WidgetPath);
			}
		}
		
		UE_LOG(LogTemp, Error, TEXT("Steps to fix:"));
		UE_LOG(LogTemp, Error, TEXT("1. Open your character Blueprint"));
		UE_LOG(LogTemp, Error, TEXT("2. Find 'Lock On Widget Class' in the UI category"));
		UE_LOG(LogTemp, Error, TEXT("3. Set it to Widget_LockOnIcon or UI_LockOnWidget"));
		UE_LOG(LogTemp, Error, TEXT("4. Ensure the Widget has 'UpdateLockOnPostition' Custom Event"));
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Socket projection enabled: %s"), bUseSocketProjection ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Warning, TEXT("Target Socket Name: %s"), *TargetSocketName.ToString());
	UE_LOG(LogTemp, Warning, TEXT("Socket Offset: (%.1f, %.1f, %.1f)"), SocketOffset.X, SocketOffset.Y, SocketOffset.Z);
	UE_LOG(LogTemp, Warning, TEXT("Current widget instance: %s"), LockOnWidgetInstance ? TEXT("EXISTS") : TEXT("NULL"));
	
	// ���PlayerController
	APlayerController* PC = Cast<APlayerController>(GetController());
	UE_LOG(LogTemp, Warning, TEXT("PlayerController available: %s"), PC ? TEXT("YES") : TEXT("NO"));
	
	// ��鵱ǰ����Ŀ��
	if (CurrentLockOnTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Current lock-on target: %s"), *CurrentLockOnTarget->GetName());
		
		// ���Ŀ���Ƿ���ָ����Socket
		bool bHasSocket = HasValidSocket(CurrentLockOnTarget);
		UE_LOG(LogTemp, Warning, TEXT("Target has Socket '%s': %s"), *TargetSocketName.ToString(), bHasSocket ? TEXT("YES") : TEXT("NO"));
		
		if (bHasSocket)
		{
			FVector SocketLocation = GetTargetSocketWorldLocation(CurrentLockOnTarget);
			UE_LOG(LogTemp, Warning, TEXT("Socket world location: (%.1f, %.1f, %.1f)"), SocketLocation.X, SocketLocation.Y, SocketLocation.Z);
			
			FVector2D ScreenPos = ProjectSocketToScreen(SocketLocation);
			UE_LOG(LogTemp, Warning, TEXT("Socket screen position: (%.1f, %.1f)"), ScreenPos.X, ScreenPos.Y);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No current lock-on target"));
	}
	
	UE_LOG(LogTemp, Warning, TEXT("========================"));
}

// ==================== UI��غ��� ====================
void AMyCharacter::ShowLockOnWidget()
{
	if (!CurrentLockOnTarget)
		return;

	// ������������Ŀ���UI
	HideAllLockOnWidgets();

	// �������SocketͶ�䣬ʹ���µ�SocketͶ��UIϵͳ
	if (bUseSocketProjection)
	{
		ShowSocketProjectionWidget();
		return;
	}

	// ��ͳ��3D�ռ�UI��ʾ�����ּ����ԣ�
	UActorComponent* WidgetComp = CurrentLockOnTarget->GetComponentByClass(UWidgetComponent::StaticClass());
	if (WidgetComp)
	{
		UWidgetComponent* WidgetComponent = Cast<UWidgetComponent>(WidgetComp);
		if (WidgetComponent)
		{
			WidgetComponent->SetVisibility(true);
			PreviousLockOnTarget = CurrentLockOnTarget;
			return;
		}
	}

	// ���÷�����ʹ����Ļ�ռ�UI
	if (!LockOnWidgetClass)
		return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
		return;

	if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
	{
		LockOnWidgetInstance->RemoveFromViewport();
	}

	LockOnWidgetInstance = CreateWidget<UUserWidget>(PC, LockOnWidgetClass);
	if (LockOnWidgetInstance)
	{
		LockOnWidgetInstance->AddToViewport();
		PreviousLockOnTarget = CurrentLockOnTarget;
	}
}

void AMyCharacter::HideLockOnWidget()
{
	HideAllLockOnWidgets();

	// ����SocketͶ��UI
	if (bUseSocketProjection)
	{
		HideSocketProjectionWidget();
	}

	if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
	{
		LockOnWidgetInstance->RemoveFromViewport();
	}

	LockOnWidgetInstance = nullptr;
	PreviousLockOnTarget = nullptr;
}

void AMyCharacter::UpdateLockOnWidget()
{
	// �����������״̬��ȷ������UI������
	if (!bIsLockedOn)
	{
		HideAllLockOnWidgets();
		if (bUseSocketProjection)
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

	// ���Ŀ���Ƿ����仯
	bool bTargetChanged = (CurrentLockOnTarget != PreviousLockOnTarget);
	
	if (bTargetChanged && IsValid(PreviousLockOnTarget))
	{
		// ������һ��Ŀ���UI
		if (bUseSocketProjection)
		{
			HideSocketProjectionWidget();
		}
		else
		{
			UActorComponent* PrevWidgetComp = PreviousLockOnTarget->GetComponentByClass(UWidgetComponent::StaticClass());
			if (PrevWidgetComp)
			{
				UWidgetComponent* PrevWidgetComponent = Cast<UWidgetComponent>(PrevWidgetComp);
				if (PrevWidgetComponent && PrevWidgetComponent->IsVisible())
				{
					PrevWidgetComponent->SetVisibility(false);
				}
			}
		}
		
		PreviousLockOnTarget = CurrentLockOnTarget;
	}

	// �����ǰ����%%$�Ŀ���Ҵ�������״̬������UI
	if (bIsLockedOn && IsValid(CurrentLockOnTarget))
	{
		if (bUseSocketProjection)
		{
			// ʹ��SocketͶ�����UIλ��
			UpdateSocketProjectionWidget();
		}
		else
		{
			// ��ͳ3D�ռ�UI����
			UActorComponent* WidgetComp = CurrentLockOnTarget->GetComponentByClass(UWidgetComponent::StaticClass());
			if (WidgetComp)
			{
				UWidgetComponent* WidgetComponent = Cast<UWidgetComponent>(WidgetComp);
				if (WidgetComponent && (!WidgetComponent->IsVisible() || bTargetChanged))
				{
					WidgetComponent->SetVisibility(true);
				}
			}
		}
	}
}

// ==================== SocketͶ��ϵͳʵ�� ====================
FVector AMyCharacter::GetTargetSocketWorldLocation(AActor* Target) const
{
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetTargetSocketWorldLocation: Target is null"));
		return FVector::ZeroVector;
	}

	// ���İ�ȡĿ��Ĺ����������
	USkeletalMeshComponent* SkeletalMesh = Target->FindComponentByClass<USkeletalMeshComponent>();
	if (SkeletalMesh && SkeletalMesh->DoesSocketExist(TargetSocketName))
	{
		// ���Socket���ڣ�����Socket������λ�ü���ƫ��
		FVector SocketLocation = SkeletalMesh->GetSocketLocation(TargetSocketName);
		FVector FinalLocation = SocketLocation + SocketOffset;
		
		// �����Ż��� ع�͸�Ƶ��־
		// UE_LOG(LogTemp, Verbose, TEXT("Socket '%s' found on target '%s' at location (%.1f, %.1f, %.1f)"), 
		//		*TargetSocketName.ToString(), *Target->GetName(), FinalLocation.X, FinalLocation.Y, FinalLocation.Z);
		
		return FinalLocation;
	}
	else
	{
		// ���û��Socket������Actor��λ�ü���ƫ��
		FVector FallbackLocation = Target->GetActorLocation() + SocketOffset;
		
		// �����Ż��� ع�͸�Ƶ��־
		// UE_LOG(LogTemp, Verbose, TEXT("Socket '%s' not found on target '%s', using fallback location (%.1f, %.1f, %.1f)"), 
		//		*TargetSocketName.ToString(), *Target->GetName(), FallbackLocation.X, FallbackLocation.Y, FallbackLocation.Z);
		
		return FallbackLocation;
	}
}

bool AMyCharacter::HasValidSocket(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	// ���Ŀ���Ƿ���ָ����Socket
	USkeletalMeshComponent* SkeletalMesh = Target->FindComponentByClass<USkeletalMeshComponent>();
	if (SkeletalMesh)
	{
		bool bSocketExists = SkeletalMesh->DoesSocketExist(TargetSocketName);
		// �����Ż��� ع�͸�Ƶ��־
		// UE_LOG(LogTemp, Verbose, TEXT("Socket check for '%s': Socket '%s' exists = %s"), 
		//		*Target->GetName(), *TargetSocketName.ToString(), bSocketExists ? TEXT("YES") : TEXT("NO"));
		return bSocketExists;
	}

	// �����Ż��� ع�͸�Ƶ��־
	// UE_LOG(LogTemp, Verbose, TEXT("Target '%s' has no SkeletalMeshComponent"), *Target->GetName());
	return false;
}

FVector2D AMyCharacter::ProjectSocketToScreen(const FVector& SocketWorldLocation) const
{
	// ��ȡ��ҿ�����
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProjectSocketToScreen: No PlayerController"));
		return FVector2D::ZeroVector;
	}

	// ������λ��Ͷ�䵽��Ļ����
	FVector2D ScreenLocation;
	bool bProjected = PC->ProjectWorldLocationToScreen(SocketWorldLocation, ScreenLocation);
	
	if (bProjected)
	{
		// �����Ż���ע�͸�Ƶ��־
		// UE_LOG(LogTemp, Verbose, TEXT("Projection successful: World(%.1f, %.1f, %.1f) -> Screen(%.1f, %.1f)"), 
		//	SocketWorldLocation.X, SocketWorldLocation.Y, SocketWorldLocation.Z,
		//	ScreenLocation.X, ScreenLocation.Y);
		return ScreenLocation;
	}
	else
	{
		// �����Ż���ع�͸�Ƶ��־
		// UE_LOG(LogTemp, Verbose, TEXT("Projection failed for world location (%.1f, %.1f, %.1f)"), 
		//	SocketWorldLocation.X, SocketWorldLocation.Y, SocketWorldLocation.Z);
		return FVector2D::ZeroVector;
	}
}

void AMyCharacter::ShowSocketProjectionWidget()
{
	if (!CurrentLockOnTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("ShowSocketProjectionWidget: No current lock-on target"));
		return;
	}

	// ���LockOnWidgetClassû�����ã�����������ʱ����
	if (!LockOnWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowSocketProjectionWidget: LockOnWidgetClass is not set! Attempting runtime search..."));
		
		// ���Զ��ֿ��ܵ�Widget�����ƺ�·��
		TArray<FString> PossibleWidgetPaths = {
			TEXT("/Game/Levels/Widget_LockOnIcon.Widget_LockOnIcon_C"),
			TEXT("/Game/LockOnTS/Widgets/UI_LockOnWidget.UI_LockOnWidget_C"),
			TEXT("Widget_LockOnIcon_C"),
			TEXT("UI_LockOnWidget_C"),
			TEXT("/Game/Levels/Widget_LockOnIcon"),
			TEXT("/Game/LockOnTS/Widgets/UI_LockOnWidget")
		};
		
		for (const FString& WidgetPath : PossibleWidgetPaths)
		{
			UClass* FoundWidgetClass = LoadObject<UClass>(nullptr, *WidgetPath);
			
			if (!FoundWidgetClass)
			{
				// ����ͨ�����Ʋ���
				FoundWidgetClass = FindObject<UClass>(ANY_PACKAGE, *WidgetPath);
			}
			
			if (FoundWidgetClass && FoundWidgetClass->IsChildOf(UUserWidget::StaticClass()))
			{
				LockOnWidgetClass = FoundWidgetClass;
				UE_LOG(LogTemp, Warning, TEXT("Found Widget class at runtime: %s (Path: %s)"), 
					*FoundWidgetClass->GetName(), *WidgetPath);
				break;
			}
			else
			{
				UE_LOG(LogTemp, Verbose, TEXT("Widget not found at path: %s"), *WidgetPath);
			}
		}
		
		if (!LockOnWidgetClass)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not find any Widget class at runtime!"));
			UE_LOG(LogTemp, Error, TEXT("Available Widget files found in project:"));
			UE_LOG(LogTemp, Error, TEXT("- F:\\soul\\Content\\Levels\\Widget_LockOnIcon.uasset"));
			UE_LOG(LogTemp, Error, TEXT("- F:\\soul\\Content\\LockOnTS\\Widgets\\UI_LockOnWidget.uasset"));
			UE_LOG(LogTemp, Error, TEXT("Please ensure:"));
			UE_LOG(LogTemp, Error, TEXT("1. One of these Widgets is compiled and valid"));
			UE_LOG(LogTemp, Error, TEXT("2. LockOnWidgetClass is set in Blueprint to one of these Widgets"));
			UE_LOG(LogTemp, Error, TEXT("3. Widget contains UpdateLockOnPostition event"));
			UE_LOG(LogTemp, Error, TEXT("4. Call DebugWidgetSetup() function for more info"));
			return;
		}
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("ShowSocketProjectionWidget: No valid PlayerController"));
		return;
	}

	// �������ʵ�������ӿ��U����Ƴ�
	if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
	{
		UE_LOG(LogTemp, Warning, TEXT("Removing existing UMG widget instance"));
		LockOnWidgetInstance->RemoveFromViewport();
	}

	// �����µ�Widgetʵ��
	LockOnWidgetInstance = CreateWidget<UUserWidget>(PC, LockOnWidgetClass);
	if (LockOnWidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Successfully created UMG widget instance: %s"), *LockOnWidgetInstance->GetClass()->GetName());
		
		LockOnWidgetInstance->AddToViewport();
		UE_LOG(LogTemp, Warning, TEXT("UMG widget added to viewport"));
		
		// ��������λ��
		UpdateSocketProjectionWidget();
		
		PreviousLockOnTarget = CurrentLockOnTarget;
		
		UE_LOG(LogTemp, Warning, TEXT("Socket projection widget created and shown for target: %s"), 
			*CurrentLockOnTarget->GetName());
		UE_LOG(LogTemp, Warning, TEXT("Using Socket: %s"), *TargetSocketName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UMG widget instance! LockOnWidgetClass: %s"), 
			LockOnWidgetClass ? *LockOnWidgetClass->GetName() : TEXT("NULL"));
	}
}

void AMyCharacter::UpdateSocketProjectionWidget()
{
	if (!CurrentLockOnTarget || !LockOnWidgetInstance || !LockOnWidgetInstance->IsInViewport())
		return;

	// Get target Socket world location
	FVector SocketWorldLocation = GetTargetSocketWorldLocation(CurrentLockOnTarget);
	
	// Project to screen coordinates
	FVector2D ScreenPosition = ProjectSocketToScreen(SocketWorldLocation);
	
	// Check if projection is successful (screen coordinates are valid)
	if (ScreenPosition != FVector2D::ZeroVector)
	{
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
							if ( StructProp->Struct && StructProp->Struct->GetFName() == FName("Vector2D"))
							{
								// Set parameter value
								FVector2D* ParamPtr = reinterpret_cast<FVector2D*>(Params + Property->GetOffset_ForInternal());
								*ParamPtr = ScreenPosition;
								bFoundParam = true;
								// �����Ż���ع�͸�Ƶ��־
								// UE_LOG(LogTemp, Warning, TEXT("Setting UMG position parameter: (%.1f, %.1f) using function UpdateLockOnPostition"), 
								//	ScreenPosition.X, ScreenPosition.Y);
								break;
							}
						}
					}
				}
				
				if (bFoundParam)
				{
					// Call function
					LockOnWidgetInstance->ProcessEvent(UpdateFunction, Params);
					// �����Ż���ע�͸�Ƶ��־
					// UE_LOG(LogTemp, Warning, TEXT("Successfully called UMG update function UpdateLockOnPostition with position: (%.1f, %.1f)"), 
					//	ScreenPosition.X, ScreenPosition.Y);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Could not find Vector2D parameter in UMG function UpdateLockOnPostition"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("UMG function UpdateLockOnPostition has no parameters"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Could not find UMG update function UpdateLockOnPostition in widget class"));
 	 }
		
		// Ensure Widget is visible
		if (!LockOnWidgetInstance->IsVisible())
		{
			LockOnWidgetInstance->SetVisibility(ESlateVisibility::Visible);
			UE_LOG(LogTemp, Warning, TEXT("Set UMG widget visibility to Visible"));
		}
		
		// Debug log - including Socket information - ���ӽ�Ƶ����
		if (bEnableCameraDebugLogs)
		{
			static float LastSocketProjectionLogTime = 0.0f;
			float CurrentTime = GetWorld()->GetTimeSeconds();
			if (CurrentTime - LastSocketProjectionLogTime > 1.0f) // ÿ1���¼һ��
			{
				UE_LOG(LogTemp, Warning, TEXT("Socket projection updated: Socket(%s) World(%.1f, %.1f, %.1f) -> Screen(%.1f, %.1f)"), 
					*TargetSocketName.ToString(),
					SocketWorldLocation.X, SocketWorldLocation.Y, SocketWorldLocation.Z,
					ScreenPosition.X, ScreenPosition.Y);
				LastSocketProjectionLogTime = CurrentTime;
			}
		}
	}
	else
	{
		// Target is off-screen, hide Widget
		if (LockOnWidgetInstance->IsVisible())
		{
			LockOnWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
			UE_LOG(LogTemp, Warning, TEXT("Target off-screen, hiding UMG widget"));
		}
	}
}

void AMyCharacter::HideSocketProjectionWidget()
{
	if (LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
	{
		LockOnWidgetInstance->RemoveFromViewport();
		UE_LOG(LogTemp, Warning, TEXT("Socket projection widget hidden"));
	}
	
	LockOnWidgetInstance = nullptr;
}

// ==================== 调试辅助函数实现 ====================

void AMyCharacter::LogAllComponents()
{
	UE_LOG(LogTemp, Warning, TEXT("=== ALL COMPONENTS ON ACTOR ==="));
	
	TInlineComponentArray<UActorComponent*> AllComponents(this);
	for (UActorComponent* Comp : AllComponents)
	{
		if (Comp)
		{
			UE_LOG(LogTemp, Warning, TEXT("  - %s (Class: %s)"), 
				*Comp->GetName(), 
				*Comp->GetClass()->GetName());
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Total: %d components"), AllComponents.Num());
	UE_LOG(LogTemp, Warning, TEXT("================================"));
}

void AMyCharacter::LogCameraState(const FString& Context)
{
	if (!CameraBoom || !FollowCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Camera components not available"), *Context);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("=== CAMERA STATE [%s] ==="), *Context);
	UE_LOG(LogTemp, Warning, TEXT("  SpringArm:"));
	UE_LOG(LogTemp, Warning, TEXT("    - TargetArmLength: %.1f"), CameraBoom->TargetArmLength);
	UE_LOG(LogTemp, Warning, TEXT("    - RelativeRotation: (%.1f, %.1f, %.1f)"),
		CameraBoom->GetRelativeRotation().Pitch,
		CameraBoom->GetRelativeRotation().Yaw,
		CameraBoom->GetRelativeRotation().Roll);
	UE_LOG(LogTemp, Warning, TEXT("    - UsePawnControlRotation: %s"), 
		CameraBoom->bUsePawnControlRotation ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Warning, TEXT("    - CameraLag: %s (Speed: %.1f)"),
		CameraBoom->bEnableCameraLag ? TEXT("ON") : TEXT("OFF"),
		CameraBoom->CameraLagSpeed);
	
	UE_LOG(LogTemp, Warning, TEXT("  Camera:"));
	UE_LOG(LogTemp, Warning, TEXT("    - RelativeLocation: (%.1f, %.1f, %.1f)"),
		FollowCamera->GetRelativeLocation().X,
		FollowCamera->GetRelativeLocation().Y,
		FollowCamera->GetRelativeLocation().Z);
	UE_LOG(LogTemp, Warning, TEXT("================================"));
}

// ==================== 敌人尺寸分析接口实现 ====================

EEnemySizeCategory AMyCharacter::GetTargetSizeCategory(AActor* Target)
{
	if (TargetDetectionComponent)
	{
		return TargetDetectionComponent->GetTargetSizeCategory(Target);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::GetTargetSizeCategory: TargetDetectionComponent is null!"));
		return EEnemySizeCategory::Medium;
	}
}

TArray<AActor*> AMyCharacter::GetTargetsBySize(EEnemySizeCategory SizeCategory)
{
	if (TargetDetectionComponent)
	{
		return TargetDetectionComponent->GetTargetsBySize(SizeCategory);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::GetTargetsBySize: TargetDetectionComponent is null!"));
		return TArray<AActor*>();
	}
}

AActor* AMyCharacter::GetNearestTargetBySize(EEnemySizeCategory SizeCategory)
{
	if (TargetDetectionComponent)
	{
		return TargetDetectionComponent->GetNearestTargetBySize(SizeCategory);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::GetNearestTargetBySize: TargetDetectionComponent is null!"));
		return nullptr;
	}
}

TMap<EEnemySizeCategory, int32> AMyCharacter::GetSizeCategoryStatistics()
{
	if (TargetDetectionComponent)
	{
		return TargetDetectionComponent->GetSizeCategoryStatistics();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::GetSizeCategoryStatistics: TargetDetectionComponent is null!"));
		return TMap<EEnemySizeCategory, int32>();
	}
}

void AMyCharacter::DebugDisplayTargetSizes()
{
	// 直接在MyCharacter中实现目标尺寸显示，不依赖TargetDetectionComponent
	if (!IsValid(this))
	{
		UE_LOG(LogTemp, Error, TEXT("MyCharacter::DebugDisplayTargetSizes: Invalid character reference!"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("=== TARGET SIZE ANALYSIS ==="));
	
	// 如果没有候选目标，先查找
	if (LockOnCandidates.Num() == 0)
	{
		FindLockOnCandidates();
	}
	
	if (LockOnCandidates.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No targets found in detection range"));
		UE_LOG(LogTemp, Warning, TEXT("============================"));
		return;
	}
	
	// 统计各尺寸分类
	TMap<EEnemySizeCategory, int32> SizeStats;
	SizeStats.Add(EEnemySizeCategory::Small, 0);
	SizeStats.Add(EEnemySizeCategory::Medium, 0);
	SizeStats.Add(EEnemySizeCategory::Large, 0);
	SizeStats.Add(EEnemySizeCategory::Giant, 0);
	
	// 分析每个目标
	for (AActor* Target : LockOnCandidates)
	{
		if (IsValid(Target))
		{
			EEnemySizeCategory Size = GetTargetSizeCategory(Target);
			SizeStats[Size]++;
			
			float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
			UE_LOG(LogTemp, Warning, TEXT("  Target: %s"), *Target->GetName());
			UE_LOG(LogTemp, Warning, TEXT("    Size Category: %s"), *UEnum::GetValueAsString(Size));
			UE_LOG(LogTemp, Warning, TEXT("    Distance: %.1f"), Distance);
		}
	}
	
	// 显示统计信息
	UE_LOG(LogTemp, Warning, TEXT("Statistics:"));
	UE_LOG(LogTemp, Warning, TEXT("  Small: %d"), SizeStats[EEnemySizeCategory::Small]);
	UE_LOG(LogTemp, Warning, TEXT("  Medium: %d"), SizeStats[EEnemySizeCategory::Medium]);
	UE_LOG(LogTemp, Warning, TEXT("  Large: %d"), SizeStats[EEnemySizeCategory::Large]);
	UE_LOG(LogTemp, Warning, TEXT("  Giant: %d"), SizeStats[EEnemySizeCategory::Giant]);
	UE_LOG(LogTemp, Warning, TEXT("  Total: %d"), LockOnCandidates.Num());
	UE_LOG(LogTemp, Warning, TEXT("============================"));
}

void AMyCharacter::TestCameraSystem()
{
	UE_LOG(LogTemp, Warning, TEXT("=== TESTING CAMERA SYSTEM ==="));
	
	// 验证组件
	ValidateAndCacheComponents();
	
	// 测试不同体型的相机配置
	if (SizeBasedCameraConfigs.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Testing size-based camera configs:"));
		
		// 测试小型
		UE_LOG(LogTemp, Warning, TEXT("  Testing SMALL enemy config..."));
		ApplyCameraConfigForEnemySize(EEnemySizeCategory::Small);
		LogCameraState(TEXT("Small Enemy Test"));
		
		// 使用正确的FTimerHandle语法
		FTimerHandle TimerHandle1;
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle1,
			[this]()
			{
				// 测试中型
				UE_LOG(LogTemp, Warning, TEXT("  Testing MEDIUM enemy config..."));
				ApplyCameraConfigForEnemySize(EEnemySizeCategory::Medium);
				LogCameraState(TEXT("Medium Enemy Test"));
			},
			0.1f,
			false
		);
		
		FTimerHandle TimerHandle2;
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle2,
			[this]()
			{
				// 测试大型
				UE_LOG(LogTemp, Warning, TEXT("  Testing LARGE enemy config..."));
				ApplyCameraConfigForEnemySize(EEnemySizeCategory::Large);
				LogCameraState(TEXT("Large Enemy Test"));
			},
			0.2f,
			false
		);
		
		FTimerHandle TimerHandle3;
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle3,
			[this]()
			{
				// 测试巨型
				UE_LOG(LogTemp, Warning, TEXT("  Testing GIANT enemy config..."));
				ApplyCameraConfigForEnemySize(EEnemySizeCategory::Giant);
				LogCameraState(TEXT("Giant Enemy Test"));
			},
			0.3f,
			false
		);
		
		FTimerHandle TimerHandle4;
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle4,
			[this]()
			{
				// 重置到默认
				UE_LOG(LogTemp, Warning, TEXT("  Resetting to default config..."));
				ResetCameraToDefaultConfig();
				LogCameraState(TEXT("Reset to Default"));
				
				UE_LOG(LogTemp, Warning, TEXT("=== CAMERA SYSTEM TEST COMPLETE ==="));
			},
			0.4f,
			false
		);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("No size-based camera configs found!"));
	}
}

// ==================== 相机组件查找辅助函数实现 ====================

USpringArmComponent* AMyCharacter::FindCameraBoomComponent()
{
	// 策略1: 直接使用成员变量（如果已初始化）
	if (CameraBoom && CameraBoom->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Log, TEXT("FindCameraBoom: Using existing member variable"));
		return CameraBoom;
	}
	
	// 策略2: 通过名字查找
	USpringArmComponent* FoundBoom = FindComponentByClass<USpringArmComponent>();
	if (FoundBoom)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindCameraBoom: Found by class search"));
		return FoundBoom;
	}
	
	// 策略3: 遍历所有组件查找
	TArray<UActorComponent*> Components;
	GetComponents(Components);
	for (UActorComponent* Comp : Components)
	{
		USpringArmComponent* SpringArm = Cast<USpringArmComponent>(Comp);
		if (SpringArm)
		{
			UE_LOG(LogTemp, Warning, TEXT("FindCameraBoom: Found by component iteration: %s"), *SpringArm->GetName());
			return SpringArm;
		}
	}
	
	UE_LOG(LogTemp, Error, TEXT("FindCameraBoom: No SpringArmComponent found!"));
	return nullptr;
}

UCameraComponent* AMyCharacter::FindFollowCameraComponent()
{
	// 策略1: 直接使用成员变量（如果已初始化）
	if (FollowCamera && FollowCamera->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Log, TEXT("FindFollowCamera: Using existing member variable"));
		return FollowCamera;
	}
	
	// 策略2: 通过类查找
	UCameraComponent* FoundCamera = FindComponentByClass<UCameraComponent>();
	if (FoundCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindFollowCamera: Found by class search"));
		return FoundCamera;
	}
	
	// 策略3: 遍历所有组件查找
	TArray<UActorComponent*> Components;
	GetComponents(Components);
	for (UActorComponent* Comp : Components)
	{
		UCameraComponent* Camera = Cast<UCameraComponent>(Comp);
		if (Camera)
		{
			UE_LOG(LogTemp, Warning, TEXT("FindFollowCamera: Found by component iteration: %s"), *Camera->GetName());
			return Camera;
		}
	}
	
	UE_LOG(LogTemp, Error, TEXT("FindFollowCamera: No CameraComponent found!"));
	return nullptr;
}

void AMyCharacter::ValidateAndCacheComponents()
{
	UE_LOG(LogTemp, Warning, TEXT("=== VALIDATING CAMERA COMPONENTS ==="));
	
	// 查找CameraBoom
	if (!CameraBoom || !CameraBoom->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraBoom is null or invalid, attempting to find..."));
		CameraBoom = FindCameraBoomComponent();
	}
	
	// 查找FollowCamera
	if (!FollowCamera || !FollowCamera->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("FollowCamera is null or invalid, attempting to find..."));
		FollowCamera = FindFollowCameraComponent();
	}
	
	// 验证结果
	if (CameraBoom && FollowCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("✓ Camera components validated successfully"));
		UE_LOG(LogTemp, Warning, TEXT("  - CameraBoom: %s"), *CameraBoom->GetName());
		UE_LOG(LogTemp, Warning, TEXT("  - FollowCamera: %s"), *FollowCamera->GetName());
		
		// 记录当前状态
		LogCameraState(TEXT("Validation"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("✗ Camera component validation FAILED"));
		if (!CameraBoom) UE_LOG(LogTemp, Error, TEXT("  - CameraBoom is NULL"));
		if (!FollowCamera) UE_LOG(LogTemp, Error, TEXT("  - FollowCamera is NULL"));
		
		// 输出所有组件以帮助调试
		LogAllComponents();
	}
	
	UE_LOG(LogTemp, Warning, TEXT("===================================="));
}

// ==================== 相机配置动态调整接口实现 ====================

void AMyCharacter::ApplyCameraConfig(const FCameraSetupConfig& NewConfig)
{
	// === 安全检查（防御性编程）===
	if (!CameraBoom || !FollowCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("ApplyCameraConfig: Camera components not available, attempting to find..."));
		ValidateAndCacheComponents();
		if (!CameraBoom || !FollowCamera)
		{
			UE_LOG(LogTemp, Error, TEXT("ApplyCameraConfig: Failed to find camera components"));
			return;
		}
	}
	
	// === 记录变更前的值（用于调试）===
	float OldArmLength = CameraBoom->TargetArmLength;
	FRotator OldRotation = CameraBoom->GetRelativeRotation();
	FVector OldCameraOffset = FollowCamera->GetRelativeLocation();
	
	// === 应用相机臂设置 ===
	CameraBoom->TargetArmLength = NewConfig.ArmLength;
	CameraBoom->SetRelativeRotation(NewConfig.InitialRotation);  // 修复点1：应用旋转
	CameraBoom->bUsePawnControlRotation = NewConfig.bUsePawnControlRotation;
	CameraBoom->bEnableCameraLag = NewConfig.bEnableCameraLag;
	CameraBoom->CameraLagSpeed = NewConfig.CameraLagSpeed;
	
	// === 修复点2：正确应用锁定高度偏移 ===
	FVector CameraOffset = NewConfig.SocketOffset;
	
	if (bIsLockedOn)
	{
		// 方案：累加锁定高度偏移到相机位置
		CameraOffset += NewConfig.LockOnHeightOffset;
		
		// 备选方案（注释保留）:
		// CameraBoom->SocketOffset = NewConfig.LockOnHeightOffset;
		
		UE_LOG(LogTemp, Warning, TEXT("ApplyCameraConfig: Lock-on active, applying height offset: %s"), 
			*NewConfig.LockOnHeightOffset.ToString());
	}
	
	// 应用最终的相机偏移
	FollowCamera->SetRelativeLocation(CameraOffset);
	
	// === 同步更新 CameraControlComponent ===
	if (CameraControlComponent)
	{
		FCameraSettings UpdatedSettings = CameraControlComponent->CameraSettings;
		// 确保 TargetLocationOffset 被正确传递
		UpdatedSettings.TargetLocationOffset = TargetLocationOffset;  // 使用成员变量
		CameraControlComponent->SetCameraSettings(UpdatedSettings);
		
		UE_LOG(LogTemp, Warning, TEXT("ApplyCameraConfig: Synced settings to CameraControlComponent"));
	}
	
	// === 验证日志 ===
	UE_LOG(LogTemp, Warning, TEXT("=== ApplyCameraConfig Complete ==="));
	UE_LOG(LogTemp, Warning, TEXT("  ArmLength: %.1f -> %.1f"), OldArmLength, NewConfig.ArmLength);
	UE_LOG(LogTemp, Warning, TEXT("  Rotation: %s -> %s"), *OldRotation.ToString(), *NewConfig.InitialRotation.ToString());
	UE_LOG(LogTemp, Warning, TEXT("  CameraOffset: %s -> %s"), *OldCameraOffset.ToString(), *CameraOffset.ToString());
	UE_LOG(LogTemp, Warning, TEXT("  LockOnHeightOffset applied: %s"), bIsLockedOn ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Warning, TEXT("==================================="));
}

void AMyCharacter::ApplyCameraConfigForEnemySize(EEnemySizeCategory SizeCategory)
{
	if (!SizeBasedCameraConfigs.Contains(SizeCategory))
	{
		UE_LOG(LogTemp, Warning, TEXT("No camera config found for size category: %s, using default multiplier"), 
			*UEnum::GetValueAsString(SizeCategory));
		
		// 使用默认配置的倍数调整
		if (CameraBoom)
		{
			CameraBoom->TargetArmLength = CameraSetupConfig.ArmLength * CameraSetupConfig.LockOnArmLengthMultiplier;
		}
		return;
	}
	
	const FCameraSetupConfig& Config = SizeBasedCameraConfigs[SizeCategory];
	ApplyCameraConfig(Config);
	
	UE_LOG(LogTemp, Log, TEXT("Applied camera config for %s enemy: ArmLength=%.1f"), 
		*UEnum::GetValueAsString(SizeCategory), Config.ArmLength);
}

void AMyCharacter::ResetCameraToDefaultConfig()
{
	ApplyCameraConfig(CameraSetupConfig);
	
	// 清除锁定高度偏移
	if (CameraBoom)
	{
		CameraBoom->TargetOffset = FVector::ZeroVector;
	}
	
	UE_LOG(LogTemp, Log, TEXT("Reset camera to default configuration"));
}

FCameraSetupConfig AMyCharacter::GetCurrentCameraConfig() const
{
	FCameraSetupConfig CurrentConfig;
	
	if (CameraBoom && FollowCamera)
	{
		CurrentConfig.ArmLength = CameraBoom->TargetArmLength;
		CurrentConfig.InitialRotation = CameraBoom->GetRelativeRotation();
		CurrentConfig.SocketOffset = FollowCamera->GetRelativeLocation();
		CurrentConfig.bUsePawnControlRotation = CameraBoom->bUsePawnControlRotation;
		CurrentConfig.bEnableCameraLag = CameraBoom->bEnableCameraLag;
		CurrentConfig.CameraLagSpeed = CameraBoom->CameraLagSpeed;
		CurrentConfig.LockOnHeightOffset = CameraBoom->TargetOffset;
	}
	
	return CurrentConfig;
}

// ==================== UI辅助函数实现 ====================

void AMyCharacter::HideAllLockOnWidgets()
{
	// 隐藏所有候选目标的UI
	for (AActor* Candidate : LockOnCandidates)
	{
		if (IsValid(Candidate))
		{
			UActorComponent* WidgetComp = Candidate->GetComponentByClass(UWidgetComponent::StaticClass());
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
	
	// 如果使用Socket投射UI，也隐藏它
	if (bUseSocketProjection && LockOnWidgetInstance && LockOnWidgetInstance->IsInViewport())
	{
		LockOnWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}
}// ==================== 扇形锁定辅助函数实现 ====================

bool AMyCharacter::HasCandidatesInSphere()
{
	// 确保候选列表是最新的
	if (LockOnCandidates.Num() == 0)
	{
		FindLockOnCandidates();
	}
	
	return LockOnCandidates.Num() > 0;
}

AActor* AMyCharacter::TryGetSectorLockTarget()
{
	if (TargetDetectionComponent)
	{
		return TargetDetectionComponent->TryGetSectorLockTarget();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TryGetSectorLockTarget: TargetDetectionComponent is null!"));
		return nullptr;
	}
}

AActor* AMyCharacter::TryGetCameraCorrectionTarget()
{
	if (TargetDetectionComponent)
	{
		return TargetDetectionComponent->TryGetCameraCorrectionTarget();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TryGetCameraCorrectionTarget: TargetDetectionComponent is null!"));
		return nullptr;
	}
}

void AMyCharacter::StartCameraCorrectionForTarget(AActor* Target)
{
	if (CameraControlComponent && Target)
	{
		CameraControlComponent->StartCameraCorrectionForTarget(Target);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StartCameraCorrectionForTarget: CameraControlComponent or Target is null!"));
	}
}

void AMyCharacter::StartCameraAutoCorrection(AActor* Target)
{
	if (CameraControlComponent && Target)
	{
		CameraControlComponent->StartCameraAutoCorrection(Target);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StartCameraAutoCorrection: CameraControlComponent or Target is null!"));
	}
}

void AMyCharacter::UpdateCameraAutoCorrection()
{
	if (CameraControlComponent)
	{
		CameraControlComponent->UpdateCameraAutoCorrection();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UpdateCameraAutoCorrection: CameraControlComponent is null!"));
	}
}

void AMyCharacter::DelayedCameraCorrection()
{
	if (CameraControlComponent)
	{
		CameraControlComponent->DelayedCameraCorrection();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("DelayedCameraCorrection: CameraControlComponent is null!"));
	}
}

void AMyCharacter::RestoreCameraFollowState()
{
	if (CameraControlComponent)
	{
		CameraControlComponent->RestoreCameraFollowState();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("RestoreCameraFollowState: CameraControlComponent is null!"));
	}
}

// ==================== 调试命令设置 ====================

void AMyCharacter::SetupDebugCommands()
{
	UE_LOG(LogTemp, Warning, TEXT("=== DEBUG COMMANDS AVAILABLE ==="));
	UE_LOG(LogTemp, Warning, TEXT("Press F: General debug info"));
	UE_LOG(LogTemp, Warning, TEXT("Press G: Display target sizes"));
	UE_LOG(LogTemp, Warning, TEXT("================================"));
}

// ==================== 编辑器实时更新接口实现 ====================

#if WITH_EDITOR

void AMyCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (!PropertyChangedEvent.Property)
		return;
	
	FName PropertyName = PropertyChangedEvent.Property->GetFName();
	
	// 处理相机配置结构体的修改
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMyCharacter, CameraSetupConfig))
	{
		// 如果相机组件存在，应用新配置
		if (CameraBoom && FollowCamera)
		{
			ApplyCameraConfig(CameraSetupConfig);
			
			// 强制刷新编辑器视口
			CameraBoom->MarkRenderStateDirty();
			FollowCamera->MarkRenderStateDirty();
			
			UE_LOG(LogTemp, Warning, TEXT("Editor: Camera config struct updated - ArmLength=%.1f"), 
				CameraSetupConfig.ArmLength);
		}
	}
	
	// 原始代码保留：锁定范围的处理
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMyCharacter, LockOnRange))
	{
		if (LockOnDetectionSphere)
		{
			LockOnDetectionSphere->SetSphereRadius(LockOnRange);
			UE_LOG(LogTemp, Warning, TEXT("Editor: Detection sphere radius updated to %.1f"), 
				LockOnRange);
		}
		
		if (TargetDetectionComponent)
		{
			TargetDetectionComponent->LockOnSettings.LockOnRange = LockOnRange;
		}
	}
}

void AMyCharacter::PreviewCameraConfigInEditor()
{
	// 安全检查（安全宪法：防御性编程）
	if (!CameraBoom || !FollowCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("PreviewCameraConfig: Camera components not found"));
		ValidateAndCacheComponents();
		return;
	}
	
	// 应用当前配置（使用已有的安全函数）
	ApplyCameraConfig(CameraSetupConfig);
	
	// 强制刷新视口显示
	CameraBoom->MarkRenderStateDirty();
	FollowCamera->MarkRenderStateDirty();
	
	// 输出当前配置信息
	UE_LOG(LogTemp, Warning, TEXT("=== Camera Config Preview Applied ==="));
	UE_LOG(LogTemp, Warning, TEXT("Arm Length: %.1f"), CameraSetupConfig.ArmLength);
	UE_LOG(LogTemp, Warning, TEXT("Initial Rotation: Pitch=%.1f, Yaw=%.1f, Roll=%.1f"), 
		CameraSetupConfig.InitialRotation.Pitch,
		CameraSetupConfig.InitialRotation.Yaw,
		CameraSetupConfig.InitialRotation.Roll);
	UE_LOG(LogTemp, Warning, TEXT("Socket Offset: X=%.1f, Y=%.1f, Z=%.1f"), 
		CameraSetupConfig.SocketOffset.X,
		CameraSetupConfig.SocketOffset.Y,
		CameraSetupConfig.SocketOffset.Z);
	UE_LOG(LogTemp, Warning, TEXT("Camera Lag: %s, Speed: %.1f"),
		CameraSetupConfig.bEnableCameraLag ? TEXT("Enabled") : TEXT("Disabled"),
		CameraSetupConfig.CameraLagSpeed);
	UE_LOG(LogTemp, Warning, TEXT("====================================="));
}

void AMyCharacter::ResetCameraConfigInEditor()
{
	// 创建默认配置
	FCameraSetupConfig DefaultConfig;
	
	// 保存旧配置用于日志
	FCameraSetupConfig OldConfig = CameraSetupConfig;
	
	// 应用默认配置
	CameraSetupConfig = DefaultConfig;
	ApplyCameraConfig(CameraSetupConfig);
	
	UE_LOG(LogTemp, Warning, TEXT("Camera config reset: ArmLength %.1f -> %.1f"), 
		OldConfig.ArmLength, DefaultConfig.ArmLength);
}

void AMyCharacter::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	
	if (bFinished && bEnableCameraDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("Editor: Character moved, final position: %s"), 
			*GetActorLocation().ToString());
	}
}

void AMyCharacter::SyncCameraConfigFromComponents()
{
	if (!CameraBoom || !FollowCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("SyncCameraConfig: Camera components not available"));
		return;
	}
	
	// 从组件读取当前值并更新配置
	CameraSetupConfig.ArmLength = CameraBoom->TargetArmLength;
	CameraSetupConfig.InitialRotation = CameraBoom->GetRelativeRotation();
	CameraSetupConfig.SocketOffset = FollowCamera->GetRelativeLocation();
	CameraSetupConfig.bUsePawnControlRotation = CameraBoom->bUsePawnControlRotation;
	CameraSetupConfig.bEnableCameraLag = CameraBoom->bEnableCameraLag;
	CameraSetupConfig.CameraLagSpeed = CameraBoom->CameraLagSpeed;
	
	UE_LOG(LogTemp, Warning, TEXT("Synced config from components: ArmLength=%.1f, Pitch=%.1f"), 
		CameraSetupConfig.ArmLength, CameraSetupConfig.InitialRotation.Pitch);
}

#endif // WITH_EDITOR
