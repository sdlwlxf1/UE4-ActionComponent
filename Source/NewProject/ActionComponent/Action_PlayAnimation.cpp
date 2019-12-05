// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_PlayAnimation.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "VisualLogger.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ActionComponent.h"

DEFINE_LOG_CATEGORY(LogAction_PlayAnimation);

TSharedPtr<FAction_PlayAnimation> FAction_PlayAnimation::CreateAction(UAnimationAsset *InAnimationToPlay, float InPlayRate /*= 1.0f*/, float InBlendInTime /*= -1.0f*/, float InBlendOutTime /*= -1.0f*/, bool InbLooping /*= false*/, EAction_AnimationPriority InPriority /*= EAction_AnimationPriority::Normal*/, FName InSlotNodeName /*= NAME_None*/, bool InbNonBlocking /*= false*/)
{
	if (auto Montage = Cast<UAnimMontage>(InAnimationToPlay))
	{
		if (Montage->HasRootMotion())
		{
			ensureMsgf(false, TEXT("RootMotion«Î”√ FAction_PlayRootMotion!"));
			return NULL;
		}
	}
	TSharedPtr<FAction_PlayAnimation> Action = MakeShareable(new FAction_PlayAnimation());
	if (Action.IsValid())
	{
		Action->AnimationToPlay = InAnimationToPlay;
		Action->bNonBlocking = InbNonBlocking;
		Action->PlayRate = InPlayRate;
		Action->SlotNodeName = InSlotNodeName;
		Action->Priority = InPriority;
		Action->bLooping = InbLooping;
		Action->BlendInTime = InBlendInTime;
		Action->BlendOutTime = InBlendOutTime;
	}
	return Action;
}

