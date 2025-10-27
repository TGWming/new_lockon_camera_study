#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PerformanceProfiler.generated.h"

/**
 * �������ݽṹ��
 * �洢��������ͳ����Ϣ
 */
USTRUCT(BlueprintType)
struct SOUL_API FPerformanceData
{
	GENERATED_BODY()

	/** �������� */
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	FString FunctionName;

	/** ƽ��ִ��ʱ�䣨���룩 */
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	float AverageTime = 0.0f;

	/** ���ִ��ʱ�䣨���룩 */
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	float MaxTime = 0.0f;

	/** ���ô��� */
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	int32 CallCount = 0;

	/** ��ִ��ʱ�䣨���룩 */
	float TotalTime = 0.0f;

	/** ��Сִ��ʱ�䣨���룩 */
	float MinTime = 0.0f;

	FPerformanceData()
	{
		FunctionName = TEXT("");
		AverageTime = 0.0f;
		MaxTime = 0.0f;
		CallCount = 0;
		TotalTime = 0.0f;
		MinTime = FLT_MAX;
	}

	FPerformanceData(const FString& InFunctionName)
	{
		FunctionName = InFunctionName;
		AverageTime = 0.0f;
		MaxTime = 0.0f;
		CallCount = 0;
		TotalTime = 0.0f;
		MinTime = FLT_MAX;
	}
};

/**
 * ���ܷ�������ϵͳ
 * �����ռ��ͷ�������ִ������
 */
UCLASS(BlueprintType)
class SOUL_API UPerformanceProfiler : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * ��¼����ִ��ʱ��
	 * @param FunctionName ��������
	 * @param ElapsedTime ִ��ʱ�䣨���룩
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance Profiler")
	void RecordFunctionTime(const FString& FunctionName, float ElapsedTime);

	/**
	 * ��ȡ���ܱ���
	 * @return �������к����������ݵ�����
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance Profiler")
	TArray<FPerformanceData> GetPerformanceReport();

	/**
	 * ������������
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance Profiler")
	void ResetPerformanceData();

	/**
	 * ��ӡ���ܱ��浽��־
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance Profiler")
	void PrintPerformanceReport();

	/**
	 * ��ȡ�ض���������������
	 * @param FunctionName ��������
	 * @return �������������ݣ�����������򷵻ؿ�����
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance Profiler")
	FPerformanceData GetFunctionPerformanceData(const FString& FunctionName);

	/**
	 * ����Ƿ��������ܼ��
	 * @return �����򷵻�true
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance Profiler")
	bool IsPerformanceMonitoringEnabled() const;

	/**
	 * ���û�ر����ܼ��
	 * @param bEnabled �Ƿ�����
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance Profiler")
	void SetPerformanceMonitoringEnabled(bool bEnabled);

	/**
	 * ��ȡ���ܷ�����ʵ��
	 * @param WorldContext ���������ĵĶ���
	 * @return ���ܷ�����ʵ��������������򷵻�nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance Profiler", meta = (WorldContext = "WorldContextObject"))
	static UPerformanceProfiler* GetPerformanceProfiler(const UObject* WorldContextObject);

private:
	/** ��������ӳ��� */
	UPROPERTY()
	TMap<FString, FPerformanceData> PerformanceMap;

	/** �Ƿ��������ܼ�� */
	UPROPERTY()
	bool bIsPerformanceMonitoringEnabled = true;

	/** ������������ͳ����Ϣ */
	void UpdatePerformanceStatistics(FPerformanceData& Data, float ElapsedTime);

	/** ��ʽ��ʱ��Ϊ�ɶ��ַ��� */
	FString FormatTime(float TimeInMs) const;
};

#ifndef FSOUL_PERFORMANCE_SCOPE_DEFINED
#define FSOUL_PERFORMANCE_SCOPE_DEFINED
/**
 * ����������ṹ��
 * �����Զ����㺯��ִ��ʱ��
 */
struct SOUL_API FSoulPerformanceScope
{
public:
	FSoulPerformanceScope(const FString& InFunctionName);
	~FSoulPerformanceScope();

private:
	FString FunctionName;
	double StartTime;
};
#endif

// ���ܼ��궨�壨��������ʱ������
#ifndef SOUL_PERFORMANCE_SCOPE
#define SOUL_PERFORMANCE_SCOPE(FunctionName) \
	FSoulPerformanceScope PerformanceScope(FunctionName)
#endif

#ifndef SOUL_PERFORMANCE_SCOPE_CONDITIONAL
#define SOUL_PERFORMANCE_SCOPE_CONDITIONAL(FunctionName, Condition) \
	TOptional<FSoulPerformanceScope> PerformanceScope; \
	if (Condition) \
	{ \
		PerformanceScope.Emplace(FunctionName); \
	}
#endif