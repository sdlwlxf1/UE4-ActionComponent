// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Action_InterpScaleTo.h"

class UCurveFloat;
class UCurveVector;
class ACharacter;
class FBehaviorLock;

class NEWPROJECT_API FAction_ServerMoveTo : public FAction_MoveTo
{

public:
	static TSharedPtr<FAction_ServerMoveTo> CreateAction(const FVector& InDestLocation, float Speed = -1.0f, float InAcceptanceRadius = 1.0f, bool bInbWithOutControl = false);
	static TSharedPtr<FAction_ServerMoveTo> CreateAction(const AActor* InGoal, float Speed = -1.0f, float InAcceptanceRadius = 1.0f, bool InbWithOutControl = false);

	virtual EActionResult ExecuteAction() override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;
	virtual EActionResult TickAction(float DeltaTime) override;

	uint32 bOrientRotationToMovement : 1;

	virtual FName GetName() const override;
	virtual FString GetDescription() const override;

protected:
	void PhysicsRotation(float DeltaTime, const FVector& Velocity);
	float GetAxisDeltaRotation(float InAxisRotationRate, float DeltaTime);

	bool HasReached(float GoalRadius, float GoalHalfHeight, const FVector& CurLocation, FVector& NewLocation, float DeltaTime);
	FRotator RotationRate = FRotator(2400.0f, 2400.0f, 2400.0f);

private:
	TWeakObjectPtr<ACharacter> Character;
	TWeakObjectPtr<AActor> Goal;
	FVector DestLocation;
	float Speed;
	bool bWithOutControl;
	float AcceptanceRadius;
	bool LastHasReachedXY = false;
	bool LastHasReachedZ = false;
};
