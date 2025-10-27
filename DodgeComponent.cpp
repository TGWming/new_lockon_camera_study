// Fill out your copyright notice in the Description page of Project Settings.

#include "DodgeComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"

// Sets default values for this component's properties
UDodgeComponent::UDodgeComponent()
{
	// Set this component to be ticked every frame
	PrimaryComponentTick.bCanEverTick = true;
	
	// ��ʼ��״̬����
	bIsDodging = false;
	bIsInvincible = false;
	CurrentDodgeDirection = EDodgeDirection::None;
	LastDodgeDirection = EDodgeDirection::None;
	DodgeStartLocation = FVector::ZeroVector;
	DodgeTargetLocation = FVector::ZeroVector;
	DodgeStartTime = 0.0f;
	StaminaCost = 25.0f;
	bEnableDebugLogs = false;
	
	// ����ʱ�������
	DodgeTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DodgeTimeline"));
	
	// ��ʼ�������������Ϊ��
	StaminaComponent = nullptr;
}

// Called when the game starts
void UDodgeComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// ��ʼ��ʱ����
	InitializeTimeline();
	
	// ���Ի�ȡ����������ã���ѡ��
	AActor* Owner = GetOwner();
	if (Owner)
	{
		// �����κο��ܵľ������
		TArray<UActorComponent*> Components = Owner->GetComponents().Array();
		for (UActorComponent* Component : Components)
		{
			if (Component && Component->GetClass()->GetName().Contains(TEXT("Stamina")))
			{
				StaminaComponent = Component;
				break;
			}
		}
		
		if (!StaminaComponent && bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: No StaminaComponent found on owner %s - stamina checks disabled"), *Owner->GetName());
		}
		else if (StaminaComponent && bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Found StaminaComponent: %s"), *StaminaComponent->GetClass()->GetName());
		}
	}
	
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Initialized successfully"));
	}
}

// Called every frame
void UDodgeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// ����������ܣ������ƶ�
	if (bIsDodging)
	{
		UpdateDodgeMovement(DeltaTime);
	}
}

// ==================== ���Ľӿں���ʵ�� ====================

bool UDodgeComponent::StartDodge(const FVector& InputDirection)
{
	// ����Ƿ��������
	if (!CanDodge())
	{
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Cannot dodge - conditions not met"));
		}
		return false;
	}
	
	// �������ܷ���
	EDodgeDirection Direction = CalculateDodgeDirection(InputDirection);
	
	if (Direction == EDodgeDirection::None)
	{
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Invalid dodge direction calculated"));
		}
		return false;
	}
	
	return StartDodgeInDirection(Direction);
}

bool UDodgeComponent::StartDodgeInDirection(EDodgeDirection Direction)
{
	// ����Ƿ��������
	if (!CanDodge() || Direction == EDodgeDirection::None)
	{
		return false;
	}
	
	// ���ľ����������StaminaComponent��
	if (StaminaComponent)
	{
		// ʹ�÷�����÷���������������
		UFunction* ConsumeStaminaFunc = StaminaComponent->GetClass()->FindFunctionByName(TEXT("ConsumeStamina"));
		if (ConsumeStaminaFunc)
		{
			struct FConsumeStaminaParams
			{
				float Amount;
				bool ReturnValue;
			};
			
			FConsumeStaminaParams Params;
			Params.Amount = StaminaCost;
			Params.ReturnValue = false;
			
			StaminaComponent->ProcessEvent(ConsumeStaminaFunc, &Params);
			
			if (!Params.ReturnValue)
			{
				if (bEnableDebugLogs)
				{
					UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Not enough stamina for dodge"));
				}
				return false;
			}
		}
	}
	
	// ��������Ŀ��λ��
	FVector TargetLocation = CalculateDodgeTargetLocation(Direction);
	
	// ���·����ȫ��
	if (!IsPathSafe(GetOwner()->GetActorLocation(), TargetLocation))
	{
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Dodge path is not safe"));
		}
		
		// �˻������������StaminaComponent��
		if (StaminaComponent)
		{
			UFunction* RestoreStaminaFunc = StaminaComponent->GetClass()->FindFunctionByName(TEXT("RestoreStamina"));
			if (RestoreStaminaFunc)
			{
				struct FRestoreStaminaParams
				{
					float Amount;
				};
				
				FRestoreStaminaParams Params;
				Params.Amount = StaminaCost;
				
				StaminaComponent->ProcessEvent(RestoreStaminaFunc, &Params);
			}
		}
		return false;
	}
	
	// ��������״̬
	bIsDodging = true;
	CurrentDodgeDirection = Direction;
	LastDodgeDirection = Direction;
	DodgeStartLocation = GetOwner()->GetActorLocation();
	DodgeTargetLocation = TargetLocation;
	DodgeStartTime = GetWorld()->GetTimeSeconds();
	
	// �������ܶ���
	PlayDodgeAnimation(Direction);
	
	// ����ʱ����
	if (DodgeTimeline)
	{
		DodgeTimeline->PlayFromStart();
	}
	
	// �ӳ������޵�֡
	if (DodgeSettings.InvincibilityStartTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			InvincibilityTimerHandle,
			this,
			&UDodgeComponent::StartInvincibilityFrames,
			DodgeSettings.InvincibilityStartTime,
			false
		);
	}
	else
	{
		StartInvincibilityFrames();
	}
	
	// ������ʼ�¼�
	OnDodgeStarted.Broadcast(Direction);
	
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Started dodge in direction: %d"), (int32)Direction);
	}
	
	return true;
}

