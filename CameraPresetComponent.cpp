#include "CameraPresetComponent.h"
#include "CameraControlComponent.h"
#include "GameFramework/Actor.h"

UCameraPresetComponent::UCameraPresetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	ActivePresetIndex = 0;
	
	// 初始化默认预设
	InitializeDefaultPresets();
}

void UCameraPresetComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 应用初始预设
	if (Presets.IsValidIndex(ActivePresetIndex))
	{
		SwitchToPreset(ActivePresetIndex);
	}
}

void UCameraPresetComponent::InitializeDefaultPresets()
{
	// 标准预设
	FCameraPreset StandardPreset;
	StandardPreset.PresetName = TEXT("Standard");
	StandardPreset.CameraSettings.CameraInterpSpeed = 5.0f;
	StandardPreset.CameraSettings.bEnableSmoothCameraTracking = true;
	StandardPreset.AdvancedSettings.bEnableDistanceAdaptiveCamera = true;
	Presets.Add(StandardPreset);
	
	// 快速预设
	FCameraPreset FastPreset;
	FastPreset.PresetName = TEXT("Fast");
	FastPreset.CameraSettings.CameraInterpSpeed = 10.0f;
	FastPreset.CameraSettings.bEnableSmoothCameraTracking = true;
	FastPreset.AdvancedSettings.bEnableDistanceAdaptiveCamera = false;
	Presets.Add(FastPreset);
	
	// 电影预设
	FCameraPreset CinematicPreset;
	CinematicPreset.PresetName = TEXT("Cinematic");
	CinematicPreset.CameraSettings.CameraInterpSpeed = 2.0f;
	CinematicPreset.CameraSettings.bEnableSmoothCameraTracking = true;
	CinematicPreset.AdvancedSettings.bEnableDistanceAdaptiveCamera = true;
	Presets.Add(CinematicPreset);
}

void UCameraPresetComponent::SwitchToPreset(int32 PresetIndex)
{
	if (!Presets.IsValidIndex(PresetIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraPresetComponent: Invalid preset index %d"), PresetIndex);
		return;
	}
	
	ActivePresetIndex = PresetIndex;
	ApplyPresetToCameraControl(Presets[PresetIndex]);
	
	UE_LOG(LogTemp, Log, TEXT("Switched to camera preset: %s"), *Presets[PresetIndex].PresetName);
}

void UCameraPresetComponent::SwitchToPresetByName(const FString& PresetName)
{
	for (int32 i = 0; i < Presets.Num(); i++)
	{
		if (Presets[i].PresetName == PresetName)
		{
			SwitchToPreset(i);
			return;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("CameraPresetComponent: Preset '%s' not found"), *PresetName);
}

FCameraPreset UCameraPresetComponent::GetCurrentPreset() const
{
	if (Presets.IsValidIndex(ActivePresetIndex))
	{
		return Presets[ActivePresetIndex];
	}
	
	return FCameraPreset();
}

void UCameraPresetComponent::AddPreset(const FCameraPreset& NewPreset)
{
	Presets.Add(NewPreset);
	UE_LOG(LogTemp, Log, TEXT("Added new camera preset: %s"), *NewPreset.PresetName);
}

void UCameraPresetComponent::SaveCurrentAsPreset(const FString& PresetName)
{
	// 获取当前相机控制组件的设置
	AActor* Owner = GetOwner();
	if (!Owner)
		return;
	
	UCameraControlComponent* CameraControl = Owner->FindComponentByClass<UCameraControlComponent>();
	if (!CameraControl)
		return;
	
	FCameraPreset NewPreset;
	NewPreset.PresetName = PresetName;
	NewPreset.CameraSettings = CameraControl->CameraSettings;
	NewPreset.AdvancedSettings = CameraControl->AdvancedCameraSettings;
	
	AddPreset(NewPreset);
}

void UCameraPresetComponent::ApplyPresetToCameraControl(const FCameraPreset& Preset)
{
	AActor* Owner = GetOwner();
	if (!Owner)
		return;
	
	UCameraControlComponent* CameraControl = Owner->FindComponentByClass<UCameraControlComponent>();
	if (!CameraControl)
		return;
	
	CameraControl->SetCameraSettings(Preset.CameraSettings);
	CameraControl->SetAdvancedCameraSettings(Preset.AdvancedSettings);
}
