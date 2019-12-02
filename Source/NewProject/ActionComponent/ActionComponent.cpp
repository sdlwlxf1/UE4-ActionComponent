// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ActionComponent.h"
#include "VisualLogger/VisualLogger.h"
#include "VisualLogger/VisualLoggerTypes.h"
#include "Action.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY(LogActionComponent)

UActorComponent::UActorComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	bAutoActivate = true;
	bWantsInitializeComponent = true;
}

void UActionComponent::StopMoveAction(const FString& Reason /*= EActionFinishReason::CustomStop*/)
{
	StopActionsByType(EActionType::Move, true, Reason);
}

void UActionComponent::StopAction(FAction *InAction, const FString& Reason /*= EActionFinishReason::CustomStop*/)
{
	FinishAction(InAction, EActionResult::Abort, Reason);
}

void UActionComponent::StopActionsByType(EActionType InType, bool bForce /*= true*/, const FString& Reason /*= EActionFinishReason::CustomStop*/)
{
	TMap<EActionType, TArray<TSharedPtr<FAction>>> TempActions = Actions;
	for (auto Pair : TempActions)
	{
		EActionType IsType = Pair.Key;
		EActionType LType = InType;
		EActionType RType = IsType;
		if (bForce == false)
		{
			LType &= (EActionType)(~(uint32)(EActionType::Animation));
			RType &= (EActionType)(~(uint32)(EActionType::Animation));
		}
		if (FAction::TypeIsAType(LType, RType))
		{
			Actions[IsType].Reset();
			for (auto Action : Pair.Value)
			{
				if (Action.IsValid())
				{
					if (Action->DoFinishAction(EActionResult::Abort, Reason, InType) == false)
					{
						if (!Actions.Contains(Action->GetType()))
						{
							Actions.Add(Action->GetType(), TArray<TSharedPtr<FAction>>());
						}
						Actions[Action->GetType()].Add(Action);
					}
				}
			}
		}
	}

}

bool UActionComponent::IsContainType(EActionType InType)
{
	for (auto Pair : Actions)
	{
		if (Pair.Value.Num() > 0)
		{
			if (FAction::TypeIsAType(Pair.Key, InType))
			{
				return true;
			}
		}
	}
	return false;
}

void UActionComponent::Initialize()
{
	UpdatePawn(true);
}

void UActionComponent::Cleanup()
{
	TArray<EActionType> KeyArray;
	Actions.GetKeys(KeyArray);
	for (auto Key : KeyArray)
	{
		EActionType IsType = Key;
		FinishActionsByType(IsType, EActionResult::Clean);
	}
	Actions.Empty();
}

void UActionComponent::InitializeComponent()
{
	Super::InitializeComponent();
	Initialize();
}

void UActionComponent::UninitializeComponent()
{
	Cleanup();
	Super::UninitializeComponent();
}

bool UActionComponent::GetComponentClassCanReplicate() const
{
	return false;
}

void UActionComponent::FinishActionsByType(EActionType InType, EActionResult Result /*= EActionResult::Abort*/, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	UpdatePawn();

	if (Actions.Contains(InType))
	{
		TArray<TSharedPtr<FAction>> TypeActions = Actions[InType];
		Actions[InType].Reset();
		for (auto Action : TypeActions)
		{
			if (Action.IsValid())
			{
				if (Action->DoFinishAction(Result, Reason, StopType) == false)
				{
					if (!Actions.Contains(Action->GetType()))
					{
						Actions.Add(Action->GetType(), TArray<TSharedPtr<FAction>>());
					}
					Actions[Action->GetType()].Add(Action);
				}
			}
		}
	}
}

void UActionComponent::FinishAction(FAction *InAction, EActionResult Result /*= EActionResult::Abort*/, const FString& Reason /*= EActionFinishReason::UnKnown*/)
{
	if (!InAction)
		return;

	UpdatePawn();

	if (Actions.Contains(InAction->GetType()))
	{
		TSharedPtr<FAction> Action;
		for (auto Iter : Actions[InAction->GetType()])
		{
			if (Iter.Get() == InAction)
			{
				Action = Iter;
				break;
			}
		}
		Actions[InAction->GetType()].Remove(Action);
		if (Action.IsValid())
		{
			Action->DoFinishAction(Result);
		}
	}
}

void UActionComponent::ActionTypeChanged(FAction *InAction, EActionType OldType)
{
	if (!InAction)
		return;
	TSharedPtr<FAction> Action;
	if (Actions.Contains(OldType))
	{
		for (auto Iter : Actions[OldType])
		{
			if (Iter.Get() == InAction)
			{
				Action = Iter;
				break;
			}
		}
		if (Action.IsValid())
		{
			Actions[OldType].Remove(Action);
		}
	}
	if (Action.IsValid())
	{
		if (!Actions.Contains(Action->GetType()))
		{
			Actions.Add(Action->GetType(), TArray<TSharedPtr<FAction>>());
		}
		Actions[Action->GetType()].Add(Action);
	}
}

void UActionComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TMap<EActionType, TArray<TSharedPtr<FAction>>> TempActions = Actions;
	for (auto& Pair : TempActions)
	{
		for (auto& Action : Pair.Value)
		{
			if (Action.IsValid())
			{
				EActionResult Result = Action->DoTickAction(DeltaTime);
				if (Result != EActionResult::Wait)
				{
					FinishAction(Action.Get(), Result);
				}
			}
		}
	}
}

bool UActionComponent::UpdatePawn(bool bForce /*= false*/)
{
	if (Pawn == nullptr || bForce == true)
	{
		Pawn = Cast<APawn>(GetOwner());
		Character = Cast<ACharacter>(GetOwner());
		if (Pawn == nullptr)
		{
			AController *MyController = Cast<AController>(GetOwner());
			if (MyController)
			{
				Pawn = Cast<APawn>(MyController->GetPawn());
				Character = Cast<ACharacter>(Pawn);
			}
		}
	}

	return Pawn != NULL;
}

void UActionComponent::ExecuteAction(TSharedPtr<FAction> NewAction)
{
	if (!NewAction.IsValid())
		return;

	UpdatePawn();

	StopActionsByType(NewAction->GetType(), false);

	NewAction->SetActionComponent(this);
	NewAction->SetOwner(Pawn);
	EActionResult Result = NewAction->DoExecuteAction();
	if (Result == EActionResult::Wait)
	{
		if (!Actions.Contains(NewAction->GetType()))
		{
			Actions.Add(NewAction->GetType(), TArray<TSharedPtr<FAction>>());
		}
		Actions[NewAction->GetType()].Add(NewAction);
	}
}

void UActionComponent::StopAllAction(const FString& Reason /*= EActionFinishReason::CustomStop*/)
{
	TArray<EActionType> KeyArray;
	Actions.GetKeys(KeyArray);
	for (auto Key : KeyArray)
	{
		EActionType IsType = Key;
		FinishActionsByType(IsType, EActionResult::Abort, Reason);
	}
	Actions.Empty();
}

