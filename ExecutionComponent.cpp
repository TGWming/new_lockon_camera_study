// Fill out your copyright notice in the Description page of Project Settings.

#include "ExecutionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"
#include "CollisionQueryParams.h"

// Sets default values for this component's properties
UExecutionComponent::UExecutionComponent()
{
	// Set this component to be ticked every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryComponentTick.bCanEverTick = true;

	// ��ʼ����������
	ExecutionSettings = FExecutionSettings();
	
	// ��ʼ��״̬����
	CurrentExecutionType = EExecutionType::None;
	CurrentExecutionTarget = nullptr;
	bRiposteWindowActive = false;
	bIsPositioningForExecution = false;
	ExecutionStartTime = 0.0f;
	bExecutionAnimationPlaying = false;
	ExecutionStartPosition = FVector::ZeroVector;
	TargetExecutionPosition = FVector::ZeroVector;
}

// Called when the game starts
void UExecutionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// ��ʼ������ϵͳ
	ResetExecutionState();
	
	UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: Component initialized successfully"));
	UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: BackstabRange=%.1f, BackstabAngle=%.1f"), 
		ExecutionSettings.BackstabRange, ExecutionSettings.BackstabAngle);
}

// Called every frame
void UExecutionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ���´���״̬
	if (IsExecuting())
	{
		UpdateExecutionState(DeltaTime);
	}

	// ����λ�õ���
	if (bIsPositioningForExecution)
	{
		UpdatePositioning(DeltaTime);
	}

	// ���ڼ�鱳�̻��ᣨ����Ƶ�����Ż����ܣ�
	static float LastBackstabCheckTime = 0.0f;
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastBackstabCheckTime > 0.2f) // ÿ0.2����һ��
	{
		// ��鸽���Ƿ��пɱ��̵�Ŀ��
		TArray<AActor*> NearbyActors;
		// ����������Ӹ����ӵ�Ŀ�������߼�
		// Ϊ�˱�����������ԣ���ʱ��������ʵ��
		
		LastBackstabCheckTime = CurrentTime;
	}
}

// ==================== ���Ľӿں���ʵ�� ====================

bool UExecutionComponent::CanExecute(AActor* Target, EExecutionType ExecutionType) const
{
	// ������֤
	if (!ValidateExecutionTarget(Target))
	{
		return false;
	}

	// ����Ƿ��Ѿ���ִ�д���
	if (IsExecuting())
	{
		return false;
	}

	// ���ݴ������ͽ��о�����
	switch (ExecutionType)
	{
		case EExecutionType::Backstab:
			return CheckBackstabOpportunity(Target);
			
		case EExecutionType::Riposte:
			return CanRiposte(Target);
			
		case EExecutionType::Plunge:
			return CanPlungeAttack();
			
		case EExecutionType::Critical:
			return IsTargetVulnerableToBackstab(Target); // ����ʹ�����Ʊ��̵�����
			
		default:
			return false;
	}
}

bool UExecutionComponent::StartExecution(AActor* Target, EExecutionType ExecutionType)
{
	// ����Ƿ����ִ��
	if (!CanExecute(Target, ExecutionType))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: Cannot execute %s on target %s"), 
			*UEnum::GetValueAsString(ExecutionType), 
			Target ? *Target->GetName() : TEXT("NULL"));
		return false;
	}

	// ���ô���״̬
	CurrentExecutionType = ExecutionType;
	CurrentExecutionTarget = Target;
	ExecutionStartTime = GetWorld()->GetTimeSeconds();
	bExecutionAnimationPlaying = false;

	// ����������ʼ�¼�
	OnExecutionStarted.Broadcast(Target, ExecutionType);

	// ��ʼλ�õ���
	PositionCharacterForExecution(Target, ExecutionType);

	// ���Ŵ�������
	PlayExecutionAnimation(ExecutionType);

	UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: Started execution %s on target %s"), 
		*UEnum::GetValueAsString(ExecutionType), *Target->GetName());

	return true;
}

