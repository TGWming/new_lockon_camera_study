#include "CameraDebugComponent.h"
#include "CameraControlComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

static FAutoConsoleCommand CmdToggleDebugVisualization(
	TEXT("Camera.Debug.Toggle"),
	TEXT("Toggle camera debug visualization"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		for (TObjectIterator<UCameraDebugComponent> It; It; ++It)
		{
			if (It->GetWorld() && !It->GetWorld()->bIsTearingDown)
			{
				It->ToggleDebugMode();
			}
		}
	})
);

static FAutoConsoleCommand CmdSetDebugMode(
	TEXT("Camera.Debug.SetMode"),
	TEXT("Set debug visualization mode (0=None, 1=TargetInfo, 2=CameraMetrics, 3=LockOnTrace, 4=Performance, 5=All)"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		if (Args.Num() > 0)
		{
			int32 Mode = FCString::Atoi(*Args[0]);
			for (TObjectIterator<UCameraDebugComponent> It; It; ++It)
			{
				if (It->GetWorld() && !It->GetWorld()->bIsTearingDown)
				{
					It->VisualizationMode = (EDebugVisualizationMode)FMath::Clamp(Mode, 0, 5);
					It->bEnableDebugVisualization = (Mode > 0);
					UE_LOG(LogTemp, Warning, TEXT("Debug mode set to: %d"), Mode);
				}
			}
		}
	})
);

static FAutoConsoleCommand CmdClearDebugDraw(
	TEXT("Camera.Debug.Clear"),
	TEXT("Clear all debug drawing"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		for (TObjectIterator<UCameraDebugComponent> It; It; ++It)
		{
			if (It->GetWorld() && !It->GetWorld()->bIsTearingDown)
			{
				It->ClearAllDebugDrawing();
			}
		}
	})
);

UCameraDebugComponent::UCameraDebugComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    
    // 默认设置
    bEnableDebugVisualization = false;
    VisualizationMode = EDebugVisualizationMode::None;
    DebugLineColor = FLinearColor::Yellow;
    DebugDrawDuration = 0.0f;  // 0表示单帧绘制
    bShowPerformanceStats = false;
    bShowCameraParameters = true;
    
    // 性能统计初始化
    LastTickTime = 0.0f;
    AverageTickTime = 0.0f;
    FrameCounter = 0;
}

void UCameraDebugComponent::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Log, TEXT("CameraDebugComponent initialized - Debug visualization for development only"));
}

void UCameraDebugComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // 性能统计
    if (bShowPerformanceStats)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float TickDelta = CurrentTime - LastTickTime;
        LastTickTime = CurrentTime;
        
        // 计算平均Tick时间
        AverageTickTime = (AverageTickTime * FrameCounter + TickDelta) / (FrameCounter + 1);
        FrameCounter++;
        
        if (FrameCounter > 100) // 每100帧重置
        {
            FrameCounter = 0;
            AverageTickTime = TickDelta;
        }
    }
    
    if (!bEnableDebugVisualization || VisualizationMode == EDebugVisualizationMode::None)
        return;
    
    UCameraControlComponent* CameraControl = GetCameraControlComponent();
    if (!CameraControl)
        return;
    
    AActor* CurrentTarget = CameraControl->GetCurrentLockOnTarget();
    
    // 根据模式绘制不同的Debug信息
    switch (VisualizationMode)
    {
    case EDebugVisualizationMode::TargetInfo:
        if (CurrentTarget)
        {
            DrawTargetBounds(CurrentTarget);
        }
        break;
        
    case EDebugVisualizationMode::CameraMetrics:
        DrawCameraMetrics();
        break;
        
    case EDebugVisualizationMode::LockOnTrace:
        if (CurrentTarget && GetOwner())
        {
            DrawLockOnTrace(GetOwner()->GetActorLocation(), CurrentTarget->GetActorLocation());
        }
        break;
        
    case EDebugVisualizationMode::Performance:
        DrawPerformanceStats();
        break;
        
    case EDebugVisualizationMode::All:
        if (CurrentTarget)
        {
            DrawTargetBounds(CurrentTarget);
            if (GetOwner())
            {
                DrawLockOnTrace(GetOwner()->GetActorLocation(), CurrentTarget->GetActorLocation());
            }
        }
        DrawCameraMetrics();
        DrawPerformanceStats();
        break;
    }
}

