#include "EnemyCameraConfigComponent.h"
#include "Engine/World.h"

UEnemyCameraConfigComponent::UEnemyCameraConfigComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// ==================== 初始化默认值 ====================
	CameraType = EEnemyCameraType::Medium;
	bUseCustomSettings = false;
	
	// 自定义设置初始化为0（表示使用默认值）
	CustomCameraDistance = 0.0f;
	CustomHeightOffset = 0.0f;
	CustomPitchOffset = 0.0f;
	CustomZBiasNear = 0.0f;
	CustomZBiasFar = 0.0f;
	CustomFOVAdjustment = 0.0f;
	CustomTrackingSpeed = 0.0f;
	CustomSwitchSpeed = 0.0f;
	
	// 调试选项默认关闭
	bEnableDebugInfo = false;
}

void UEnemyCameraConfigComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 验证设置的有效性
	ValidateSettings();
	
	// 如果启用了调试，输出配置信息
	if (bEnableDebugInfo)
	{
		DebugPrintCurrentConfig();
	}
}

// ==================== 配置获取接口 ====================

FCameraTypeConfig UEnemyCameraConfigComponent::GetCameraConfig() const
{
	// 获取基础默认配置
	FCameraTypeConfig Config = GetDefaultConfigForType(CameraType);
	
	// 如果使用自定义设置，则应用覆盖
	if (bUseCustomSettings)
	{
		if (CustomCameraDistance > 0.0f)
			Config.CameraDistance = CustomCameraDistance;
		
		if (CustomHeightOffset != 0.0f)
			Config.HeightOffset = CustomHeightOffset;
		
		if (CustomPitchOffset != 0.0f)
			Config.PitchOffset = CustomPitchOffset;
		
		if (CustomZBiasNear != 0.0f)
			Config.ZBiasNear = CustomZBiasNear;
		
		if (CustomZBiasFar != 0.0f)
			Config.ZBiasFar = CustomZBiasFar;
		
		if (CustomFOVAdjustment != 0.0f)
			Config.FOVAdjustment = CustomFOVAdjustment;
		
		if (CustomTrackingSpeed > 0.0f)
			Config.TrackingSpeed = CustomTrackingSpeed;
		
		if (CustomSwitchSpeed > 0.0f)
			Config.SwitchSpeed = CustomSwitchSpeed;
	}
	
	return Config;
}

void UEnemyCameraConfigComponent::SetCameraType(EEnemyCameraType NewType)
{
	CameraType = NewType;
	
	if (bEnableDebugInfo)
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy Camera Config: Camera type changed to %s"), 
			*UEnum::GetValueAsString(CameraType));
	}
}

float UEnemyCameraConfigComponent::GetEffectiveCameraDistance() const
{
	if (bUseCustomSettings && CustomCameraDistance > 0.0f)
	{
		return CustomCameraDistance;
	}
	
	return GetDefaultConfigForType(CameraType).CameraDistance;
}

float UEnemyCameraConfigComponent::GetEffectiveHeightOffset() const
{
	if (bUseCustomSettings && CustomHeightOffset != 0.0f)
	{
		return CustomHeightOffset;
	}
	
	return GetDefaultConfigForType(CameraType).HeightOffset;
}

float UEnemyCameraConfigComponent::GetEffectivePitchOffset() const
{
	if (bUseCustomSettings && CustomPitchOffset != 0.0f)
	{
		return CustomPitchOffset;
	}
	
	return GetDefaultConfigForType(CameraType).PitchOffset;
}

// ==================== 静态默认配置 ====================

FCameraTypeConfig UEnemyCameraConfigComponent::GetDefaultConfigForType(EEnemyCameraType Type)
{
	FCameraTypeConfig Config;
	
	switch (Type)
	{
	case EEnemyCameraType::Small:
		Config.CameraDistance = 350.0f;
		Config.HeightOffset = 60.0f;
		Config.PitchOffset = -3.0f;
		Config.ZBiasNear = -200.0f;
		Config.ZBiasFar = -350.0f;
		Config.FOVAdjustment = 2.0f;
		Config.TrackingSpeed = 7.0f;
		Config.SwitchSpeed = 10.0f;
		break;
		
	case EEnemyCameraType::Medium:
		Config.CameraDistance = 450.0f;
		Config.HeightOffset = 100.0f;
		Config.PitchOffset = -5.0f;
		Config.ZBiasNear = -120.0f;
		Config.ZBiasFar = -200.0f;
		Config.FOVAdjustment = 0.0f;
		Config.TrackingSpeed = 5.0f;
		Config.SwitchSpeed = 8.0f;
		break;
		
	case EEnemyCameraType::Large:
		Config.CameraDistance = 600.0f;
		Config.HeightOffset = 150.0f;
		Config.PitchOffset = -8.0f;
		Config.ZBiasNear = -80.0f;
		Config.ZBiasFar = -120.0f;
		Config.FOVAdjustment = -3.0f;
		Config.TrackingSpeed = 4.0f;
		Config.SwitchSpeed = 6.0f;
		break;
		
	case EEnemyCameraType::Boss:
		Config.CameraDistance = 800.0f;
		Config.HeightOffset = 250.0f;
		Config.PitchOffset = -12.0f;
		Config.ZBiasNear = -60.0f;
		Config.ZBiasFar = -100.0f;
		Config.FOVAdjustment = -5.0f;
		Config.TrackingSpeed = 3.0f;
		Config.SwitchSpeed = 5.0f;
		break;
		
	case EEnemyCameraType::Unknown:
	default:
		// 默认使用Medium配置
		return GetDefaultConfigForType(EEnemyCameraType::Medium);
	}
	
	return Config;
}