void UExecutionComponent::CompleteExecution()
{
	if (!IsExecuting())
	{
		return;
	}

	AActor* Target = CurrentExecutionTarget;
	EExecutionType ExecutionType = CurrentExecutionType;

	// Ӧ�ô����˺�
	if (Target)
	{
		ApplyExecutionDamage(Target, ExecutionType);
	}

	// ������������¼�
	OnExecutionCompleted.Broadcast(Target, ExecutionType);

	UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: Completed execution %s"), 
		*UEnum::GetValueAsString(ExecutionType));

	// ����״̬
	ResetExecutionState();
}

void UExecutionComponent::CancelExecution()
{
	if (!IsExecuting())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: Cancelled execution %s"), 
		*UEnum::GetValueAsString(CurrentExecutionType));

	// ֹͣ����
	StopExecutionAnimation();

	// ����״̬
	ResetExecutionState();
}

// ==================== ����ϵͳ����ʵ�� ====================

bool UExecutionComponent::CheckBackstabOpportunity(AActor* Target) const
{
	if (!ValidateExecutionTarget(Target))
	{
		return false;
	}

	// ������
	if (!IsBackstabDistanceValid(Target))
	{
		return false;
	}

	// ���Ƕ�
	if (!IsBackstabAngleValid(Target))
	{
		return false;
	}

	// ���Ŀ���Ƿ������ܵ�����
	if (!IsTargetVulnerableToBackstab(Target))
	{
		return false;
	}

	return true;
}

bool UExecutionComponent::IsTargetVulnerableToBackstab(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	// ����������Ӹ����ӵ��߼���������Ŀ��״̬
	// ���磺Ŀ���Ƿ��ڹ������Ƿ�ѣ�ε�
	
	// ������飺Ŀ�������Character����
	ACharacter* TargetCharacter = Cast<ACharacter>(Target);
	if (!TargetCharacter)
	{
		return false;
	}

	// ���Ŀ���Ƿ���
	// ������Ը�����Ŀ��Ҫ���ӽ�������߼�
	
	return true;
}

FVector UExecutionComponent::GetOptimalBackstabPosition(AActor* Target) const
{
	if (!Target)
	{
		return FVector::ZeroVector;
	}

	return GetBackstabPositionBehindTarget(Target);
}

// ==================== ����ϵͳ����ʵ�� ====================

bool UExecutionComponent::CanRiposte(AActor* Target) const
{
	if (!ValidateExecutionTarget(Target))
	{
		return false;
	}

	// ��鵯�������Ƿ񼤻�
	if (!bRiposteWindowActive)
	{
		return false;
	}

	// �����루��������ȱ�����Զ��
	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	if (Distance > ExecutionSettings.BackstabRange * 1.2f)
	{
		return false;
	}

	return true;
}

void UExecutionComponent::StartRiposteWindow(float Duration)
{
	float WindowDuration = (Duration > 0.0f) ? Duration : ExecutionSettings.RiposteWindow;
	
	bRiposteWindowActive = true;
	
	// ���ö�ʱ����������������
	GetWorld()->GetTimerManager().SetTimer(
		RiposteWindowTimerHandle,
		this,
		&UExecutionComponent::OnRiposteWindowExpired,
		WindowDuration,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("ExecutionComponent: Riposte window started (%.2f seconds)"), WindowDuration);
}

void UExecutionComponent::EndRiposteWindow()
{
	bRiposteWindowActive = false;
	
	// �����ʱ��
	if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(RiposteWindowTimerHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(RiposteWindowTimerHandle);
	}

	UE_LOG(LogTemp, Log, TEXT("ExecutionComponent: Riposte window ended"));
}

// ==================== ׹�乥������ʵ�� ====================

bool UExecutionComponent::CanPlungeAttack() const
{
	// ����ɫ�Ƿ��ڿ���
	if (!IsCharacterInAir())
	{
		return false;
	}

	// ���߶��Ƿ��㹻
	float Height = GetCharacterHeight();
	if (Height < ExecutionSettings.PlungeHeightRequirement)
	{
		return false;
	}

	// ����·��Ƿ��пɹ�����Ŀ��
	TArray<AActor*> Targets = GetPlungeTargets();
	return Targets.Num() > 0;
}

