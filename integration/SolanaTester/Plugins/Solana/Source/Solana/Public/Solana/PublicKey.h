#pragma once

class FPublicKey : FString
{
public:
	using FString::FString;
	FPublicKey(FString String);
	TArray<uint8_t> DecodeBase58() const;
};