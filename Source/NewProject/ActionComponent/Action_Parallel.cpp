// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_Parallel.h"

TSharedPtr<FAction_Parallel> FAction_Parallel::CreateAction(TSharedPtr<FAction> InMajor, TSharedPtr<FAction> InMinor)
{
	TSharedPtr<FAction_Parallel> Action = MakeShareable(new FAction_Parallel());
	if (Action.IsValid())
	{
		Action->PendingMajor = InMajor;
		Action->PendingMinor = InMinor;
		Action->PendingMajor->ParentAction = Action;
		Action->PendingMinor->ParentAction = Action;
		Action->NotifyTypeChanged();
	}
	return Action;
}

TArray<const FAction*> FAction_Parallel::GetActiveActions() const
{
	TArray<const FAction*> Ret;
	if (Major.IsValid())
		Ret.Append(Major->GetActiveActions());
	if (Minor.IsValid())
		Ret.Append(Minor->GetActiveActions());
	return Ret;
}

FName FAction_Parallel::GetName() const
{
	return TEXT("Action_Parallel");
}

FString FAction_Parallel::GetDescription() const
{
	FAction *MajorPtr = Major.Get();
	if (MajorPtr == nullptr)
	{
		MajorPtr = PendingMajor.Get();
	}
	FAction *MinorPtr = Minor.Get();
	if (MinorPtr == nullptr)
	{
		MinorPtr = PendingMinor.Get();
	}
	FString ParallelString = TEXT("MajorPtr : [") + (MajorPtr ? MajorPtr->GetDescription() : FString()) + TEXT("], MinorPtr : [") + (MinorPtr ? MinorPtr->GetDescription() : FString()) + TEXT("]");
	return FString::Printf(TEXT("%s (Parallel:{%s}"), *GetName().ToString(), *ParallelString);
}

EActionResult FAction_Parallel::ExecuteAction()
{
	if (!PendingMajor.IsValid())
		return EActionResult::Fail;
	PendingMajor->SetActionComponent(GetActionComponent());
	PendingMajor->SetOwner(GetOwner());
	EActionResult MajorResult = PendingMajor->DoExecuteAction();
	if (MajorResult == EActionResult::Wait)
	{
		Major = PendingMajor;
	}
	PendingMajor.Reset();
	NotifyTypeChanged();

	PendingMinor->SetActionComponent(GetActionComponent());
	PendingMinor->SetOwner(GetOwner());
	EActionResult MinorResult = PendingMinor->DoExecuteAction();
	if (MinorResult == EActionResult::Wait)
	{
		Minor = PendingMinor;
	}
	PendingMinor.Reset();
	NotifyTypeChanged();

	return MajorResult;
}

bool FAction_Parallel::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	if (Major.IsValid())
	{
		TSharedPtr<FAction> Action = Major;
		Major.Reset();
		if (Action->DoFinishAction(InResult, Reason, StopType) == false)
		{
			Major = Action;
			NotifyTypeChanged();
			return false;
		}
	}
	if (Minor.IsValid())
	{
		TSharedPtr<FAction> Action = Minor;
		Minor.Reset();
		Action->DoFinishAction(EActionResult::Abort, Reason, StopType);
	}
	return true;
}

EActionResult FAction_Parallel::TickAction(float DeltaTime)
{
	EActionResult Result = Major->DoTickAction(DeltaTime);
	if (Result != EActionResult::Wait)
	{
		FinishChildAction(Major.Get(), Result);
		return Result;
	}
	if (Minor.IsValid())
	{
		Result = Minor->DoTickAction(DeltaTime);
		if (Result != EActionResult::Wait)
		{
			FinishChildAction(Minor.Get(), Result);
		}
	}
	return EActionResult::Wait;
}

void FAction_Parallel::UpdateType()
{
	Type = EActionType::Default;
	if (Major.IsValid())
	{
		Type = Major->GetType();
	}
	if (Minor.IsValid() && Major.IsValid())
	{
		Type = Major->GetType() | Minor->GetType();
	}
	if (PendingMajor.IsValid())
	{
		Type = PendingMajor->GetType();
	}
	if (PendingMinor.IsValid() && PendingMajor.IsValid())
	{
		Type = PendingMajor->GetType() | PendingMinor->GetType();
	}
}

bool FAction_Parallel::FinishChildAction(FAction* InAction, EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	if (InAction)
	{
		TSharedPtr<FAction> Action;
		if (InAction == Major.Get())
		{
			Action = Major;
			Major.Reset();
			if (Action->DoFinishAction(InResult, Reason, StopType) == false)
			{
				Major = Action;
				NotifyTypeChanged();
				return false;
			}
			else
			{
				NotifyActionFinish(InResult, Reason);
			}
		}
		else if (InAction == Minor.Get())
		{
			Action = Minor;
			Minor.Reset();
			if (Action->DoFinishAction(InResult, Reason, StopType))
			{
				if (bStopSeparateType || (InResult != EActionResult::Abort && InResult != EActionResult::Clean))
				{
					NotifyTypeChanged();
				}
				else
				{
					NotifyActionFinish(InResult, Reason);
				}
			}
			else
			{
				Minor = Action;
				NotifyTypeChanged();
				return false;
			}
		}
	}
	return true;
}

