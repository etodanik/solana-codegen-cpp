#include "Solana/PublicKey.h"

#include "Crypto/Base58.h"

TArray<uint8_t> FPublicKey::DecodeBase58() const
{
	const FPublicKey Self = *this;
	return FBase58::DecodeBase58(Self);
}

FPublicKey::FPublicKey(FString String)
	: FString(String) {}