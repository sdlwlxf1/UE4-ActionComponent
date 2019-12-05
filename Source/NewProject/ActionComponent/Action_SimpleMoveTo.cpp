// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Action_SimpleMoveTo.h"
#include "VisualLogger.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AI/Navigation/NavigationTypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationQueryFilter.h"
#include "NavigationSystemTypes.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY(LogAction_SimpleMoveTo);
DECLARE_CYCLE_STAT(TEXT("MoveTo"), STAT_MoveTo, STATGROUP_AI);

TSharedPtr<FAction_SimpleMoveTo> FAction_SimpleMoveTo::CreateAction(AActor* InGoal /*= nullptr*/, float InMaxSpeed /*= -1*/, float InAcceptanceRadius /*= 1.0f*/, bool InbUsePathfinding /*= false*/, bool InbUsePathCost /*= false*/, bool InbMoveWithAccelerate /*= false*/, bool InbWithOutControl /*= false*/, FAIMoveRequest * ExtraMoveRequest /*= nullptr*/)
{
	if (InGoal == NULL)
		return NULL;

	TSharedPtr<FAction_SimpleMoveTo> Action = MakeShareable(new FAction_SimpleMoveTo());
	if (Action.IsValid())
	{
		Action->Goal = InGoal;
		Action->MaxSpeed = InMaxSpeed;
		Action->AcceptanceRadius = InAcceptanceRadius;
		Action->bUsePathfinding = InbUsePathfinding;
		Action->bMoveWithAccelerate = InbMoveWithAccelerate;
		Action->bUsePathCoat = InbUsePathCost;
		Action->AIMoveRequest = ExtraMoveRequest;
	}
	return Action;
}

TSharedPtr<FAction_SimpleMoveTo> FAction_SimpleMoveTo::CreateAction(const FVector& InDest, float InMaxSpeed /*= -1*/, float InAcceptanceRadius /*= 1.0f*/, bool InbUsePathfinding /*= false*/, bool InbUsePathCost /*= false*/, bool InbMoveWithAccelerate /*= false*/, bool InbWithOutControl /*= false*/, FAIMoveRequest * ExtraMoveRequest /*= nullptr*/)
{
	TSharedPtr<FAction_SimpleMoveTo> Action = MakeShareable(new FAction_SimpleMoveTo());
	if (Action.IsValid())
	{
		Action->Dest = InDest;
		Action->MaxSpeed = InMaxSpeed;
		Action->AcceptanceRadius = InAcceptanceRadius;
		Action->bUsePathfinding = InbUsePathfinding;
		Action->bMoveWithAccelerate = InbMoveWithAccelerate;
		Action->bUsePathCoat = InbUsePathCost;
		Action->AIMoveRequest = ExtraMoveRequest;
	}
	return Action;
}

