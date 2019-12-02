// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SharedPointer.h"
#include "WeakObjectPtrTemplates.h"
#include "ActionEnums.h"

class AActor;
class UActionComponent;

class FAction : public TSharedFromThis<FAction>
{
	DECLARE_DELEGATE_RetVal_OneParam(bool, FPrerequisite, FAction*);
	DECLARE_DELEGATE_OneParam(FPreExecute, FAction*);
	DECLARE_DELEGATE_ThreeParams(FPostFinish, FAction*, EActionResult, const FString&);

	friend class UActionComponent;
	friend class FAction_Parallel;
	friend class FAction_Sequence;
public:
	FAction()
		: ParentAction(nullptr)
		, Type(EActionType::Default)
		, Owner(nullptr)
		, ActionComponent(nullptr)
	{}

	virtual ~FAction() {}

	virtual float GetTimeRadio() const { return 0.0f; }

	FPrerequisite Prerequisite;
	FPreExecute PreExecute;
	FPostFinish PostFinish;

	virtual TArray<const FAction*> GetActiveActions() const { return { this }; }

	FORCEINLINE bool IsType(EActionType InType) const { return TypeIsAType(Type, InType); }
	FORCEINLINE EActionType GetType() const { return Type; }

	static bool TypeIsAType(EActionType InType, EActionType IsType) { return ((InType & IsType) != EActionType::Default); }

	virtual FName GetName() const { return TEXT("Action"); }
	virtual FString GetDescription() const { return FString(); }

	FORCEINLINE AActor* GetOwner() const { return Owner.IsValid() ? Owner.Get() : nullptr; }
	FORCEINLINE UActionComponent* GetActionComponent() const { return ActionComponent.IsValid() ? ActionComponent.Get() : nullptr; }

protected:

	virtual EActionResult ExecuteAction() { return EActionResult::Wait; }
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) { return true; }
	virtual EActionResult TickAction(float DeltaTime) { return EActionResult::Wait; }

	void SetOwner(AActor* InOwner);

	void SetActionComponent(UActionComponent* InActionComponent);

	TWeakPtr<FAction> ParentAction = nullptr;

	void NotifyActionFinish(EActionResult Result, const FString& Reason = EActionFinishReason::UnKnown);
	void NotifyTypeChanged();
	virtual void UpdateType() {}

	virtual bool FinishChildAction(FAction* InAction, EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) { return true; }

	EActionType Type;

private:
	bool DoFinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default);
	EActionResult DoExecuteAction();
	EActionResult DoTickAction(float DeltaTime);

	TWeakObjectPtr<AActor> Owner;
	TWeakObjectPtr<UActionComponent> ActionComponent;
};
