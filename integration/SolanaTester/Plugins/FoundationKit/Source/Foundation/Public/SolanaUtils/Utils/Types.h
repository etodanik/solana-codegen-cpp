﻿/*
Copyright 2022 ATMTA, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Author: Jon Sawler
Contributers: Daniele Calanna, Riccardo Torrisi, Federico Arona
*/
#pragma once

#include "Types.generated.h"

// |  ENT  | CS | ENT+CS |  MS  |
// +-------+----+--------+------+
// |  128  |  4 |   132  |  12  |
// |  160  |  5 |   165  |  15  |
// |  192  |  6 |   198  |  18  |
// |  224  |  7 |   231  |  21  |
// |  256  |  8 |   264  |  24  |

constexpr int PublicKeySize = 32;
constexpr int PrivateKeySize = 64;

constexpr int Base58PubKeySize = 44;
constexpr int Base58PrKeySize = 88;

constexpr int AccountDataSize = 165;

const FString ComputeBudgetProgramId = "ComputeBudget111111111111111111111111111111";
const FString SystemProgramId = "11111111111111111111111111111111";
const FString TokenProgramId = "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA";
const FString SpeedrunProgramId = "5QUyb8xZ2ALyHmwtPPmjf7Yfpbtru1kkhMz4vfZuJfPs";
const FString SpeedrunAuthorityId = "2iU7W7vXQHbP5RFcRMvCVkuyNq7YrqX7KtYKfn6R1pm1";
const FString SpeedrunSolanaStatePdaId = "A35j2Rpbbhz3iAusqNmxy33fTEBKZiJwKs7N9dPH8uhv";
const FString SpeedrunSolanaSettingsPdaId = "F2GeoxfhhF1CWrwuRLjQAth3yPLTkqiG96nSuzsV6yxR";

USTRUCT()
struct FBalanceContextJson
{
	GENERATED_BODY()
	UPROPERTY()
	double slot;
};

USTRUCT()
struct FBalanceResultJson
{
	GENERATED_BODY()
	UPROPERTY()
	FBalanceContextJson context;
	UPROPERTY()
	double value;
};

USTRUCT()
struct FAccountInfoJson
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> data;
	UPROPERTY()
	bool executable;
	UPROPERTY()
	double lamports;
	UPROPERTY()
	FString owner;
	UPROPERTY()
	int32 rentEpoch;
};

USTRUCT()
struct FProgramAccountJson
{
	GENERATED_BODY()

	UPROPERTY()
	FString data;
	UPROPERTY()
	bool executable;
	UPROPERTY()
	double lamports;
	UPROPERTY()
	FString owner;
	UPROPERTY()
	double rentEpoch;
};


USTRUCT()
struct FTokenUIBalanceJson
{
	GENERATED_BODY()

	UPROPERTY()
	FString amount;
	UPROPERTY()
	double decimals;
	UPROPERTY()
	double uiAmount;
	UPROPERTY()
	FString uiAmountString;
};

USTRUCT()
struct FTokenInfoJson
{
	GENERATED_BODY()

	UPROPERTY()
	FTokenUIBalanceJson tokenAmount;
	UPROPERTY()
	FString delegate;
	UPROPERTY()
	FTokenUIBalanceJson delegatedAmount;
	UPROPERTY()
	FString state;
	UPROPERTY()
	bool isNative;
	UPROPERTY()
	FString mint;
	UPROPERTY()
	FString owner;
};

USTRUCT()
struct FParsedTokenDataJson
{
	GENERATED_BODY()

	UPROPERTY()
	FString accountType;
	UPROPERTY()
	FTokenInfoJson info;
	UPROPERTY()
	double space;
};

USTRUCT()
struct FTokenDataJson
{
	GENERATED_BODY()

	UPROPERTY()
	FString program;
	UPROPERTY()
	FParsedTokenDataJson parsed;
	UPROPERTY()
	double space;
};

USTRUCT()
struct FTokenAccountDataJson
{
	GENERATED_BODY()

	UPROPERTY()
	FTokenDataJson data;
	UPROPERTY()
	bool executable;
	UPROPERTY()
	double lamports;
	UPROPERTY()
	FString owner;
	UPROPERTY()
	double rentEpoch;
};

USTRUCT()
struct FTokenBalanceDataJson
{
	GENERATED_BODY()

	UPROPERTY()
	FTokenAccountDataJson account;
	UPROPERTY()
	FString pubkey;
};

USTRUCT()
struct FTokenAccountArrayJson
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FTokenBalanceDataJson> value;
};

UENUM(BlueprintType)
enum class EOwnableItemType : uint8
{
	None,
	Ship,
	Structure,
	Collectible,
	Access,
	Resource,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ERequestEncoding : uint8
{
	Base58,
	Base64
};

USTRUCT()
struct FOwnable
{
	GENERATED_BODY()

	FOwnable()
	{
	}

	FOwnable(const FName& InName, EOwnableItemType InItemType) : Name(InName), ItemType(InItemType)
	{
	}

	UPROPERTY()
	FName Name;
	UPROPERTY()
	FString Mint;
	UPROPERTY()
	EOwnableItemType ItemType;
};

USTRUCT(BlueprintType)
struct FArrayOfOwnable
{
	GENERATED_BODY()

	TArray<FOwnable> OwnableArray;

	static FArrayOfOwnable EmptyArrayOfOwnable;
};

USTRUCT(BlueprintType)
struct FOwnableData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<EOwnableItemType, FArrayOfOwnable> Ownables;

	TArray<FOwnable> GetAllOwnables() const
	{
		TArray<FOwnable> OutOwnables;

		TArray<FArrayOfOwnable> ArrayOfOwnables;
		Ownables.GenerateValueArray(ArrayOfOwnables);

		for (const FArrayOfOwnable& ArrayOfOwnable : ArrayOfOwnables)
		{
			OutOwnables.Append(ArrayOfOwnable.OwnableArray);
		}

		return OutOwnables;
	}
};

USTRUCT()
struct FInventoryItem
{
	GENERATED_BODY()

	FInventoryItem(): Amount(0)
	{
	}

	FInventoryItem(const FOwnable& Ownable, int64 Amount)
		: Ownable(Ownable), Amount(Amount)
	{
	}

	UPROPERTY()
	FOwnable Ownable;
	UPROPERTY()
	int64 Amount;

	bool operator==(const FInventoryItem& Other) const { return Other.Ownable.Mint == Ownable.Mint; }

	friend uint32 GetTypeHash(const FInventoryItem& Other)
	{
		return GetTypeHash(Other.Ownable.Mint);
	}
};

USTRUCT()
struct FTokenData
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;
	UPROPERTY()
	FString Mint;
	UPROPERTY()
	int64 Balance;
};

UENUM(BlueprintType)
enum class EComputeBudgetInstructionIndex : uint8
{
	RequestUnits,
	RequestHeapFrame,
	SetComputeUnitLimit,
	SetComputeUnitPrice
};
