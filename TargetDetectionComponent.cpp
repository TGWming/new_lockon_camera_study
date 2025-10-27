// Fill out your copyright notice in the Description page of Project Settings.

#include "TargetDetectionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

UTargetDetectionComponent::UTargetDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// ��ʼ���ڲ�״̬
	LockOnDetectionSphere = nullptr;
	LastTargetSearchTime = 0.0f;
	LastSizeUpdateTime = 0.0f;
	
	// �������
	LockOnCandidates.Empty();
	EnemySizeCache.Empty();
	
	UE_LOG(LogTemp, Log, TEXT("TargetDetectionComponent: Initialized"));
}

void UTargetDetectionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("TargetDetectionComponent: BeginPlay called"));
}

void UTargetDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!LockOnDetectionSphere || !GetOwnerCharacter())
	{
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();

	// ���ڲ��ҿ�����Ŀ��
	if (CurrentTime - LastTargetSearchTime > TARGET_SEARCH_INTERVAL)
	{
		FindLockOnCandidates();
		LastTargetSearchTime = CurrentTime;

		// ��Ƶ��־�Ż� - ���ӽ�Ƶ����
		if (bEnableTargetDetectionDebugLogs)
		{
			static int32 FrameCounter = 0;
			if (++FrameCounter % 300 == 0) // ÿ5��ֻ��¼һ�Σ�60fps * 5��
			{
				UE_LOG(LogTemp, Verbose, TEXT("TargetDetectionComponent: Target search completed, found %d candidates"), LockOnCandidates.Num());
			}
		}
	}

	// ���ڸ��µ��˳ߴ绺��
	if (CurrentTime - LastSizeUpdateTime > SIZE_UPDATE_INTERVAL)
	{
		UpdateEnemySizeCache();
		LastSizeUpdateTime = CurrentTime;

		// ��Ƶ��־�Ż� - ���ӽ�Ƶ����
		if (bEnableSizeAnalysisDebugLogs)
		{
			static int32 SizeFrameCounter = 0;
			if (++SizeFrameCounter % 300 == 0) // ÿ5��ֻ��¼һ��
			{
				UE_LOG(LogTemp, Verbose, TEXT("TargetDetectionComponent: Size cache updated, tracking %d enemies"), EnemySizeCache.Num());
			}
		}
	}
}

void UTargetDetectionComponent::SetLockOnDetectionSphere(USphereComponent* DetectionSphere)
{
	LockOnDetectionSphere = DetectionSphere;
	
	if (LockOnDetectionSphere)
	{
		// ���¼������뾶
		LockOnDetectionSphere->SetSphereRadius(LockOnSettings.LockOnRange);
		UE_LOG(LogTemp, Warning, TEXT("TargetDetectionComponent: Detection sphere set and configured"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TargetDetectionComponent: Detection sphere is null!"));
	}
}

EEnemySizeCategory UTargetDetectionComponent::GetTargetSizeCategory(AActor* Target)
{
	if (!Target)
	{
		return EEnemySizeCategory::Unknown;
	}

	// ��黺��
	EEnemySizeCategory* CachedSize = EnemySizeCache.Find(Target);
	if (CachedSize)
	{
		return *CachedSize;
	}

	// ������������
	EEnemySizeCategory SizeCategory = AnalyzeTargetSize(Target);
	EnemySizeCache.Add(Target, SizeCategory);

	// �����¼�
	if (OnValidTargetFound.IsBound())
	{
		OnValidTargetFound.Broadcast(Target, SizeCategory);
	}

	return SizeCategory;
}

void UTargetDetectionComponent::FindLockOnCandidates()
{
	if (!LockOnDetectionSphere)
	{
		UE_LOG(LogTemp, Error, TEXT("TargetDetectionComponent::FindLockOnCandidates: LockOnDetectionSphere is null!"));
		return;
	}

	// ����ϴεĺ�ѡ�б�
	LockOnCandidates.Empty();

	// ��ȡ���巶Χ�ڵ������ص�Actor
	TArray<AActor*> OverlappingActors;
	LockOnDetectionSphere->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	// ��ʱ�洢��ЧĿ��
	TArray<AActor*> ValidTargets;
	
	for (AActor* Actor : OverlappingActors)
	{
		if (IsValidLockOnTarget(Actor))
		{
			ValidTargets.Add(Actor);
		}
	}

	// ���Ƕ�������ЧĿ�� - ������
	if (ValidTargets.Num() > 1)
	{
		SortCandidatesByDirection(ValidTargets);
	}

	// ��������Ŀ�����ӵ���ѡ�б�
	LockOnCandidates = ValidTargets;

	// ����Ŀ������¼�
	if (OnTargetsUpdated.IsBound())
	{
		OnTargetsUpdated.Broadcast(LockOnCandidates);
	}

	// ��Ƶ��־�Ż� - ���ӽ�Ƶ����
	if (bEnableTargetDetectionDebugLogs)
	{
		static int32 FindFrameCounter = 0;
		if (++FindFrameCounter % 120 == 0) // ÿ2��ֻ��¼һ��
		{
			UE_LOG(LogTemp, Verbose, TEXT("TargetDetectionComponent: Lock-on candidates updated: %d targets available"), LockOnCandidates.Num());
		}
	}
}

bool UTargetDetectionComponent::IsValidLockOnTarget(AActor* Target)
{
	if (!ValidateBasicTargetConditions(Target))
	{
		return false;
	}

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return false;
	}

	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();

	// ������
	float Distance = FVector::Dist(PlayerLocation, TargetLocation);
	if (Distance > LockOnSettings.LockOnRange)
	{
		return false;
	}

	// ִ�����߼���������ڵ�
	if (!PerformLineOfSightCheck(Target))
	{
		return false;
	}

	return true;
}

bool UTargetDetectionComponent::IsTargetStillLockable(AActor* Target)
{
	if (!ValidateBasicTargetConditions(Target))
	{
		return false;
	}

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return false;
	}

	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();

	// ʹ�ø���ľ��뷶Χ����������
	float Distance = FVector::Dist(PlayerLocation, TargetLocation);
	float ExtendedLockOnRange = LockOnSettings.LockOnRange * LockOnSettings.ExtendedLockRangeMultiplier;
	
	if (Distance > ExtendedLockOnRange)
	{
		return false;
	}

	return true;
}

