// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_PlayAnimation.h"
#include "GameFramework/Character.h"

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
			UAnimMontage *AnimInst = CachedSkelMesh->GetAnimInstance();
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
							FOnMontageBlendingOutStarted Delegate = FOnMontageBlendingOutStarted::CreateWeakLambda(this, [LocalAnimInstance, LocalMontageInstanceID](UAnimMontage* Montage, bool Result) {
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
						MontageInstanceID = 
					}
				}

			}
		}
	}
}

bool FAction_PlayAnimation::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{

}

EActionResult FAction_PlayAnimation::TickAction(float DeltaTime)
{

}

void FAction_PlayAnimation::MontageFinished(UAnimMontage *Montage, bool bInterrupted)
{

}

bool FAction_PlayAnimation::CanPlayAnimation(FName GroupName)
{

}

FName FAction_PlayAnimation::GetName() const
{

}

FString FAction_PlayAnimation::GetDescription() const
{

}
