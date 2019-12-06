// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "Action_MoveTo.h"

class UCurveFloat;
class UCurveBase;
class UCurveVector;
class ACharacter;

class FAction_InterpMoveTo : public FAction_MoveTo
{
public:
	static TSharedPtr<FAction_InterpMoveTo> CreateAction(const FVector& InDestLocation, float InDuration = 0.001f, UCurveBase* InCurve = nullptr, bool InbWithControl = false);
	static TSharedPtr<FAction_InterpMoveTo> CreateAction(const AActor* InGoal, float InDuration = 0.001f, UCurveBase* InCurve = nullptr, bool InbWithOutControl = false);

	virtual EActionResult ExecuteAction() override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;
	virtual EActionResult TickAction(float DeltaTime) override;

	uint32 bOrientRotationToMovement : 1;

	virtual FName GetName() const override;
	virtual FString GetDescription() const override;
protected:
	void PhysicsRotation(float DeltaTime, const FVector& Velocity);
	float GetAxisDeltaRotation(float InAxisRotationRate, float DeltaTime);

	FRotator RotationRate = FRotator(2400.0f, 2400.0f, 2400.0f);

private:
	TWeakObjectPtr<ACharacter> Character;
	TWeakObjectPtr<AActor> Goal;

	FVector StartLocation;
	FVector TargetLocation;

	FVector DestLocation;
	float Duration;
	bool bKeepVelocity;
	float DurationOfMovement;
	float TimeMoveStarted;
	float TimeMoveWillEnd;
	bool bWithOutControl;
	FVector GoalMoveLocation;
	FVector GoalStartLocation;

	TWeakObjectPtr<UCurveFloat> LerpCurve;
	TWeakObjectPtr<UCurveVector> LerpCurveVector;
};

