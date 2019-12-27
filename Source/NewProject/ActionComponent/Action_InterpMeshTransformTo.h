// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Action.h"
#include "Engine/EngineTypes.h"

class UCharacterMovementComponent;

class NEWPROJECT_API FAction_InterpMeshTransformTo : public FAction
{

public:
	FAction_InterpMeshTransformTo() { Type = EActionType::MeshMove | EActionType::MeshRotate | EActionType::MeshScale; }

	static TSharedPtr<FAction_InterpMeshTransformTo> CreateAction(const FTransform& InTransform, float InDuration);

protected:

	virtual EActionResult ExecuteAction() override;
	virtual EActionResult TickAction(float DeltaTime) override;

	FTransform TargetTransform;

	float Duration;
	FTransform OriginalMeshTransformOffset;
	FTransform MeshTransformOffset;
	FTransform MeshBaseTransform;
	float CurrentTimeStamp;
	float FinishTimeStamp;
};
