#include "PerformanceProfiler.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/DateTime.h"

// ǰ������
class USoulDebugSettings;

// UPerformanceProfilerʵ��

void UPerformanceProfiler::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogTemp, Log, TEXT("PerformanceProfiler: Subsystem initialized"));
	
	// ��ʱӲ�����������ܼ�أ�����ѭ������?	bIsPerformanceMonitoringEnabled = true;
	
	// ��ʼ����������ӳ���?	PerformanceMap.Reset();
	
	UE_LOG(LogTemp, Warning, TEXT("PerformanceProfiler: Performance monitoring %s"), 
		bIsPerformanceMonitoringEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void UPerformanceProfiler::Deinitialize()
{
	if (bIsPerformanceMonitoringEnabled && PerformanceMap.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PerformanceProfiler: Shutting down with %d recorded functions"), PerformanceMap.Num());
		PrintPerformanceReport();
	}
	
	PerformanceMap.Reset();
	Super::Deinitialize();
}

void UPerformanceProfiler::RecordFunctionTime(const FString& FunctionName, float ElapsedTime)
{
	if (!bIsPerformanceMonitoringEnabled)
	{
		return;
	}

	if (FunctionName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PerformanceProfiler: Cannot record performance for empty function name"));
		return;
	}

	// ���һ򴴽���������
	if (!PerformanceMap.Contains(FunctionName))
	{
		PerformanceMap.Add(FunctionName, FPerformanceData(FunctionName));
	}

	FPerformanceData& Data = PerformanceMap[FunctionName];
	UpdatePerformanceStatistics(Data, ElapsedTime);

	// ��ϸ������־
	UE_LOG(LogTemp, VeryVerbose, TEXT("PerformanceProfiler: Recorded %s - Time: %.3fms (Avg: %.3fms, Max: %.3fms, Calls: %d)"), 
		*FunctionName, ElapsedTime, Data.AverageTime, Data.MaxTime, Data.CallCount);
}

TArray<FPerformanceData> UPerformanceProfiler::GetPerformanceReport()
{
	TArray<FPerformanceData> Report;
	
	for (const auto& Pair : PerformanceMap)
	{
		Report.Add(Pair.Value);
	}
	
	// ��ƽ��ִ��ʱ�併������
	Report.Sort([](const FPerformanceData& A, const FPerformanceData& B)
	{
		return A.AverageTime > B.AverageTime;
	});
	
	return Report;
}

void UPerformanceProfiler::ResetPerformanceData()
{
	int32 PreviousCount = PerformanceMap.Num();
	PerformanceMap.Reset();
	
	UE_LOG(LogTemp, Warning, TEXT("PerformanceProfiler: Reset performance data (%d functions cleared)"), PreviousCount);
}

void UPerformanceProfiler::PrintPerformanceReport()
{
	if (PerformanceMap.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PerformanceProfiler: No performance data to report"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT(""));
	UE_LOG(LogTemp, Warning, TEXT("=== SOUL PERFORMANCE REPORT ==="));
	UE_LOG(LogTemp, Warning, TEXT("Generated at: %s"), *FDateTime::Now().ToString());
	UE_LOG(LogTemp, Warning, TEXT("Total functions tracked: %d"), PerformanceMap.Num());
	UE_LOG(LogTemp, Warning, TEXT(""));

	// ��ȡ�����ı���
	TArray<FPerformanceData> SortedReport = GetPerformanceReport();

	// ��ӡ��ͷ
	UE_LOG(LogTemp, Warning, TEXT("%-40s | %10s | %10s | %10s | %8s"), 
		TEXT("Function Name"), TEXT("Avg (ms)"), TEXT("Max (ms)"), TEXT("Min (ms)"), TEXT("Calls"));
	UE_LOG(LogTemp, Warning, TEXT("%-40s-|-%10s-|-%10s-|-%10s-|-%8s"), 
		TEXT("----------------------------------------"), 
		TEXT("----------"), TEXT("----------"), TEXT("----------"), TEXT("--------"));

	// ��ӡÿ����������������
	for (const FPerformanceData& Data : SortedReport)
	{
		FString FunctionDisplayName = Data.FunctionName;
		if (FunctionDisplayName.Len() > 40)
		{
			FunctionDisplayName = FunctionDisplayName.Left(37) + TEXT("...");
		}

		// ������Сʱ����ʾֵ
		float MinTimeDisplay = (Data.MinTime == FLT_MAX) ? 0.0f : Data.MinTime;

		UE_LOG(LogTemp, Warning, TEXT("%-40s | %10s | %10s | %10s | %8d"), 
			*FunctionDisplayName,
			*FormatTime(Data.AverageTime),
			*FormatTime(Data.MaxTime),
			*FormatTime(MinTimeDisplay),
			Data.CallCount);
	}

	UE_LOG(LogTemp, Warning, TEXT(""));

	// ͳ����Ϣ
	float TotalTime = 0.0f;
	int32 TotalCalls = 0;
	float MaxAvgTime = 0.0f;
	FString SlowestFunction;

	for (const FPerformanceData& Data : SortedReport)
	{
		TotalTime += Data.TotalTime;
		TotalCalls += Data.CallCount;
		
		if (Data.AverageTime > MaxAvgTime)
		{
			MaxAvgTime = Data.AverageTime;
			SlowestFunction = Data.FunctionName;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("SUMMARY:"));
	UE_LOG(LogTemp, Warning, TEXT("- Total execution time: %s"), *FormatTime(TotalTime));
	UE_LOG(LogTemp, Warning, TEXT("- Total function calls: %d"), TotalCalls);
	UE_LOG(LogTemp, Warning, TEXT("- Slowest function: %s (avg: %s)"), *SlowestFunction, *FormatTime(MaxAvgTime));
	UE_LOG(LogTemp, Warning, TEXT("=============================="));
	UE_LOG(LogTemp, Warning, TEXT(""));
}

FPerformanceData UPerformanceProfiler::GetFunctionPerformanceData(const FString& FunctionName)
{
	if (PerformanceMap.Contains(FunctionName))
	{
		return PerformanceMap[FunctionName];
	}
	
	// ���ؿյ���������
	return FPerformanceData();
}

bool UPerformanceProfiler::IsPerformanceMonitoringEnabled() const
{
	return bIsPerformanceMonitoringEnabled;
}

void UPerformanceProfiler::SetPerformanceMonitoringEnabled(bool bEnabled)
{
	bool bPreviousState = bIsPerformanceMonitoringEnabled;
	bIsPerformanceMonitoringEnabled = bEnabled;
	
	UE_LOG(LogTemp, Warning, TEXT("PerformanceProfiler: Performance monitoring %s -> %s"), 
		bPreviousState ? TEXT("ENABLED") : TEXT("DISABLED"),
		bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
	
	if (!bEnabled && PerformanceMap.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("PerformanceProfiler: Performance monitoring disabled, current data preserved"));
	}
}

UPerformanceProfiler* UPerformanceProfiler::GetPerformanceProfiler(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Warning, TEXT("PerformanceProfiler: Invalid WorldContextObject"));
		return nullptr;
	}

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("PerformanceProfiler: Could not get World from context object"));
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("PerformanceProfiler: No GameInstance found"));
		return nullptr;
	}

	return GameInstance->GetSubsystem<UPerformanceProfiler>();
}

