// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Action.h"

class NEWPROJECT_API FAction_Parallel : public FAction
{
public:
	FAction_Parallel() { Type = EActionType::Default; }

	static TSharedPtr<FAction_Parallel> CreateAction(TSharedPtr<FAction> InMajor, TSharedPtr<FAction> InMinor);

	bool bStopSeparateType = false;

	virtual TArray<const FAction*> GetActiveActions() const override;

	virtual FName GetName() const override;
	virtual FString GetDescription() const override;

protected:
	virtual EActionResult ExecuteAction() override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;
	virtual EActionResult TickAction(float DeltaTime) override;

	virtual void UpdateType() override;

	virtual bool FinishChildAction(FAction* InAction, EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;

	TSharedPtr<FAction> PendingMajor;
	TSharedPtr<FAction> PendingMinor;

	TSharedPtr<FAction> Major;
	TSharedPtr<FAction> Minor;
};

