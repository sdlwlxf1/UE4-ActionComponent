// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Action_RootMotionForce.h"

class UCharacterMovementComponent;
class UAnimMontage;
class ACharacter;

NEWPROJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogAction_RootMotionJump, Warning, All);

class NEWPROJECT_API FAction_RootMotionJump : public FAction_RootMotionForce
{
public:
	FAction_RootMotionJump() { Type = EActionType::Move; }

	static TSharedPtr<FAction_RootMotionJump> CreateAction(
		FVector WorldDirection,
		float Strength,
		float Duration = -1.0,
		bool bIsAdditive = false,
		UCurveFloat* StrengthOverTime = nullptr,
		ERootMotionFinishVelocityMode VelocityOnFinishMode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity,
		FVector SetVelocityOnFinish = FVector::ZeroVector,
		float ClampVelocityOnFinish = 0.0f
	);

	virtual EActionResult ExecuteAction() override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;
	virtual EActionResult TickAction(float DeltaTime) override;
protected:

	FVector WorldDirection;
	float Strength;
	float Duration;
	bool bIsAdditive;

	UCurveFloat* StrengthOverTime;
};
