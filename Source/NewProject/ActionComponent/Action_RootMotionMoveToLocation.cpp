// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_RootMotionMoveToLocation.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

TSharedPtr<FAction_RootMotionMoveToLocation> FAction_RootMotionMoveToLocation::CreateAction(FVector TargetLocation, float Duration /*= FLT_MAX*/, bool bSetNewMovementMode /*= false*/, EMovementMode MovementMode /*= MOVE_Flying*/, bool bRestrictSpeedToExpected /*= false*/, UCurveVector* PathOffsetCurve /*= nullptr*/, ERootMotionFinishVelocityMode VelocityOnFinishMode /*= ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity*/, FVector SetVelocityOnFinish /*= FVector::ZeroVector*/, float ClampVelocityOnFinish /*= 0.0f*/, float InPlayRate /*= 1.f*/, FName InStartSectionName /*= NAME_None*/)
{
	TSharedPtr<FAction_RootMotionMoveToLocation> Action = MakeShareable(new FAction_RootMotionMoveToLocation());
	if (Action.IsValid())
	{
		Action->TargetLocation = TargetLocation;
		Action->Duration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
		Action->bSetNewMovementMode = bSetNewMovementMode;
		Action->NewMovementMode = MovementMode;
		Action->bRestrictSpeedToExpected = bRestrictSpeedToExpected;
		Action->PathOffsetCurve = PathOffsetCurve;
		Action->FinishVelocityMode = VelocityOnFinishMode;
		Action->FinishSetVelocity = SetVelocityOnFinish;
		Action->FinishClampVelocity = ClampVelocityOnFinish;
	}
	return Action;
}

EActionResult FAction_RootMotionMoveToLocation::ExecuteAction()
{
	EActionResult Result = EActionResult::Wait;
	ACharacter *Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent *MovementComp = nullptr;
	if (Character)
	{
		MovementComp = Character->GetCharacterMovement();
	}
	if (!MovementComp)
	{
		return Result;
	}
	StartLocation = Character->GetActorLocation();

	FRootMotionSource_MoveToForce* MoveToForce = new FRootMotionSource_MoveToForce();
	MoveToForce->InstanceName = FName("FAction_RootMotionMoveToLocation");
	MoveToForce->AccumulateMode = ERootMotionAccumulateMode::Override;
	MoveToForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::UseSensitiveLiftoffCheck);
	MoveToForce->Priority = 1000;
	MoveToForce->TargetLocation = TargetLocation;
	MoveToForce->StartLocation = StartLocation;
	MoveToForce->Duration = Duration;
	MoveToForce->bRestrictSpeedToExpected = bRestrictSpeedToExpected;
	MoveToForce->PathOffsetCurve = PathOffsetCurve;
	MoveToForce->FinishVelocityParams.Mode = FinishVelocityMode;
	MoveToForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
	MoveToForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
	RootMotionSourceID = MovementComp->ApplyRootMotionSource(MoveToForce);

	return Result;
}

bool FAction_RootMotionMoveToLocation::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	return true;
}

EActionResult FAction_RootMotionMoveToLocation::TickAction(float DeltaTime)
{
	if (AActor *MyActor = GetOwner())
	{
		const bool bTimedOut = HasTimedOut();
		const float ReachedDestinationDistanceSqr = 50.f * 50.f;
		const bool bReachedDestination = FVector::DistSquared(TargetLocation, MyActor->GetActorLocation()) < ReachedDestinationDistanceSqr;

		if (bTimedOut)
		{
			if (bReachedDestination)
			{

			}
			else
			{

			}
			return EActionResult::Success;
		}
	}
	else
	{
		return EActionResult::Fail;
	}
	return EActionResult::Wait;

}
