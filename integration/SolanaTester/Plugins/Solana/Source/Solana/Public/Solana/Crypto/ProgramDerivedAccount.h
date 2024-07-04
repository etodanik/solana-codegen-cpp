#pragma once

#include "CoreMinimal.h"

class FOUNDATION_API FProgramDerivedAccount
{
public:
	static TTuple<FString, int32> FindProgramAddress(const TArray<TArray<uint8>>& Seeds, const TArray<uint8>& ProgramId);
	static TTuple<FString, int32> FindProgramAddress(const TArray<FString>& Seeds, const TArray<uint8>& ProgramId);
	static TArray<uint8> StringToByteArray(FString InString);

private:
	static FString CreateProgramAddress(const TArray<TArray<uint8>>& Seeds, TArray<uint8> ProgramId);
	static bool IsOnCurve(TArray<uint8> HashOutput);
};
