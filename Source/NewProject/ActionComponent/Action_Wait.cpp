// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_Wait.h"
#include "Engine/World.h"

TSharedPtr<FAction_Wait> FAction_Wait::CreateAction(float InDelay)
{
	TSharedPtr<FAction_Wait> Action = MakeShareable(new FAction_Wait());
	Action->Delay = InDelay;
	return Action;
}

EActionResult FAction_Wait::ExecuteAction()
{
	if (!GetOwner())
		return EActionResult::Fail;
	UWorld* World = GetOwner()->GetWorld();
	TWeakObjectPtr<UWorld> WorldPtr = World;
	if (!World)
		return EActionResult::Fail;

	TimeCount = 0;

	if (FMath::IsNearlyZero(Delay))
		return EActionResult::Success;

	return EActionResult::Wait;
}

EActionResult FAction_Wait::TickAction(float DeltaTime)
{
	if (!GetOwner())
		return EActionResult::Fail;

	UWorld* World = GetOwner()->GetWorld();
	TWeakObjectPtr<UWorld> WorldPtr = World;
	if (!World)
		return EActionResult::Fail;

	TimeCount += DeltaTime;
	if (TimeCount < Delay)
		return EActionResult::Wait;
	else
		return EActionResult::Success;
}

bool FAction_Wait::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	if (!GetOwner())
		return true;
	UWorld* World = GetOwner()->GetWorld();
	if (!World)
		return true;
	return true;
}

FName FAction_Wait::GetName() const
{
	return TEXT("Action_Wait");
}

FString FAction_Wait::GetDescription() const
{
	return FString::Printf(TEXT("%s (Delay:(%s.3f))"), *GetName().ToString(), Delay);
}
