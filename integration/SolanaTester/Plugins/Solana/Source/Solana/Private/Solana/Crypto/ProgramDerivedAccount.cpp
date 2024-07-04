#include "Solana/Crypto/ProgramDerivedAccount.h"
#include "CoreMinimal.h"
#include "Crypto/CryptoUtils.h"
#include "Crypto/Base58.h"

FString BytesToHexString(const uint8* Bytes, int32 Length)
{
	FString Result;
	for (int32 i = 0; i < Length; ++i)
	{
		Result += FString::Printf(TEXT("%02x"), Bytes[i]);
	}
	return Result;
}

FString FProgramDerivedAccount::CreateProgramAddress(const TArray<TArray<uint8>>& Seeds, TArray<uint8> ProgramId)
{
	constexpr int32 MaxSeedLength = 32;
	TArray<uint8> Buffer;

	for (const TArray<uint8>& Seed : Seeds)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Seed length: %d"), Seed.Num());
		if (Seed.Num() > MaxSeedLength)
		{
			UE_LOG(LogTemp, Error, TEXT("Max seed length exceeded"));
			return FString();
		}

		Buffer.Append(Seed);
	}

	Buffer.Append(ProgramId);

	const FString PDAString = TEXT("ProgramDerivedAddress");
	TArray<uint8> PDABytes;
	const auto UTF8PDAString = StringCast<ANSICHAR>(*PDAString);
	PDABytes.Append(reinterpret_cast<const uint8*>(UTF8PDAString.Get()), UTF8PDAString.Length());
	Buffer.Append(PDABytes);

	auto HashOutput = FCryptoUtils::SHA256_Digest(Buffer.GetData(), Buffer.Num());

	if (IsOnCurve(HashOutput))
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Invalid seeds, address must fall off the curve"));
		return FString();
	}

	return FBase58::EncodeBase58(HashOutput.GetData(), HashOutput.Num());
}

bool FProgramDerivedAccount::IsOnCurve(const TArray<uint8> &HashOutput)
{
	return FCryptoUtils::IsPointOnCurve(HashOutput);
}

TTuple<FString, int32> FProgramDerivedAccount::FindProgramAddress(const TArray<TArray<uint8>>& Seeds, const TArray<uint8>& ProgramId)
{
	int32 Nonce = 255;

	while (Nonce >= 0)
	{
		TArray<TArray<uint8>> SeedsWithNonce = Seeds;
		TArray<uint8> NonceArray = {static_cast<uint8>(Nonce)};
		SeedsWithNonce.Add(NonceArray);

		FString Address = CreateProgramAddress(SeedsWithNonce, ProgramId);
		if (!Address.IsEmpty())
		{
			return MakeTuple(Address, Nonce);
		}

		Nonce--;
	}

	UE_LOG(LogTemp, Error, TEXT("Unable to find a viable program address nonce"));
	return MakeTuple(FString(), -1);
}

TTuple<FString, int32> FProgramDerivedAccount::FindProgramAddress(const TArray<FString>& Seeds, const TArray<uint8>& ProgramId)
{
	TArray<TArray<uint8>> SeedByteArrays;

	for (const FString& Seed : Seeds)
	{
		SeedByteArrays.Add(StringToByteArray(Seed));
	}

	return FindProgramAddress(SeedByteArrays, ProgramId);
}

TArray<uint8> FProgramDerivedAccount::StringToByteArray(FString InString)
{
	const auto UTF8String = StringCast<ANSICHAR>(*InString);
	const ANSICHAR* UTF8CharArray = UTF8String.Get();

	TArray<uint8> ByteArray;
	for (int32 Index = 0; Index < UTF8String.Length(); ++Index)
	{
		ByteArray.Add(static_cast<uint8>(UTF8CharArray[Index]));
	}

	return ByteArray;
}