EActionResult FAction_PlayAnimation::ExecuteAction()
{
	EActionResult Result = EActionResult::Fail;

	if (!GetOwner())
		return Result;

	TimerHandle.Invalidate();
	bAutoHasFinished = false;
	bHasUnbinded = false;

	if (AnimationToPlay.IsValid())
	{
		ACharacter* const Character = Cast<ACharacter>(GetOwner());
		if (Character)
		{
			CachedSkelMesh = Character->GetMesh();
			LastVelocity = Character->GetCharacterMovement()->Velocity;
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
				AnimMontage = Cast<UAnimMontage>(AnimationToPlay);
				if (AnimMontage.IsValid())
				{
					FName GroupName = AnimMontage->GetGroupName();
					if (SlotNodeName != NAME_None)
					{
						GroupName = AnimMontage->GetSkeleton()->GetSlotGroupName(SlotNodeName);
					}
					if (CanPlayAnimation(GroupName) == false)
						return EActionResult::Fail;

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

					const float FinishDelay = Character->PlayAnimMontage(AnimMontage.Get(), PlayRate);
					if (bNonBlocking == false && FinishDelay > 0)
					{
						FOnMontageBlendingOutStarted Delegate = FOnMontageBlendingOutStarted::CreateSP(this, &FAction_PlayAnimation::MontageFinished);
						AnimInst->Montage_SetBlendingOutDelegate(Delegate, AnimMontage.Get());
						MontageInstanceID = AnimInst->GetActiveInstanceForMontage(AnimMontage.Get())->GetInstanceID();
						Result = EActionResult::Wait;
					}
					else
					{
						if (FinishDelay > 0)
						{
							TWeakObjectPtr<UAnimInstance> LocalAnimInstance = AnimInst;
							int32 LocalMontageInstanceID = MontageInstanceID;
							FOnMontageBlendingOutStarted Delegate = FOnMontageBlendingOutStarted::CreateWeakLambda(GetOwner(), [LocalAnimInstance, LocalMontageInstanceID](UAnimMontage* Montage, bool Result) {
								if (LocalAnimInstance.IsValid())
								{
									FAnimMontageInstance* MontageInstance = LocalAnimInstance->GetMontageInstanceForID(LocalMontageInstanceID);
									if (MontageInstance)
									{
										MontageInstance->OnMontageBlendingOutStarted.Unbind();
										MontageInstance->OnMontageEnded.Unbind();
									}
								}
							});
							AnimInst->Montage_SetBlendingOutDelegate(Delegate, AnimMontage.Get());
						}
						if (FinishDelay < 0)
						{
							UE_CVLOG(FinishDelay < 0, GetOwner(), LogAction_PlayAnimation, Log, TEXT("Instant fail due to montage priority or invalid AnimationToPlay or Character with SkelMesh"));
							Result = EActionResult::Fail;
						}
						else
						{
							UE_CVLOG(FinishDelay >= 0, GetOwner(), LogAction_PlayAnimation, Log, TEXT("Instant success due to having a vaild AnimationToPlay and Character with SkelMesh, but 0-length animation"));
						}
					}
				}
				else if (auto AnimSequence = Cast<UAnimSequenceBase>(AnimationToPlay))
				{
					float FinishDelay = 0.0f;
					int32 LoopCount = bLooping ? INT32_MAX : 1;
					if (BlendInTime == -1.0f) BlendInTime = 0.25f;
					if (BlendOutTime == -1.0f) BlendOutTime = 0.25f;

					if (SlotNodeName == NAME_None)
					{
						SlotNodeName = TEXT("DefaultSlot");
					}
					FName GroupName = FAnimSlotGroup::DefaultGroupName;
					USkeleton* MySkeleton = AnimSequence->GetSkeleton();
					if (MySkeleton)
					{
						GroupName = MySkeleton->GetSlotGroupName(SlotNodeName);
					}
					if (CanPlayAnimation(GroupName) == false)
						return EActionResult::Fail;

					AnimMontage = AnimInst->PlaySlotAnimationAsDynamicMontage(AnimSequence, SlotNodeName, BlendInTime, BlendOutTime, PlayRate, LoopCount);
					if (AnimMontage.IsValid())
					{
						FinishDelay = AnimMontage->GetPlayLength();
					}

					if (bNonBlocking == false && FinishDelay > 0)
					{
						FOnMontageBlendingOutStarted Delegate = FOnMontageBlendingOutStarted::CreateSP(this, &FAction_PlayAnimation::MontageFinished);
						AnimInst->Montage_SetBlendingOutDelegate(Delegate, AnimMontage.Get());
						MontageInstanceID = AnimInst->GetActiveInstanceForMontage(AnimMontage.Get())->GetInstanceID();
						Result = EActionResult::Wait;
					}
					else
					{
						if (FinishDelay > 0)
						{
							TWeakObjectPtr<UAnimInstance> LocalAnimInstance = AnimInst;
							int32 LocalMontageInstanceID = MontageInstanceID;
							FOnMontageBlendingOutStarted Delegate = FOnMontageBlendingOutStarted::CreateWeakLambda(GetOwner(), [LocalAnimInstance, LocalMontageInstanceID](UAnimMontage *Montage, bool Result) {
								if (LocalAnimInstance.IsValid())
								{
									FAnimMontageInstance *MontageInstance = LocalAnimInstance->GetMontageInstanceForID(LocalMontageInstanceID);
									if (MontageInstance)
									{
										MontageInstance->OnMontageBlendingOutStarted.Unbind();
										MontageInstance->OnMontageEnded.Unbind();
									}
								}
							});
							AnimInst->Montage_SetBlendingOutDelegate(Delegate, AnimMontage.Get());
						}
						UE_CVLOG(bNonBlocking == false, GetOwner(), LogAction_PlayAnimation, Log, TEXT("Instant success due to having a valid AnimationToPlay and Character with SkelMesh, but 0-length animation"));
						Result = EActionResult::Success;
					}
				}
			}
			else
			{
				PreviousAnimationMode = CachedSkelMesh->GetAnimationMode();
				CachedSkelMesh->PlayAnimation(AnimationToPlay.Get(), bLooping);
				const float FinishDelay = AnimationToPlay->GetMaxCurrentTime();

				if (bNonBlocking == false && FinishDelay > 0)
				{
					if (bLooping == false)
					{
						TimerDelegate = FTimerDelegate::CreateWeakLambda(GetOwner(), [this]() {
							NotifyActionFinish(EActionResult::Success);
						});
						GetOwner()->GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, FinishDelay, false);
					}
					Result = EActionResult::Wait;
				}
				else
				{
					UE_CVLOG(bNonBlocking == false, GetOwner(), LogAction_PlayAnimation, Log, TEXT("Instant success due to having a valid AnimationToPlay and Character with SkelMesh, but 0-length animation"));
					TWeakObjectPtr<USkeletalMeshComponent> SkelMesh = CachedSkelMesh;
					EAnimationMode::Type PreAnimationMode = PreviousAnimationMode;
					TimerDelegate = FTimerDelegate::CreateWeakLambda(GetOwner(), [SkelMesh, PreAnimationMode]() {
						if (SkelMesh.IsValid() && PreAnimationMode == EAnimationMode::AnimationBlueprint)
						{
							SkelMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
						}
					});
					GetOwner()->GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, FinishDelay, false);
					Result = EActionResult::Success;
				}
			}
		}
	}
	return Result;
}

