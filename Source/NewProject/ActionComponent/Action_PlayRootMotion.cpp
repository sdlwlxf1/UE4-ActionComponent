// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_PlayRootMotion.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "VisualLogger.h"

DEFINE_LOG_CATEGORY(LogAction_PlayRootMotion);

TSharedPtr<FAction_PlayRootMotion> FAction_PlayRootMotion::CreateAction(UAnimMontage* InAnimMontage, float InPlayRate /*= 1.0f*/, float InBlendInTime /*= -1.0f*/, float InBlendOutTime /*= -1.0f*/, bool InbLooping /*= false*/, FName InSlotNodeName /*= NAME_None*/, bool InbNonBlocking /*= false*/)
{
	if (InAnimMontage == nullptr)
		return nullptr;

	if (InAnimMontage->HasRootMotion() == false)
		return nullptr;

	TSharedPtr<FAction_PlayRootMotion> Action = MakeShareable(new FAction_PlayRootMotion());
	if (Action.IsValid())
	{
		Action->AnimMontage = InAnimMontage;
		Action->bNonBlocking = InbNonBlocking;
		Action->PlayRate = InPlayRate;
		Action->SlotNodeName = InSlotNodeName;
		Action->bLooping = InbLooping;
		Action->BlendInTime = InBlendInTime;
		Action->BlendOutTime = InBlendOutTime;
	}
	return Action;
}

EActionResult FAction_PlayRootMotion::ExecuteAction()
{
	EActionResult Result = EActionResult::Fail;
	ACharacter *Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent *MovementComp = nullptr;
	if (Character)
	{
		MovementComp = Character->GetCharacterMovement();
	}
	if (!MovementComp)
		return Result;

	bAutoHasFinished = false;
	bHasUnbinded = false;

	if (AnimMontage.IsValid())
	{
		if (Character)
		{
			CachedSkelMesh = Character->GetMesh();
		}
		else
		{
			CachedSkelMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
		}

		if (CachedSkelMesh.IsValid())
		{
			UAnimInstance *AnimInst = CachedSkelMesh->GetAnimInstance();
			if (AnimInst)
			{
				if (AnimMontage->SlotAnimTracks.Num() > 0)
				{
					if (SlotNodeName != NAME_None)
					{
						AnimMontage->SlotAnimTracks[0].SlotName = SlotNodeName;
					}
					if (AnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num() > 0)
					{
						AnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].LoopingCount = bLooping ? INT_MAX : 1;
					}
				}

				if (BlendInTime != -1.0f)
					AnimMontage->BlendIn.SetBlendTime(BlendInTime);
				else
					BlendInTime = AnimMontage->BlendIn.GetBlendTime();

				if (BlendOutTime != -1.0f)
					AnimMontage->BlendOut.SetBlendTime(BlendOutTime);
				else
					BlendOutTime = AnimMontage->BlendOut.GetBlendTime();

				float PlayLength = AnimMontage->GetPlayLength();
				if (Duration == -1.0f)
				{
					Duration = PlayLength - AnimMontage->BlendOut.GetBlendTime();
				}
				Duration = FMath::Max(Duration, KINDA_SMALL_NUMBER);

				if (PlayRate == -1.0f && PlayLength != 0.0f)
				{
					PlayRate = (PlayLength - AnimMontage->BlendOut.GetBlendTime()) / Duration;
				}
				else
				{
					PlayRate = FMath::Max(PlayRate, KINDA_SMALL_NUMBER);
				}

				FinishDelay = Character->PlayAnimMontage(AnimMontage.Get(), PlayRate);
				if (bNonBlocking == false && FinishDelay > 0)
				{
					if (bSetNewMovementMode)
					{
						StorgeMovementMode = MovementComp->MovementMode;
						MovementComp->SetMovementMode(NewMovementMode);
					}
					StorgeRotation = Character->GetActorRotation();
					FOnMontageBlendingOutStarted BlendingOutDelegate = FOnMontageBlendingOutStarted::CreateRaw(this, &FAction_PlayRootMotion::RootMotionFinished);
					AnimInst->Montage_SetBlendingOutDelegate(BlendingOutDelegate, AnimMontage.Get());
					MontageInstanceID = AnimInst->GetActiveInstanceForMontage(AnimMontage.Get())->GetInstanceID();
					Result = EActionResult::Wait;
				}
				else
				{
					if (FinishDelay > 0)
					{
						TWeakObjectPtr<UAnimInstance> LocalAnimInstance = AnimInst;
						TWeakObjectPtr<UAnimMontage> LocalAnimMontage = AnimMontage;
						FRotator LocalStorgeRotation = Character->GetActorRotation();
						bool SetNewMovementMode = bSetNewMovementMode;
						EMovementMode LocalStorgeMoveMode = MovementComp->MovementMode;
						if (bSetNewMovementMode)
						{
							MovementComp->SetMovementMode(NewMovementMode);
						}

						TWeakObjectPtr<UCharacterMovementComponent> MovementCompPtr = MovementComp;
						FOnMontageEnded Delegate = FOnMontageEnded::CreateLambda([SetNewMovementMode, LocalStorgeMoveMode, LocalStorgeRotation, MovementCompPtr, LocalAnimInstance, LocalAnimMontage](UAnimMontage* Montage, bool bInterrupted) {
							if (SetNewMovementMode)
							{
								if (MovementCompPtr.IsValid())
								{
									MovementCompPtr->SetMovementMode(LocalStorgeMoveMode);
									MovementCompPtr->UpdatedComponent->SetWorldRotation(LocalStorgeRotation);
								}
							}
							if (LocalAnimInstance.IsValid())
							{
								FAnimMontageInstance* MontageInstance = LocalAnimInstance->GetActiveInstanceForMontage(LocalAnimMontage.Get());
								if (MontageInstance)
								{
									MontageInstance->OnMontageBlendingOutStarted.Unbind();
									MontageInstance->OnMontageEnded.Unbind();
								}
							}
						});
						AnimInst->Montage_SetEndDelegate(Delegate, AnimMontage.Get());
					}
					UE_CVLOG(bNonBlocking == false, GetOwner(), LogAction_PlayRootMotion, Log, TEXT("Instant success due to having a valid AnimationToPlay and Character with SkelMesh, but 0-length animation"));
					Result = EActionResult::Success;
				}
			}
		}
	}
	else if (!AnimMontage.IsValid())
	{
		UE_CVLOG(!AnimMontage.IsValid(), GetOwner(), LogAction_PlayRootMotion, Warning, TEXT("Instant success but having a nullptr Animation to play"));
		return EActionResult::Success;
	}
	return Result;
}

