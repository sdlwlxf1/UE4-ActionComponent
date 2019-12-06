// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Action_RootMotionForce.h"

class UCharacterMovementComponent;
class UAnimMontage;
class ACharacter;

NEWPROJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogAction_RootMotionMoveToLocation, Warning, All);

class NEWPROJECT_API FAction_RootMotionMoveToLocation : public FAction_RootMotionForce
{
public:
	FAction_RootMotionMoveToLocation() { Type = EActionType::Animation | EActionType::Move | EActionType::Rotate; }

	static TSharedPtr<FAction_RootMotionMoveToLocation> CreateAction(
		FVector TargetLocation,
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
	float Duration;

	bool bRestrictSpeedToExpected;

	UCurveVector* PathOffsetCurve;
};
