// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_RootMotionRadial.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogAction_RootMotionRadial)

void FRootMotionSource_NewRadialForce::PrepareRootMotion(float SimulationTime, float MovementTickTime, const ACharacter& Character, const UCharacterMovementComponent& MoveComponent)
{
	RootMotionParams.Clear();

	const FVector CharacterLocation = Character.GetActorLocation();
	FVector Force = FVector::ZeroVector;
	const FVector ForceLocation = LocationActor ? LocationActor->GetActorLocation() : Location;
	float Distance = FVector::Dist(ForceLocation, CharacterLocation);
	if (Distance < Radius)
	{
		float CurrentStrength = Strength;
		{
			float AdditiveStrengthFactor = 1.0f;
			if (StrengthDistanceFalloff)
			{
				const float DistanceFactor = StrengthDistanceFalloff->GetFloatValue(FMath::Clamp(Distance / Radius, 0.f, 1.f));
				AdditiveStrengthFactor -= (1.f - DistanceFactor);
			}
		}
	}
}

TSharedPtr<FAction_RootMotionRadial> FAction_RootMotionRadial::CreateAction(FVector Location, AActor* LocationActor /*= nullptr*/, float Strength /*= 100.0f*/, float Duration /*= -1.0f*/, float Radius /*= 100.0f*/, bool bIsPush /*= true*/, bool bIsAdditive /*= true*/, bool bNoZForce /*= true*/, UCurveFloat* StrengthDistanceFalloff /*= nullptr*/, UCurveFloat* StrengthOverTime /*= nullptr*/, bool bUseFixedWorldDirection /*= false*/, FRotator FixedWorldDirection /*= FRotator(0.0f, 0.0f, 0.0f)*/, ERootMotionFinishVelocityMode VelocityOnFinishMode /*= ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity*/, FVector SetVelocityOnFinish /*= FVector::ZeroVector*/, float ClampVelocityOnFinish /*= 0.0f*/)
{
	TSharedPtr<FAction_RootMotionRadial> Action = MakeShareable(new FAction_RootMotionRadial());
	if (Action.IsValid())
	{
		Action->TargetLocation = Location;
		Action->TargetLocationActor = LocationActor;
		Action->Strength = Strength;
		Action->Duration = Duration;
		Action->Radius = Radius;
		Action->bIsPush = bIsPush;
		Action->bIsAdditive = bIsAdditive;

		if (bIsAdditive)
		{
			Action->Type = EActionType::Default;
		}
		else
		{
			Action->Type = EActionType::Move;
		}

		Action->bNoZForce = bNoZForce;
		Action->StrengthDistanceFalloff = StrengthDistanceFalloff;
		Action->StrengthOverTime = StrengthOverTime;
		Action->bUseFixedWorldDirection = bUseFixedWorldDirection;
		Action->FixedWorldDirection = FixedWorldDirection;
		Action->FinishVelocityMode = VelocityOnFinishMode;
		Action->FinishSetVelocity = SetVelocityOnFinish;
		Action->FinishClampVelocity = ClampVelocityOnFinish;
	}
	return Action;
}

EActionResult FAction_RootMotionRadial::ExecuteAction()
{
	ACharacter *Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent *MovementComponent = nullptr;
	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();
	}
	if (!MovementComponent)
	{
		return EActionResult::Fail;
	}
	StartTime = GetOwner()->GetWorld()->GetTimeSeconds();

}

bool FAction_RootMotionRadial::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{

}

EActionResult FAction_RootMotionRadial::TickAction(float DeltaTime)
{

}