TArray<AActor*> UExecutionComponent::GetPlungeTargets() const
{
	TArray<AActor*> PlungeTargets;

	if (!GetOwner())
	{
		return PlungeTargets;
	}

	// �ӽ�ɫλ���������߼��
	FVector StartLocation = GetOwner()->GetActorLocation();
	FVector EndLocation = StartLocation - FVector(0.0f, 0.0f, ExecutionSettings.PlungeHeightRequirement * 2.0f);

	// �������߼�����
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = false;

	// ִ�����߼��
	TArray<FHitResult> HitResults;
	bool bHit = GetWorld()->LineTraceMultiByChannel(
		HitResults,
		StartLocation,
		EndLocation,
		ECC_Pawn,
		QueryParams
	);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			if (Hit.GetActor() && ValidateExecutionTarget(Hit.GetActor()))
			{
				PlungeTargets.Add(Hit.GetActor());
			}
		}
	}

	return PlungeTargets;
}

// ==================== ˽�и�������ʵ�� ====================

bool UExecutionComponent::ValidateExecutionTarget(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	// ���Ŀ���Ƿ���Ч
	if (!IsValid(Target))
	{
		return false;
	}

	// ���Ŀ�겻���Լ�
	if (Target == GetOwner())
	{
		return false;
	}

	// ���Ŀ���Ƿ���Character����
	ACharacter* TargetCharacter = Cast<ACharacter>(Target);
	if (!TargetCharacter)
	{
		return false;
	}

	return true;
}

void UExecutionComponent::PositionCharacterForExecution(AActor* Target, EExecutionType ExecutionType)
{
	if (!Target || !GetOwner())
	{
		return;
	}

	bIsPositioningForExecution = true;
	ExecutionStartPosition = GetOwner()->GetActorLocation();

	// ���ݴ������ͼ���Ŀ��λ��
	switch (ExecutionType)
	{
		case EExecutionType::Backstab:
		case EExecutionType::Critical:
			TargetExecutionPosition = GetBackstabPositionBehindTarget(Target);
			break;
			
		case EExecutionType::Riposte:
			// ����λ����Ŀ��ǰ��
			TargetExecutionPosition = Target->GetActorLocation() + 
				Target->GetActorForwardVector() * ExecutionSettings.BackstabRange * 0.8f;
			break;
			
		case EExecutionType::Plunge:
			// ׹�乥�����ֵ�ǰλ��
			TargetExecutionPosition = GetOwner()->GetActorLocation();
			bIsPositioningForExecution = false;
			break;
			
		default:
			TargetExecutionPosition = GetOwner()->GetActorLocation();
			bIsPositioningForExecution = false;
			break;
	}

	UE_LOG(LogTemp, Log, TEXT("ExecutionComponent: Positioning for execution from (%.1f,%.1f,%.1f) to (%.1f,%.1f,%.1f)"),
		ExecutionStartPosition.X, ExecutionStartPosition.Y, ExecutionStartPosition.Z,
		TargetExecutionPosition.X, TargetExecutionPosition.Y, TargetExecutionPosition.Z);
}

float UExecutionComponent::CalculateExecutionDamage(EExecutionType ExecutionType) const
{
	// �����˺�������ʹ�ù̶�ֵ��ʵ����Ŀ��Ӧ�ôӽ�ɫ���Ի�ȡ��
	float BaseDamage = 100.0f;

	// ���ݴ������ͼ����˺�����
	float DamageMultiplier = 1.0f;
	
	switch (ExecutionType)
	{
		case EExecutionType::Backstab:
			DamageMultiplier = 2.5f;
			break;
			
		case EExecutionType::Riposte:
			DamageMultiplier = 2.0f;
			break;
			
		case EExecutionType::Plunge:
			DamageMultiplier = 3.0f;
			break;
			
		case EExecutionType::Critical:
			DamageMultiplier = ExecutionSettings.CriticalDamageMultiplier;
			break;
			
		default:
			DamageMultiplier = 1.0f;
			break;
	}

	return BaseDamage * DamageMultiplier;
}

bool UExecutionComponent::IsCharacterInAir() const
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return false;
	}

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (!MovementComp)
	{
		return false;
	}

	return MovementComp->IsFalling();
}

