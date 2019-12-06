// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_InterpMoveTo.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Curves/CurveVector.h"

DECLARE_CYCLE_STAT(TEXT("MoveTo"), STAT_InterpMoveTo, STATGROUP_AI);

TSharedPtr<FAction_InterpMoveTo> FAction_InterpMoveTo::CreateAction(const FVector& InDestLocation, float InDuration /*= 0.001f*/, UCurveBase* InCurve /*= nullptr*/, bool InbWithOutControl /*= false*/)
{
	TSharedPtr<FAction_InterpMoveTo> Action = MakeShareable(new FAction_InterpMoveTo());
	if (Action.IsValid())
	{
		Action->DestLocation = InDestLocation;
		Action->Duration = InDuration;

		Action->DurationOfMovement = 0.0f;
		Action->TimeMoveStarted = 0.0f;
		Action->TimeMoveWillEnd = 0.0f;
		Action->LerpCurve = nullptr;
		Action->LerpCurveVector = nullptr;
		Action->bWithOutControl = InbWithOutControl;
		Action->bOrientRotationToMovement = true;
		if (UCurveFloat* CurveFloat = Cast<UCurveFloat>(InCurve))
		{
			Action->LerpCurve = CurveFloat;
		}
		else if (UCurveVector* CurveVector = Cast<UCurveVector>(InCurve))
		{
			Action->LerpCurveVector = CurveVector;
		}
	}
	return Action;
}

TSharedPtr<FAction_InterpMoveTo> FAction_InterpMoveTo::CreateAction(const AActor* InGoal, float InDuration /*= 0.001f*/, UCurveBase* InCurve /*= nullptr*/, bool InbWithOutControl /*= false*/)
{
	TSharedPtr<FAction_InterpMoveTo> Action = MakeShareable(new FAction_InterpMoveTo());
	if (Action.IsValid())
	{
		Action->Duration = InDuration;
		Action->Goal = InGoal;

		Action->DurationOfMovement = 0.0f;
		Action->TimeMoveStarted = 0.0f;
		Action->TimeMoveWillEnd = 0.0f;
		Action->LerpCurve = nullptr;
		Action->LerpCurveVector = nullptr;
		Action->bWithOutControl = InbWithOutControl;
		Action->bOrientRotationToMovement = true;
		if (UCurveFloat* CurveFloat = Cast<UCurveFloat>(InCurve))
		{
			Action->LerpCurve = CurveFloat;
		}
		else if (UCurveVector* CurveVector = Cast<UCurveVector>(InCurve))
		{
			Action->LerpCurveVector = CurveVector;
		}
	}
	return Action;

}

EActionResult FAction_InterpMoveTo::ExecuteAction()
{
	Character = Cast<ACharacter>(GetOwner());
	MovementComp = Character.IsValid() ? Character->GetCharacterMovement() : nullptr;
	if (!MovementComp.IsValid())
	{
		return EActionResult::Fail;
	}
	if (FMath::IsNearlyZero(Duration))
	{
		Character->TeleportTo(DestLocation, Character->GetActorRotation());
		return EActionResult::Success;
	}

	StorgeMovementMode = MovementComp->MovementMode;
	MovementComp->StopMovementImmediately();
	MovementComp->SetMovementMode(MOVE_Custom, 0);

	StartLocation = Character->GetActorLocation();
	TargetLocation = DestLocation;
	if (Goal.IsValid())
	{
		TargetLocation = Goal->GetActorLocation();
		GoalMoveLocation = TargetLocation;
		GoalStartLocation = TargetLocation;
	}
	DurationOfMovement = FMath::Max(Duration, 0.001f);
	TimeMoveStarted = Character->GetWorld()->GetTimeSeconds();
	TimeMoveWillEnd = TimeMoveStarted + DurationOfMovement;
	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (PlayerController)
	{
		if (bWithOutControl)
		{

		}
	}
	return EActionResult::Wait;
}

