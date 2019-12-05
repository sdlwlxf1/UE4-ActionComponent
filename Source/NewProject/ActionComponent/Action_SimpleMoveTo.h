// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Action_MoveTo.h"
#include "AITypes.h"

class ACharacter;
class UPathFollowingComponent;
struct FPathFindingQuery;
class UNavigationQueryFilter;

NEWPROJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogAction_SimpleMoveTo, Warning, All);

class NEWPROJECT_API FAction_SimpleMoveTo : public FAction_MoveTo
{
public:
	static TSharedPtr<FAction_SimpleMoveTo> CreateAction(AActor* InGoal = nullptr, float InMaxSpeed = -1, float InAcceptanceRadius = 1.0f, bool InbUsePathfinding = false, bool InbUsePathCost = false, bool InbMoveWithAccelerate = false, bool InbWithOutControl = false, FAIMoveRequest * ExtraMoveRequest = nullptr);
	static TSharedPtr<FAction_SimpleMoveTo> CreateAction(const FVector& InDest, float InMaxSpeed = -1, float InAcceptanceRadius = 1.0f, bool InbUsePathfinding = false, bool InbUsePathCost = false, bool InbMoveWithAccelerate = false, bool InbWithOutControl = false, FAIMoveRequest * ExtraMoveRequest = nullptr);

	virtual EActionResult ExecuteAction() override;
	virtual bool FinishAction(EActionResult InResult, const FString& Reason = EActionFinishReason::UnKnown, EActionType StopType = EActionType::Default) override;
	virtual EActionResult TickAction(float DeltaTime) override;
	virtual void UpdateType() override;

	TSubclassOf<UNavigationQueryFilter> DefaultNavigationFilterClass;

	TSubclassOf<UNavigationQueryFilter> FilterClass;

	FVector Dest;
	float MaxSpeed;
	float AcceptanceRadius;
	bool bUsePathfinding = false;
	bool bWithOutControl = false;
	bool bMoveWithAccelerate = true;

	bool bUsePathCoat = false;
	FAIMoveRequest* AIMoveRequest = nullptr;

	TWeakObjectPtr<AActor> Goal;
	FAIMoveRequest MoveRequest;

	virtual FName GetName() const override;
	virtual FString GetDescription() const override;

protected:
	uint32 bAllowStrafe : 1;

	virtual void FindPathForMoveRequest(const FAIMoveRequest& InMoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const;
	bool BuildPathfindingQuery(const FAIMoveRequest& InMoveRequest, FPathFindingQuery& Query) const;

	TWeakObjectPtr<UPathFollowingComponent> PathFollowingComponent;

private:
	TWeakObjectPtr<ACharacter> Character;

	FDelegateHandle FinishedHandle;

	bool bStorgeRequestMoveWithAccelerate;
	bool bStorgeFindPathWithAccelerate;
};
