// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "UnrealString.h"

namespace EActionFinishReason {
	const static FString UnKnown(TEXT("UnKnown"));
	const static FString UEInternalStop(TEXT("UEInternalStop"));
	const static FString CustomStop(TEXT("CustomStop"));
};

UENUM(BlueprintType)
enum class EAction_AnimationPriority : uint8
{
	Chusheng		UMETA(DisplayName = "出场动作"),
	Normal			UMETA(DisplayName = "技能动作 交互动作"),
	Behit			UMETA(DisplayName = "受击动作"),
	TurnBody		UMETA(DisplayName = "转身动作"),
	IdleLeisure		UMETA(DisplayName = "休闲动作"),

	Last			UMETA(Hidden)

};

UENUM(BlueprintType)
enum class EActionResult : uint8
{
	Wait,
	Success,
	Fail,
	Abort,
	Clean
};

enum class EActionType
{
	Default		= 0x0000,
	Move		= 0x0001,
	Rotate		= 0x0002,
	Animation	= 0x0004,
	Scale		= 0x0008,
	MeshMove	= 0x0010,
	MeshRotate  = 0x0020,
	MeshScale	= 0x0040
};

FORCEINLINE EActionType operator|(EActionType Arg1, EActionType Arg2) { return EActionType(uint32(Arg1) | uint32(Arg2)); }
FORCEINLINE EActionType operator&(EActionType Arg1, EActionType Arg2) { return EActionType(uint32(Arg1) & uint32(Arg2)); }
FORCEINLINE void operator&=(EActionType &Dest, EActionType Arg) { Dest = EActionType(Dest & Arg); }
FORCEINLINE void operator|=(EActionType &Dest, EActionType Arg) { Dest = EActionType(Dest | Arg); }

