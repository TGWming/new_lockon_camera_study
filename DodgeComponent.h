// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/DataTable.h"
#include "Animation/AnimMontage.h"
#include "Curves/CurveFloat.h"
#include "DodgeComponent.generated.h"

// ǰ������
class UStaminaComponent;

// ���ܷ���ö��
UENUM(BlueprintType)
enum class EDodgeDirection : uint8
{
	None			UMETA(DisplayName = "None"),
	Forward			UMETA(DisplayName = "Forward"),
	Backward		UMETA(DisplayName = "Backward"),
	Left			UMETA(DisplayName = "Left"),
	Right			UMETA(DisplayName = "Right"),
	ForwardLeft		UMETA(DisplayName = "Forward Left"),
	ForwardRight	UMETA(DisplayName = "Forward Right"),
	BackwardLeft	UMETA(DisplayName = "Backward Left"),
	BackwardRight	UMETA(DisplayName = "Backward Right")
};

// �������ýṹ��
USTRUCT(BlueprintType)
struct FDodgeSettings
{
	GENERATED_BODY()

	// ���ܾ���
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge Settings", meta = (ClampMin = "100.0", ClampMax = "1000.0"))
	float DodgeDistance = 400.0f;

	// ���ܳ���ʱ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge Settings", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float DodgeDuration = 0.6f;

	// �޵�֡��ʼʱ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Invincibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InvincibilityStartTime = 0.1f;

	// �޵�֡����ʱ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Invincibility", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float InvincibilityDuration = 0.4f;

	// �����ٶ�����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge Settings")
	UCurveFloat* DodgeSpeedCurve;

	// ǰ�����ܶ�����̫��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	UAnimMontage* ForwardDodgeMontage;

	// �������ܶ�����̫��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	UAnimMontage* BackwardDodgeMontage;

	// �������ܶ�����̫��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	UAnimMontage* LeftDodgeMontage;

	// �������ܶ�����̫��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	UAnimMontage* RightDodgeMontage;

	// Ĭ�Ϲ��캯��
	FDodgeSettings()
	{
		DodgeDistance = 400.0f;
		DodgeDuration = 0.6f;
		InvincibilityStartTime = 0.1f;
		InvincibilityDuration = 0.4f;
		DodgeSpeedCurve = nullptr;
		ForwardDodgeMontage = nullptr;
		BackwardDodgeMontage = nullptr;
		LeftDodgeMontage = nullptr;
		RightDodgeMontage = nullptr;
	}
};

// �¼�ί������
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDodgeStarted, EDodgeDirection, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDodgeEnded, EDodgeDirection, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvincibilityChanged, bool, bIsInvincible);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SOUL_API UDodgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDodgeComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== ���Ľӿں��� ====================
	
	// �������뷽��ʼ����
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	bool StartDodge(const FVector& InputDirection);

	// ����ָ������ʼ����
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	bool StartDodgeInDirection(EDodgeDirection Direction);

	// ��������
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void EndDodge();

	// ����Ƿ��������
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dodge")
	bool CanDodge() const;

	// ȡ������
	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void CancelDodge();

	// ==================== �޵�֡���� ====================
	
	// ��ʼ�޵�֡
	UFUNCTION(BlueprintCallable, Category = "Invincibility")
	void StartInvincibilityFrames();

	// �����޵�֡
	UFUNCTION(BlueprintCallable, Category = "Invincibility")
	void EndInvincibilityFrames();

	// ����Ƿ����޵�״̬
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Invincibility")
	bool IsInvincible() const;

	// ==================== ״̬��ѯ���� ====================
	
	// ����Ƿ���������
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dodge")
	bool IsDodging() const;

	// ��ȡ���һ�����ܷ���
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dodge")
	EDodgeDirection GetLastDodgeDirection() const;

	// ��ȡ���ܽ��� (0.0 - 1.0)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dodge")
	float GetDodgeProgress() const;

	// ==================== �¼�ί�� ====================
	
	// ���ܿ�ʼ�¼�
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDodgeStarted OnDodgeStarted;

	// ���ܽ����¼�
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDodgeEnded OnDodgeEnded;

	// �޵�״̬�仯�¼�
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnInvincibilityChanged OnInvincibilityChanged;

	// ==================== �������� ====================
	
	// ��������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FDodgeSettings DodgeSettings;

	// �������ĵľ���ֵ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float StaminaCost = 25.0f;

	// �Ƿ����õ�����־
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableDebugLogs = false;

protected:
	// ==================== ������� ====================
	
	// ����ʱ�������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* DodgeTimeline;

	// ����������ã�ʹ��UActorComponent*����������
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	UActorComponent* StaminaComponent;

	// ==================== ״̬���� ====================
	
	// �Ƿ���������
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDodging;

	// �Ƿ����޵�״̬
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsInvincible;

	// ��ǰ���ܷ���
	UPROPERTY(BlueprintReadOnly, Category = "State")
	EDodgeDirection CurrentDodgeDirection;

	// ���һ�����ܷ���
	UPROPERTY(BlueprintReadOnly, Category = "State")
	EDodgeDirection LastDodgeDirection;

	// ���ܿ�ʼλ��
	FVector DodgeStartLocation;

	// ����Ŀ��λ��
	FVector DodgeTargetLocation;

	// ���ܿ�ʼʱ��
	float DodgeStartTime;

	// �޵�֡��ʱ�����
	FTimerHandle InvincibilityTimerHandle;

private:
	// ==================== ˽�и������� ====================
	
	// ���������ƶ�
	void UpdateDodgeMovement(float DeltaTime);

	// �������ܷ���
	EDodgeDirection CalculateDodgeDirection(const FVector& InputDirection) const;

	// ���ݷ����ȡ��Ӧ�Ķ�����̫��
	UAnimMontage* GetDodgeMontageForDirection(EDodgeDirection Direction) const;

	// ʱ������»ص�
	UFUNCTION()
	void OnDodgeTimelineUpdate(float Value);

	// ʱ������ɻص�
	UFUNCTION()
	void OnDodgeTimelineFinished();

	// ��ʼ��ʱ����
	void InitializeTimeline();

	// �������ܶ���
	void PlayDodgeAnimation(EDodgeDirection Direction);

	// ��������Ŀ��λ��
	FVector CalculateDodgeTargetLocation(EDodgeDirection Direction) const;

	// ��ȡ��������
	FVector GetDirectionVector(EDodgeDirection Direction) const;

	// �������·���Ƿ�ȫ
	bool IsPathSafe(const FVector& StartLocation, const FVector& EndLocation) const;
};