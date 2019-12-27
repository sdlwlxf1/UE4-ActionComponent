// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_InterpMeshTransformTo.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"

TSharedPtr<FAction_InterpMeshTransformTo> FAction_InterpMeshTransformTo::CreateAction(const FTransform& InTransform, float InDuration)
{
	TSharedPtr<FAction_InterpMeshTransformTo> Action = MakeShareable(new FAction_InterpMeshTransformTo());
	if (Action.IsValid())
	{
		Action->TargetTransform = InTransform;
		Action->Duration = InDuration;
	}
	return Action;
}

EActionResult FAction_InterpMeshTransformTo::ExecuteAction()
{
	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (!PawnOwner)
	{
		return EActionResult::Fail;
	}

	CurrentTimeStamp = 0;
	FinishTimeStamp = CurrentTimeStamp + Duration;
	FTransform StorgeTransform;

	OriginalMeshTransformOffset = PawnOwner->GetActorTransform();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		auto ClientData = Character->GetCharacterMovement()->GetPredictionData_Client_Character();
		ClientData->MeshRotationOffset = ClientData->MeshRotationOffset;
		ClientData->MeshTranslationOffset = FVector::ZeroVector;
		Character->GetCharacterMovement()->bNetworkSmoothingComplete = true;
		StorgeTransform = Character->GetMesh()->GetComponentTransform();
	}

	if (TargetTransform.GetTranslation() == FVector(FLT_MAX, FLT_MAX, FLT_MAX))
	{
		TargetTransform.SetTranslation(PawnOwner->GetActorLocation());
	}
	if (TargetTransform.Rotator() == FRotator(FLT_MAX, FLT_MAX, FLT_MAX))
	{
		TargetTransform.SetRotation(PawnOwner->GetActorQuat());
	}
	if (TargetTransform.GetScale3D() == FVector(FLT_MAX, FLT_MAX, FLT_MAX))
	{
		TargetTransform.SetScale3D(PawnOwner->GetActorScale3D());
	}

	PawnOwner->SetActorTransform(TargetTransform);
	if (Character)
	{
		MeshTransformOffset = StorgeTransform.GetRelativeTransform(Character->GetActorTransform());
	}

	if (FMath::IsNearlyZero(Duration))
	{
		return EActionResult::Success;
	}
	if (OriginalMeshTransformOffset.Equals(TargetTransform))
	{
		return EActionResult::Success;
	}
	if (Character)
	{
		Character->GetMesh()->SetWorldTransform(StorgeTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
	return EActionResult::Wait;
}

EActionResult FAction_InterpMeshTransformTo::TickAction(float DeltaTime)
{
	ACharacter* PawnOwner = Cast<ACharacter>(GetOwner());
	if (!PawnOwner)
	{
		return EActionResult::Fail;
	}

	CurrentTimeStamp += DeltaTime;

	float LerpPercent = 0.f;
	const float LerpLimit = 1.15f;
	const float TargetDelta = Duration;
	if (TargetDelta > SMALL_NUMBER)
	{
		const float RemainingTime = FinishTimeStamp - CurrentTimeStamp;
		const float CurrentSmoothTime = TargetDelta - RemainingTime;
		LerpPercent = FMath::Clamp(CurrentSmoothTime / TargetDelta, 0.0f, LerpLimit);
	}
	else
	{
		LerpPercent = 1.0f;
	}

	bool bFinished = false;
	FTransform CurMeshTransform = FTransform(PawnOwner->GetBaseRotationOffset(), PawnOwner->GetBaseTranslationOffset(), PawnOwner->GetActorScale());
	if (LerpPercent >= 1.0f - KINDA_SMALL_NUMBER)
	{
		CurMeshTransform = FTransform(PawnOwner->GetBaseRotationOffset(), PawnOwner->GetBaseTranslationOffset(), PawnOwner->GetActorScale());
		bFinished = true;
	}
	else
	{
		CurMeshTransform = UKismetMathLibrary::TLerp(MeshTransformOffset, FTransform(PawnOwner->GetBaseRotationOffset(), PawnOwner->GetBaseTranslationOffset(), PawnOwner->GetActorScale()), LerpPercent);
	}

	PawnOwner->GetMesh()->SetRelativeTransform(CurMeshTransform);
	if (bFinished)
		return EActionResult::Success;
	else
		return EActionResult::Wait;
}
