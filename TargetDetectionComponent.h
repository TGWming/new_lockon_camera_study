// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnConfig.h"
#include "TargetDetectionComponent.generated.h"

// ǰ������
class USphereComponent;

// Ŀ������¼�ί��
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetsUpdated, const TArray<AActor*>&, UpdatedTargets);

// ��ЧĿ�귢���¼�ί��
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnValidTargetFound, AActor*, Target, EEnemySizeCategory, SizeCategory);

/**
 * ������Ŀ�������
 * ����������Ŀ����������֤������ͳߴ��������
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SOUL_API UTargetDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetDetectionComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ==================== �¼�ί�� ====================
	/** Ŀ���б�����ʱ���� */
	UPROPERTY(BlueprintAssignable, Category = "Target Detection Events")
	FOnTargetsUpdated OnTargetsUpdated;

	/** ������ЧĿ��ʱ���� */
	UPROPERTY(BlueprintAssignable, Category = "Target Detection Events")
	FOnValidTargetFound OnValidTargetFound;

	// ==================== ���ò��� ====================
	/** ����ϵͳ���� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock-On Settings")
	FLockOnSettings LockOnSettings;

	/** ���ϵͳ���� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	FCameraSettings CameraSettings;

	/** �߼�������� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Camera Settings")
	FAdvancedCameraSettings AdvancedCameraSettings;

	/** SocketͶ������ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket Projection Settings")
	FSocketProjectionSettings SocketProjectionSettings;

	// ==================== �������������� ====================
	/** ������������������ӵ���ߴ��룩 */
	UPROPERTY()
	USphereComponent* LockOnDetectionSphere;

	// ==================== ���Կ��� ====================
	/** �Ƿ�����Ŀ���������־ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableTargetDetectionDebugLogs;

	/** �Ƿ����óߴ����������־ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableSizeAnalysisDebugLogs;

protected:
	// ==================== �ڲ�״̬���� ====================
	/** ��ǰ������Ŀ���б� */
	UPROPERTY()
	TArray<AActor*> LockOnCandidates;

	/** ���˳ߴ���໺�� */
	UPROPERTY()
	TMap<AActor*, EEnemySizeCategory> EnemySizeCache;

	/** �ϴ�Ŀ������ʱ�� */
	float LastTargetSearchTime;

	/** �ϴγߴ����ʱ�� */
	float LastSizeUpdateTime;

	/** �����Ż���Ŀ��������� */
	static constexpr float TARGET_SEARCH_INTERVAL = 0.2f;

	/** �����Ż����ߴ���¼�� */
	static constexpr float SIZE_UPDATE_INTERVAL = 1.0f;

public:
	// ==================== ��Ҫ�ӿں��� ====================
	
	/** ���ü������������� */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	void SetLockOnDetectionSphere(USphereComponent* DetectionSphere);

	/** ��ȡ��ǰ������Ŀ���б� */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	const TArray<AActor*>& GetLockOnCandidates() const { return LockOnCandidates; }

	/** ��ȡĿ��ĳߴ���� */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	EEnemySizeCategory GetTargetSizeCategory(AActor* Target);

	// ==================== ��MyCharacterǨ�Ƶĺ��� ====================
	
	/** ���ҿ�����Ŀ�� */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	void FindLockOnCandidates();

	/** ���Ŀ���Ƿ���Ч */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	bool IsValidLockOnTarget(AActor* Target);

	/** ���Ŀ���Ƿ���Ȼ���Ա������� */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	bool IsTargetStillLockable(AActor* Target);

	/** ��Ŀ���б��л�ȡ���Ŀ�� */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	AActor* GetBestTargetFromList(const TArray<AActor*>& TargetList);

	/** ��ȡ���������ڵ��������Ŀ�� */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	AActor* GetBestSectorLockTarget();

	/** ���Ի�ȡ���������ڵ�����Ŀ�� */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	AActor* TryGetSectorLockTarget();

	/** ���Ի�ȡ��Ҫ���������Ŀ�� */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	AActor* TryGetCameraCorrectionTarget();

	/** ������Ƕ������ѡĿ�� */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	void SortCandidatesByDirection(TArray<AActor*>& Targets);

	/** ����������Ƿ��к�ѡĿ�� */
	UFUNCTION(BlueprintCallable, Category = "Target Detection")
	bool HasCandidatesInSphere();

	// ==================== �����ĵ��˳ߴ�������� ====================
	
	/** ���ݳߴ�����ȡĿ���б� */
	UFUNCTION(BlueprintCallable, Category = "Enemy Size Analysis")
	TArray<AActor*> GetTargetsBySize(EEnemySizeCategory SizeCategory);

	/** ��ȡָ���ߴ�����������Ŀ�� */
	UFUNCTION(BlueprintCallable, Category = "Enemy Size Analysis")
	AActor* GetNearestTargetBySize(EEnemySizeCategory SizeCategory);

	/** ��ȡ���гߴ����ͳ����Ϣ */
	UFUNCTION(BlueprintCallable, Category = "Enemy Size Analysis")
	TMap<EEnemySizeCategory, int32> GetSizeCategoryStatistics();

	/** ǿ�Ƹ���ָ��Ŀ��ĳߴ���� */
	UFUNCTION(BlueprintCallable, Category = "Enemy Size Analysis")
	void UpdateTargetSizeCategory(AActor* Target);

	/** ������ЧĿ��ĳߴ绺�� */
	UFUNCTION(BlueprintCallable, Category = "Enemy Size Analysis")
	void CleanupSizeCache();

protected:
	// ==================== �ڲ��������� ====================
	
	/** ����Ŀ��ı߽�гߴ� */
	float CalculateTargetBoundingBoxSize(AActor* Target) const;

	/** ����Ŀ��ĳߴ���� */
	EEnemySizeCategory AnalyzeTargetSize(AActor* Target);

	/** ���Ŀ���Ƿ����������������� */
	bool IsTargetInSectorLockZone(AActor* Target) const;

	/** ���Ŀ���Ƿ��ڱ�Ե��������� */
	bool IsTargetInEdgeDetectionZone(AActor* Target) const;

	/** ���㵽Ŀ��ĽǶȲ��� */
	float CalculateAngleToTarget(AActor* Target) const;

	/** ����Ŀ���������ҵķ���Ƕ� */
	float CalculateDirectionAngle(AActor* Target) const;

	/** ���µ��˳ߴ绺�� */
	void UpdateEnemySizeCache();

	/** ��ȡӵ���߽�ɫ */
	class ACharacter* GetOwnerCharacter() const;

	/** ��ȡӵ���ߵĿ����� */
	class AController* GetOwnerController() const;

private:
	// ==================== ˽�и������� ====================
	
	/** �ڲ�������ִ�����߼�� */
	bool PerformLineOfSightCheck(AActor* Target) const;

	/** �ڲ�����������Ŀ������ */
	float CalculateTargetScore(AActor* Target) const;

	/** �ڲ���������֤Ŀ��Ļ������� */
	bool ValidateBasicTargetConditions(AActor* Target) const;
};