// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IDelegateInstance.h"
#include "ActionComponent.generated.h"

class ACharacter;
class APawn;
class UCharacterMovementComponent;
class Action;

NEWPROJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogActionComponent, Warning, All);

UCLASS()
class NEWPROJECT_API UActionComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	friend class FAction;

public:

	virtual void Initialize();

	virtual void Cleanup();

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;

	virtual bool GetComponentClassCanReplicate() const override;

	void ExecuteAction(TSharedPtr<FAction> NewAction);
	void StopAllAction();
	void StopMoveAction();
	void StopAction(FAction *InAction);
	void StopActionsByType(EActionType InType, bool bForce = true);

	const TMap<EActionType, TArray<TSharedPtr<FAction>>> &GetAllActions() const { return Actions; }

	bool IsContainType(EActionType InType);

protected:

	void FinishActionsByType(EActionType InType, EActionResult Result = EActionResult::Abort, EActionType StopType = EActionType::Default);
	void FinishAction(FAction *InAction, EActionResult Result = EActionResult:Abort);
	void ActionTypeChanged(FAction *InAction, EActionType OldType);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	UPROPERTY(Transient)
	ACharacter *Character;

	UPROPERTY(Transient)
	APawn *Pawn;

	TMap<EActionType, TArray<TSharedPtr<FAction>>> Actions;

	TMap<FString, FAction*> SyncActions;

	virtual bool UpdatePawn(bool bForce = false);

public:
	UFUNCTION(BlueprintCallable, Category="Action")
	ACharacter *GetCharacter() const { return Character; }

	UFUNCTION(BlueprintCallable, Category="Action")
	APawn *GetPawn() const { return Pawn; }
};