void UEnemyCameraConfigComponent::ResetCustomSettingsToDefault()
{
	CustomCameraDistance = 0.0f;
	CustomHeightOffset = 0.0f;
	CustomPitchOffset = 0.0f;
	CustomZBiasNear = 0.0f;
	CustomZBiasFar = 0.0f;
	CustomFOVAdjustment = 0.0f;
	CustomTrackingSpeed = 0.0f;
	CustomSwitchSpeed = 0.0f;
	
	UE_LOG(LogTemp, Warning, TEXT("Enemy Camera Config: Custom settings reset to default"));
}

// ==================== 向后兼容接口 ====================

FCameraTypeConfig UEnemyCameraConfigComponent::GetEffectiveCameraConfig(const FCameraTypeConfig& DefaultConfig) const
{
	if (!bUseCustomSettings)
	{
		return DefaultConfig; // 使用传入的默认值
	}

	// 使用自定义设置覆盖默认值
	FCameraTypeConfig CustomConfig = DefaultConfig;
	
	if (CustomCameraDistance > 0.0f)
		CustomConfig.CameraDistance = CustomCameraDistance;
	
	if (CustomHeightOffset != 0.0f)
		CustomConfig.HeightOffset = CustomHeightOffset;
	
	if (CustomPitchOffset != 0.0f)
		CustomConfig.PitchOffset = CustomPitchOffset;

	return CustomConfig;
}

// ==================== 调试功能 ====================

void UEnemyCameraConfigComponent::DebugPrintCurrentConfig()
{
	FCameraTypeConfig Config = GetCameraConfig();
	
	UE_LOG(LogTemp, Warning, TEXT("=== ENEMY CAMERA CONFIG DEBUG ==="));
	UE_LOG(LogTemp, Warning, TEXT("Owner: %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
	UE_LOG(LogTemp, Warning, TEXT("Camera Type: %s"), *UEnum::GetValueAsString(CameraType));
	UE_LOG(LogTemp, Warning, TEXT("Use Custom Settings: %s"), bUseCustomSettings ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Warning, TEXT("Final Config:"));
	UE_LOG(LogTemp, Warning, TEXT("  - Distance: %.1f"), Config.CameraDistance);
	UE_LOG(LogTemp, Warning, TEXT("  - Height Offset: %.1f"), Config.HeightOffset);
	UE_LOG(LogTemp, Warning, TEXT("  - Pitch Offset: %.1f"), Config.PitchOffset);
	UE_LOG(LogTemp, Warning, TEXT("  - Z Bias Near: %.1f"), Config.ZBiasNear);
	UE_LOG(LogTemp, Warning, TEXT("  - Z Bias Far: %.1f"), Config.ZBiasFar);
	UE_LOG(LogTemp, Warning, TEXT("  - FOV Adjustment: %.1f"), Config.FOVAdjustment);
	UE_LOG(LogTemp, Warning, TEXT("  - Tracking Speed: %.1f"), Config.TrackingSpeed);
	UE_LOG(LogTemp, Warning, TEXT("  - Switch Speed: %.1f"), Config.SwitchSpeed);
	UE_LOG(LogTemp, Warning, TEXT("====================================="));
}

// ==================== 内部函数 ====================

void UEnemyCameraConfigComponent::InitializeDefaults()
{
	// 在构造函数中已经初始化，这里预留扩展空间
}

void UEnemyCameraConfigComponent::ValidateSettings()
{
	// 验证自定义设置的合理性
	if (bUseCustomSettings)
	{
		// 确保距离值在合理范围内
		if (CustomCameraDistance > 0.0f && (CustomCameraDistance < 200.0f || CustomCameraDistance > 2000.0f))
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy Camera Config: CustomCameraDistance %.1f is out of recommended range (200-2000)"), 
				CustomCameraDistance);
		}
		
		// 确保高度偏移在合理范围内
		if (CustomHeightOffset != 0.0f && (CustomHeightOffset < -500.0f || CustomHeightOffset > 500.0f))
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy Camera Config: CustomHeightOffset %.1f is out of recommended range (-500 to 500)"), 
				CustomHeightOffset);
		}
		
		// 确保俯仰角在合理范围内
		if (CustomPitchOffset != 0.0f && (CustomPitchOffset < -45.0f || CustomPitchOffset > 45.0f))
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy Camera Config: CustomPitchOffset %.1f is out of recommended range (-45 to 45)"), 
				CustomPitchOffset);
		}
	}
}

#if WITH_EDITOR
void UEnemyCameraConfigComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.Property)
	{
		FName PropertyName = PropertyChangedEvent.Property->GetFName();
		
		// 当相机类型改变时，给出提示
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UEnemyCameraConfigComponent, CameraType))
		{
			UE_LOG(LogTemp, Log, TEXT("Enemy Camera Config: Camera type changed to %s"), 
				*UEnum::GetValueAsString(CameraType));
		}
		
		// 当启用自定义设置时，给出提示
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UEnemyCameraConfigComponent, bUseCustomSettings))
		{
			UE_LOG(LogTemp, Log, TEXT("Enemy Camera Config: Custom settings %s"), 
				bUseCustomSettings ? TEXT("ENABLED") : TEXT("DISABLED"));
		}
		
		// 验证设置
		ValidateSettings();
	}
}
#endif