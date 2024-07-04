#pragma once

#include "CoreMinimal.h"

extern inline const FString StateNamespace = TEXT("state");
extern inline const FString GlobalNamespace = TEXT("global");

TArray<uint8> GetAnchorInstructionSighash(const FString& Namespace, const FString& IxName);
