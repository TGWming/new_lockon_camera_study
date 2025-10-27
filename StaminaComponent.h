// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "StaminaComponent.generated.h"

// ����״̬ö��
UENUM(BlueprintType)
enum class EStaminaState : uint8
{
	Normal		UMETA(DisplayName = "Normal"),		// ����״̬
	Depleted	UMETA(DisplayName = "Depleted"),	// �����ľ�
	Recovering	UMETA(DisplayName = "Recovering"),	// �ָ���
	Exhausted	UMETA(DisplayName = "Exhausted")	// ����ƣ��״̬
};

// �������Ķ���ö��
UENUM(BlueprintType)
enum class EStaminaAction : uint8
{
	Attack			UMETA(DisplayName = "Attack"),			// ��ͨ����
	HeavyAttack		UMETA(DisplayName = "Heavy Attack"),	// �ع���
	Dodge			UMETA(DisplayName = "Dodge"),			// ����
	Block			UMETA(DisplayName = "Block"),			// ��
	Sprint			UMETA(DisplayName = "Sprint"),			// ���
	WeaponSkill		UMETA(DisplayName = "Weapon Skill")		// ��������
};

// �������ýṹ��
USTRUCT(BlueprintType)
struct FStaminaSettings
{
	GENERATED_BODY()

	// �����ֵ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina Settings", meta = (ClampMin = "10.0", ClampMax = "500.0"))
	float MaxStamina = 100.0f;

	// �����ָ��ٶȣ�ÿ�룩
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina Settings", meta = (ClampMin = "1.0", ClampMax = "100.0"))
	float StaminaRecoveryRate = 15.0f;

	// �����ָ��ӳ٣��룩
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina Settings", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float StaminaRecoveryDelay = 1.0f;

	// ����ƣ��ʱ�Ļָ��ͷ���������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina Settings", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ExhaustedRecoveryPenalty = 0.5f;

	// ��ͨ������������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Costs", meta = (ClampMin = "1.0", ClampMax = "100.0"))
	float AttackStaminaCost = 20.0f;

	// �ع�����������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Costs", meta = (ClampMin = "1.0", ClampMax = "100.0"))
	float HeavyAttackStaminaCost = 35.0f;

	// ���ܾ�������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Costs", meta = (ClampMin = "1.0", ClampMax = "100.0"))
	float DodgeStaminaCost = 25.0f;

	// �񵲾�������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Costs", meta = (ClampMin = "1.0", ClampMax = "50.0"))
	float BlockStaminaCost = 5.0f;

	// ���ÿ�뾫������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Costs", meta = (ClampMin = "1.0", ClampMax = "50.0"))
	float SprintStaminaCostPerSecond = 10.0f;

	// �������ܾ�������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Costs", meta = (ClampMin = "10.0", ClampMax = "100.0"))
	float WeaponSkillStaminaCost = 50.0f;

	// ���캯��
	FStaminaSettings()
	{
		// ʹ��Ĭ��ֵ
	}
};

// �����仯�¼�ί��
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, CurrentStamina, float, MaxStamina);

// �����ľ��¼�ί��
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDepleted);

// ������ȫ�ָ��¼�ί��
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaFullyRecovered);

// ����״̬�仯�¼�ί��
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaStateChanged, EStaminaState, OldState, EStaminaState, NewState);

