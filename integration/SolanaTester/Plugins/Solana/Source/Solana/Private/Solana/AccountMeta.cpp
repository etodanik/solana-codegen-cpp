#include "Solana/AccountMeta.h"

#include "Crypto/Base58.h"

FAccountMeta::FAccountMeta(FPublicKey InKey, bool InIsSigner, bool InIsWriteable)
	: Key(InKey)
	, IsSigner(InIsSigner)
	, IsWritable(InIsWriteable) {}


FAccountMeta::FAccountMeta(TArray<uint8> InKeyData, bool InIsSigner, bool InIsWriteable)
	: Key(FBase58::EncodeBase58(InKeyData))
	, IsSigner(InIsSigner)
	, IsWritable(InIsWriteable) {}