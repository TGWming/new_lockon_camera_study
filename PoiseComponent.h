// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "PoiseComponent.generated.h"

// ����״̬ö��
UENUM(BlueprintType)
enum class EPoiseState : uint8
{
	Normal		UMETA(DisplayName = "Normal"),
	Damaged		UMETA(DisplayName = "Damaged"),
	Broken		UMETA(DisplayName = "Broken"),
	Recovering	UMETA(DisplayName = "Recovering"),
	Immune		UMETA(DisplayName = "Immune")
};

// �������ýṹ��
USTRUCT(BlueprintType)
struct FPoiseSettings
{
	GENERATED_BODY()

	// �������ֵ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise Settings", meta = (ClampMin = "1.0", ClampMax = "1000.0"))
	float MaxPoise = 100.0f;

	// ���Իָ����ʣ�ÿ��ָ�������ֵ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise Settings", meta = (ClampMin = "0.1", ClampMax = "100.0"))
	float PoiseRecoveryRate = 10.0f;

	// ���Իָ��ӳ٣��ܵ��˺����ÿ�ʼ�ָ���
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise Settings", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float PoiseRecoveryDelay = 3.0f;

	// ����Ӳֱ����ʱ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise Settings", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float BaseStaggerDuration = 1.5f;

	// �����ƻ��������ʱ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise Settings", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float PoiseImmuneTimeAfterBreak = 0.5f;

	// ���캯��
	FPoiseSettings()
		: MaxPoise(100.0f)
		, PoiseRecoveryRate(10.0f)
		, PoiseRecoveryDelay(3.0f)
		, BaseStaggerDuration(1.5f)
		, PoiseImmuneTimeAfterBreak(0.5f)
	{
	}
};

// �¼�ί������
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPoiseBreak, AActor*, OwnerActor, float, StaggerDuration, AActor*, DamageSource);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPoiseChanged, AActor*, OwnerActor, float, CurrentPoise, float, MaxPoise);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaggerStarted, AActor*, OwnerActor, float, StaggerDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaggerEnded, AActor*, OwnerActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPoiseStateChanged, AActor*, OwnerActor, EPoiseState, NewState);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SOUL_API UPoiseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPoiseComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== �������� ====================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise Settings")
	FPoiseSettings PoiseSettings;

	// ==================== �¼�ί�� ====================
	UPROPERTY(BlueprintAssignable, Category = "Poise Events")
	FOnPoiseBreak OnPoiseBreak;

	UPROPERTY(BlueprintAssignable, Category = "Poise Events")
	FOnPoiseChanged OnPoiseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Poise Events")
	FOnStaggerStarted OnStaggerStarted;

	UPROPERTY(BlueprintAssignable, Category = "Poise Events")
	FOnStaggerEnded OnStaggerEnded;

	UPROPERTY(BlueprintAssignable, Category = "Poise Events")
	FOnPoiseStateChanged OnPoiseStateChanged;

	// ==================== ���Ľӿں��� ====================
	
	/**
	 * ����������˺�
	 * @param PoiseDamage �����˺�ֵ
	 * @param DamageSource �˺���Դ����ѡ��
	 * @return �Ƿ�ɹ���������˺�
	 */
	UFUNCTION(BlueprintCallable, Category = "Poise System")
	bool TakePoiseDamage(float PoiseDamage, AActor* DamageSource = nullptr);

	/**
	 * ǿ���ƻ�����
	 * @param DamageSource �˺���Դ����ѡ��
	 */
	UFUNCTION(BlueprintCallable, Category = "Poise System")
	void BreakPoise(AActor* DamageSource = nullptr);

	/**
	 * �ָ�����
	 * @param RecoveryAmount �ָ��������Ϊ0��ʹ��Ĭ�ϻָ����ʣ�
	 */
	UFUNCTION(BlueprintCallable, Category = "Poise System")
	void RecoverPoise(float RecoveryAmount = 0.0f);

	/**
	 * �������Ե����ֵ
	 */
	UFUNCTION(BlueprintCallable, Category = "Poise System")
	void ResetPoise();

	/**
	 * ������������״̬
	 * @param bImmune �Ƿ�����
	 * @param Duration ���߳���ʱ�䣨0��ʾ��������ֱ���ֶ�ȡ����
	 */
	UFUNCTION(BlueprintCallable, Category = "Poise System")
	void SetPoiseImmune(bool bImmune, float Duration = 0.0f);

	// ==================== ״̬��ѯ���� ====================

	/**
	 * ��������Ƿ����ƻ�
	 */
	UFUNCTION(BlueprintPure, Category = "Poise System")
	bool IsPoiseBroken() const;

	/**
	 * ����Ƿ�����������״̬
	 */
	UFUNCTION(BlueprintPure, Category = "Poise System")
	bool IsPoiseImmune() const;

	/**
	 * ��ȡ��ǰ����ֵ
	 */
	UFUNCTION(BlueprintPure, Category = "Poise System")
	float GetCurrentPoise() const;

	/**
	 * ��ȡ���԰ٷֱ�
	 */
	UFUNCTION(BlueprintPure, Category = "Poise System")
	float GetPoisePercentage() const;

	/**
	 * ��ȡ��ǰ����״̬
	 */
	UFUNCTION(BlueprintPure, Category = "Poise System")
	EPoiseState GetPoiseState() const;

	/**
	 * ��ȡ�������ֵ
	 */
	UFUNCTION(BlueprintPure, Category = "Poise System")
	float GetMaxPoise() const;

	/**
	 * ����Ƿ�����Ӳֱ��
	 */
	UFUNCTION(BlueprintPure, Category = "Poise System")
	bool IsStaggering() const;

	// ==================== ���ú��� ====================

	/**
	 * ������������
	 */
	UFUNCTION(BlueprintCallable, Category = "Poise System")
	void UpdatePoiseSettings(const FPoiseSettings& NewSettings);

	/**
	 * �����������ֵ
	 */
	UFUNCTION(BlueprintCallable, Category = "Poise System")
	void SetMaxPoise(float NewMaxPoise);

