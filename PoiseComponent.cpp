// Fill out your copyright notice in the Description page of Project Settings.

#include "PoiseComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
UPoiseComponent::UPoiseComponent()
{
	// Set this component to be ticked every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryComponentTick.bCanEverTick = true;

	// ��ʼ����������
	PoiseSettings = FPoiseSettings();

	// ��ʼ��״̬����
	CurrentPoise = PoiseSettings.MaxPoise;
	CurrentPoiseState = EPoiseState::Normal;
	LastDamageTime = 0.0f;
	PoiseImmuneEndTime = 0.0f;
	OwnerCharacter = nullptr;
	bIsStaggering = false;
	bManuallySetImmune = false;

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Constructor completed"));
}

// Called when the game starts
void UPoiseComponent::BeginPlay()
{
	Super::BeginPlay();

	// ��ȡӵ���߽�ɫ����
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("PoiseComponent: Owner is not a Character! Component will not function properly."));
		return;
	}

	// ��ʼ������ֵ
	CurrentPoise = PoiseSettings.MaxPoise;
	CurrentPoiseState = EPoiseState::Normal;
	LastDamageTime = 0.0f;
	PoiseImmuneEndTime = 0.0f;
	bIsStaggering = false;
	bManuallySetImmune = false;

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: BeginPlay completed for %s"), 
		OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"));
	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Initial Poise: %.1f/%.1f"), CurrentPoise, PoiseSettings.MaxPoise);
}

// Called every frame
void UPoiseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValidForPoiseOperations())
	{
		return;
	}

	// �������Իָ�
	UpdatePoiseRecovery(DeltaTime);

	// �����������״̬
	if (IsPoiseImmune() && !bManuallySetImmune)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime >= PoiseImmuneEndTime)
		{
			EndPoiseImmune();
		}
	}
}

// ==================== ���Ľӿں���ʵ�� ====================

bool UPoiseComponent::TakePoiseDamage(float PoiseDamage, AActor* DamageSource)
{
	if (!IsValidForPoiseOperations() || PoiseDamage <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Invalid poise damage attempt - Damage: %.1f"), PoiseDamage);
		return false;
	}

	// �����������
	if (IsPoiseImmune())
	{
		UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Poise damage blocked by immunity"));
		return false;
	}

	// ����Ѿ��ƻ����������ܵ������˺�
	if (IsPoiseBroken())
	{
		UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Poise damage blocked - already broken"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Taking poise damage %.1f from %s"), 
		PoiseDamage, DamageSource ? *DamageSource->GetName() : TEXT("Unknown"));

	// ��¼�˺�ʱ��
	LastDamageTime = GetWorld()->GetTimeSeconds();

	// ��������ֵ
	float PreviousPoise = CurrentPoise;
	CurrentPoise = FMath::Max(0.0f, CurrentPoise - PoiseDamage);

	// ����״̬
	if (CurrentPoise <= 0.0f)
	{
		// �����ƻ�
		BreakPoise(DamageSource);
	}
	else if (CurrentPoise < PoiseSettings.MaxPoise)
	{
		// ��������
		SetPoiseState(EPoiseState::Damaged);
	}

	// �㲥���Ա仯�¼�
	BroadcastPoiseChanged();

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Poise changed from %.1f to %.1f"), PreviousPoise, CurrentPoise);

	return true;
}

void UPoiseComponent::BreakPoise(AActor* DamageSource)
{
	if (!IsValidForPoiseOperations())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Breaking poise for %s"), 
		OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("Unknown"));

	// ��������Ϊ0
	CurrentPoise = 0.0f;

	// �����ƻ�״̬
	SetPoiseState(EPoiseState::Broken);

	// ����Ӳֱ����ʱ��
	float StaggerDuration = CalculateStaggerDuration(0.0f); // ʹ�û���Ӳֱʱ��

	// ��ʼӲֱ
	StartStagger(StaggerDuration, DamageSource);

	// �㲥�����ƻ��¼�
	OnPoiseBreak.Broadcast(GetOwner(), StaggerDuration, DamageSource);

	// �㲥���Ա仯�¼�
	BroadcastPoiseChanged();

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Poise broken - Stagger duration: %.2f"), StaggerDuration);
}

void UPoiseComponent::RecoverPoise(float RecoveryAmount)
{
	if (!IsValidForPoiseOperations())
	{
		return;
	}

	// �����������״̬��Ӳֱ�У����ָܻ�����
	if (IsPoiseImmune() || bIsStaggering)
	{
		return;
	}

	// ���û��ָ���ָ�����ʹ��Ĭ������
	if (RecoveryAmount <= 0.0f)
	{
		RecoveryAmount = PoiseSettings.PoiseRecoveryRate * GetWorld()->GetDeltaSeconds();
	}

	float PreviousPoise = CurrentPoise;
	CurrentPoise = FMath::Min(PoiseSettings.MaxPoise, CurrentPoise + RecoveryAmount);

	// ����״̬
	if (CurrentPoise >= PoiseSettings.MaxPoise)
	{
		SetPoiseState(EPoiseState::Normal);
	}
	else if (CurrentPoise > 0.0f && CurrentPoiseState == EPoiseState::Broken)
	{
		SetPoiseState(EPoiseState::Recovering);
	}

	// �㲥���Ա仯�¼�
	if (FMath::Abs(CurrentPoise - PreviousPoise) > 0.01f)
	{
		BroadcastPoiseChanged();
	}
}