AActor* UTargetDetectionComponent::GetBestTargetFromList(const TArray<AActor*>& TargetList)
{
	if (TargetList.Num() == 0)
		return nullptr;

	AActor* BestTarget = nullptr;
	float BestScore = -1.0f;

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	AController* OwnerController = GetOwnerController();
	
	if (!OwnerCharacter || !OwnerController)
		return nullptr;

	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector CameraForward = OwnerController->GetControlRotation().Vector();

	for (AActor* Candidate : TargetList)
	{
		if (!IsValid(Candidate))
			continue;
			
		float Score = CalculateTargetScore(Candidate);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

AActor* UTargetDetectionComponent::GetBestSectorLockTarget()
{
	FindLockOnCandidates();

	if (LockOnCandidates.Num() == 0)
		return nullptr;

	// ���ȳ���������������������Ŀ��
	TArray<AActor*> SectorTargets;
	TArray<AActor*> EdgeTargets;

	for (AActor* Candidate : LockOnCandidates)
	{
		if (!Candidate)
			continue;
			
		if (IsTargetInSectorLockZone(Candidate))
		{
			SectorTargets.Add(Candidate);
		}
		else if (IsTargetInEdgeDetectionZone(Candidate))
		{
			EdgeTargets.Add(Candidate);
		}
	}

	// ���ȴ�����������ѡ��Ŀ��
	if (SectorTargets.Num() > 0)
	{
		return GetBestTargetFromList(SectorTargets);
	}

	// �������������û��Ŀ�꣬����Ե����
	if (EdgeTargets.Num() > 0)
	{
		return GetBestTargetFromList(EdgeTargets);
	}

	return nullptr;
}

AActor* UTargetDetectionComponent::TryGetSectorLockTarget()
{
	if (LockOnCandidates.Num() == 0)
		return nullptr;

	// ɸѡ���������ڵ�Ŀ��
	TArray<AActor*> SectorTargets;
	
	for (AActor* Candidate : LockOnCandidates)
	{
		if (Candidate && IsTargetInSectorLockZone(Candidate))
		{
			SectorTargets.Add(Candidate);
		}
	}

	// ���������������Ŀ�꣬�������Ŀ��
	if (SectorTargets.Num() > 0)
	{
		if (bEnableTargetDetectionDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("TargetDetectionComponent: Found %d targets in sector lock zone"), SectorTargets.Num());
		}
		return GetBestTargetFromList(SectorTargets);
	}

	return nullptr;
}

AActor* UTargetDetectionComponent::TryGetCameraCorrectionTarget()
{
	if (LockOnCandidates.Num() == 0)
		return nullptr;

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return nullptr;

	// Ѱ�������Ŀ�꣨�����ƽǶȣ�
	AActor* ClosestTarget = nullptr;
	float ClosestDistance = FLT_MAX;
	
	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	
	for (AActor* Candidate : LockOnCandidates)
	{
		if (!Candidate)
			continue;
			
		float Distance = FVector::Dist(PlayerLocation, Candidate->GetActorLocation());
		
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestTarget = Candidate;
		}
	}

	// ��������Ŀ���Ƿ��ں�����������Χ��
	if (ClosestTarget)
	{
		float AngleToTarget = CalculateAngleToTarget(ClosestTarget);
		
		// ֻ�ԽǶ��ں�����Χ�ڵ�Ŀ����������������������������ĵ��ˣ�
		if (AngleToTarget <= 160.0f) // ���160�ȣ�������160�ȷ�Χ��
		{
			if (bEnableTargetDetectionDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("TargetDetectionComponent: Found camera correction target: %s (Distance: %.1f, Angle: %.1f degrees)"), 
					*ClosestTarget->GetName(), ClosestDistance, AngleToTarget);
			}
			return ClosestTarget;
		}
		else
		{
			if (bEnableTargetDetectionDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("TargetDetectionComponent: Closest target %s is too far behind (%.1f degrees), no correction"), 
					*ClosestTarget->GetName(), AngleToTarget);
			}
		}
	}

	return nullptr;
}