void UDodgeComponent::EndDodge()
{
	if (!bIsDodging)
	{
		return;
	}
	
	EDodgeDirection EndedDirection = CurrentDodgeDirection;
	
	// ����״̬
	bIsDodging = false;
	CurrentDodgeDirection = EDodgeDirection::None;
	
	// ֹͣʱ����
	if (DodgeTimeline)
	{
		DodgeTimeline->Stop();
	}
	
	// �����޵�֡
	EndInvincibilityFrames();
	
	// �����ʱ��
	if (InvincibilityTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(InvincibilityTimerHandle);
	}
	
	// ���������¼�
	OnDodgeEnded.Broadcast(EndedDirection);
	
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Ended dodge"));
	}
}

bool UDodgeComponent::CanDodge() const
{
	// ����������
	if (bIsDodging)
	{
		return false;
	}
	
	// ����ɫ״̬
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character)
	{
		return false;
	}
	
	// ����ɫ�Ƿ��ڵ�����
	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	if (!MovementComponent || !MovementComponent->IsMovingOnGround())
	{
		return false;
	}
	
	// ��龫���������StaminaComponent������Ӧ������
	if (StaminaComponent)
	{
		UFunction* HasEnoughStaminaFunc = StaminaComponent->GetClass()->FindFunctionByName(TEXT("HasEnoughStamina"));
		if (HasEnoughStaminaFunc)
		{
			struct FHasEnoughStaminaParams
			{
				float Amount;
				bool ReturnValue;
			};
			
			FHasEnoughStaminaParams Params;
			Params.Amount = StaminaCost;
			Params.ReturnValue = true; // Ĭ��Ϊtrue�����û�о���ϵͳ����������
			
			StaminaComponent->ProcessEvent(HasEnoughStaminaFunc, &Params);
			
			if (!Params.ReturnValue)
			{
				return false;
			}
		}
		// ���û���ҵ�HasEnoughStamina�������ͼ������㹻����
	}
	
	return true;
}

void UDodgeComponent::CancelDodge()
{
	if (bIsDodging)
	{
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Dodge cancelled"));
		}
		
		EndDodge();
	}
}

// ==================== �޵�֡����ʵ�� ====================

void UDodgeComponent::StartInvincibilityFrames()
{
	if (!bIsInvincible)
	{
		bIsInvincible = true;
		OnInvincibilityChanged.Broadcast(true);
		
		// �����޵�֡������ʱ��
		if (DodgeSettings.InvincibilityDuration > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(
				InvincibilityTimerHandle,
				this,
				&UDodgeComponent::EndInvincibilityFrames,
				DodgeSettings.InvincibilityDuration,
				false
			);
		}
		
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("DodgeComponent: Invincibility frames started"));
		}
	}
}

void UDodgeComponent::EndInvincibilityFrames()
{
	if (bIsInvincible)
	{
		bIsInvincible = false;
		OnInvincibilityChanged.Broadcast(false);
		
		// �����ʱ��
		if (InvincibilityTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(InvincibilityTimerHandle);
		}
		
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Log, TEXT("DodgeComponent: Invincibility frames ended"));
		}
	}
}

bool UDodgeComponent::IsInvincible() const
{
	return bIsInvincible;
}

// ==================== ״̬��ѯ����ʵ�� ====================

bool UDodgeComponent::IsDodging() const
{
	return bIsDodging;
}

