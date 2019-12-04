// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Action.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"

class UAnimationAsset;
class UAnimMontage;

NEWPROJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogAction_PlayAnimation, Warning, All);

class FAction_PlayAnimation : public FAction
{
	DECLARE_DELEGATE_TwoParams(FOnBlendingInDelegate, FAction*, EActionResult);

public:
	FAction_PlayAnimation() { Type = EActionType::Animation; }

	static TSharedPtr<FAction_PlayAnimation> CreateAction(UAnimationAsset *InAnimationToPlay, float InPlayRate = 1.0f, float InBlendInTime = -1.0f, float InBlendOutTime = -1.0f, bool InbLooping = false, EAction_AnimationPriority InPriority = EAction_AnimationPriority::Normal, FName InSlotNodeName = NAME_None, bool InbNonBlocking = false);

	virtual EActionResult ExecuteAction() override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;
	virtual EActionResult TickAction(float DeltaTime) override;
	virtual void MontageFinished(UAnimMontage *Montage, bool bInterrupted);

	FOnBlendingInDelegate BlendingInDelegate;

	TWeakObjectPtr<UAnimationAsset> AnimationToPlay;

	EAction_AnimationPriority Priority;
	FName SlotNodeName = NAME_None;
	uint32 bLooping : 1;
	uint32 bNonBlocking : 1;
	float BlendInTime = -1.0f;
	float BlendOutTime = -1.0f;

	bool bStopWhenMoving = false;

	float PlayRate;

	TWeakObjectPtr<UAnimMontage> AnimMontage;
	int32 StorgeLoopCount;

	virtual bool CanPlayAnimation(FName GroupName);

	virtual FName GetName() const override;
	virtual FString GetDescription() const override;

private:
	TWeakObjectPtr<USkeletalMeshComponent> CachedSkelMesh;
	EAnimationMode::Type PreviousAnimationMode;
	FTimerDelegate TimerDelegate;
	FTimerHandle TimerHandle;
	int32 MontageInstanceID;
	bool bAutoHasFinished = false;
	bool bHasUnbinded = false;

	FVector LastVelocity;

	float CurrentTime = 0;
	float BlendingInTime = 0;
};

