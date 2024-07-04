#pragma once

class SOLANA_API FPublicKey : public FString
{
public:
	using FString::FString;
	FPublicKey(FString String);
	TArray<uint8_t> DecodeBase58() const;
};