/**
 * �����������
 * 
 * ��������������ɫ�ľ���ϵͳ��������
 * - �������ĺͻָ�
 * - ��ͬ�����ľ����ɱ�����
 * - ����״̬�������������ľ����ָ��С�����ƣ�ͣ�
 * - ��������¼��Ĵ���
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SOUL_API UStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// ���캯��
	UStaminaComponent();

protected:
	// ��Ϸ��ʼʱ����
	virtual void BeginPlay() override;

public:	
	// ÿ֡����
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== ���ľ��������ӿ� ====================
	
	/**
	 * ����ָ�������ľ���
	 * @param Amount Ҫ���ĵľ���ֵ
	 * @return �Ƿ�ɹ����ģ���������ʱ����true��
	 */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool ConsumeStamina(float Amount);

	/**
	 * Ϊ�ض��������ľ���
	 * @param Action Ҫִ�еĶ�������
	 * @return �Ƿ�ɹ����ľ���
	 */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool ConsumeStaminaForAction(EStaminaAction Action);

	/**
	 * ����Ƿ��ܹ�ִ��ָ���������Ƿ����㹻������
	 * @param Action Ҫ���Ķ�������
	 * @return �Ƿ����㹻����ִ�иö���
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	bool CanPerformAction(EStaminaAction Action) const;

	/**
	 * �ָ�ָ�������ľ���
	 * @param Amount Ҫ�ָ��ľ���ֵ
	 */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void RecoverStamina(float Amount);

	/**
	 * ���þ��������ֵ
	 */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void ResetStamina();

	/**
	 * ���þ����ָ��Ƿ�����
	 * @param bEnabled �Ƿ������Զ��ָ�
	 */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void SetStaminaRecoveryEnabled(bool bEnabled);

	// ==================== ״̬��ѯ�ӿ� ====================

	/**
	 * ��ȡ��ǰ����ֵ
	 * @return ��ǰ����ֵ
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	float GetCurrentStamina() const { return CurrentStamina; }

	/**
	 * ��ȡ�����ٷֱȣ�0.0 - 1.0��
	 * @return �����ٷֱ�
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	float GetStaminaPercentage() const;

	/**
	 * ��龫���Ƿ��Ѻľ�
	 * @return �����Ƿ��Ѻľ�
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	bool IsStaminaDepleted() const { return CurrentStaminaState == EStaminaState::Depleted; }

	/**
	 * ��ȡ��ǰ����״̬
	 * @return ��ǰ����״̬
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	EStaminaState GetStaminaState() const { return CurrentStaminaState; }

	/**
	 * ��ȡ�����ֵ
	 * @return �����ֵ
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	float GetMaxStamina() const { return StaminaSettings.MaxStamina; }

	/**
	 * ��龫���Ƿ�����
	 * @return �����Ƿ�����
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	bool IsStaminaFull() const;

	// ==================== ���ýӿ� ====================

	/**
	 * ���þ�������
	 * @param NewSettings �µľ�������
	 */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void SetStaminaSettings(const FStaminaSettings& NewSettings);

	/**
	 * ��ȡ��������
	 * @return ��ǰ��������
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stamina")
	FStaminaSettings GetStaminaSettings() const { return StaminaSettings; }

	/**
	 * ���������ֵ���������ǰ����������
	 * @param NewMaxStamina �µ������ֵ
	 */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void SetMaxStamina(float NewMaxStamina);

	// ==================== �¼�ί�� ====================

	// �����仯�¼�
	UPROPERTY(BlueprintAssignable, Category = "Stamina Events")
	FOnStaminaChanged OnStaminaChanged;

	// �����ľ��¼�
	UPROPERTY(BlueprintAssignable, Category = "Stamina Events")
	FOnStaminaDepleted OnStaminaDepleted;

	// ������ȫ�ָ��¼�
	UPROPERTY(BlueprintAssignable, Category = "Stamina Events")
	FOnStaminaFullyRecovered OnStaminaFullyRecovered;

	// ����״̬�仯�¼�
	UPROPERTY(BlueprintAssignable, Category = "Stamina Events")
	FOnStaminaStateChanged OnStaminaStateChanged;

protected:
	// ==================== �������� ====================

	// ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina Settings")
	FStaminaSettings StaminaSettings;

	// ==================== ����ʱ״̬ ====================

	// ��ǰ����ֵ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stamina State")
	float CurrentStamina;

	// ��ǰ����״̬
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stamina State")
	EStaminaState CurrentStaminaState;

	// �����ָ��Ƿ�����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stamina State")
	bool bStaminaRecoveryEnabled;

	// ���һ��ʹ�þ�����ʱ��
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stamina State")
	float LastStaminaUseTime;

	// �����ָ���ʼʱ��
	float StaminaRecoveryStartTime;

	// �Ƿ����ڻָ�����
	bool bIsRecoveringStamina;

	// ����ƣ�ͼ�����
	int32 ExhaustedCounter;

private:
	// ==================== ˽�и������� ====================

	/**
	 * ���¾����ָ��߼�
	 * @param DeltaTime ʱ������
	 */
	void UpdateStaminaRecovery(float DeltaTime);

	/**
	 * ��ȡָ�������ľ�������
	 * @param Action ��������
	 * @return ��������ֵ
	 */
	float GetActionStaminaCost(EStaminaAction Action) const;

	/**
	 * ���¾���״̬
	 * @param NewState ��״̬
	 */
	void UpdateStaminaState(EStaminaState NewState);

	/**
	 * ���������仯�¼�
	 */
	void TriggerStaminaChangedEvent();

	/**
	 * ��鲢���������ľ�״̬
	 */
	void CheckStaminaDepletion();

	/**
	 * ��鲢����������ȫ�ָ�
	 */
	void CheckStaminaFullRecovery();

	/**
	 * ��ȡ��ǰ�ָ��ٶȣ����ǹ���ƣ�ͳͷ���
	 * @return ��ǰ�ָ��ٶ�
	 */
	float GetCurrentRecoveryRate() const;

	/**
	 * ��֤����ֵ��Χ
	 */
	void ClampStaminaValue();
};