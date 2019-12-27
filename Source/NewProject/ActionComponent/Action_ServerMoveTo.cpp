// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_ServerMoveTo.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

DECLARE_CYCLE_STAT(TEXT("MoveTo"), STAT_ServerMoveTo, STATGROUP_AI);

TSharedPtr<FAction_ServerMoveTo> FAction_ServerMoveTo::CreateAction(const FVector& InDestLocation, float Speed /*= -1.0f*/, float InAcceptanceRadius /*= 1.0f*/, bool bInbWithOutControl /*= false*/)
{
	TSharedPtr<FAction_ServerMoveTo> Action = MakeShareable(new FAction_ServerMoveTo());
	if (Action.IsValid())
	{
		Action->DestLocation = InDestLocation;
		Action->AcceptanceRadius = InAcceptanceRadius;
		Action->bWithOutControl = bInbWithOutControl;
		Action->Speed = Speed;
		Action->bOrientRotationToMovement = true;
	}
	return Action;
}

TSharedPtr<FAction_ServerMoveTo> FAction_ServerMoveTo::CreateAction(const AActor* InGoal, float Speed /*= -1.0f*/, float InAcceptanceRadius /*= 1.0f*/, bool InbWithOutControl /*= false*/)
{
	TSharedPtr<FAction_ServerMoveTo> Action = MakeShareable(new FAction_ServerMoveTo());
	if (Action.IsValid())
	{
		Action->Goal = InGoal;
		Action->AcceptanceRadius = InAcceptanceRadius;
		Action->bWithOutControl = InbWithOutControl;
		Action->Speed = Speed;
		Action->bOrientRotationToMovement = true;
	}
	return Action;
}

EActionResult FAction_ServerMoveTo::ExecuteAction()
{
	Character = Cast<ACharacter>(GetOwner());
	MovementComp = Character.IsValid() ? Character->GetCharacterMovement() : nullptr;
	if (!MovementComp.IsValid())
	{
		return EActionResult::Fail;
	}

	float GoalRadius = 0.0f;
	float GoalHalfHeight = 0.0f;
	if (Goal.IsValid())
	{
		Goal.Get()->GetSimpleCollisionCylinder(GoalRadius, GoalHalfHeight);
		DestLocation = Goal->GetActorLocation();
	}
	FVector NewLocation;
	if (HasReached(GoalRadius, GoalHalfHeight, MovementComp->GetActorFeetLocation(), NewLocation, 0))
	{
		return EActionResult::Success;
	}
	MovementComp->StopMovementImmediately();
	if (Speed < 0)
	{
		Speed = MovementComp->GetMaxSpeed();
	}
	StorgeMovementMode = MovementComp->MovementMode;
	MovementComp->SetMovementMode(MOVE_Custom, 0);
	return EActionResult::Wait;
}

bool FAction_ServerMoveTo::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	if (MovementComp.IsValid())
	{
		MovementComp->SetMovementMode(StorgeMovementMode);
	}
	return true;
}

EActionResult FAction_ServerMoveTo::TickAction(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_ServerMoveTo);
	if (Character.IsValid())
	{
		float GoalRadius = 0.0f;
		float GoalHalfHeight = 0.0f;
		if (Goal.IsValid())
		{
			Goal.Get()->GetSimpleCollisionCylinder(GoalRadius, GoalHalfHeight);
			DestLocation = Goal->GetActorLocation();
		}

		FVector CurLocation = MovementComp->GetActorFeetLocation();
		FVector NewLocation;
		bool Reached = HasReached(GoalRadius, GoalHalfHeight, CurLocation, NewLocation, DeltaTime);
		NewLocation += FVector(0, 0, Character->GetRootComponent()->Bounds.BoxExtent.Z);
		Character->SetActorLocation(NewLocation);

		if (StorgeMovementMode == EMovementMode::MOVE_Walking || StorgeMovementMode == EMovementMode::MOVE_NavWalking)
		{
			float StorgeStepHeight = MovementComp->MaxStepHeight;
			MovementComp->MaxStepHeight = MovementComp->GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2;
			MovementComp->FindFloor(NewLocation, MovementComp->CurrentFloor, false);
			MovementComp->AdjustFloorHeight();
			MovementComp->SetBaseFromFloor(MovementComp->CurrentFloor);
			MovementComp->MaxStepHeight = StorgeStepHeight;
		}
		FVector Velocity = (NewLocation - CurLocation) / DeltaTime;
		if (Reached)
		{
			return EActionResult::Success;
		}
		else if (bOrientRotationToMovement)
		{
			PhysicsRotation(DeltaTime, Velocity);
		}
		return EActionResult::Wait;
	}
	else
	{
		return EActionResult::Fail;
	}
}

FName FAction_ServerMoveTo::GetName() const
{
	return TEXT("Action_ServerMoveTo");
}

FString FAction_ServerMoveTo::GetDescription() const
{
	return FString::Printf(TEXT("%s (Goal:(%s))"), *GetName().ToString(), *(Goal.IsValid() ? Goal->GetName() : DestLocation.ToString()));
}

