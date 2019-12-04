// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_Sequence.h"


TSharedPtr<FAction_Sequence> FAction_Sequence::CreateAction(const std::initializer_list<TSharedPtr<FAction>>& InActions)
{
	TSharedPtr<FAction_Sequence> Action = MakeShareable(new FAction_Sequence());
	if (Action.IsValid())
	{
		for (auto& SingleAction : InActions)
		{
			SingleAction->ParentAction = Action;
			Action->Sequence.Add(SingleAction);
		}
		Action->NotifyTypeChanged();
	}
	return Action;
}

TArray<const FAction*> FAction_Sequence::GetActiveActions() const
{
	if (Sequence.Num() > 0 && Sequence[0].IsValid())
		return Sequence[0]->GetActiveActions();
	else
		return {};
}

FName FAction_Sequence::GetName() const
{
	return TEXT("Action_Sequence");
}

FString FAction_Sequence::GetDescription() const
{
	FString SequenceString;
	for (auto &Action : Sequence)
	{
		SequenceString += Action->GetDescription() + TEXT(", ");
	}
	SequenceString.RemoveFromEnd(TEXT(", "));
	return FString::Printf(TEXT("%s (Sequence:{%s})"), *GetName().ToString(), *SequenceString);
}

EActionResult FAction_Sequence::ExecuteAction()
{
	if (Sequence.Num() == 0)
		return EActionResult::Success;
	for (auto& Action : Sequence)
	{
		if (!Action.IsValid())
			return EActionResult::Fail;
		Action->SetActionComponent(GetActionComponent());
		Action->SetOwner(GetOwner());
		EActionResult Result = Action->DoExecuteAction();
		if (Result != EActionResult::Wait)
		{
			if (Result == EActionResult::Fail)
				return EActionResult::Fail;
			if (Result == EActionResult::Success)
				Action.Reset();
		}
		else
		{
			break;
		}
	}
	Sequence.RemoveAll([](const TSharedPtr<FAction>& Action) { return !Action.IsValid(); });
	NotifyTypeChanged();

	return Sequence.Num() == 0 ? EActionResult::Success : EActionResult::Wait;
}

bool FAction_Sequence::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	if (Sequence.Num() == 0 || !Sequence[0].IsValid())
	{
		return true;
	}
	TSharedPtr<FAction> CurAction = Sequence[0];
	Sequence[0].Reset();
	if (CurAction->DoFinishAction(InResult, Reason, StopType) == false)
	{
		Sequence[0] = CurAction;
		NotifyTypeChanged();
		return false;
	}
	return true;
}

EActionResult FAction_Sequence::TickAction(float DeltaTime)
{
	if (Sequence.Num() == 0)
		return EActionResult::Abort;
	EActionResult Result = Sequence[0]->DoTickAction(DeltaTime);
	if (Result != EActionResult::Wait)
	{
		FinishChildAction(Sequence[0].Get(), Result);
		return EActionResult::Wait;
	}
	return Result;
}

void FAction_Sequence::UpdateType()
{
	Type = EActionType::Default;
	for (auto& Action : Sequence)
	{
		if (Action.IsValid())
		{
			Type = Action->GetType();
			break;
		}
	}
}

bool FAction_Sequence::FinishChildAction(FAction* InAction, EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	if (InAction)
	{
		ensure(InAction == Sequence[0].Get());
		TSharedPtr<FAction> Action = Sequence[0];
		Sequence[0].Reset();
		if (Action->DoFinishAction(InResult, Reason, StopType))
		{
			if (InResult == EActionResult::Success)
			{
				for (auto& SingleAction : Sequence)
				{
					if (!SingleAction.IsValid())
						continue;
					SingleAction->SetActionComponent(GetActionComponent());
					SingleAction->SetOwner(GetOwner());
					InResult = SingleAction->DoExecuteAction();
					if (InResult == EActionResult::Success)
					{
						SingleAction.Reset();
					}
					else
					{
						break;
					}
				}
			}

			if (InResult == EActionResult::Fail || InResult == EActionResult::Abort || InResult == EActionResult::Clean)
			{
				Sequence.Empty();
				NotifyActionFinish(InResult, Reason);
			}
			else
			{
				Sequence.RemoveAll([](const TSharedPtr<FAction>& Action) { return !Action.IsValid(); });
				if (Sequence.Num() == 0)
				{
					NotifyActionFinish(InResult, Reason);
				}
				else
				{
					NotifyTypeChanged();
				}
			}
		}
		else
		{
			Sequence[0] = Action;
			NotifyTypeChanged();
			return false;
		}
	}
	return true;
}
