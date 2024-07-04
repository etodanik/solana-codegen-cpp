#include "Solana/AccountMeta.h"

#include "Crypto/Base58.h"

FAccountMeta::FAccountMeta(FPublicKey InKey, bool InIsSigner, bool InisWritable)
	: Key(InKey)
	, IsSigner(InIsSigner)
	, IsWritable(InisWritable) {}


FAccountMeta::FAccountMeta(TArray<uint8> InKeyData, bool InIsSigner, bool InisWritable)
	: Key(FBase58::EncodeBase58(InKeyData))
	, IsSigner(InIsSigner)
	, IsWritable(InisWritable) {}