void UPerformanceProfiler::UpdatePerformanceStatistics(FPerformanceData& Data, float ElapsedTime)
{
	// ���µ��ô���
	Data.CallCount++;
	
	// ������ʱ��
	Data.TotalTime += ElapsedTime;
	
	// ����ƽ��ʱ��
	Data.AverageTime = Data.TotalTime / static_cast<float>(Data.CallCount);
	
	// �������ʱ��?	if (ElapsedTime > Data.MaxTime)
	{
		Data.MaxTime = ElapsedTime;
	}
	
	// ������Сʱ��
	if (ElapsedTime < Data.MinTime)
	{
		Data.MinTime = ElapsedTime;
	}
}

FString UPerformanceProfiler::FormatTime(float TimeInMs) const
{
	if (TimeInMs < 0.001f)
	{
		return FString::Printf(TEXT("%.3f?s"), TimeInMs * 1000.0f);
	}
	else if (TimeInMs < 1.0f)
	{
		return FString::Printf(TEXT("%.3fms"), TimeInMs);
	}
	else if (TimeInMs < 1000.0f)
	{
		return FString::Printf(TEXT("%.2fms"), TimeInMs);
	}
	else
	{
		return FString::Printf(TEXT("%.2fs"), TimeInMs / 1000.0f);
	}
}

// FSoulPerformanceScopeʵ��

FSoulPerformanceScope::FSoulPerformanceScope(const FString& InFunctionName)
	: FunctionName(InFunctionName)
	, StartTime(0.0)
{
	// ��¼��ʼʱ�䣨����Ϊ��λ��
	StartTime = FPlatformTime::Seconds();
}

FSoulPerformanceScope::~FSoulPerformanceScope()
{
	// ����ִ��ʱ�䣨ת��Ϊ���룩
	double EndTime = FPlatformTime::Seconds();
	float ElapsedTimeMs = static_cast<float>((EndTime - StartTime) * 1000.0);
	
	// ��ȡ���ܷ�����ʵ������¼ʱ��
	if (GEngine && GEngine->GetCurrentPlayWorld())
	{
		UPerformanceProfiler* Profiler = UPerformanceProfiler::GetPerformanceProfiler(GEngine->GetCurrentPlayWorld());
		if (Profiler)
		{
			Profiler->RecordFunctionTime(FunctionName, ElapsedTimeMs);
		}
	}
}