// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Action.h"
#include "Engine/EngineTypes.h"

class UCharacterMovementComponent;

class NEWPROJECT_API FAction_MoveTo : public FAction
{

public:
	FAction_MoveTo() { Type = EActionType::Move; }

protected:

	TWeakObjectPtr<UCharacterMovementComponent> MovementComp;

	EMovementMode StorgeMovementMode;
	float StorgeMoveMaxSpeed;
};