bool FAction_InterpMoveTo::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (PlayerController)
	{
		if (bWithOutControl)
		{

		}
	}
	if (MovementComp.IsValid())
	{
		MovementComp->SetMovementMode(StorgeMovementMode);
	}
	return true;
}

EActionResult FAction_InterpMoveTo::TickAction(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_InterpMoveTo);
	if (Character.IsValid())
	{
		float CurrentTime = GetOwner()->GetWorld()->GetTimeSeconds();

		if (CurrentTime >= TimeMoveWillEnd)
		{
			if (Goal.IsValid())
			{
				Character->TeleportTo(Goal->GetActorLocation(), Character->GetActorRotation());
			}
			else
			{
				Character->TeleportTo(TargetLocation, Character->GetActorRotation());
			}
			return EActionResult::Success;
		}
		else
		{
			FVector NewLocation;

			float MoveFraction = (CurrentTime - TimeMoveStarted) / DurationOfMovement;
			if (LerpCurveVector.IsValid())
			{
				const FVector ComponentInterpolationFraction = LerpCurveVector->GetVectorValue(MoveFraction);
				NewLocation = FMath::Lerp<FVector, FVector>(StartLocation, TargetLocation, ComponentInterpolationFraction);
			}
			else
			{
				if (LerpCurve.IsValid())
				{
					MoveFraction = LerpCurve->GetFloatValue(MoveFraction);
				}

				NewLocation = FMath::Lerp<FVector, float>(StartLocation, TargetLocation, MoveFraction);
			}

			if (Goal.IsValid())
			{
				FVector OffsetVector = Goal->GetActorLocation() - GoalMoveLocation;
				if (OffsetVector.Size() > 0)
				{
					GoalMoveLocation += (OffsetVector.Size() / (TimeMoveWillEnd - CurrentTime)) * OffsetVector.GetSafeNormal() * DeltaTime;
				}

				NewLocation += GoalMoveLocation - GoalStartLocation;
			}
			FVector OldLocation = Character->GetActorLocation();
			Character->SetActorLocation(NewLocation);
			return EActionResult::Wait;
			}
	}
	else
	{
		return EActionResult::Fail;
	}
}

FName FAction_InterpMoveTo::GetName() const
{
	return TEXT("Action_InterpMoveTo");
}

FString FAction_InterpMoveTo::GetDescription() const
{
	return FString::Printf(TEXT("%s (Goal:(%s))"), *GetName().ToString(), *(Goal.IsValid() ? Goal->GetName() : DestLocation.ToString()));
}

void FAction_InterpMoveTo::PhysicsRotation(float DeltaTime, const FVector& Velocity)
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
		// PITCH
		if (!FMath::IsNearlyEqual(CurrentRotation.Pitch, DesiredRotation.Pitch, AngleTolerance))
		{
			DesiredRotation.Pitch = FMath::FixedTurn(CurrentRotation.Pitch, DesiredRotation.Pitch, DeltaRot.Pitch);
		}

		// YAW
		if (!FMath::IsNearlyEqual(CurrentRotation.Yaw, DesiredRotation.Yaw, AngleTolerance))
		{
			DesiredRotation.Yaw = FMath::FixedTurn(CurrentRotation.Yaw, DesiredRotation.Yaw, DeltaRot.Yaw);
		}

		// ROLL
		if (!FMath::IsNearlyEqual(CurrentRotation.Roll, DesiredRotation.Roll, AngleTolerance))
		{
			DesiredRotation.Roll = FMath::FixedTurn(CurrentRotation.Roll, DesiredRotation.Roll, DeltaRot.Roll);
		}

		// Set the new rotation.
		DesiredRotation.DiagnosticCheckNaN(TEXT("MoveAlongSplineComponent::PhysicsRotation() : DesiredRotation"));
		Actor->SetActorRotation(DesiredRotation);
	}
}

float FAction_InterpMoveTo::GetAxisDeltaRotation(float InAxisRotationRate, float DeltaTime)
{
	return (InAxisRotationRate >= 0.f) ? (InAxisRotationRate * DeltaTime) : 360.f;

}