protected:
	// ==================== Protected��Ա���� ====================

	// ��ǰ����ֵ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poise State")
	float CurrentPoise;

	// ��ǰ����״̬
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poise State")
	EPoiseState CurrentPoiseState;

	// ����ܵ��˺���ʱ��
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poise State")
	float LastDamageTime;

	// �������߽���ʱ��
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poise State")
	float PoiseImmuneEndTime;

	// ӵ���߽�ɫ����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poise State")
	ACharacter* OwnerCharacter;

	// Ӳֱ��ʱ�����
	FTimerHandle StaggerTimerHandle;

	// ���߶�ʱ�����
	FTimerHandle ImmuneTimerHandle;

	// �Ƿ�����Ӳֱ��
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poise State")
	bool bIsStaggering;

	// �Ƿ��ֶ��������ߣ��������ƻ�����Զ����ߣ�
	bool bManuallySetImmune;

	// ==================== ˽�и������� ====================

private:
	/**
	 * �������Իָ��߼�
	 */
	void UpdatePoiseRecovery(float DeltaTime);

	/**
	 * ��ʼӲֱ
	 */
	void StartStagger(float StaggerDuration, AActor* DamageSource = nullptr);

	/**
	 * ����Ӳֱ
	 */
	void EndStagger();

	/**
	 * ��������״̬
	 */
	void SetPoiseState(EPoiseState NewState);

	/**
	 * ������������
	 */
	void EndPoiseImmune();

	/**
	 * ����Ӳֱ����ʱ��
	 */
	float CalculateStaggerDuration(float PoiseDamage) const;

	/**
	 * �㲥���Ա仯�¼�
	 */
	void BroadcastPoiseChanged();

	/**
	 * ��֤���״̬
	 */
	bool IsValidForPoiseOperations() const;
};