EDodgeDirection UDodgeComponent::GetLastDodgeDirection() const
{
	return LastDodgeDirection;
}

float UDodgeComponent::GetDodgeProgress() const
{
	if (!bIsDodging || DodgeSettings.DodgeDuration <= 0.0f)
	{
		return 0.0f;
	}
	
	float ElapsedTime = GetWorld()->GetTimeSeconds() - DodgeStartTime;
	return FMath::Clamp(ElapsedTime / DodgeSettings.DodgeDuration, 0.0f, 1.0f);
}

// ==================== ˽�и�������ʵ�� ====================

void UDodgeComponent::UpdateDodgeMovement(float DeltaTime)
{
	if (!bIsDodging || !DodgeTimeline)
	{
		return;
	}
	
	// ʱ������Զ������ƶ�����
	// ����������Ӷ�����ƶ��߼�����ײ���
}

EDodgeDirection UDodgeComponent::CalculateDodgeDirection(const FVector& InputDirection) const
{
	if (InputDirection.IsNearlyZero())
	{
		return EDodgeDirection::None;
	}
	
	// ��ȡ��ɫ��ǰ������������
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return EDodgeDirection::None;
	}
	
	FVector Forward = Owner->GetActorForwardVector();
	FVector Right = Owner->GetActorRightVector();
	
	// �����뷽��ͶӰ����ɫ�ı�������ϵ
	float ForwardDot = FVector::DotProduct(InputDirection.GetSafeNormal(), Forward);
	float RightDot = FVector::DotProduct(InputDirection.GetSafeNormal(), Right);
	
	// ������ֵ
	const float Threshold = 0.5f;
	
	// �ж���Ҫ����
	bool bForward = ForwardDot > Threshold;
	bool bBackward = ForwardDot < -Threshold;
	bool bRight = RightDot > Threshold;
	bool bLeft = RightDot < -Threshold;
	
	// ��Ϸ����ж�
	if (bForward && bLeft)
	{
		return EDodgeDirection::ForwardLeft;
	}
	else if (bForward && bRight)
	{
		return EDodgeDirection::ForwardRight;
	}
	else if (bBackward && bLeft)
	{
		return EDodgeDirection::BackwardLeft;
	}
	else if (bBackward && bRight)
	{
		return EDodgeDirection::BackwardRight;
	}
	else if (bForward)
	{
		return EDodgeDirection::Forward;
	}
	else if (bBackward)
	{
		return EDodgeDirection::Backward;
	}
	else if (bLeft)
	{
		return EDodgeDirection::Left;
	}
	else if (bRight)
	{
		return EDodgeDirection::Right;
	}
	
	return EDodgeDirection::None;
}

UAnimMontage* UDodgeComponent::GetDodgeMontageForDirection(EDodgeDirection Direction) const
{
	switch (Direction)
	{
	case EDodgeDirection::Forward:
	case EDodgeDirection::ForwardLeft:
	case EDodgeDirection::ForwardRight:
		return DodgeSettings.ForwardDodgeMontage;
		
	case EDodgeDirection::Backward:
	case EDodgeDirection::BackwardLeft:
	case EDodgeDirection::BackwardRight:
		return DodgeSettings.BackwardDodgeMontage;
		
	case EDodgeDirection::Left:
		return DodgeSettings.LeftDodgeMontage;
		
	case EDodgeDirection::Right:
		return DodgeSettings.RightDodgeMontage;
		
	default:
		return nullptr;
	}
}

void UDodgeComponent::OnDodgeTimelineUpdate(float Value)
{
	if (!bIsDodging)
	{
		return;
	}
	
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	
	// Ӧ���ٶ����ߵ���
	float SpeedMultiplier = 1.0f;
	if (DodgeSettings.DodgeSpeedCurve)
	{
		SpeedMultiplier = DodgeSettings.DodgeSpeedCurve->GetFloatValue(Value);
	}
	
	// ���㵱ǰλ��
	FVector CurrentLocation = FMath::Lerp(DodgeStartLocation, DodgeTargetLocation, Value * SpeedMultiplier);
	
	// ����λ��
	Owner->SetActorLocation(CurrentLocation, true);
	
	if (bEnableDebugLogs)
	{
		static float LastLogTime = 0.0f;
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastLogTime > 0.1f) // ÿ0.1���¼һ��
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("DodgeComponent: Timeline progress: %.2f, Speed: %.2f"), Value, SpeedMultiplier);
			LastLogTime = CurrentTime;
		}
	}
}

