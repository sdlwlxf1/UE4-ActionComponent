// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Action_RootMotionForce.h"

class UCharacterMovementComponent;
class UAnimMontage;
class ACharacter;

enum class ERootMotionMoveToActorTargetOffsetType : uint8 
{
	// Align target offset vector from target to source, ignoring height difference
	AlignFromTargetToSource = 0,
	// Align from target actor location to target actor forward
	AlignToTargetForward,
	// Align in world space
	AlignToWorldSpace
};

NEWPROJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogAction_RootMotionMoveToActor, Warning, All);

class NEWPROJECT_API FAction_RootMotionMoveToActor : public FAction_RootMotionForce
{
public:
	FAction_RootMotionMoveToActor() { Type = EActionType::Animation | EActionType::Move | EActionType::Rotate; }

	static TSharedPtr<FAction_RootMotionMoveToActor> CreateAction(
		AActor* InTargetActor,
		float Duration = FLT_MAX,
		bool bSetNewMovementMode = false,
		EMovementMode MovementMode = MOVE_Flying,
		bool bRestrictSpeedToExpected = false,
		UCurveVector* PathOffsetCurve = nullptr,
		ERootMotionFinishVelocityMode VelocityOnFinishMode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity,
		FVector SetVelocityOnFinish = FVector::ZeroVector,
		float ClampVelocityOnFinish = 0.0f,
		float InPlayRate = 1.f,
		FName InStartSectionName = NAME_None);

	virtual EActionResult ExecuteAction() override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;
	virtual EActionResult TickAction(float DeltaTime) override;
protected:

	FVector StartLocation;
	FVector TargetLocation;
	AActor* TargetActor;
	FVector TargetLocationOffset;
	ERootMotionMoveToActorTargetOffsetType OffsetAlignment;

	float Duration;

	bool bRestrictSpeedToExpected;

	UCurveVector* PathOffsetCurve;

	UCurveFloat* TimeMappingCurve;
	UCurveFloat* TargetLerpSpeedHorizonCurve;
	UCurveFloat* TargetLerpSpeedVerticalCurve;
};