float UExecutionComponent::GetCharacterHeight() const
{
	return GetDistanceToGround();
}

float UExecutionComponent::GetDistanceToGround() const
{
	if (!GetOwner())
	{
		return 0.0f;
	}

	FVector StartLocation = GetOwner()->GetActorLocation();
	FVector EndLocation = StartLocation - FVector(0.0f, 0.0f, 10000.0f); // ����100��

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = false;

	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_WorldStatic,
		QueryParams
	);

	if (bHit)
	{
		return FVector::Dist(StartLocation, HitResult.Location);
	}

	return 0.0f;
}

float UExecutionComponent::CalculateAngleBetweenActors(AActor* Actor1, AActor* Actor2) const
{
	if (!Actor1 || !Actor2)
	{
		return 180.0f;
	}

	FVector Actor1Forward = Actor1->GetActorForwardVector();
	FVector ToActor2 = (Actor2->GetActorLocation() - Actor1->GetActorLocation()).GetSafeNormal();

	float DotProduct = FVector::DotProduct(Actor1Forward, ToActor2);
	DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);

	float AngleRadians = FMath::Acos(DotProduct);
	return FMath::RadiansToDegrees(AngleRadians);
}

USkeletalMeshComponent* UExecutionComponent::GetCharacterMesh() const
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return nullptr;
	}

	return Character->GetMesh();
}

bool UExecutionComponent::PlayExecutionAnimation(EExecutionType ExecutionType)
{
	USkeletalMeshComponent* Mesh = GetCharacterMesh();
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: No mesh component found for animation"));
		return false;
	}

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: No anim instance found"));
		return false;
	}

	// ���ݴ�������ѡ�񶯻�
	UAnimMontage* MontageToPlay = nullptr;
	
	switch (ExecutionType)
	{
		case EExecutionType::Backstab:
			MontageToPlay = ExecutionSettings.BackstabMontage;
			break;
			
		case EExecutionType::Riposte:
			MontageToPlay = ExecutionSettings.RiposteMontage;
			break;
			
		case EExecutionType::Plunge:
			MontageToPlay = ExecutionSettings.PlungeMontage;
			break;
			
		case EExecutionType::Critical:
			MontageToPlay = ExecutionSettings.BackstabMontage; // ʹ�ñ��̶���
			break;
			
		default:
			break;
	}

	if (!MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: No animation montage set for execution type %s"), 
			*UEnum::GetValueAsString(ExecutionType));
		return false;
	}

	// ���Ŷ���
	float PlayLength = AnimInstance->Montage_Play(MontageToPlay);
	if (PlayLength > 0.0f)
	{
		bExecutionAnimationPlaying = true;
		
		// ���ö�����ɻص� - �޸��󶨷���
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUFunction(this, FName("OnExecutionAnimationCompleted"));
		AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToPlay);
		
		UE_LOG(LogTemp, Log, TEXT("ExecutionComponent: Playing execution animation (%.2f seconds)"), PlayLength);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: Failed to play execution animation"));
	return false;
}

void UExecutionComponent::StopExecutionAnimation()
{
	USkeletalMeshComponent* Mesh = GetCharacterMesh();
	if (!Mesh)
	{
		return;
	}

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// ֹͣ��ǰ���ŵ���̫��
	AnimInstance->Montage_Stop(0.2f);
	bExecutionAnimationPlaying = false;
	
	UE_LOG(LogTemp, Log, TEXT("ExecutionComponent: Stopped execution animation"));
}

void UExecutionComponent::OnExecutionAnimationCompleted(UAnimMontage* Montage, bool bInterrupted)
{
	bExecutionAnimationPlaying = false;
	
	UE_LOG(LogTemp, Log, TEXT("ExecutionComponent: Execution animation completed (Interrupted: %s)"), 
		bInterrupted ? TEXT("Yes") : TEXT("No"));
	
	// ֻ����δ����ϵ�����²��Զ���ɴ���
	if (!bInterrupted)
	{
		CompleteExecution();
	}
}

void UExecutionComponent::OnRiposteWindowExpired()
{
	EndRiposteWindow();
}

