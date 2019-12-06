// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_RootMotionConstant.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"


TSharedPtr<FAction_RootMotionConstant> FAction_RootMotionConstant::CreateAction(FVector WorldDirection, float Strength, float Duration /*= -1.0*/, bool bIsAdditive /*= false*/, UCurveFloat* StrengthOverTime /*= nullptr*/, ERootMotionFinishVelocityMode VelocityOnFinishMode /*= ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity*/, FVector SetVelocityOnFinish /*= FVector::ZeroVector*/, float ClampVelocityOnFinish /*= 0.0f */)
{
	TSharedPtr<FAction_RootMotionConstant> Action = MakeShareable(new FAction_RootMotionConstant());

	if (Action.IsValid())
	{
		Action->WorldDirection = WorldDirection.GetSafeNormal();
		Action->Strength = Strength;
		Action->Duration = Duration;
		Action->bIsAdditive = bIsAdditive;
		if (bIsAdditive)
		{
			Action->Type = EActionType::Default;
		}
		else
		{
			Action->Type = EActionType::Move;
		}
		Action->StrengthOverTime = StrengthOverTime;
		Action->FinishVelocityMode = VelocityOnFinishMode;
		Action->FinishSetVelocity = SetVelocityOnFinish;
		Action->FinishClampVelocity = ClampVelocityOnFinish;
	}
	return Action;
}

EActionResult FAction_RootMotionConstant::ExecuteAction()
{
	EActionResult Result = EActionResult::Wait;
	ACharacter *Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent *MovementComponent = nullptr;
	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();
	}
	if (!MovementComponent)
	{
		return Result;
	}

	FRootMotionSource_ConstantForce* ConstantForce = new FRootMotionSource_ConstantForce();
	ConstantForce->InstanceName = FName("FAction_RootMotionConstant");
	ConstantForce->AccumulateMode = bIsAdditive ? ERootMotionAccumulateMode::Additive : ERootMotionAccumulateMode::Override;
	ConstantForce->Priority = 5;
	ConstantForce->Force = WorldDirection * Strength;
	ConstantForce->Duration = Duration;
	ConstantForce->StrengthOverTime = StrengthOverTime;
	ConstantForce->FinishVelocityParams.Mode = FinishVelocityMode;
	ConstantForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
	ConstantForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
	RootMotionSourceID = MovementComponent->ApplyRootMotionSource(ConstantForce);

	return Result;
}

bool FAction_RootMotionConstant::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	ACharacter *Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent *MovementComponent = nullptr;
	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();
	}
	if (MovementComponent)
	{
		MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);
	}
	return true;
}

EActionResult FAction_RootMotionConstant::TickAction(float DeltaTime)
{
	if (GetOwner())
	{
		const bool bTimedOut = HasTimedOut();
		const bool bIsInfiniteDuration = Duration < 0.f;

		if (!bIsInfiniteDuration && bTimedOut)
		{
			return EActionResult::Success;
		}
	}
	else
	{
		return EActionResult::Fail;
	}
	return EActionResult::Wait;

}
