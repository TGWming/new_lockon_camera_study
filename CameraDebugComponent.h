#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraDebugComponent.generated.h"

/** Debug可视化模式 */
UENUM(BlueprintType)
enum class EDebugVisualizationMode : uint8
{
    None            UMETA(DisplayName = "None"),
    TargetInfo      UMETA(DisplayName = "Target Info"),
    CameraMetrics   UMETA(DisplayName = "Camera Metrics"),
    LockOnTrace     UMETA(DisplayName = "Lock-On Trace"),
    Performance     UMETA(DisplayName = "Performance Stats"),
    All             UMETA(DisplayName = "All")
};

/**
 * 相机调试可视化组件 - 仅用于调试信息显示
 * 不处理任何UMG Widget绘制（UMG由UIManagerComponent通过Socket投射处理）
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SOUL_API UCameraDebugComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCameraDebugComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // ==================== Debug设置 ====================
    /** 启用Debug可视化 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings")
    bool bEnableDebugVisualization;
    
    /** 可视化模式 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings")
    EDebugVisualizationMode VisualizationMode;
    
    /** Debug线条颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings")
    FLinearColor DebugLineColor;
    
    /** Debug绘制持续时间 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float DebugDrawDuration;
    
    /** 显示性能统计 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings")
    bool bShowPerformanceStats;
    
    /** 显示相机参数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Settings")
    bool bShowCameraParameters;
    
    // ==================== 纯Debug功能（不涉及UMG） ====================
    /** 绘制目标边界和中心点 */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DrawTargetBounds(AActor* Target);
    
    /** 绘制锁定追踪线 */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DrawLockOnTrace(const FVector& CameraLocation, const FVector& TargetLocation);
    
    /** 绘制相机参数信息 */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DrawCameraMetrics();
    
    /** 显示性能统计信息 */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DrawPerformanceStats();
    
    /** 清除所有Debug绘制 */
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void ClearAllDebugDrawing();
    
    /** 切换Debug模式 */
    UFUNCTION(BlueprintCallable, Category = "Debug", meta = (CallInEditor = "true"))
    void ToggleDebugMode();
    
    /** 输出当前相机状态到日志 */
    UFUNCTION(BlueprintCallable, Category = "Debug", meta = (CallInEditor = "true"))
    void DumpCameraStateToLog();

private:
    /** 获取相机控制组件 */
    class UCameraControlComponent* GetCameraControlComponent() const;
    
    /** 性能统计 */
    float LastTickTime;
    float AverageTickTime;
    int32 FrameCounter;
    
    /** 绘制3D文本 */
    void Draw3DDebugString(const FVector& Location, const FString& Text, const FColor& Color);
};
