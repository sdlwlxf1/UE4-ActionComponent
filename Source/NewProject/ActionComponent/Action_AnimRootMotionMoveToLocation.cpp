// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_AnimRootMotionMoveToLocation.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"

DEFINE_LOG_CATEGORY(LogAction_AnimRootMotionMoveToLocation);

TSharedPtr<FAction_AnimRootMotionMoveToLocation> FAction_AnimRootMotionMoveToLocation::CreateAction(UAnimMontage* InAnimMontage, FTransform TargetTransform /*= FTransform(FRotator(FLT_MAX, FLT_MAX, FLT_MAX), FVector(FLT_MAX, FLT_MAX, FLT_MAX))*/, float Duration /*= -1.0f*/, float InMoveDurationRatio /*= 0.05f*/, bool bSetNewMovementMode /*= true*/, EMovementMode MovementMode /*= MOVE_Flying*/, UCurveVector* PathOffsetCurve /*= nullptr*/, ERootMotionFinishVelocityMode VelocityOnFinishMode /*= ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity*/, FVector SetVelocityOnFinish /*= FVector::ZeroVector*/, float ClampVelocityOnFinish /*= 0.0f*/, float InPlayRate /*= -1.0f*/)
{
	if (InAnimMontage == nullptr)
	{
		return nullptr;
	}
	if (InAnimMontage->HasRootMotion() == false)
	{
		return nullptr;
	}

	TSharedPtr<FAction_AnimRootMotionMoveToLocation> Action = MakeShareable(new FAction_AnimRootMotionMoveToLocation());
	if (Action.IsValid())
	{
		Action->AnimMontage = InAnimMontage;
		Action->PlayRate = InPlayRate;
		Action->TargetTransform = TargetTransform;
		Action->Duration = Duration;
		Action->bSetNewMovementMode = bSetNewMovementMode;
		Action->NewMovementMode = MovementMode;
		Action->FinishVelocityMode = VelocityOnFinishMode;
		Action->FinishSetVelocity = SetVelocityOnFinish;
		Action->FinishClampVelocity = ClampVelocityOnFinish;
		Action->bNonBlocking = false;
		Action->TimeMoveStarted = 0.0f;
		Action->TimeMoveWillEnd = 0.0f;
		Action->TimeMoveRatio = InMoveDurationRatio;
		Action->SlotNodeName = TEXT("fullbody");
	}
	return Action;
}

EActionResult FAction_AnimRootMotionMoveToLocation::ExecuteAction()
{
	EActionResult Result = FAction_PlayRootMotion::ExecuteAction();
	if (Result != EActionResult::Wait)
	{
		return Result;
	}
	ACharacter *Character = Cast<ACharacter>(GetOwner());
	MovementCompPtr = nullptr;
	if (Character)
	{
		MovementCompPtr = Character->GetCharacterMovement();
	}
	if (!MovementCompPtr.IsValid())
	{
		return Result;
	}

	StartTransform = Character->GetRootComponent()->GetRelativeTransform();
	DistanceTransform = FTransform::Identity;
	if (TargetTransform.GetTranslation() != FVector(FLT_MAX, FLT_MAX, FLT_MAX))
	{
		USceneComponent* Parent = Character->GetRootComponent()->GetAttachParent();
		if (Parent)
		{
			TargetTransform = TargetTransform.GetRelativeTransform(Parent->GetSocketTransform(Character->GetRootComponent()->GetAttachSocketName()));
		}
		FTransform TotalTransform = AnimMontage->ExtractRootMotionFromTrackRange(0.0f, Duration);
		TotalTransform.SetTranslation(StartTransform.TransformVector(Character->GetBaseRotationOffset().RotateVector(TotalTransform.GetTranslation())));
		FTransform AnimTargetTransform = StartTransform;
		AnimTargetTransform.Accumulate(TotalTransform);
		DistanceTransform.SetTranslation(TargetTransform.GetTranslation() - AnimTargetTransform.GetTranslation());
		DistanceTransform.SetRotation(TargetTransform.GetRotation() * AnimTargetTransform.GetRotation().Inverse());
	}

	CurrentTime = 0.0f;

	MovementCompPtr->ProcessRootMotionPreConvertToWorld.BindRaw(this, &FAction_AnimRootMotionMoveToLocation::ProcessRootMotionPreConvertToWorld);
	MovementCompPtr->ProcessRootMotionPostConvertToWorld.BindRaw(this, &FAction_AnimRootMotionMoveToLocation::ProcessRootMotionPostConvertToWorld);
	
	return Result;
}

bool FAction_AnimRootMotionMoveToLocation::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	if (MovementCompPtr.IsValid())
	{
		MovementCompPtr->ProcessRootMotionPreConvertToWorld.Unbind();
		MovementCompPtr->ProcessRootMotionPostConvertToWorld.Unbind();

		if (FinishVelocityMode == ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity)
		{
			FVector AnimRootMotionVelocity = MovementCompPtr->CalcAnimRootMotionVelocity(LastVelocity, 0.03f, MovementCompPtr->Velocity);
			MovementCompPtr->Velocity = MovementCompPtr->ConstrainAnimRootMotionVelocity(AnimRootMotionVelocity, MovementCompPtr->Velocity);
		}
		else if (FinishVelocityMode == ERootMotionFinishVelocityMode::SetVelocity)
		{
			MovementCompPtr->Velocity = FinishSetVelocity;
		}
	}
	return FAction_PlayRootMotion::FinishAction(InResult, TEXT(""), StopType);
}

EActionResult FAction_AnimRootMotionMoveToLocation::TickAction(float DeltaTime)
{
	CurrentTime += DeltaTime;
	ACharacter *Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		FTransform CurrentTransform = AnimMontage->ExtractRootMotionFromTrackRange(0.0f, CurrentTime);
		FVector MoveDelta = StartTransform.TransformVector(Character->GetBaseRotationOffset().RotateVector(CurrentTransform.GetTranslation()));
		FTransform CurrentDistanceTransform = FTransform::Identity;
		CurrentDistanceTransform = UKismetMathLibrary::TLerp(FTransform::Identity, DistanceTransform, CurrentTime / Duration);

		FVector NewLocation = StartTransform.GetTranslation() + MoveDelta + CurrentDistanceTransform.GetTranslation();
		FQuat NewRotation = CurrentDistanceTransform.GetRotation() * CurrentTransform.GetRotation() * StartTransform.GetRotation();
		Character->GetRootComponent()->SetRelativeLocationAndRotation(NewLocation, NewRotation);
	}
	return EActionResult::Wait;
}

FName FAction_AnimRootMotionMoveToLocation::GetName() const
{
	return TEXT("Action_AnimRootMotionMoveToLocation");
}

FString FAction_AnimRootMotionMoveToLocation::GetDescription() const
{
	return FString::Printf(TEXT("%s (RootMotion:%s)"), *GetName().ToString(), *(AnimMontage.IsValid() ? AnimMontage->GetPathName() : FString()));
}

FTransform FAction_AnimRootMotionMoveToLocation::ProcessRootMotionPreConvertToWorld(const FTransform& InTransform, UCharacterMovementComponent* InMovementComp)
{
	return FTransform::Identity;
}

FTransform FAction_AnimRootMotionMoveToLocation::ProcessRootMotionPostConvertToWorld(const FTransform& InTransform, UCharacterMovementComponent* InMovementComp)
{
	return InTransform;
}