void UTargetDetectionComponent::SortCandidatesByDirection(TArray<AActor*>& Targets)
{
	if (Targets.Num() <= 1)
		return;

	// �Ƴ���Чָ��
	Targets.RemoveAll([](AActor* Actor) { return !IsValid(Actor); });

	if (Targets.Num() <= 1)
		return;

	// ʹ��Lambda����ʽ�������򣬴����ң��Ƕȴ�С����
	Targets.Sort([this](const AActor& A, const AActor& B) {
		if (!IsValid(&A) || !IsValid(&B)) 
			return false;
		
		float AngleA = CalculateDirectionAngle(const_cast<AActor*>(&A));
		float AngleB = CalculateDirectionAngle(const_cast<AActor*>(&B));
		return AngleA < AngleB;
	});
}

bool UTargetDetectionComponent::HasCandidatesInSphere()
{
	FindLockOnCandidates();
	return LockOnCandidates.Num() > 0;
}

// ==================== �����ĵ��˳ߴ�������� ====================

TArray<AActor*> UTargetDetectionComponent::GetTargetsBySize(EEnemySizeCategory SizeCategory)
{
	TArray<AActor*> FilteredTargets;

	for (AActor* Candidate : LockOnCandidates)
	{
		if (Candidate && GetTargetSizeCategory(Candidate) == SizeCategory)
		{
			FilteredTargets.Add(Candidate);
		}
	}

	return FilteredTargets;
}

AActor* UTargetDetectionComponent::GetNearestTargetBySize(EEnemySizeCategory SizeCategory)
{
	TArray<AActor*> TargetsOfSize = GetTargetsBySize(SizeCategory);
	
	if (TargetsOfSize.Num() == 0)
		return nullptr;

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return nullptr;

	AActor* NearestTarget = nullptr;
	float NearestDistance = FLT_MAX;
	FVector PlayerLocation = OwnerCharacter->GetActorLocation();

	for (AActor* Target : TargetsOfSize)
	{
		if (!Target)
			continue;

		float Distance = FVector::Dist(PlayerLocation, Target->GetActorLocation());
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			NearestTarget = Target;
		}
	}

	return NearestTarget;
}

TMap<EEnemySizeCategory, int32> UTargetDetectionComponent::GetSizeCategoryStatistics()
{
	TMap<EEnemySizeCategory, int32> Statistics;
	
	// ��ʼ�����з���Ϊ0
	Statistics.Add(EEnemySizeCategory::Small, 0);
	Statistics.Add(EEnemySizeCategory::Medium, 0);
	Statistics.Add(EEnemySizeCategory::Large, 0);
	Statistics.Add(EEnemySizeCategory::Unknown, 0);

	// ͳ��ÿ�����������
	for (AActor* Candidate : LockOnCandidates)
	{
		if (Candidate)
		{
			EEnemySizeCategory Category = GetTargetSizeCategory(Candidate);
			if (int32* Count = Statistics.Find(Category))
			{
				(*Count)++;
			}
		}
	}

	return Statistics;
}

