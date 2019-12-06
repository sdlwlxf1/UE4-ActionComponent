// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_RootMotionForce.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY(LogAction_RootMotionForce);

bool FAction_RootMotionForce::HasTimedOut() const
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		UCharacterMovementComponent *MovementComponent = Character->GetCharacterMovement();
		const TSharedPtr<FRootMotionSource> RMS = (MovementComponent ? MovementComponent->GetRootMotionSourceByID(RootMotionSourceID) : nullptr);
		if (!RMS.IsValid())
		{
			return true;
		}
		return RMS->Status.HasFlag(ERootMotionSourceStatusFlags::Finished);
	}
	return true;
}

float FAction_RootMotionForce::GetTimeRadio() const
{
	ACharacter *Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		UCharacterMovementComponent *MovementComponent = Character->GetCharacterMovement();
		const TSharedPtr<FRootMotionSource> RMS = (MovementComponent ? MovementComponent->GetRootMotionSourceByID(RootMotionSourceID) : nullptr);
		if (RMS.IsValid() && RMS->Duration != 0.0f)
		{
			return RMS->CurrentTime / RMS->Duration;
		}
	}
	return 0.0f;
}