bool FAction_PlayRootMotion::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	BlendingInDelegate.ExecuteIfBound(this, InResult);
	BlendingInDelegate.Unbind();
	ACharacter *Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		UAnimInstance *AnimInst = CachedSkelMesh->GetAnimInstance();
		if (AnimInst)
		{
			FAnimMontageInstance* MontageInstance = AnimInst->GetMontageInstanceForID(MontageInstanceID);
			if (MontageInstance)
			{
				if (StopType == EActionType::Move && InResult == EActionResult::Abort && bStopSeparateType)
				{
					MontageInstance->PushDisableRootMotion();
					MoveHasAbort = true;
					RecoverMoveStatue();
					NotifyTypeChanged();
					return false;
				}

				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
				bHasUnbinded = true;
			}
			if (InResult == EActionResult::Abort && bAutoHasFinished == false)
			{
				AnimInst->DispatchQueuedAnimEvents();
				Character->StopAnimMontage(AnimMontage.Get());
			}
		}
		if (bNonBlocking == false && FinishDelay > 0 && MoveHasAbort == false)
		{
			RecoverMoveStatue();
		}
	}
	return true;
}

EActionResult FAction_PlayRootMotion::TickAction(float DeltaTime)
{
	CurrentTime += DeltaTime;
	if (CurrentTime > BlendInTime)
	{
		BlendingInDelegate.ExecuteIfBound(this, EActionResult::Success);
		BlendingInDelegate.Unbind();
	}
	if (bLooping == false && CurrentTime > Duration * RecoverMovementModeTime * 1.01f)
	{
		RecoverMoveStatue();
	}
	ACharacter *Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		UCharacterMovementComponent *Movement = Character->GetCharacterMovement();
		if (Movement)
		{
			bool CanSend = false;

			uint8 ClientMovementMode = Movement->MovementMode;
		}
	}
	return EActionResult::Wait;
}

float FAction_PlayRootMotion::GetTimeRadio() const
{
	ACharacter *Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		UAnimInstance *AnimInst = CachedSkelMesh->GetAnimInstance();
		if (AnimInst)
		{
			FAnimMontageInstance* MontageInstance = AnimInst->GetMontageInstanceForID(MontageInstanceID);
			if (MontageInstance)
			{
				return MontageInstance->GetPosition() / FinishDelay;
			}
		}
	}
	return 0.0f;
}

void FAction_PlayRootMotion::RootMotionFinished(UAnimMontage* Montage, bool bInterrupted)
{
	if (bHasUnbinded == false)
	{
		if (bAutoHasFinished == false)
		{
			bAutoHasFinished = true;
			NotifyActionFinish(bInterrupted ? EActionResult::Abort : EActionResult::Success, EActionFinishReason::UEInternalStop);
		}
	}
}

FName FAction_PlayRootMotion::GetName() const
{
	return TEXT("Action_PlayRootMotion");

}

FString FAction_PlayRootMotion::GetDescription() const
{
	return FString::Printf(TEXT("%s (RootMotion:%s)"), *GetName().ToString(), *(AnimMontage.IsValid() ? AnimMontage->GetPathName() : FString()));
}

void FAction_PlayRootMotion::UpdateType()
{
	if (MoveHasAbort)
	{
		Type = EActionType::Animation;
	}
	else
	{
		Type = (EActionType::Animation | EActionType::Move | EActionType::Rotate);
	}

}

void FAction_PlayRootMotion::RecoverMoveStatue()
{
	if (bHasRecoverMovementMode == false)
	{
		bHasRecoverMovementMode = true;
		ACharacter *Character = Cast<ACharacter>(GetOwner());
		if (Character)
		{
			UCharacterMovementComponent *MovementComp = Character->GetCharacterMovement();
			if (MovementComp)
			{
				if (bSetNewMovementMode)
				{
					MovementComp->SetMovementMode(StorgeMovementMode);
				}
			}
		}
	}

}
