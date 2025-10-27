#include "DebugManager.h"
#include "Engine/Engine.h"

// ǰ������
class UPerformanceProfiler;

// ���캯��ʵ��
USoulDebugSettings::USoulDebugSettings()
{
	// ��ʼ���������õ�Ĭ��ֵ
	DebugSettings.bEnableDebugLogs = false;
	DebugSettings.bEnableLockOnDebugLogs = false;
	DebugSettings.bEnableCameraDebugLogs = false;
	DebugSettings.bEnableTargetDetectionDebugLogs = false;
	DebugSettings.bEnableUIDebugLogs = false;
	DebugSettings.bEnableSocketProjectionDebugLogs = false;
	DebugSettings.bEnableAdvancedCameraDebugLogs = false;
	DebugSettings.bEnablePerformanceMonitoring = false;
	DebugSettings.GlobalLogLevel = EDebugLogLevel::Warning;
}

// ��̬���ʺ���ʵ��

const USoulDebugSettings* USoulDebugSettings::GetSoulDebugSettings()
{
	// ��ȡĬ�϶���ʵ����ʹ��UE4/5�л�ȡ���õı�׼��ʽ��
	const USoulDebugSettings* Settings = GetDefault<USoulDebugSettings>();
	
	if (!Settings)
	{
		UE_LOG(LogTemp, Error, TEXT("USoulDebugSettings::GetSoulDebugSettings: Failed to get default settings object!"));
		// ����һ����ʱ��Ĭ��ʵ��
		static USoulDebugSettings FallbackSettings;
		return &FallbackSettings;
	}
	
	return Settings;
}

void USoulDebugSettings::EnableAllDebugLogs(bool bEnable)
{
	// ��ȡ���޸ĵ�����ʵ��
	USoulDebugSettings* MutableSettings = GetMutableDefault<USoulDebugSettings>();
	if (!MutableSettings)
	{
		UE_LOG(LogTemp, Error, TEXT("USoulDebugSettings::EnableAllDebugLogs: Failed to get mutable settings!"));
		return;
	}

	// ���û�������еĵ�����־ѡ��
	MutableSettings->DebugSettings.bEnableDebugLogs = bEnable;
	MutableSettings->DebugSettings.bEnableLockOnDebugLogs = bEnable;
	MutableSettings->DebugSettings.bEnableCameraDebugLogs = bEnable;
	MutableSettings->DebugSettings.bEnableTargetDetectionDebugLogs = bEnable;
	MutableSettings->DebugSettings.bEnableUIDebugLogs = bEnable;
	MutableSettings->DebugSettings.bEnableSocketProjectionDebugLogs = bEnable;
	MutableSettings->DebugSettings.bEnableAdvancedCameraDebugLogs = bEnable;
	MutableSettings->DebugSettings.bEnablePerformanceMonitoring = bEnable;

	UE_LOG(LogTemp, Warning, TEXT("USoulDebugSettings::EnableAllDebugLogs: All debug logs %s"), 
		bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void USoulDebugSettings::EnableModuleDebugLogs(const FString& ModuleName, bool bEnable)
{
	// ��ȡ���޸ĵ�����ʵ��
	USoulDebugSettings* MutableSettings = GetMutableDefault<USoulDebugSettings>();
	if (!MutableSettings)
	{
		UE_LOG(LogTemp, Error, TEXT("USoulDebugSettings::EnableModuleDebugLogs: Failed to get mutable settings!"));
		return;
	}

	// ����ģ�����������ض��ĵ�����־
	if (ModuleName == TEXT("LockOn"))
	{
		MutableSettings->DebugSettings.bEnableLockOnDebugLogs = bEnable;
	}
	else if (ModuleName == TEXT("Camera"))
	{
		MutableSettings->DebugSettings.bEnableCameraDebugLogs = bEnable;
	}
	else if (ModuleName == TEXT("TargetDetection"))
	{
		MutableSettings->DebugSettings.bEnableTargetDetectionDebugLogs = bEnable;
	}
	else if (ModuleName == TEXT("UI"))
	{
		MutableSettings->DebugSettings.bEnableUIDebugLogs = bEnable;
	}
	else if (ModuleName == TEXT("SocketProjection"))
	{
		MutableSettings->DebugSettings.bEnableSocketProjectionDebugLogs = bEnable;
	}
	else if (ModuleName == TEXT("AdvancedCamera"))
	{
		MutableSettings->DebugSettings.bEnableAdvancedCameraDebugLogs = bEnable;
	}
	else if (ModuleName == TEXT("Performance"))
	{
		MutableSettings->DebugSettings.bEnablePerformanceMonitoring = bEnable;
	}
	else
	{
		// ����δ֪ģ�飬Ӱ��ȫ�ֵĵ�����־
		MutableSettings->DebugSettings.bEnableDebugLogs = bEnable;
		UE_LOG(LogTemp, Warning, TEXT("USoulDebugSettings::EnableModuleDebugLogs: Unknown module '%s', affecting global debug logs"), *ModuleName);
	}

	UE_LOG(LogTemp, Warning, TEXT("USoulDebugSettings::EnableModuleDebugLogs: Module '%s' debug logs %s"), 
		*ModuleName, bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void USoulDebugSettings::SetGlobalLogLevel(EDebugLogLevel NewRequestedLogLevel)
{
	// ��ȡ���޸ĵ�����ʵ��
	USoulDebugSettings* MutableSettings = GetMutableDefault<USoulDebugSettings>();
	if (!MutableSettings)
	{
		UE_LOG(LogTemp, Error, TEXT("USoulDebugSettings::SetGlobalLogLevel: Failed to get mutable settings!"));
		return;
	}

	// �����µ�ȫ����־����
	EDebugLogLevel OldLogLevel = MutableSettings->DebugSettings.GlobalLogLevel;
	MutableSettings->DebugSettings.GlobalLogLevel = NewRequestedLogLevel;

	// �����־�����Ϣ
	FString OldLevelName = UEnum::GetValueAsString(OldLogLevel);
	FString NewLevelName = UEnum::GetValueAsString(NewRequestedLogLevel);
	UE_LOG(LogTemp, Warning, TEXT("USoulDebugSettings::SetGlobalLogLevel: Changed from %s to %s"), 
		*OldLevelName, *NewLevelName);
}

// ע�⣺FSoulPerformanceScope��ʵ�����ƶ���PerformanceProfiler.cpp��