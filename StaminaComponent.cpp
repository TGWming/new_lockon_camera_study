// Fill out your copyright notice in the Description page of Project Settings.

#include "StaminaComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

// ���캯��
UStaminaComponent::UStaminaComponent()
{
	// ����ÿ֡����
	PrimaryComponentTick.bCanEverTick = true;

	// ��ʼ����������ΪĬ��ֵ
	StaminaSettings = FStaminaSettings();

	// ��ʼ������ʱ״̬
	CurrentStamina = StaminaSettings.MaxStamina;
	CurrentStaminaState = EStaminaState::Normal;
	bStaminaRecoveryEnabled = true;
	LastStaminaUseTime = 0.0f;
	StaminaRecoveryStartTime = 0.0f;
	bIsRecoveringStamina = false;
	ExhaustedCounter = 0;

	UE_LOG(LogTemp, Warning, TEXT("StaminaComponent: Component created with MaxStamina=%.1f"), StaminaSettings.MaxStamina);
}

// ��Ϸ��ʼʱ����
void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	// ȷ������ֵ����Ч��Χ��
	CurrentStamina = StaminaSettings.MaxStamina;
	ClampStaminaValue();

	// ����ʱ���¼
	if (UWorld* World = GetWorld())
	{
		LastStaminaUseTime = World->GetTimeSeconds();
		StaminaRecoveryStartTime = LastStaminaUseTime;
	}

	// ������ʼ�¼�
	TriggerStaminaChangedEvent();

	UE_LOG(LogTemp, Warning, TEXT("StaminaComponent: BeginPlay completed. Current stamina: %.1f/%.1f"), 
		CurrentStamina, StaminaSettings.MaxStamina);
}

// ÿ֡����
void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ���¾����ָ��߼�
	if (bStaminaRecoveryEnabled && CurrentStamina < StaminaSettings.MaxStamina)
	{
		UpdateStaminaRecovery(DeltaTime);
	}
}

// ==================== ���ľ��������ӿ�ʵ�� ====================

bool UStaminaComponent::ConsumeStamina(float Amount)
{
	// ���������Ч��
	if (Amount <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("StaminaComponent: Invalid stamina amount: %.1f"), Amount);
		return false;
	}

	// ����Ƿ����㹻����
	if (CurrentStamina < Amount)
	{
		UE_LOG(LogTemp, Log, TEXT("StaminaComponent: Insufficient stamina. Required: %.1f, Available: %.1f"), 
			Amount, CurrentStamina);
		return false;
	}

	// ���ľ���
	float OldStamina = CurrentStamina;
	CurrentStamina -= Amount;
	ClampStaminaValue();

	// ����ʱ���¼
	if (UWorld* World = GetWorld())
	{
		LastStaminaUseTime = World->GetTimeSeconds();
		bIsRecoveringStamina = false; // ֹͣ�ָ�״̬
	}

	// ��龫���ľ�
	CheckStaminaDepletion();

	// �����¼�
	TriggerStaminaChangedEvent();

	UE_LOG(LogTemp, Verbose, TEXT("StaminaComponent: Consumed %.1f stamina. %.1f -> %.1f"), 
		Amount, OldStamina, CurrentStamina);

	return true;
}

bool UStaminaComponent::ConsumeStaminaForAction(EStaminaAction Action)
{
	float ActionCost = GetActionStaminaCost(Action);
	
	if (ActionCost <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("StaminaComponent: Invalid action cost for action: %d"), (int32)Action);
		return false;
	}

	bool bSuccess = ConsumeStamina(ActionCost);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("StaminaComponent: Action executed. Type: %d, Cost: %.1f"), 
			(int32)Action, ActionCost);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("StaminaComponent: Action failed due to insufficient stamina. Type: %d, Cost: %.1f, Available: %.1f"), 
			(int32)Action, ActionCost, CurrentStamina);
	}

	return bSuccess;
}

bool UStaminaComponent::CanPerformAction(EStaminaAction Action) const
{
	float ActionCost = GetActionStaminaCost(Action);
	bool bCanPerform = (CurrentStamina >= ActionCost) && (ActionCost > 0.0f);
	
	return bCanPerform;
}