void UTargetDetectionComponent::UpdateTargetSizeCategory(AActor* Target)
{
	if (!Target)
		return;

	EEnemySizeCategory NewCategory = AnalyzeTargetSize(Target);
	EnemySizeCache.FindOrAdd(Target) = NewCategory;

	if (bEnableSizeAnalysisDebugLogs)
	{
		UE_LOG(LogTemp, Log, TEXT("TargetDetectionComponent: Updated size category for %s: %s"), 
			*Target->GetName(), 
			*UEnum::GetValueAsString(NewCategory));
	}
}

void UTargetDetectionComponent::CleanupSizeCache()
{
	TArray<AActor*> InvalidActors;

	// ������Ч��Actor
	for (auto& Pair : EnemySizeCache)
	{
		if (!IsValid(Pair.Key))
		{
			InvalidActors.Add(Pair.Key);
		}
	}

	// �Ƴ���Ч�Ļ�����Ŀ
	for (AActor* InvalidActor : InvalidActors)
	{
		EnemySizeCache.Remove(InvalidActor);
	}

	if (bEnableSizeAnalysisDebugLogs && InvalidActors.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("TargetDetectionComponent: Cleaned up %d invalid size cache entries"), InvalidActors.Num());
	}
}

// ==================== �ڲ��������� ====================

float UTargetDetectionComponent::CalculateTargetBoundingBoxSize(AActor* Target) const
{
	if (!Target)
		return 0.0f;

	// ��ȡActor�ı߽��
	FVector Origin, BoxExtent;
	Target->GetActorBounds(false, Origin, BoxExtent);

	// �������ά����Ϊ�ߴ�ο�
	float MaxDimension = FMath::Max3(BoxExtent.X, BoxExtent.Y, BoxExtent.Z) * 2.0f; // BoxExtent�ǰ볤��

	return MaxDimension;
}

EEnemySizeCategory UTargetDetectionComponent::AnalyzeTargetSize(AActor* Target)
{
	if (!Target)
		return EEnemySizeCategory::Unknown;

	float BoundingBoxSize = CalculateTargetBoundingBoxSize(Target);

	// ���ݸ߼���������е���ֵ���з���
	if (BoundingBoxSize <= AdvancedCameraSettings.SmallEnemySizeThreshold)
	{
		return EEnemySizeCategory::Small;
	}
	else if (BoundingBoxSize <= AdvancedCameraSettings.LargeEnemySizeThreshold)
	{
		return EEnemySizeCategory::Medium;
	}
	else
	{
		return EEnemySizeCategory::Large;
	}
}

bool UTargetDetectionComponent::IsTargetInSectorLockZone(AActor* Target) const
{
	AController* OwnerController = GetOwnerController();
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	
	if (!Target || !OwnerController || !OwnerCharacter)
		return false;

	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector CameraForward = OwnerController->GetControlRotation().Vector();
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

	// ����Ƕ�
	float DotProduct = FVector::DotProduct(CameraForward, ToTarget);
	DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	
	float AngleRadians = FMath::Acos(DotProduct);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	// ����Ƿ�����������������
	bool bInSectorZone = AngleDegrees <= (LockOnSettings.SectorLockAngle * 0.5f);

	return bInSectorZone;
}

bool UTargetDetectionComponent::IsTargetInEdgeDetectionZone(AActor* Target) const
{
	AController* OwnerController = GetOwnerController();
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	
	if (!Target || !OwnerController || !OwnerCharacter)
		return false;

	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector CameraForward = OwnerController->GetControlRotation().Vector();
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

	// ����Ƕ�
	float DotProduct = FVector::DotProduct(CameraForward, ToTarget);
	DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	
	float AngleRadians = FMath::Acos(DotProduct);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	// ����Ƿ��ڱ�Ե���������
	bool bInEdgeZone = (AngleDegrees > (LockOnSettings.SectorLockAngle * 0.5f)) && 
					   (AngleDegrees <= (LockOnSettings.EdgeDetectionAngle * 0.5f));

	return bInEdgeZone;
}

