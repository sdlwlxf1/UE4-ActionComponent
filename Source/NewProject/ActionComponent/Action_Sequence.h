// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Action.h"

class NEWPROJECT_API FAction_Sequence : public FAction
{
public:
	static TSharedPtr<FAction_Sequence> CreateAction(const std::initializer_list<TSharedPtr<FAction>>& InActions);

	virtual TArray<const FAction*> GetActiveActions() const override;

	virtual FName GetName() const override;
	virtual FString GetDescription() const override;

protected:
	virtual EActionResult ExecuteAction() override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;
	virtual EActionResult TickAction(float DeltaTime) override;

	virtual void UpdateType() override;

	virtual bool FinishChildAction(FAction* InAction, EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;

	TArray<TSharedPtr<FAction>> Sequence;
};

