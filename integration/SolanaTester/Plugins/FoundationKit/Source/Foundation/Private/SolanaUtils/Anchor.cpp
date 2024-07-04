#include "SolanaUtils/Anchor.h"
#include "Crypto/CryptoUtils.h"

FString ToSnakeCase(const FString& Input)
{
	FString Result;
	for (const TCHAR Character : Input)
	{
		if (FChar::IsUpper(Character))
		{
			if (!Result.IsEmpty())
			{
				Result.AppendChar('_');
			}
			Result.AppendChar(FChar::ToLower(Character));
		}
		else
		{
			Result.AppendChar(Character);
		}
	}
	return Result;
}

TArray<uint8> GetAnchorInstructionSighash(const FString& Namespace, const FString& IxName)
{
	const FString Name = ToSnakeCase(IxName);
	const FString Preimage = Namespace + TEXT(":") + Name;

	const FTCHARToUTF8 Utf8String(*Preimage);
	const uint8* PreimageBytes = reinterpret_cast<const uint8*>(Utf8String.Get());
	TArray<uint8> Hash = FCryptoUtils::SHA256_Digest(PreimageBytes, Utf8String.Length());

	TArray<uint8> Result;
	for (int32 Index = 0; Index < 8 && Index < Hash.Num(); ++Index)
	{
		Result.Add(Hash[Index]);
	}

	return Result;
}