float UTargetDetectionComponent::CalculateAngleToTarget(AActor* Target) const
{
	AController* OwnerController = GetOwnerController();
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	
	if (!IsValid(Target) || !OwnerController || !OwnerCharacter)
		return 180.0f;

	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FVector CurrentCameraForward = OwnerController->GetControlRotation().Vector();
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();

	// ����ǶȲ���
	float DotProduct = FVector::DotProduct(CurrentCameraForward, ToTarget);
	DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
	
	float AngleRadians = FMath::Acos(DotProduct);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	return AngleDegrees;
}

float UTargetDetectionComponent::CalculateDirectionAngle(AActor* Target) const
{
	AController* OwnerController = GetOwnerController();
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	
	if (!IsValid(Target) || !OwnerController || !OwnerCharacter)
		return 0.0f;

	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();
	FRotator ControlRotation = OwnerController->GetControlRotation();
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

void UTargetDetectionComponent::UpdateEnemySizeCache()
{
	// ����������Ч����
	CleanupSizeCache();

	// Ϊ�·��ֵ�Ŀ����³ߴ����
	for (AActor* Candidate : LockOnCandidates)
	{
		if (Candidate && !EnemySizeCache.Contains(Candidate))
		{
			UpdateTargetSizeCategory(Candidate);
		}
	}
}

ACharacter* UTargetDetectionComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

AController* UTargetDetectionComponent::GetOwnerController() const
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	return OwnerCharacter ? OwnerCharacter->GetController() : nullptr;
}

// ==================== ˽�и������� ====================

bool UTargetDetectionComponent::PerformLineOfSightCheck(AActor* Target) const
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !Target)
		return false;

	FHitResult HitResult;
	FVector StartLocation = OwnerCharacter->GetActorLocation() + FVector(0, 0, LockOnSettings.RaycastHeightOffset);
	FVector EndLocation = Target->GetActorLocation() + FVector(0, 0, LockOnSettings.RaycastHeightOffset);
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.bTraceComplex = false;
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		QueryParams
	);

	// ������ڵ��һ��еĲ���Ŀ�걾��������Ч
	return !bHit || HitResult.GetActor() == Target;
}

float UTargetDetectionComponent::CalculateTargetScore(AActor* Target) const
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	AController* OwnerController = GetOwnerController();
	
	if (!OwnerCharacter || !OwnerController || !Target)
		return -1.0f;

	FVector PlayerLocation = OwnerCharacter->GetActorLocation();
	FVector ToTarget = (Target->GetActorLocation() - PlayerLocation).GetSafeNormal();
	float Distance = FVector::Dist(PlayerLocation, Target->GetActorLocation());
	
	// ��ֹ�������
	if (LockOnSettings.LockOnRange <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("TargetDetectionComponent: LockOnRange is zero or negative!"));
		return -1.0f;
	}
	
	// ʹ�����ǰ������������
	FVector CameraForward = OwnerController->GetControlRotation().Vector();
	float DotProduct = FVector::DotProduct(CameraForward, ToTarget);

	// �����㷨���Ƕ�����ռ70%����������ռ30%
	float NormalizedDistance = FMath::Sqrt(Distance / LockOnSettings.LockOnRange);
	float AngleFactor = DotProduct;
	float DistanceFactor = 1.0f - NormalizedDistance;
	
	float Score = (AngleFactor * 0.7f) + (DistanceFactor * 0.3f);

	// �ų���������ͬλ�õ�Ŀ��
	if (Distance < 50.0f)
	{
		Score -= 0.5f; // �������֣����������������ص���Ŀ��
	}

	return Score;
}

bool UTargetDetectionComponent::ValidateBasicTargetConditions(AActor* Target) const
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	
	if (!Target || Target == OwnerCharacter)
	{
		return false;
	}

	// ���ӵ���ʶ����
	if (Target->ActorHasTag(FName("Friendly")) || Target->ActorHasTag(FName("Player")))
	{
		return false;
	}

	// ���Ŀ���Ƿ񻹻��ţ����Ŀ��ΪPawn��	
	if (APawn* TargetPawn = Cast<APawn>(Target))
	{
		// ���Pawn�Ƿ����ٻ���Ч
		if (!IsValid(TargetPawn) || TargetPawn->IsPendingKill())
		{
			return false;
		}
	}

	return true;
}