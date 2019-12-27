// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "Action_PlayRootMotion.h"
#include "GameFramework/RootMotionSource.h"

class UCharacterMovementComponent;

NEWPROJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogAction_AnimRootMotionMoveToLocation, Warning, All);

DECLARE_DELEGATE_RetVal_OneParam(FVector, FOnChangeAnimRootMotionLocation, const FVector&)

class NEWPROJECT_API FAction_AnimRootMotionMoveToLocation : public FAction_PlayRootMotion
{
public:
	FAction_AnimRootMotionMoveToLocation() { Type = EActionType::Animation | EActionType::Move | EActionType::Rotate; }

	static TSharedPtr<FAction_AnimRootMotionMoveToLocation> CreateAction(UAnimMontage* InAnimMontage, FTransform TargetTransform = FTransform(FRotator(FLT_MAX, FLT_MAX, FLT_MAX), FVector(FLT_MAX, FLT_MAX, FLT_MAX)), float Duration = -1.0f, float InMoveDurationRatio = 0.05f, bool bSetNewMovementMode = true, EMovementMode MovementMode = MOVE_Flying, UCurveVector* PathOffsetCurve = nullptr, ERootMotionFinishVelocityMode VelocityOnFinishMode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity, FVector SetVelocityOnFinish = FVector::ZeroVector, float ClampVelocityOnFinish = 0.0f, float InPlayRate = -1.0f);

	virtual EActionResult ExecuteAction() override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;
	virtual EActionResult TickAction(float DeltaTime) override;

	FOnChangeAnimRootMotionLocation OnChangeAnimRootMotionLocation;

	virtual FName GetName() const override;
	virtual FString GetDescription() const override;
protected:
	FTransform StartTransform;
	FTransform TargetTransform;
	ERootMotionFinishVelocityMode FinishVelocityMode;
	FVector FinishSetVelocity;
	float FinishClampVelocity;
	bool bRootMotionFinished = false;
	float DurationOfMovement;
	float TimeMoveStarted;
	float TimeMoveWillEnd;
	float TimeMoveRatio;

	FVector LastParentLocation = FVector::ZeroVector;
	FTransform DistanceTransform;

	float CurrentTime;
	FTransform ProcessRootMotionPreConvertToWorld(const FTransform& InTransform, UCharacterMovementComponent* InMovementComp);
	FTransform ProcessRootMotionPostConvertToWorld(const FTransform& InTransform, UCharacterMovementComponent* InMovementComp);

	TWeakObjectPtr<UCharacterMovementComponent> MovementCompPtr = nullptr;

	FVector AnimRootMotionTranslationScale = FVector(1.0f, 1.0f, 1.0f);

	FVector LastVelocity;
};
