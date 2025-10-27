#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DebugManager.generated.h"

// ǰ������
class UPerformanceProfiler;

/**
 * ������־����ö��
 */
UENUM(BlueprintType)
enum class EDebugLogLevel : uint8
{
	None			UMETA(DisplayName = "None"),
	Error			UMETA(DisplayName = "Error"),
	Warning			UMETA(DisplayName = "Warning"),
	Info			UMETA(DisplayName = "Info"),
	Verbose			UMETA(DisplayName = "Verbose"),
	VeryVerbose		UMETA(DisplayName = "Very Verbose")
};

/**
 * �������ýṹ��
 */
USTRUCT(BlueprintType)
struct SOUL_API FDebugSettings
{
	GENERATED_BODY()

	/** �Ƿ����õ�����־ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Debug")
	bool bEnableDebugLogs = false;

	/** �Ƿ���������ϵͳ��־ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On Debug")
	bool bEnableLockOnDebugLogs = false;

	/** �Ƿ��������ϵͳ��־ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Debug")
	bool bEnableCameraDebugLogs = false;

	/** �Ƿ�����Ŀ������־ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Detection Debug")
	bool bEnableTargetDetectionDebugLogs = false;

	/** �Ƿ�����UI������־ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Debug")
	bool bEnableUIDebugLogs = false;

	/** �Ƿ�����SocketͶ�������־ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket Projection Debug")
	bool bEnableSocketProjectionDebugLogs = false;

	/** �Ƿ����ø߼����������־ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Camera Debug")
	bool bEnableAdvancedCameraDebugLogs = false;

	/** �Ƿ��������ܼ�� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Debug")
	bool bEnablePerformanceMonitoring = false;

	/** ȫ����־���� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Debug")
	EDebugLogLevel GlobalLogLevel = EDebugLogLevel::Warning;
};

/**
 * Soul����������
 * �̳���UObject��ȷ��������
 */
UCLASS(BlueprintType, meta = (DisplayName = "Soul Debug Settings"))
class SOUL_API USoulDebugSettings : public UObject
{
	GENERATED_BODY()

public:
	USoulDebugSettings();

	/** �������� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings")
	FDebugSettings DebugSettings;

	// ��̬���ʺ���
	
	/** ��ȡSoul��������ʵ�� */
	static const USoulDebugSettings* GetSoulDebugSettings();

	/** ���û�������е�����־ */
	static void EnableAllDebugLogs(bool bEnable);

	/** ����ָ��ģ��ĵ�����־ */
	static void EnableModuleDebugLogs(const FString& ModuleName, bool bEnable);

	/** ����Ƿ�Ӧ��Ϊָ��ģ�������־�����ݵ�����־���� */
	static bool ShouldLogForModule(const FString& ModuleName, EDebugLogLevel RequestedLogLevel);

	/** ����ȫ����־���� */
	static void SetGlobalLogLevel(EDebugLogLevel NewRequestedLogLevel);
};

/**
 * ����������ṹ��
 * �����Զ���������ִ��ʱ��
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

// ==================== ���ܵ��Ժ궨�� ====================

/**
 * ���ܵ��Ժ� - ���󼶱�
 * ֻ�е�ģ�����־��������ʱ�������־
 */
#define SOUL_LOG_ERROR(ModuleName, Format, ...) \
    if (USoulDebugSettings::ShouldLogForModule(ModuleName, EDebugLogLevel::Error)) \
    { \
        UE_LOG(LogTemp, Error, TEXT("[%s] ") Format, *ModuleName, ##__VA_ARGS__); \
    }

/**
 * ���ܵ��Ժ� - ���漶��
 * ֻ�е�ģ�����־��������ʱ�������־
 */
#define SOUL_LOG_WARNING(ModuleName, Format, ...) \
    if (USoulDebugSettings::ShouldLogForModule(ModuleName, EDebugLogLevel::Warning)) \
    { \
        UE_LOG(LogTemp, Warning, TEXT("[%s] ") Format, *ModuleName, ##__VA_ARGS__); \
    }

/**
 * ���ܵ��Ժ� - ��Ϣ����
 * ֻ�е�ģ�����־��������ʱ�������־
 */
#define SOUL_LOG_INFO(ModuleName, Format, ...) \
    if (USoulDebugSettings::ShouldLogForModule(ModuleName, EDebugLogLevel::Info)) \
    { \
        UE_LOG(LogTemp, Log, TEXT("[%s] ") Format, *ModuleName, ##__VA_ARGS__); \
    }

/**
 * ���ܵ��Ժ� - ��ϸ����
 * ֻ�е�ģ�����־��������ʱ�������־
 */
#define SOUL_LOG_VERBOSE(ModuleName, Format, ...) \
    if (USoulDebugSettings::ShouldLogForModule(ModuleName, EDebugLogLevel::Verbose)) \
    { \
        UE_LOG(LogTemp, Verbose, TEXT("[%s] ") Format, *ModuleName, ##__VA_ARGS__); \
    }

/**
 * ���ܵ��Ժ� - �ǳ���ϸ����
 * ֻ�е�ģ�����־��������ʱ�������־
 */
#define SOUL_LOG_VERY_VERBOSE(ModuleName, Format, ...) \
    if (USoulDebugSettings::ShouldLogForModule(ModuleName, EDebugLogLevel::VeryVerbose)) \
    { \
        UE_LOG(LogTemp, VeryVerbose, TEXT("[%s] ") Format, *ModuleName, ##__VA_ARGS__); \
    }

// ���ܼ�غ궨��
#define SOUL_PERFORMANCE_SCOPE(FunctionName) \
	FSoulPerformanceScope PerformanceScope(FunctionName)

#define SOUL_PERFORMANCE_SCOPE_CONDITIONAL(FunctionName, Condition) \
	TOptional<FSoulPerformanceScope> PerformanceScope; \
	if (Condition) \
	{ \
		PerformanceScope.Emplace(FunctionName); \
	}