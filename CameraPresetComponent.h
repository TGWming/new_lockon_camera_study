#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnConfig.h"
#include "CameraPresetComponent.generated.h"

/** 相机预设数据 */
USTRUCT(BlueprintType)
struct FCameraPreset
{
	GENERATED_BODY()
	
	/** 预设名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FString PresetName;
	
	/** 相机设置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FCameraSettings CameraSettings;
	
	/** 高级相机设置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FAdvancedCameraSettings AdvancedSettings;
	
	FCameraPreset()
	{
		PresetName = TEXT("Default");
	}
};

/**
 * 相机预设组件
 * 管理和应用不同的相机配置预设
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SOUL_API UCameraPresetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraPresetComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ==================== 预设管理 ====================
	/** 当前激活的预设索引 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Presets", meta = (ClampMin = "0"))
	int32 ActivePresetIndex;
	
	/** 预设列表 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Presets")
	TArray<FCameraPreset> Presets;
	
	/** 切换到指定预设 */
	UFUNCTION(BlueprintCallable, Category = "Camera Presets")
	void SwitchToPreset(int32 PresetIndex);
	
	/** 按名称切换预设 */
	UFUNCTION(BlueprintCallable, Category = "Camera Presets")
	void SwitchToPresetByName(const FString& PresetName);
	
	/** 获取当前预设 */
	UFUNCTION(BlueprintPure, Category = "Camera Presets")
	FCameraPreset GetCurrentPreset() const;
	
	/** 添加新预设 */
	UFUNCTION(BlueprintCallable, Category = "Camera Presets")
	void AddPreset(const FCameraPreset& NewPreset);
	
	/** 保存当前设置为预设 */
	UFUNCTION(BlueprintCallable, Category = "Camera Presets")
	void SaveCurrentAsPreset(const FString& PresetName);

private:
	/** 初始化默认预设 */
	void InitializeDefaultPresets();
	
	/** 应用预设到相机控制组件 */
	void ApplyPresetToCameraControl(const FCameraPreset& Preset);
};
