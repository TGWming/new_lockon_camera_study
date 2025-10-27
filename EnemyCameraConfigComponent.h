#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnConfig.h"
#include "EnemyCameraConfigComponent.generated.h"

/**
 * Enemy Camera Configuration Component
 * FromSoftware式敌人体型相机系统组件
 * 
 * 使用方法：
 * 1. 在敌人蓝图中添加此组件
 * 2. 选择适当的Camera Type（Small/Medium/Large/Boss）
 * 3. 可选：勾选Use Custom Settings进行精细调整
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName="Enemy Camera Config"))
class SOUL_API UEnemyCameraConfigComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyCameraConfigComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ==================== 基础配置 ====================
	
	/** 敌人相机类型分类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Camera Type", meta = (
		DisplayName = "Camera Type",
		ToolTip = "选择敌人的相机行为类型。Small适用于小型快速敌人，Medium适用于标准敌人，Large适用于大型敌人，Boss适用于BOSS级敌人。"
	))
	EEnemyCameraType CameraType = EEnemyCameraType::Medium;

	/** 是否使用自定义设置覆盖默认值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Camera Type", meta = (
		DisplayName = "Use Custom Settings",
		ToolTip = "勾选此项可以覆盖默认的相机设置，进行精细调整。"
	))
	bool bUseCustomSettings = false;

	// ==================== 自定义相机配置（仅在Use Custom Settings为true时有效） ====================
	
	/** 自定义相机距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Camera Settings", meta = (
		DisplayName = "Custom Camera Distance",
		ToolTip = "自定义相机到目标的距离。仅在Use Custom Settings为true时生效。",
		ClampMin = "200.0", ClampMax = "2000.0",
		EditCondition = "bUseCustomSettings"
	))
	float CustomCameraDistance = 0.0f;

	/** 自定义高度偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Camera Settings", meta = (
		DisplayName = "Custom Height Offset", 
		ToolTip = "自定义相机的高度偏移。正值向上，负值向下。仅在Use Custom Settings为true时生效。",
		ClampMin = "-500.0", ClampMax = "500.0",
		EditCondition = "bUseCustomSettings"
	))
	float CustomHeightOffset = 0.0f;

	/** 自定义俯仰角度偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Camera Settings", meta = (
		DisplayName = "Custom Pitch Offset",
		ToolTip = "自定义相机的俯仰角度偏移。正值向上倾斜，负值向下倾斜。仅在Use Custom Settings为true时生效。",
		ClampMin = "-45.0", ClampMax = "45.0",
		EditCondition = "bUseCustomSettings"
	))
	float CustomPitchOffset = 0.0f;

	/** 近距离Z轴偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Camera Settings", meta = (
		DisplayName = "Near Distance Z Bias",
		ToolTip = "近距离时的Z轴偏移量。用于调整相机在近距离时的垂直位置。",
		EditCondition = "bUseCustomSettings"
	))
	float CustomZBiasNear = 0.0f;

	/** 远距离Z轴偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Camera Settings", meta = (
		DisplayName = "Far Distance Z Bias",
		ToolTip = "远距离时的Z轴偏移量。用于调整相机在远距离时的垂直位置。",
		EditCondition = "bUseCustomSettings"
	))
	float CustomZBiasFar = 0.0f;

	/** 自定义视野调整 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Camera Settings", meta = (
		DisplayName = "Custom FOV Adjustment",
		ToolTip = "自定义视野角度调整。正值增加视野，负值减少视野。仅在Use Custom Settings为true时生效。",
		ClampMin = "-20.0", ClampMax = "20.0",
		EditCondition = "bUseCustomSettings"
	))
	float CustomFOVAdjustment = 0.0f;

	/** 自定义跟踪速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Camera Settings", meta = (
		DisplayName = "Custom Tracking Speed",
		ToolTip = "自定义相机跟踪目标的速度。数值越高跟踪越快。仅在Use Custom Settings为true时生效。",
		ClampMin = "1.0", ClampMax = "20.0",
		EditCondition = "bUseCustomSettings"
	))
	float CustomTrackingSpeed = 0.0f;

	/** 自定义切换速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Camera Settings", meta = (
		DisplayName = "Custom Switch Speed",
		ToolTip = "自定义目标切换时的相机移动速度。数值越高切换越快。仅在Use Custom Settings为true时生效。",
		ClampMin = "1.0", ClampMax = "20.0",
		EditCondition = "bUseCustomSettings"
	))
	float CustomSwitchSpeed = 0.0f;

	// ==================== 调试信息 ====================
	
	/** 启用调试信息显示 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (
		DisplayName = "Enable Debug Info",
		ToolTip = "启用此项将在游戏中显示该敌人的相机配置调试信息。"
	))
	bool bEnableDebugInfo = false;

	// ==================== 公共接口函数 ====================
	
	/** 获取当前的相机配置 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config")
	FCameraTypeConfig GetCameraConfig() const;

	/** 获取敌人的相机类型 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config")
	EEnemyCameraType GetCameraType() const { return CameraType; }

	/** 设置敌人的相机类型 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config")
	void SetCameraType(EEnemyCameraType NewType);

	/** 是否使用自定义设置 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config")
	bool IsUsingCustomSettings() const { return bUseCustomSettings; }

	/** 获取有效的相机距离（考虑自定义设置） */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config")
	float GetEffectiveCameraDistance() const;

	/** 获取有效的高度偏移（考虑自定义设置） */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config")
	float GetEffectiveHeightOffset() const;

	/** 获取有效的俯仰偏移（考虑自定义设置） */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config")
	float GetEffectivePitchOffset() const;

	// ==================== 静态默认配置 ====================
	
	/** 获取指定类型的默认相机配置 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config", meta = (CallInEditor = "true"))
	static FCameraTypeConfig GetDefaultConfigForType(EEnemyCameraType Type);

	/** 重置自定义设置为默认值 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config", meta = (CallInEditor = "true"))
	void ResetCustomSettingsToDefault();

	// ==================== 向后兼容接口 ====================
	
	/** 获取有效相机配置（向后兼容） */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config")
	FCameraTypeConfig GetEffectiveCameraConfig(const FCameraTypeConfig& DefaultConfig) const;

	/** 获取有效相机类型（向后兼容） */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config")
	EEnemyCameraType GetEffectiveCameraType() const { return CameraType; }

	// ==================== 调试功能 ====================
	
	/** 输出当前配置的调试信息 */
	UFUNCTION(BlueprintCallable, Category = "Enemy Camera Config", meta = (CallInEditor = "true"))
	void DebugPrintCurrentConfig();

private:
	// ==================== 内部函数 ====================
	
	/** 初始化默认配置 */
	void InitializeDefaults();

	/** 验证配置数值的有效性 */
	void ValidateSettings();

#if WITH_EDITOR
	// ==================== 编辑器支持 ====================
	
	/** 编辑器中属性变更时的回调 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};