void UPoiseComponent::ResetPoise()
{
	if (!IsValidForPoiseOperations())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Resetting poise to maximum"));

	CurrentPoise = PoiseSettings.MaxPoise;
	SetPoiseState(EPoiseState::Normal);

	// ���Ӳֱ״̬
	if (bIsStaggering)
	{
		EndStagger();
	}

	// �㲥���Ա仯�¼�
	BroadcastPoiseChanged();
}

void UPoiseComponent::SetPoiseImmune(bool bImmune, float Duration)
{
	if (!IsValidForPoiseOperations())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Setting poise immune: %s, Duration: %.2f"), 
		bImmune ? TEXT("True") : TEXT("False"), Duration);

	bManuallySetImmune = bImmune;

	if (bImmune)
	{
		SetPoiseState(EPoiseState::Immune);

		if (Duration > 0.0f)
		{
			// ���ö�ʱ����
			PoiseImmuneEndTime = GetWorld()->GetTimeSeconds() + Duration;
			
			// ���֮ǰ�Ķ�ʱ��
			GetWorld()->GetTimerManager().ClearTimer(ImmuneTimerHandle);
			
			// �����µĶ�ʱ��
			GetWorld()->GetTimerManager().SetTimer(
				ImmuneTimerHandle,
				this,
				&UPoiseComponent::EndPoiseImmune,
				Duration,
				false
			);
		}
		else
		{
			// ��������
			PoiseImmuneEndTime = 0.0f;
		}
	}
	else
	{
		EndPoiseImmune();
	}
}

// ==================== ״̬��ѯ����ʵ�� ====================

bool UPoiseComponent::IsPoiseBroken() const
{
	return CurrentPoiseState == EPoiseState::Broken;
}

bool UPoiseComponent::IsPoiseImmune() const
{
	return CurrentPoiseState == EPoiseState::Immune;
}

float UPoiseComponent::GetCurrentPoise() const
{
	return CurrentPoise;
}

float UPoiseComponent::GetPoisePercentage() const
{
	if (PoiseSettings.MaxPoise <= 0.0f)
	{
		return 0.0f;
	}
	return (CurrentPoise / PoiseSettings.MaxPoise) * 100.0f;
}

EPoiseState UPoiseComponent::GetPoiseState() const
{
	return CurrentPoiseState;
}

float UPoiseComponent::GetMaxPoise() const
{
	return PoiseSettings.MaxPoise;
}

bool UPoiseComponent::IsStaggering() const
{
	return bIsStaggering;
}

// ==================== ���ú���ʵ�� ====================

void UPoiseComponent::UpdatePoiseSettings(const FPoiseSettings& NewSettings)
{
	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Updating poise settings"));

	FPoiseSettings OldSettings = PoiseSettings;
	PoiseSettings = NewSettings;

	// ����������ֵ�����仯��������ǰ����ֵ
	if (OldSettings.MaxPoise != NewSettings.MaxPoise)
	{
		float PoiseRatio = (OldSettings.MaxPoise > 0.0f) ? (CurrentPoise / OldSettings.MaxPoise) : 1.0f;
		CurrentPoise = NewSettings.MaxPoise * PoiseRatio;
		CurrentPoise = FMath::Clamp(CurrentPoise, 0.0f, NewSettings.MaxPoise);

		// �㲥���Ա仯�¼�
		BroadcastPoiseChanged();
	}
}

void UPoiseComponent::SetMaxPoise(float NewMaxPoise)
{
	if (NewMaxPoise <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Invalid max poise value: %.1f"), NewMaxPoise);
		return;
	}

	float OldMaxPoise = PoiseSettings.MaxPoise;
	PoiseSettings.MaxPoise = NewMaxPoise;

	// ������������ǰ����ֵ
	if (OldMaxPoise > 0.0f)
	{
		float PoiseRatio = CurrentPoise / OldMaxPoise;
		CurrentPoise = NewMaxPoise * PoiseRatio;
	}
	else
	{
		CurrentPoise = NewMaxPoise;
	}

	CurrentPoise = FMath::Clamp(CurrentPoise, 0.0f, NewMaxPoise);

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Max poise changed from %.1f to %.1f, Current poise: %.1f"), 
		OldMaxPoise, NewMaxPoise, CurrentPoise);

	// �㲥���Ա仯�¼�
	BroadcastPoiseChanged();
}

// ==================== ˽�и�������ʵ�� ====================

