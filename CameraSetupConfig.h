// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LockOnConfig.h"
#include "CameraSetupConfig.generated.h"

// ==================== 相机配置结构体 ====================
/** 相机初始配置结构 - 支持蓝图编辑器调整 */
USTRUCT(BlueprintType)
struct SOUL_API FCameraSetupConfig
{
	GENERATED_BODY()
	
	/** 相机臂长度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
	float ArmLength;
	
	/** 相机初始旋转角度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FRotator InitialRotation;
	
	/** 相机插座偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FVector SocketOffset;
	
	/** 是否使用Pawn控制旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	bool bUsePawnControlRotation;
	
	/** 相机延迟速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float CameraLagSpeed;
	
	/** 是否启用相机延迟 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	bool bEnableCameraLag;
	
	/** 锁定时的相机臂长度调整系数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float LockOnArmLengthMultiplier;
	
	/** 锁定时的高度偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	FVector LockOnHeightOffset;
	
	FCameraSetupConfig()
	{
		ArmLength = 450.0f;
		InitialRotation = FRotator(-15.0f, 0.0f, 0.0f);
		SocketOffset = FVector(0.0f, 0.0f, 50.0f);
		bUsePawnControlRotation = true;
		CameraLagSpeed = 10.0f;
		bEnableCameraLag = false;
		LockOnArmLengthMultiplier = 1.0f;
		LockOnHeightOffset = FVector(0.0f, 0.0f, 30.0f);
	}
};
