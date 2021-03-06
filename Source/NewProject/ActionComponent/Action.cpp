// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action.h"
#include "GameFramework/Actor.h"
#include "ActionComponent.h"

void FAction::SetOwner(AActor* InOwner)
{
	Owner = InOwner;
}

void FAction::SetActionComponent(UActionComponent* InActionComponent)
{
	ActionComponent = InActionComponent;
}

void FAction::NotifyActionFinish(EActionResult Result, const FString& Reason /*= EActionFinishReason::UnKnown*/)
{
	if (ParentAction.IsValid())
		ParentAction.Pin()->FinishChildAction(this, Result, Reason);
	else if (ActionComponent.IsValid())
		ActionComponent->FinishAction(this, Result, Reason);
}

void FAction::NotifyTypeChanged()
{
	EActionType ActionType = Type;
	UpdateType();
	if (ActionType == Type)
		return;
	if (ParentAction.IsValid())
		ParentAction.Pin()->NotifyTypeChanged();
	else if (ActionComponent.IsValid())
		ActionComponent->ActionTypeChanged(this, ActionType);

}

bool FAction::DoFinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	if (FinishAction(InResult, Reason, StopType))
	{
		if (InResult != EActionResult::Clean)
		{
			PostFinish.ExecuteIfBound(this, InResult, Reason);
		}
		return true;
	}
	return false;
}

EActionResult FAction::DoExecuteAction()
{
	EActionResult Result = EActionResult::Fail;
	bool PrerequisiteResult = true;
	if (Prerequisite.IsBound())
	{
		PrerequisiteResult = Prerequisite.Execute(this);
	}
	if (PrerequisiteResult)
	{
		PreExecute.ExecuteIfBound(this);
		Result = ExecuteAction();
		if (Result != EActionResult::Wait)
		{
			PostFinish.ExecuteIfBound(this, Result, EActionFinishReason::UnKnown);
		}
	}
	return Result;
}

EActionResult FAction::DoTickAction(float DeltaTime)
{
	return TickAction(DeltaTime);
}