void UExecutionComponent::UpdateExecutionState(float DeltaTime)
{
	if (!IsExecuting())
	{
		return;
	}

	// ���Ŀ���Ƿ���Ȼ��Ч
	if (!ValidateExecutionTarget(CurrentExecutionTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: Execution target became invalid, cancelling"));
		CancelExecution();
		return;
	}

	// ���ִ��ʱ���Ƿ��������ȫ���ƣ�
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - ExecutionStartTime > 10.0f) // 10�볬ʱ
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: Execution timeout, auto-completing"));
		CompleteExecution();
		return;
	}
}

void UExecutionComponent::UpdatePositioning(float DeltaTime)
{
	if (!bIsPositioningForExecution || !GetOwner())
	{
		return;
	}

	FVector CurrentPosition = GetOwner()->GetActorLocation();
	float DistanceToTarget = FVector::Dist(CurrentPosition, TargetExecutionPosition);

	// ����Ƿ񵽴�Ŀ��λ��
	if (DistanceToTarget < EXECUTION_POSITION_THRESHOLD)
	{
		bIsPositioningForExecution = false;
		UE_LOG(LogTemp, Log, TEXT("ExecutionComponent: Reached execution position"));
		return;
	}

	// ƽ���ƶ���Ŀ��λ��
	FVector NewPosition = FMath::VInterpTo(
		CurrentPosition,
		TargetExecutionPosition,
		DeltaTime,
		BACKSTAB_POSITION_INTERP_SPEED
	);

	GetOwner()->SetActorLocation(NewPosition);
}

void UExecutionComponent::ApplyExecutionDamage(AActor* Target, EExecutionType ExecutionType)
{
	if (!Target)
	{
		return;
	}

	float Damage = CalculateExecutionDamage(ExecutionType);
	
	// ����Ӧ�õ���Ŀ������˺���
	// ���ڲ�֪��������˺�ϵͳ������ֻ����־���
	UE_LOG(LogTemp, Warning, TEXT("ExecutionComponent: Applied %.1f damage to %s (Type: %s)"), 
		Damage, *Target->GetName(), *UEnum::GetValueAsString(ExecutionType));
	
	// ���Ŀ���н���������������������
	// ���磺Target->TakeDamage(Damage, ...);
}

void UExecutionComponent::ResetExecutionState()
{
	CurrentExecutionType = EExecutionType::None;
	CurrentExecutionTarget = nullptr;
	bIsPositioningForExecution = false;
	ExecutionStartTime = 0.0f;
	bExecutionAnimationPlaying = false;
	ExecutionStartPosition = FVector::ZeroVector;
	TargetExecutionPosition = FVector::ZeroVector;
	
	// ������������
	EndRiposteWindow();
}

FVector UExecutionComponent::GetBackstabPositionBehindTarget(AActor* Target) const
{
	if (!Target)
	{
		return FVector::ZeroVector;
	}

	// ��ȡĿ�걳���λ��
	FVector TargetLocation = Target->GetActorLocation();
	FVector TargetForward = Target->GetActorForwardVector();
	
	// ��Ŀ�걳�����λ��
	FVector BackstabPosition = TargetLocation - (TargetForward * ExecutionSettings.BackstabRange * 0.8f);
	
	return BackstabPosition;
}

bool UExecutionComponent::IsBackstabAngleValid(AActor* Target) const
{
	if (!Target || !GetOwner())
	{
		return false;
	}

	// ������������Ŀ��ĽǶ�
	FVector TargetForward = Target->GetActorForwardVector();
	FVector ToPlayer = (GetOwner()->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
	
	// ����Ƕ�
	float DotProduct = FVector::DotProduct(TargetForward, ToPlayer);
	float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f)));
	
	// �������Ƿ���Ŀ�걳�󣨽Ƕȴ���90�ȱ�ʾ�ڱ���
	bool bIsBehindTarget = Angle > (180.0f - ExecutionSettings.BackstabAngle);
	
	return bIsBehindTarget;
}

bool UExecutionComponent::IsBackstabDistanceValid(AActor* Target) const
{
	if (!Target || !GetOwner())
	{
		return false;
	}

	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	return Distance <= ExecutionSettings.BackstabRange;
}