// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "ExecutionComponent.generated.h"

// ǰ������
class AActor;
class UAnimMontage;

/** ��������ö�� */
UENUM(BlueprintType)
enum class EExecutionType : uint8
{
	None		UMETA(DisplayName = "None"),
	Backstab	UMETA(DisplayName = "Backstab"),		// ����
	Riposte		UMETA(DisplayName = "Riposte"),			// ����
	Plunge		UMETA(DisplayName = "Plunge"),			// ׹�乥��
	Critical	UMETA(DisplayName = "Critical")			// ��������
};

/** �������ýṹ�� */
USTRUCT(BlueprintType)
struct FExecutionSettings
{
	GENERATED_BODY()

	/** ���̼�ⷶΧ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstab", meta = (ClampMin = "50.0", ClampMax = "300.0"))
	float BackstabRange = 150.0f;

	/** ���̽Ƕ���ֵ���ȣ� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backstab", meta = (ClampMin = "15.0", ClampMax = "90.0"))
	float BackstabAngle = 45.0f;

	/** ����ʱ�䴰�ڣ��룩 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Riposte", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float RiposteWindow = 0.5f;

	/** ׹�乥����С�߶�Ҫ�� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plunge", meta = (ClampMin = "100.0", ClampMax = "500.0"))
	float PlungeHeightRequirement = 200.0f;

	/** �����˺����� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Critical", meta = (ClampMin = "1.5", ClampMax = "10.0"))
	float CriticalDamageMultiplier = 3.0f;

	/** ���̶�����̫�� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	UAnimMontage* BackstabMontage = nullptr;

	/** ����������̫�� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	UAnimMontage* RiposteMontage = nullptr;

	/** ׹�乥��������̫�� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	UAnimMontage* PlungeMontage = nullptr;

	FExecutionSettings()
	{
		BackstabRange = 150.0f;
		BackstabAngle = 45.0f;
		RiposteWindow = 0.5f;
		PlungeHeightRequirement = 200.0f;
		CriticalDamageMultiplier = 3.0f;
		BackstabMontage = nullptr;
		RiposteMontage = nullptr;
		PlungeMontage = nullptr;
	}
};

/** ������ʼ�¼�ί�� */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExecutionStarted, AActor*, Target, EExecutionType, ExecutionType);

/** ��������¼�ί�� */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExecutionCompleted, AActor*, Target, EExecutionType, ExecutionType);

/** ���̻����¼�ί�� */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackstabOpportunity, AActor*, Target);

/**
 * ���̴������
 * ���������̡�������׹�乥���ȴ���ϵͳ����
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SOUL_API UExecutionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UExecutionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// ==================== �������� ====================
	/** ���̶�λ��ֵ�ٶ� */
	static constexpr float BACKSTAB_POSITION_INTERP_SPEED = 8.0f;
	
	/** ����λ�õ���������ֵ */
	static constexpr float EXECUTION_POSITION_THRESHOLD = 5.0f;
	
	/** �������Ƶ�ʣ��룩 */
	static constexpr float RIPOSTE_CHECK_INTERVAL = 0.05f;
	
	/** ׹����߶Ȳ���Ƶ�ʣ��룩 */
	static constexpr float PLUNGE_HEIGHT_CHECK_INTERVAL = 0.1f;
	
	/** ����״̬����Ƶ�ʣ��룩 */
	static constexpr float EXECUTION_UPDATE_INTERVAL = 0.02f;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== �������� ====================
	/** ����ϵͳ���� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execution Settings")
	FExecutionSettings ExecutionSettings;

	// ==================== �¼�ί�� ====================
	/** ������ʼ�¼� */
	UPROPERTY(BlueprintAssignable, Category = "Execution Events")
	FOnExecutionStarted OnExecutionStarted;

	/** ��������¼� */
	UPROPERTY(BlueprintAssignable, Category = "Execution Events")
	FOnExecutionCompleted OnExecutionCompleted;

	/** ���̻����¼� */
	UPROPERTY(BlueprintAssignable, Category = "Execution Events")
	FOnBackstabOpportunity OnBackstabOpportunity;

	// ==================== ���Ľӿں��� ====================
	/** ����Ƿ����ִ��ָ�����͵Ĵ��� */
	UFUNCTION(BlueprintCallable, Category = "Execution")
	bool CanExecute(AActor* Target, EExecutionType ExecutionType) const;

	/** ��ʼִ�д��� */
	UFUNCTION(BlueprintCallable, Category = "Execution")
	bool StartExecution(AActor* Target, EExecutionType ExecutionType);

	/** ��ɴ��� */
	UFUNCTION(BlueprintCallable, Category = "Execution")
	void CompleteExecution();

	/** ȡ������ */
	UFUNCTION(BlueprintCallable, Category = "Execution")
	void CancelExecution();

	// ==================== ����ϵͳ���� ====================
	/** ��鱳�̻��� */
	UFUNCTION(BlueprintCallable, Category = "Backstab")
	bool CheckBackstabOpportunity(AActor* Target) const;

	/** ���Ŀ���Ƿ������ܵ����̹��� */
	UFUNCTION(BlueprintCallable, Category = "Backstab")
	bool IsTargetVulnerableToBackstab(AActor* Target) const;

	/** ��ȡ��ѱ���λ�� */
	UFUNCTION(BlueprintCallable, Category = "Backstab")
	FVector GetOptimalBackstabPosition(AActor* Target) const;

	// ==================== ����ϵͳ���� ====================
	/** ����Ƿ���Ե��� */
	UFUNCTION(BlueprintCallable, Category = "Riposte")
	bool CanRiposte(AActor* Target) const;

	/** ��ʼ����ʱ�䴰�� */
	UFUNCTION(BlueprintCallable, Category = "Riposte")
	void StartRiposteWindow(float Duration = -1.0f);

	/** ��������ʱ�䴰�� */
	UFUNCTION(BlueprintCallable, Category = "Riposte")
	void EndRiposteWindow();

	// ==================== ׹�乥������ ====================
	/** ����Ƿ���Խ���׹�乥�� */
	UFUNCTION(BlueprintCallable, Category = "Plunge")
	bool CanPlungeAttack() const;

	/** ��ȡ׹�乥��Ŀ���б� */
	UFUNCTION(BlueprintCallable, Category = "Plunge")
	TArray<AActor*> GetPlungeTargets() const;

	// ==================== ״̬��ѯ���� ====================
	/** ����Ƿ�����ִ�д��� */
	UFUNCTION(BlueprintCallable, Category = "Execution")
	bool IsExecuting() const { return CurrentExecutionType != EExecutionType::None; }

	/** ��ȡ��ǰ�������� */
	UFUNCTION(BlueprintCallable, Category = "Execution")
	EExecutionType GetCurrentExecutionType() const { return CurrentExecutionType; }

	/** ��ȡ��ǰ����Ŀ�� */
	UFUNCTION(BlueprintCallable, Category = "Execution")
	AActor* GetExecutionTarget() const { return CurrentExecutionTarget; }