void UPoiseComponent::UpdatePoiseRecovery(float DeltaTime)
{
	// ֻ����������ָ�״̬�²����Զ��ָ�����
	if (CurrentPoiseState != EPoiseState::Normal && 
		CurrentPoiseState != EPoiseState::Damaged && 
		CurrentPoiseState != EPoiseState::Recovering)
	{
		return;
	}

	// ����Ƿ���Ҫ�ȴ��ָ��ӳ�
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastDamageTime < PoiseSettings.PoiseRecoveryDelay)
	{
		return;
	}

	// �����������������Ҫ�ָ�
	if (CurrentPoise >= PoiseSettings.MaxPoise)
	{
		if (CurrentPoiseState != EPoiseState::Normal)
		{
			SetPoiseState(EPoiseState::Normal);
		}
		return;
	}

	// ִ�����Իָ�
	float RecoveryAmount = PoiseSettings.PoiseRecoveryRate * DeltaTime;
	RecoverPoise(RecoveryAmount);
}

void UPoiseComponent::StartStagger(float StaggerDuration, AActor* DamageSource)
{
	if (!IsValidForPoiseOperations())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Starting stagger for %.2f seconds"), StaggerDuration);

	bIsStaggering = true;

	// �㲥Ӳֱ��ʼ�¼�
	OnStaggerStarted.Broadcast(GetOwner(), StaggerDuration);

	// ����Ӳֱ������ʱ��
	GetWorld()->GetTimerManager().ClearTimer(StaggerTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		StaggerTimerHandle,
		this,
		&UPoiseComponent::EndStagger,
		StaggerDuration,
		false
	);

	// ����������ӶԽ�ɫ�ƶ������Ƶ��߼�
	// ���磺���ý�ɫ�ƶ�������Ӳֱ������
	if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
	{
		// ���������������ƶ������߼�
		// OwnerCharacter->GetCharacterMovement()->DisableMovement();
	}
}

void UPoiseComponent::EndStagger()
{
	if (!IsValidForPoiseOperations())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Ending stagger"));

	bIsStaggering = false;

	// ���Ӳֱ��ʱ��
	GetWorld()->GetTimerManager().ClearTimer(StaggerTimerHandle);

	// ���������ƻ��������ʱ��
	if (PoiseSettings.PoiseImmuneTimeAfterBreak > 0.0f)
	{
		PoiseImmuneEndTime = GetWorld()->GetTimeSeconds() + PoiseSettings.PoiseImmuneTimeAfterBreak;
		SetPoiseState(EPoiseState::Immune);

		// �������߽�����ʱ��
		GetWorld()->GetTimerManager().SetTimer(
			ImmuneTimerHandle,
			this,
			&UPoiseComponent::EndPoiseImmune,
			PoiseSettings.PoiseImmuneTimeAfterBreak,
			false
		);
	}
	else
	{
		// ֱ�ӽ���ָ�״̬
		SetPoiseState(EPoiseState::Recovering);
	}

	// �㲥Ӳֱ�����¼�
	OnStaggerEnded.Broadcast(GetOwner());

	// �ָ���ɫ�ƶ�����
	if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
	{
		// ����������ָ��ƶ�����
		// OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void UPoiseComponent::SetPoiseState(EPoiseState NewState)
{
	if (CurrentPoiseState == NewState)
	{
		return;
	}

	EPoiseState OldState = CurrentPoiseState;
	CurrentPoiseState = NewState;

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: State changed from %d to %d"), 
		(int32)OldState, (int32)NewState);

	// �㲥״̬�仯�¼�
	OnPoiseStateChanged.Broadcast(GetOwner(), NewState);
}

void UPoiseComponent::EndPoiseImmune()
{
	if (!IsValidForPoiseOperations())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PoiseComponent: Ending poise immunity"));

	bManuallySetImmune = false;
	PoiseImmuneEndTime = 0.0f;

	// ������߶�ʱ��
	GetWorld()->GetTimerManager().ClearTimer(ImmuneTimerHandle);

	// ���ݵ�ǰ����ֵ���ú��ʵ�״̬
	if (CurrentPoise >= PoiseSettings.MaxPoise)
	{
		SetPoiseState(EPoiseState::Normal);
	}
	else if (CurrentPoise > 0.0f)
	{
		SetPoiseState(EPoiseState::Recovering);
	}
	else
	{
		SetPoiseState(EPoiseState::Broken);
	}
}

float UPoiseComponent::CalculateStaggerDuration(float PoiseDamage) const
{
	// ����Ӳֱʱ�䣬���Ը����˺�ֵ���е���
	float StaggerDuration = PoiseSettings.BaseStaggerDuration;

	// ���Ը��������˺�ֵ����Ӳֱʱ��
	// ���磺���ߵ��˺����¸�����Ӳֱʱ��
	if (PoiseDamage > 0.0f)
	{
		float DamageMultiplier = FMath::Clamp(PoiseDamage / PoiseSettings.MaxPoise, 0.5f, 2.0f);
		StaggerDuration *= DamageMultiplier;
	}

	return FMath::Clamp(StaggerDuration, 0.1f, 10.0f);
}

void UPoiseComponent::BroadcastPoiseChanged()
{
	if (IsValidForPoiseOperations())
	{
		OnPoiseChanged.Broadcast(GetOwner(), CurrentPoise, PoiseSettings.MaxPoise);
	}
}

bool UPoiseComponent::IsValidForPoiseOperations() const
{
	return IsValid(this) && IsValid(GetOwner()) && GetWorld() != nullptr;
}