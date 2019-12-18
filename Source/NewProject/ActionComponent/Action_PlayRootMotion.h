// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "Action.h"

class UCharacterMovementComponent;
class UAnimMontage;
class ACharacter;

NEWPROJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogAction_PlayRootMotion, Warning, All);

class NEWPROJECT_API FAction_PlayRootMotion : public FAction
{
	DECLARE_DELEGATE_TwoParams(FOnBlendingInDelegate, FAction*, EActionResult);

public:
	FAction_PlayRootMotion() { Type = (EActionType::Animation | EActionType::Move | EActionType::Rotate); }

	static TSharedPtr<FAction_PlayRootMotion> CreateAction(UAnimMontage* InAnimMontage, float InPlayRate = 1.0f, float InBlendInTime = -1.0f, float InBlendOutTime = -1.0f, bool InbLooping = false, FName InSlotNodeName = NAME_None, bool InbNonBlocking = false);
	virtual EActionResult ExecuteAction() override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;
	virtual EActionResult TickAction(float DeltaTime) override;
	virtual float GetTimeRadio() const override;
	virtual void RootMotionFinished(UAnimMontage* Montage, bool bInterrupted);

	FOnBlendingInDelegate BlendingInDelegate;

	FName SlotNodeName = NAME_None;
	uint32 bLooping : 1;
	uint32 bNonBlocking : 1;
	float BlendInTime = -1.0f;
	float BlendOutTime = -1.0f;
	float Duration = -1.0f;

	float PlayRate;
	bool bSetNewMovementMode = true;
	TEnumAsByte<EMovementMode> NewMovementMode = EMovementMode::MOVE_Flying;
	bool bStopSeparateType = false;

	float RecoverMovementModeTime = 1.0f;

	virtual FName GetName() const override;
	virtual FString GetDescription() const override;

protected:
	virtual void UpdateType() override;

	virtual void RecoverMoveStatue();

	TWeakObjectPtr<USkeletalMeshComponent> CachedSkelMesh;

	TWeakObjectPtr<UAnimMontage> AnimMontage;

	TEnumAsByte<EMovementMode> StorgeMovementMode;
	float StorgeMoveMaxSpeed;
	FRotator StorgeRotation;
	int32 MontageInstanceID;

	bool MoveHasAbort = false;

	float FinishDelay;

	bool bAutoHasFinished = false;
	bool bHasUnbinded = false;

	float CurrentTime = 0;
	bool bHasRecoverMovementMode = false;
};