EActionResult FAction_SimpleMoveTo::ExecuteAction()
{
	SCOPE_CYCLE_COUNTER(STAT_MoveTo);

	Character = Cast<ACharacter>(GetOwner());
	if (Character.IsValid())
	{
		MovementComp = Character->GetCharacterMovement();
		//PathFollowingComponent = Character->GetPathF();
	}

	if (!MovementComp.IsValid() || !PathFollowingComponent.IsValid())
	{
		return EActionResult::Fail;
	}

	MoveRequest.SetReachTestIncludesAgentRadius(false);

	if (AIMoveRequest != nullptr)
	{
		MoveRequest = *AIMoveRequest;
	}

	if (Goal.IsValid())
	{
		MoveRequest.SetGoalActor(Goal.Get());
		NotifyTypeChanged();
	}
	else
	{
		MoveRequest.SetGoalLocation(Dest);
	}
	MoveRequest.SetUsePathfinding(bUsePathfinding);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);

	UE_VLOG(GetOwner(), LogAction_SimpleMoveTo, Log, TEXT("MoveTo: %s"), *MoveRequest.ToString());
	if (MoveRequest.IsValid() == false)
	{
		UE_VLOG(GetOwner(), LogAction_SimpleMoveTo, Error, TEXT("MoveTo request failed due MoveRequest not being valid. Most probably desire Goal Actor not longer exists"), *MoveRequest.ToString());
	}
	else
	{
		ensure(MoveRequest.GetNavigationFilter() || !DefaultNavigationFilterClass);

		bool bCanRequestMove = true;
		bool bAlreadyAtGoal = false;

		if (!MoveRequest.IsMoveToActorRequest())
		{
			if (MoveRequest.GetGoalLocation().ContainsNaN() || FAISystem::IsValidLocation(MoveRequest.GetGoalLocation()) == false)
			{
				UE_VLOG(GetOwner(), LogAction_SimpleMoveTo, Error, TEXT("Destination is not valid! Goal(%s)"), TEXT_AI_LOCATION(MoveRequest.GetGoalLocation()));
				bCanRequestMove = false;
			}

			if (bCanRequestMove && MoveRequest.IsProjectingGoal())
			{
				FNavLocation ProjectedLocation;
				ProjectedLocation.Location = MoveRequest.GetGoalLocation();
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetOwner()->GetWorld());
				const FNavAgentProperties& AgentProps = Character->GetNavAgentPropertiesRef();
				if (NavSys && !NavSys->ProjectPointToNavigation(MoveRequest.GetGoalLocation(), ProjectedLocation, INVALID_NAVEXTENT, &AgentProps))
				{
					UE_VLOG_LOCATION(GetOwner(), LogAction_SimpleMoveTo, Error, MoveRequest.GetGoalLocation(), 30.f, FColor::Red, TEXT("MoveTo failed to project destination location to navmesh"));
				}

				MoveRequest.UpdateGoalLocation(ProjectedLocation.Location);
			}

			bAlreadyAtGoal = bCanRequestMove && PathFollowingComponent->HasReached(MoveRequest);
		}
		else
		{
			bAlreadyAtGoal = bCanRequestMove && PathFollowingComponent->HasReached(MoveRequest);
		}

		if (bAlreadyAtGoal)
		{
			UE_VLOG(GetOwner(), LogAction_SimpleMoveTo, Log, TEXT("MoveTo : already at goal!"));
			PathFollowingComponent->RequestMoveWithImmediateFinish(EPathFollowingResult::Success);
			return EActionResult::Success;
		}
		else if (bCanRequestMove)
		{
			FPathFindingQuery PFQuery;

			const bool bValidQuery = BuildPathfindingQuery(MoveRequest, PFQuery);
			if (bValidQuery)
			{
				FNavPathSharedPtr Path;
				FindPathForMoveRequest(MoveRequest, PFQuery, Path);

				const FAIRequestID RequestID = Path.IsValid() && PathFollowingComponent.IsValid() ? PathFollowingComponent->RequestMove(MoveRequest, Path) : FAIRequestID::InvalidRequest;

				if (RequestID.IsValid())
				{
					if (AIMoveRequest != nullptr)
					{
						*AIMoveRequest = MoveRequest;
					}

					bAllowStrafe = MoveRequest.CanStrafe();
					StorgeMoveMaxSpeed = MovementComp->GetMaxSpeed();
					/*
					if (MaxSpeed > 0)
					{
						MovementComp->SetMaxSpeed(MaxSpeed);
					}
					if (MovementComp->GetMaxSpeed() <= 0.0f)
					{
						MovementComp->SetMaxSpeed(0.1f);
					}
					*/

					bStorgeRequestMoveWithAccelerate = MovementComp->bRequestedMoveUseAcceleration;
					bStorgeFindPathWithAccelerate = MovementComp->UseAccelerationForPathFollowing();
					MovementComp->bRequestedMoveUseAcceleration = bMoveWithAccelerate;

					FinishedHandle = PathFollowingComponent->OnRequestFinished.AddLambda([this](FAIRequestID RequestID, const FPathFollowingResult& Result) {
						if (RequestID.IsValid())
						{
							if (Result.IsSuccess() || Result.HasFlag(FPathFollowingResultFlags::NewRequest) || Result.HasFlag(FPathFollowingResultFlags::ForcedScript))
							{
								NotifyActionFinish(EActionResult::Success, EActionFinishReason::UEInternalStop);
							}
							else
							{
								NotifyActionFinish(EActionResult::Fail, EActionFinishReason::UEInternalStop);
							}
						}
					});
					return EActionResult::Wait;
				}
			}
		}
	}

	PathFollowingComponent->RequestMoveWithImmediateFinish(EPathFollowingResult::Invalid);
	return EActionResult::Fail;
}