void FAction_ServerMoveTo::PhysicsRotation(float DeltaTime, const FVector& Velocity)
{
	if (!bOrientRotationToMovement)
		return;

	AActor* Actor = GetOwner();
	if (MovementComp.IsValid())
	{
		Actor = MovementComp->GetOwner();
	}
	if (!Actor)
		return;

	FRotator CurrentRotation = Actor->GetActorRotation();
	CurrentRotation.DiagnosticCheckNaN(TEXT("MoveAlongSplineComponent::PhysicsRotation(): CurrentRotation"));

	FRotator DeltaRot = FRotator(GetAxisDeltaRotation(RotationRate.Pitch, DeltaTime), GetAxisDeltaRotation(RotationRate.Yaw, DeltaTime), GetAxisDeltaRotation(RotationRate.Roll, DeltaTime));
	DeltaRot.DiagnosticCheckNaN(TEXT("MoveAlongSplineComponent::PhysicsRotation(): GetDeltaRotation"));

	FRotator DesiredRotation = CurrentRotation;
	DesiredRotation = Velocity.GetSafeNormal().Rotation();

	if (StorgeMovementMode == EMovementMode::MOVE_Walking || StorgeMovementMode == EMovementMode::MOVE_NavWalking || StorgeMovementMode == EMovementMode::MOVE_Falling || StorgeMovementMode == EMovementMode::MOVE_Flying)
	{
		DesiredRotation.Pitch = 0.f;
		DesiredRotation.Yaw = FRotator::NormalizeAxis(DesiredRotation.Yaw);
		DesiredRotation.Roll = 0.f;
	}
	else
	{
		DesiredRotation.Normalize();
	}

	const float AngleTolerance = 1e-3f;

	if (!CurrentRotation.Equals(DesiredRotation, AngleTolerance))
	{
		if (!FMath::IsNearlyEqual(CurrentRotation.Pitch, DesiredRotation.Pitch, AngleTolerance))
		{
			DesiredRotation.Pitch = FMath::FixedTurn(CurrentRotation.Pitch, DesiredRotation.Pitch, DeltaRot.Pitch);
		}

		if (!FMath::IsNearlyEqual(CurrentRotation.Yaw, DesiredRotation.Yaw, AngleTolerance))
		{
			DesiredRotation.Yaw = FMath::FixedTurn(CurrentRotation.Yaw, DesiredRotation.Yaw, DeltaRot.Yaw);
		}

		if (!FMath::IsNearlyEqual(CurrentRotation.Roll, DesiredRotation.Roll, AngleTolerance))
		{
			DesiredRotation.Roll = FMath::FixedTurn(CurrentRotation.Roll, DesiredRotation.Roll, DeltaRot.Roll);
		}

		DesiredRotation.DiagnosticCheckNaN(TEXT("PhysicsRotation(): DesiredRotation"));
		Actor->SetActorRotation(DesiredRotation);
	}
}

float FAction_ServerMoveTo::GetAxisDeltaRotation(float InAxisRotationRate, float DeltaTime)
{
	return (InAxisRotationRate >= 0.f) ? (InAxisRotationRate * DeltaTime) : 360.f;
}

bool FAction_ServerMoveTo::HasReached(float GoalRadius, float GoalHalfHeight, const FVector& CurLocation, FVector& NewLocation, float DeltaTime)
{
	bool HasReachedXY = false;
	bool HasReachedZ = false;
	FVector NewSpdDir = (DestLocation - CurLocation).GetSafeNormal();
	NewLocation = CurLocation + NewSpdDir * Speed * DeltaTime;
	FVector ToGoal = DestLocation - NewLocation;

	float AgentRadius = 0.0f;
	float AgentHalfHeight = 0.0f;
	AActor* MovingAgent = MovementComp->GetOwner();
	MovingAgent->GetSimpleCollisionCylinder(AgentRadius, AgentHalfHeight);

	const float Dist2D = ToGoal.SizeSquared2D();
	const float UseRadius = AcceptanceRadius + GoalRadius;
	int32 Dir = (ToGoal | NewSpdDir) > 0 ? 1 : -1;
	if (Dir * Dist2D <= FMath::Square(UseRadius))
	{
		HasReachedXY = true;
	}
	const float ZDiff = FMath::Abs(ToGoal.Z);
	const float UseHeight = AcceptanceRadius + GoalHalfHeight + AgentHalfHeight;
	if (Dir * ZDiff <= UseHeight)
	{
		HasReachedZ = true;
	}
	if (HasReachedXY == true && HasReachedZ == true)
	{
		if (LastHasReachedXY == false)
		{
			NewLocation = DestLocation - NewSpdDir * (UseRadius / FVector2D(NewSpdDir).Size());
		}
		if (LastHasReachedZ == false)
		{
			NewLocation = DestLocation - NewSpdDir * (UseHeight / FMath::Abs(NewSpdDir.Z));
		}
	}
	LastHasReachedXY = HasReachedXY;
	LastHasReachedZ = HasReachedZ;
	return (HasReachedXY == true && HasReachedZ == true);
}
