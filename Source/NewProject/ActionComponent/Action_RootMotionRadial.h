// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Action_RootMotionForce.h"
#include "Action_RootMotionRadial.generated.h"

class UCharacterMovementComponent;
class UAnimMontage;
class ACharacter;

NEWPROJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogAction_RootMotionRadial, Warning, All);

USTRUCT()
struct NEWPROJECT_API FRootMotionSource_NewRadialForce : public FRootMotionSource_RadialForce
{
	GENERATED_USTRUCT_BODY()

	FRootMotionSource_NewRadialForce() {}

	virtual ~FRootMotionSource_NewRadialForce() {}

	virtual void PrepareRootMotion(float SimulationTime, float MovementTickTime, const ACharacter& Character, const UCharacterMovementComponent& MoveComponent) override;
};

template<>
struct NEWPROJECT_API TStructOpsTypeTraits<FRootMotionSource_NewRadialForce> : public TStructOpsTypeTraitsBase2<FRootMotionSource_NewRadialForce>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};

class NEWPROJECT_API FAction_RootMotionRadial : public FAction_RootMotionForce
{
public:
	FAction_RootMotionRadial() { Type = EActionType::Move; }

	static TSharedPtr<FAction_RootMotionRadial> CreateAction(FVector Location, AActor* LocationActor = nullptr, float Strength = 100.0f, float Duration = -1.0f, float Radius = 100.0f, bool bIsPush = true, bool bIsAdditive = true, bool bNoZForce = true, UCurveFloat* StrengthDistanceFalloff = nullptr, UCurveFloat* StrengthOverTime = nullptr, bool bUseFixedWorldDirection = false, FRotator FixedWorldDirection = FRotator(0.0f, 0.0f, 0.0f), ERootMotionFinishVelocityMode VelocityOnFinishMode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity, FVector SetVelocityOnFinish = FVector::ZeroVector, float ClampVelocityOnFinish = 0.0f);

	virtual EActionResult ExecuteAction() override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;
	virtual EActionResult TickAction(float DeltaTime) override;

protected:
	FVector TargetLocation;
	TWeakObjectPtr<AActor> TargetLocationActor;
	float Strength;
	float Duration;
	float Radius;
	bool bIsPush;
	bool bIsAdditive;
	bool bNoZForce;
	TWeakObjectPtr<UCurveFloat> StrengthDistanceFalloff;
	TWeakObjectPtr<UCurveFloat> StrengthOverTime;
	bool bUseFixedWorldDirection;
	FRotator FixedWorldDirection;

	float StartTime;
	float EndTime;
};