bool FAction_SimpleMoveTo::FinishAction(EActionResult InResult, const FString& Reason /*= EActionFinishReason::UnKnown*/, EActionType StopType /*= EActionType::Default*/)
{
	PathFollowingComponent->OnRequestFinished.Remove(FinishedHandle);
	if (InResult == EActionResult::Abort)
	{
		if (PathFollowingComponent.IsValid() && PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle)
		{
			PathFollowingComponent->AbortMove(*GetOwner(), FPathFollowingResultFlags::ForcedScript | FPathFollowingResultFlags::NewRequest,
				FAIRequestID::CurrentRequest, EPathFollowingVelocityMode::Reset);
		}
	}
	MovementComp->bRequestedMoveUseAcceleration = bStorgeRequestMoveWithAccelerate;
	// SetUseAccelerationForPaths
	// SetMaxSpeed
	// LockBehavior
	// bAllowStrafe
	return true;
}

EActionResult FAction_SimpleMoveTo::TickAction(float DeltaTime)
{
	return EActionResult::Wait;
}

void FAction_SimpleMoveTo::UpdateType()
{
	Type = EActionType::Move;
	if (Goal.IsValid())
	{
		Type = (EActionType::Move | EActionType::Rotate);
	}
}

FName FAction_SimpleMoveTo::GetName() const
{
	return TEXT("Action_SimpleMoveTo");
}

FString FAction_SimpleMoveTo::GetDescription() const
{
	return FString::Printf(TEXT("%s (Goal:(%s))"), *GetName().ToString(), *(Goal.IsValid() ? Goal->GetName() : Dest.ToString()));
}

void FAction_SimpleMoveTo::FindPathForMoveRequest(const FAIMoveRequest& InMoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Overall);

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetOwner()->GetWorld());
	if (NavSys)
	{
		FPathFindingResult PathResult = NavSys->FindPathSync(Query);
		if (PathResult.Result != ENavigationQueryResult::Error)
		{
			if (PathResult.IsSuccessful() && PathResult.Path.IsValid())
			{
				if (MoveRequest.IsMoveToActorRequest())
				{
					PathResult.Path->SetGoalActorObservation(*MoveRequest.GetGoalActor(), 100.0f);
				}

				PathResult.Path->EnableRecalculationOnInvalidation(true);
				OutPath = PathResult.Path;
			}
		}
		else
		{
			UE_VLOG(GetOwner(), LogAction_SimpleMoveTo, Error, TEXT("Trying to find path to %s resulted in Error")
				, MoveRequest.IsMoveToActorRequest() ? *GetNameSafe(MoveRequest.GetGoalActor()) : *MoveRequest.GetGoalLocation().ToString());
			UE_VLOG_SEGMENT(GetOwner(), LogAction_SimpleMoveTo, Error, Character.IsValid() ? Character->GetActorLocation() : FAISystem::InvalidLocation
				, MoveRequest.GetGoalLocation(), FColor::Red, TEXT("Failed move to %s"), *GetNameSafe(MoveRequest.GetGoalActor()));
		}
	}

}

bool FAction_SimpleMoveTo::BuildPathfindingQuery(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query) const
{
	bool bResult = false;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetOwner()->GetWorld());
	const ANavigationData* NavData = (NavSys == nullptr) ? nullptr :
		MoveRequest.IsUsingPathfinding() ? NavSys->GetNavDataForProps(Character->GetNavAgentPropertiesRef(), Character->GetNavAgentLocation()) :
		NavSys->GetAbstractNavData();

	if (NavData)
	{
		FVector GoalLocation = MoveRequest.GetGoalLocation();
		if (MoveRequest.IsMoveToActorRequest())
		{
			const INavAgentInterface* NavGoal = Cast<const INavAgentInterface>(MoveRequest.GetGoalActor());
			if (NavGoal)
			{
				const FVector Offset = NavGoal->GetMoveGoalOffset(GetOwner());
				GoalLocation = FQuatRotationTranslationMatrix(MoveRequest.GetGoalActor()->GetActorQuat(), NavGoal->GetNavAgentLocation()).TransformPosition(Offset);
			}
			else
			{
				GoalLocation = MoveRequest.GetGoalActor()->GetActorLocation();
			}
		}

		FSharedConstNavQueryFilter NavFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, GetOwner(), MoveRequest.GetNavigationFilter());
		Query = FPathFindingQuery(GetOwner(), *NavData, Character->GetNavAgentLocation(), GoalLocation, NavFilter);
		Query.SetAllowPartialPaths(MoveRequest.IsUsingPartialPaths());

		if (PathFollowingComponent.IsValid())
		{
			PathFollowingComponent->OnPathfindingQuery(Query);
		}

		bResult = true;
	}
	else
	{
		UE_VLOG(GetOwner(), LogAction_SimpleMoveTo, Warning, TEXT("Unable to find NavigationData instance while calling AAIController::BuildPathfindingQuery"));
	}

	return bResult;
}