void UStaminaComponent::RecoverStamina(float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	float OldStamina = CurrentStamina;
	CurrentStamina += Amount;
	ClampStaminaValue();

	// �����ȫ�ָ�
	CheckStaminaFullRecovery();

	// ����״̬
	if (CurrentStamina > 0.0f && CurrentStaminaState == EStaminaState::Depleted)
	{
		UpdateStaminaState(EStaminaState::Recovering);
	}

	// �����¼�
	TriggerStaminaChangedEvent();

	UE_LOG(LogTemp, Verbose, TEXT("StaminaComponent: Recovered %.1f stamina. %.1f -> %.1f"), 
		Amount, OldStamina, CurrentStamina);
}

void UStaminaComponent::ResetStamina()
{
	float OldStamina = CurrentStamina;
	CurrentStamina = StaminaSettings.MaxStamina;
	
	// ����״̬
	UpdateStaminaState(EStaminaState::Normal);
	ExhaustedCounter = 0;
	bIsRecoveringStamina = false;

	// �����¼�
	TriggerStaminaChangedEvent();
	OnStaminaFullyRecovered.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("StaminaComponent: Stamina reset. %.1f -> %.1f"), OldStamina, CurrentStamina);
}

void UStaminaComponent::SetStaminaRecoveryEnabled(bool bEnabled)
{
	bool bOldValue = bStaminaRecoveryEnabled;
	bStaminaRecoveryEnabled = bEnabled;

	if (bEnabled && !bOldValue)
	{
		// �ָ��ձ����ã����ûָ�ʱ��
		if (UWorld* World = GetWorld())
		{
			StaminaRecoveryStartTime = World->GetTimeSeconds();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("StaminaComponent: Stamina recovery %s"), 
		bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

// ==================== ״̬��ѯ�ӿ�ʵ�� ====================

float UStaminaComponent::GetStaminaPercentage() const
{
	if (StaminaSettings.MaxStamina <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(CurrentStamina / StaminaSettings.MaxStamina, 0.0f, 1.0f);
}

bool UStaminaComponent::IsStaminaFull() const
{
	return FMath::IsNearlyEqual(CurrentStamina, StaminaSettings.MaxStamina, 0.01f);
}

// ==================== ���ýӿ�ʵ�� ====================

void UStaminaComponent::SetStaminaSettings(const FStaminaSettings& NewSettings)
{
	// ���浱ǰ�����ٷֱ�
	float CurrentPercentage = GetStaminaPercentage();

	// ��������
	StaminaSettings = NewSettings;

	// �����µ����ֵ������ǰ����
	CurrentStamina = StaminaSettings.MaxStamina * CurrentPercentage;
	ClampStaminaValue();

	// �����¼�
	TriggerStaminaChangedEvent();

	UE_LOG(LogTemp, Log, TEXT("StaminaComponent: Settings updated. New MaxStamina: %.1f, Current: %.1f"), 
		StaminaSettings.MaxStamina, CurrentStamina);
}

void UStaminaComponent::SetMaxStamina(float NewMaxStamina)
{
	if (NewMaxStamina <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("StaminaComponent: Invalid max stamina value: %.1f"), NewMaxStamina);
		return;
	}

	// ���浱ǰ�����ٷֱ�
	float CurrentPercentage = GetStaminaPercentage();

	// ���������
	StaminaSettings.MaxStamina = NewMaxStamina;

	// �����µ����ֵ������ǰ����
	CurrentStamina = StaminaSettings.MaxStamina * CurrentPercentage;
	ClampStaminaValue();

	// �����¼�
	TriggerStaminaChangedEvent();

	UE_LOG(LogTemp, Log, TEXT("StaminaComponent: Max stamina updated to %.1f, Current: %.1f"), 
		NewMaxStamina, CurrentStamina);
}

// ==================== ˽�и�������ʵ�� ====================

void UStaminaComponent::UpdateStaminaRecovery(float DeltaTime)
{
	if (!bStaminaRecoveryEnabled || CurrentStamina >= StaminaSettings.MaxStamina)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	float CurrentTime = World->GetTimeSeconds();
	float TimeSinceLastUse = CurrentTime - LastStaminaUseTime;

	// ����Ƿ��ѹ��ӳ�ʱ��
	if (TimeSinceLastUse < StaminaSettings.StaminaRecoveryDelay)
	{
		return;
	}

	// ��ʼ�ָ��������û��ʼ��
	if (!bIsRecoveringStamina)
	{
		bIsRecoveringStamina = true;
		StaminaRecoveryStartTime = CurrentTime;
		
		// ����״̬Ϊ�ָ���
		if (CurrentStaminaState != EStaminaState::Recovering && CurrentStaminaState != EStaminaState::Normal)
		{
			UpdateStaminaState(EStaminaState::Recovering);
		}
	}

	// ����ָ���
	float RecoveryRate = GetCurrentRecoveryRate();
	float RecoveryAmount = RecoveryRate * DeltaTime;
	
	// Ӧ�ûָ�
	if (RecoveryAmount > 0.0f)
	{
		RecoverStamina(RecoveryAmount);
	}
}

float UStaminaComponent::GetActionStaminaCost(EStaminaAction Action) const
{
	switch (Action)
	{
		case EStaminaAction::Attack:
			return StaminaSettings.AttackStaminaCost;
		case EStaminaAction::HeavyAttack:
			return StaminaSettings.HeavyAttackStaminaCost;
		case EStaminaAction::Dodge:
			return StaminaSettings.DodgeStaminaCost;
		case EStaminaAction::Block:
			return StaminaSettings.BlockStaminaCost;
		case EStaminaAction::Sprint:
			return StaminaSettings.SprintStaminaCostPerSecond;
		case EStaminaAction::WeaponSkill:
			return StaminaSettings.WeaponSkillStaminaCost;
		default:
			UE_LOG(LogTemp, Warning, TEXT("StaminaComponent: Unknown action type: %d"), (int32)Action);
			return 0.0f;
	}
}

void UStaminaComponent::UpdateStaminaState(EStaminaState NewState)
{
	if (CurrentStaminaState == NewState)
	{
		return;
	}

	EStaminaState OldState = CurrentStaminaState;
	CurrentStaminaState = NewState;

	// ����״̬�仯�¼�
	OnStaminaStateChanged.Broadcast(OldState, NewState);

	UE_LOG(LogTemp, Log, TEXT("StaminaComponent: State changed from %d to %d"), 
		(int32)OldState, (int32)NewState);
}

void UStaminaComponent::TriggerStaminaChangedEvent()
{
	OnStaminaChanged.Broadcast(CurrentStamina, StaminaSettings.MaxStamina);
}

void UStaminaComponent::CheckStaminaDepletion()
{
	if (CurrentStamina <= 0.0f && CurrentStaminaState != EStaminaState::Depleted)
	{
		UpdateStaminaState(EStaminaState::Depleted);
		ExhaustedCounter++;
		
		// ����Ƿ�������ƣ��״̬
		if (ExhaustedCounter >= 3) // �����ľ�3�ν������ƣ��
		{
			UpdateStaminaState(EStaminaState::Exhausted);
			UE_LOG(LogTemp, Warning, TEXT("StaminaComponent: Entered exhausted state after %d depletions"), ExhaustedCounter);
		}

		OnStaminaDepleted.Broadcast();
		UE_LOG(LogTemp, Warning, TEXT("StaminaComponent: Stamina depleted! Count: %d"), ExhaustedCounter);
	}
}

void UStaminaComponent::CheckStaminaFullRecovery()
{
	if (IsStaminaFull() && CurrentStaminaState != EStaminaState::Normal)
	{
		UpdateStaminaState(EStaminaState::Normal);
		bIsRecoveringStamina = false;
		
		// ���ù���ƣ�ͼ���������ȫ�ָ���
		if (ExhaustedCounter > 0)
		{
			ExhaustedCounter = FMath::Max(0, ExhaustedCounter - 1);
			UE_LOG(LogTemp, Log, TEXT("StaminaComponent: Exhausted counter reduced to %d"), ExhaustedCounter);
		}

		OnStaminaFullyRecovered.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("StaminaComponent: Stamina fully recovered"));
	}
}

float UStaminaComponent::GetCurrentRecoveryRate() const
{
	float BaseRate = StaminaSettings.StaminaRecoveryRate;

	// Ӧ�ù���ƣ�ͳͷ�
	if (CurrentStaminaState == EStaminaState::Exhausted)
	{
		BaseRate *= StaminaSettings.ExhaustedRecoveryPenalty;
	}

	return BaseRate;
}

void UStaminaComponent::ClampStaminaValue()
{
	CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, StaminaSettings.MaxStamina);
}