bool FAction_PlayAnimation::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	BlendingInDelegate.ExecuteIfBound(this, InResult);
	BlendingInDelegate.Unbind();
	if (CachedSkelMesh.IsValid())
	{
		UAnimInstance *AnimInst = CachedSkelMesh->GetAnimInstance();
		if (AnimInst)
		{
			FAnimMontageInstance* MontageInstance = AnimInst->GetMontageInstanceForID(MontageInstanceID);
			if (MontageInstance)
			{
				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
				bHasUnbinded = true;
			}
			if (InResult == EActionResult::Abort && bAutoHasFinished == false)
			{
				if (auto Montage = Cast<UAnimMontage>(AnimationToPlay))
				{
					AnimInst->Montage_Stop(BlendOutTime, Montage);
				}
				else if (auto AnimSequence = Cast<UAnimSequenceBase>(AnimationToPlay))
				{
					AnimInst->StopSlotAnimation(BlendOutTime, SlotNodeName);
				}
			}
		}
		else
		{
			if (AnimationToPlay.IsValid() && GetOwner() && TimerHandle.IsValid())
			{
				GetOwner()->GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
			}
			if (PreviousAnimationMode == EAnimationMode::AnimationBlueprint)
			{
				CachedSkelMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			}
			if (InResult == EActionResult::Abort)
			{
				CachedSkelMesh->Stop();
			}
			TimerHandle.Invalidate();
		}
	}
	return true;
}

EActionResult FAction_PlayAnimation::TickAction(float DeltaTime)
{
	CurrentTime += DeltaTime;
	if (CurrentTime > BlendingInTime)
	{
		BlendingInDelegate.ExecuteIfBound(this, EActionResult::Success);
		BlendingInDelegate.Unbind();
	}
	if (bStopWhenMoving)
	{
		ACharacter * const Character = Cast<ACharacter>(GetOwner());
		if (Character)
		{
			if (LastVelocity != Character->GetCharacterMovement()->Velocity)
			{
				return EActionResult::Abort;
			}
			LastVelocity = Character->GetCharacterMovement()->Velocity;
		}
	}
	return EActionResult::Wait;

}

void FAction_PlayAnimation::MontageFinished(UAnimMontage *Montage, bool bInterrupted)
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

bool FAction_PlayAnimation::CanPlayAnimation(FName GroupName)
{
	if (GetActionComponent() == false)
		return false;

	for (const auto& Pair : GetActionComponent()->GetAllActions())
	{
		if (FAction::TypeIsAType(Pair.Key, EActionType::Animation))
		{
			const auto& Array = Pair.Value;
			for (int32 i = 0; i < Array.Num(); i++)
			{
				TArray<const FAction*> ActiveActions = Array[i]->GetActiveActions();
				for (int32 j = 0; j < ActiveActions.Num(); j++)
				{
					if (ActiveActions[j]->GetType() == EActionType::Animation)
					{
						const FAction_PlayAnimation* Ani = static_cast<const FAction_PlayAnimation*>(ActiveActions[j]);
						if (Ani && Ani->AnimMontage.IsValid())
						{
							if (Ani->AnimMontage->GetGroupName() == GroupName)
							{
								if (Ani->Priority < Priority)
								{
									UE_VLOG(GetOwner(), LogAction_PlayAnimation, Warning, TEXT("Animation %s has lower Priority than current Animation %s"), *AnimationToPlay->GetPathName(), *Ani->AnimationToPlay->GetPathName());
									return false;
								}
							}
						}
					}
				}
			}
		}
	}
	return true;
}

FName FAction_PlayAnimation::GetName() const
{
	return TEXT("Action_PlayAnimation");
}

FString FAction_PlayAnimation::GetDescription() const
{
	return FString::Printf(TEXT("%s (Animation:%s)"), *GetName().ToString(), *(AnimationToPlay.IsValid() ? AnimationToPlay->GetPathName() : FString()));
}