protected:
	// ==================== �ڲ�״̬���� ====================
	/** ��ǰ�������� */
	UPROPERTY(BlueprintReadOnly, Category = "Execution State")
	EExecutionType CurrentExecutionType = EExecutionType::None;

	/** ��ǰ����Ŀ�� */
	UPROPERTY(BlueprintReadOnly, Category = "Execution State")
	AActor* CurrentExecutionTarget = nullptr;

	/** ���������Ƿ񼤻� */
	UPROPERTY(BlueprintReadOnly, Category = "Riposte State")
	bool bRiposteWindowActive = false;

	/** �������ڶ�ʱ����� */
	FTimerHandle RiposteWindowTimerHandle;

	/** ����λ�õ���״̬ */
	bool bIsPositioningForExecution = false;

	/** ������ʼʱ�� */
	float ExecutionStartTime = 0.0f;

	/** ������������״̬ */
	bool bExecutionAnimationPlaying = false;

	/** ��ɫ��ʼλ�ã�����λ�õ����� */
	FVector ExecutionStartPosition = FVector::ZeroVector;

	/** Ŀ�괦��λ�� */
	FVector TargetExecutionPosition = FVector::ZeroVector;

	// ==================== ˽�и������� ====================
	/** ��֤����Ŀ�����Ч�� */
	bool ValidateExecutionTarget(AActor* Target) const;

	/** ����ɫ��λ������λ�� */
	void PositionCharacterForExecution(AActor* Target, EExecutionType ExecutionType);

	/** ���㴦���˺� */
	float CalculateExecutionDamage(EExecutionType ExecutionType) const;

	/** ����ɫ�Ƿ��ڿ��У�����׹�乥���� */
	bool IsCharacterInAir() const;

	/** ��ȡ��ɫ��ǰ�߶� */
	float GetCharacterHeight() const;

	/** ��ȡ��ɫ������ľ��� */
	float GetDistanceToGround() const;

	/** ��������Actor֮��ĽǶȲ� */
	float CalculateAngleBetweenActors(AActor* Actor1, AActor* Actor2) const;

	/** ��ȡ��ɫ�Ĺ���������� */
	USkeletalMeshComponent* GetCharacterMesh() const;

	/** ���Ŵ������� */
	bool PlayExecutionAnimation(EExecutionType ExecutionType);

	/** ֹͣ��ǰ�������� */
	void StopExecutionAnimation();

	/** ����������ɻص� */
	UFUNCTION()
	void OnExecutionAnimationCompleted(UAnimMontage* Montage, bool bInterrupted);

	/** �������ڶ�ʱ���ص� */
	UFUNCTION()
	void OnRiposteWindowExpired();

	/** ���´���״̬ */
	void UpdateExecutionState(float DeltaTime);

	/** ���½�ɫλ�õ��� */
	void UpdatePositioning(float DeltaTime);

	/** Ӧ�ô����˺� */
	void ApplyExecutionDamage(AActor* Target, EExecutionType ExecutionType);

	/** ���ô���״̬ */
	void ResetExecutionState();

	/** ��ȡĿ�걳������λ�� */
	FVector GetBackstabPositionBehindTarget(AActor* Target) const;

	/** ��鱳�̽Ƕ��Ƿ���� */
	bool IsBackstabAngleValid(AActor* Target) const;

	/** ��鱳�̾����Ƿ���� */
	bool IsBackstabDistanceValid(AActor* Target) const;
};