void UCameraDebugComponent::DrawTargetBounds(AActor* Target)
{
    if (!Target)
        return;
    
    // 获取目标边界
    FVector Origin, BoxExtent;
    Target->GetActorBounds(false, Origin, BoxExtent);
    
    // 绘制边界盒
    DrawDebugBox(GetWorld(), Origin, BoxExtent, DebugLineColor.ToFColor(true), false, DebugDrawDuration);
    
    // 绘制中心点
    DrawDebugSphere(GetWorld(), Target->GetActorLocation(), 20.0f, 12, FColor::Red, false, DebugDrawDuration);
    
    // 绘制目标信息（仅调试文本，不是UMG）
    if (bShowCameraParameters)
    {
        FString InfoText = FString::Printf(TEXT("Target: %s\nSize: %.1fx%.1fx%.1f"), 
            *Target->GetName(), 
            BoxExtent.X * 2, BoxExtent.Y * 2, BoxExtent.Z * 2);
        Draw3DDebugString(Origin + FVector(0, 0, BoxExtent.Z + 50), InfoText, FColor::White);
    }
    
    // 绘制目标的前向方向
    FVector ForwardEnd = Target->GetActorLocation() + Target->GetActorForwardVector() * 100.0f;
    DrawDebugDirectionalArrow(GetWorld(), Target->GetActorLocation(), ForwardEnd, 
        50.0f, FColor::Blue, false, DebugDrawDuration, 0, 2.0f);
}

void UCameraDebugComponent::DrawLockOnTrace(const FVector& CameraLocation, const FVector& TargetLocation)
{
    // 绘制锁定追踪线
    DrawDebugLine(GetWorld(), CameraLocation, TargetLocation, FColor::Green, false, DebugDrawDuration, 0, 2.0f);
    
    // 计算和显示距离
    float Distance = FVector::Dist(CameraLocation, TargetLocation);
    FVector MidPoint = (CameraLocation + TargetLocation) * 0.5f;
    
    FString DistanceText = FString::Printf(TEXT("Distance: %.1f units"), Distance);
    Draw3DDebugString(MidPoint, DistanceText, FColor::Green);
    
    // 绘制中点标记
    DrawDebugSphere(GetWorld(), MidPoint, 10.0f, 8, FColor::Yellow, false, DebugDrawDuration);
}

void UCameraDebugComponent::DrawCameraMetrics()
{
    UCameraControlComponent* CameraControl = GetCameraControlComponent();
    if (!CameraControl || !GetOwner())
        return;
    
    FVector OwnerLocation = GetOwner()->GetActorLocation();
    
    // 收集相机参数
    FString MetricsText = TEXT("=== Camera Metrics ===\n");
    MetricsText += FString::Printf(TEXT("Interp Speed: %.2f\n"), CameraControl->CameraSettings.CameraInterpSpeed);
    MetricsText += FString::Printf(TEXT("Tracking Mode: %d\n"), CameraControl->CameraSettings.CameraTrackingMode);
    MetricsText += FString::Printf(TEXT("Smooth Tracking: %s\n"), 
        CameraControl->CameraSettings.bEnableSmoothCameraTracking ? TEXT("ON") : TEXT("OFF"));
    
    if (CameraControl->FreeLookSettings.bEnableFreeLook)
    {
        MetricsText += FString::Printf(TEXT("FreeLook: ON (H:%.1f° V:%.1f°)\n"), 
            CameraControl->FreeLookSettings.HorizontalLimit,
            CameraControl->FreeLookSettings.VerticalLimit);
    }
    else
    {
        MetricsText += TEXT("FreeLook: OFF\n");
    }
    
    // 在玩家上方显示
    Draw3DDebugString(OwnerLocation + FVector(0, 0, 200), MetricsText, FColor::Cyan);
}

