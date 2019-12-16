// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Action.h"


class FAction_Wait : public FAction
{
public:
	FAction_Wait() { Type = EActionType::Default; }

	static TSharedPtr<FAction_Wait> CreateAction(float InDelay);
	virtual EActionResult ExecuteAction() override;
	virtual EActionResult TickAction(float DeltaTime) override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;

	float Delay;

	float TimeCount;

	virtual FName GetName() const override;
	virtual FString GetDescription() const override;
};

