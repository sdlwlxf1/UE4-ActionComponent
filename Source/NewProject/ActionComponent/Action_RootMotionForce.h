// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "Action.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/RootMotionSource.h"

class UCharacterMovementComponent;
class UAnimMontage;
class ACharacter;

NEWPROJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogAction_RootMotionForce, Warning, All);

class NEWPROJECT_API FAction_RootMotionForce : public FAction
{
public:
	FAction_RootMotionForce() { Type = (EActionType::Animation | EActionType::Move | EActionType::Rotate); }

	virtual bool HasTimedOut() const;
	virtual float GetTimeRadio() const override;

	bool bSetNewMovementMode = true;
	TEnumAsByte<EMovementMode> NewMovementMode = EMovementMode::MOVE_Flying;

protected:
	ERootMotionFinishVelocityMode FinishVelocityMode;
	FVector FinishSetVelocity;
	float FinishClampVelocity;
	uint16 RootMotionSourceID;
};