void UCameraDebugComponent::DrawPerformanceStats()
{
    if (!bShowPerformanceStats || !GetOwner())
        return;
    
    FVector OwnerLocation = GetOwner()->GetActorLocation();
    
    // 性能统计文本
    FString PerfText = TEXT("=== Performance ===\n");
    PerfText += FString::Printf(TEXT("FPS: %.1f\n"), 1.0f / GetWorld()->GetDeltaSeconds());
    PerfText += FString::Printf(TEXT("Avg Tick: %.3f ms\n"), AverageTickTime * 1000.0f);
    
    UCameraControlComponent* CameraControl = GetCameraControlComponent();
    if (CameraControl)
    {
        int32 TargetCount = 0;
        // 这里可以添加更多性能相关的统计
        PerfText += FString::Printf(TEXT("Lock-On Active: %s\n"), 
            CameraControl->GetCurrentLockOnTarget() ? TEXT("YES") : TEXT("NO"));
    }
    
    // 在玩家左侧显示
    Draw3DDebugString(OwnerLocation + FVector(-150, 0, 100), PerfText, FColor::Orange);
}

void UCameraDebugComponent::ClearAllDebugDrawing()
{
    FlushDebugStrings(GetWorld());
    FlushPersistentDebugLines(GetWorld());
    
    UE_LOG(LogTemp, Log, TEXT("CameraDebugComponent: Cleared all debug drawing"));
}

void UCameraDebugComponent::ToggleDebugMode()
{
    bEnableDebugVisualization = !bEnableDebugVisualization;
    
    if (!bEnableDebugVisualization)
    {
        ClearAllDebugDrawing();
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Camera Debug Visualization: %s"), 
        bEnableDebugVisualization ? TEXT("Enabled") : TEXT("Disabled"));
}

void UCameraDebugComponent::DumpCameraStateToLog()
{
    UCameraControlComponent* CameraControl = GetCameraControlComponent();
    if (!CameraControl)
    {
        UE_LOG(LogTemp, Error, TEXT("No CameraControlComponent found"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("========== Camera State Dump =========="));
    UE_LOG(LogTemp, Warning, TEXT("Current Target: %s"), 
        CameraControl->GetCurrentLockOnTarget() ? *CameraControl->GetCurrentLockOnTarget()->GetName() : TEXT("None"));
    UE_LOG(LogTemp, Warning, TEXT("Camera Settings:"));
    UE_LOG(LogTemp, Warning, TEXT("  - Interp Speed: %.2f"), CameraControl->CameraSettings.CameraInterpSpeed);
    UE_LOG(LogTemp, Warning, TEXT("  - Tracking Mode: %d"), CameraControl->CameraSettings.CameraTrackingMode);
    UE_LOG(LogTemp, Warning, TEXT("  - Smooth Tracking: %s"), 
        CameraControl->CameraSettings.bEnableSmoothCameraTracking ? TEXT("Enabled") : TEXT("Disabled"));
    UE_LOG(LogTemp, Warning, TEXT("  - FreeLook: %s"), 
        CameraControl->FreeLookSettings.bEnableFreeLook ? TEXT("Enabled") : TEXT("Disabled"));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

UCameraControlComponent* UCameraDebugComponent::GetCameraControlComponent() const
{
    if (AActor* Owner = GetOwner())
    {
        return Owner->FindComponentByClass<UCameraControlComponent>();
    }
    return nullptr;
}

void UCameraDebugComponent::Draw3DDebugString(const FVector& Location, const FString& Text, const FColor& Color)
{
    DrawDebugString(GetWorld(), Location, Text, nullptr, Color, DebugDrawDuration, false);
}