void UDodgeComponent::OnDodgeTimelineFinished()
{
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("DodgeComponent: Timeline finished"));
	}
	
	EndDodge();
}

void UDodgeComponent::InitializeTimeline()
{
	if (!DodgeTimeline)
	{
		return;
	}
	
	// ����ʱ�������ߣ����û�������ٶ����ߣ�ʹ��Ĭ�ϵ��������ߣ�
	if (DodgeSettings.DodgeSpeedCurve)
	{
		// ��ʱ��������¼�
		FOnTimelineFloat UpdateDelegate;
		UpdateDelegate.BindUFunction(this, FName("OnDodgeTimelineUpdate"));
		DodgeTimeline->AddInterpFloat(DodgeSettings.DodgeSpeedCurve, UpdateDelegate);
	}
	
	// ��ʱ��������¼�
	FOnTimelineEvent FinishDelegate;
	FinishDelegate.BindUFunction(this, FName("OnDodgeTimelineFinished"));
	DodgeTimeline->SetTimelineFinishedFunc(FinishDelegate);
	
	// ����ʱ���᳤��
	DodgeTimeline->SetTimelineLength(DodgeSettings.DodgeDuration);
	
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("DodgeComponent: Timeline initialized with duration: %.2f"), DodgeSettings.DodgeDuration);
	}
}

void UDodgeComponent::PlayDodgeAnimation(EDodgeDirection Direction)
{
	UAnimMontage* MontageToPlay = GetDodgeMontageForDirection(Direction);
	
	if (!MontageToPlay)
	{
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: No animation montage set for direction: %d"), (int32)Direction);
		}
		return;
	}
	
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character || !Character->GetMesh())
	{
		return;
	}
	
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}
	
	// ���Ŷ�����̫��
	float PlayRate = 1.0f;
	if (DodgeSettings.DodgeDuration > 0.0f && MontageToPlay->GetPlayLength() > 0.0f)
	{
		PlayRate = MontageToPlay->GetPlayLength() / DodgeSettings.DodgeDuration;
	}
	
	AnimInstance->Montage_Play(MontageToPlay, PlayRate);
	
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("DodgeComponent: Playing dodge animation for direction: %d, PlayRate: %.2f"), (int32)Direction, PlayRate);
	}
}

FVector UDodgeComponent::CalculateDodgeTargetLocation(EDodgeDirection Direction) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}
	
	FVector StartLocation = Owner->GetActorLocation();
	FVector DirectionVector = GetDirectionVector(Direction);
	
	return StartLocation + (DirectionVector * DodgeSettings.DodgeDistance);
}

FVector UDodgeComponent::GetDirectionVector(EDodgeDirection Direction) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}
	
	FVector Forward = Owner->GetActorForwardVector();
	FVector Right = Owner->GetActorRightVector();
	
	switch (Direction)
	{
	case EDodgeDirection::Forward:
		return Forward;
		
	case EDodgeDirection::Backward:
		return -Forward;
		
	case EDodgeDirection::Left:
		return -Right;
		
	case EDodgeDirection::Right:
		return Right;
		
	case EDodgeDirection::ForwardLeft:
		return (Forward - Right).GetSafeNormal();
		
	case EDodgeDirection::ForwardRight:
		return (Forward + Right).GetSafeNormal();
		
	case EDodgeDirection::BackwardLeft:
		return (-Forward - Right).GetSafeNormal();
		
	case EDodgeDirection::BackwardRight:
		return (-Forward + Right).GetSafeNormal();
		
	default:
		return FVector::ZeroVector;
	}
}

bool UDodgeComponent::IsPathSafe(const FVector& StartLocation, const FVector& EndLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	
	// �������߼�⣬���·�����Ƿ����ϰ���
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;
	
	// ʹ�ý�ɫ������İ뾶��������׷��
	float CapsuleRadius = 50.0f; // Ĭ�ϰ뾶
	
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character && Character->GetCapsuleComponent())
	{
		CapsuleRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}
	
	bool bHit = World->SweepSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(CapsuleRadius),
		QueryParams
	);
	
	// ���û��ײ�����κζ�����·���ǰ�ȫ��
	bool bIsSafe = !bHit;
	
	if (bEnableDebugLogs && !bIsSafe)
	{
		UE_LOG(LogTemp, Warning, TEXT("DodgeComponent: Dodge path blocked by: %s"), 
			HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("Unknown"));
	}
	
	return bIsSafe;
}