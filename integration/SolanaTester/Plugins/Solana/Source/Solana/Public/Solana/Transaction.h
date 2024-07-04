/**
 * Originally derived from https://github.com/staratlasmeta/FoundationKit
 * Original Author: Jon Sawler
 * License: https://www.apache.org/licenses/LICENSE-2.0
**/
#pragma once

class FPublicKey;
struct FAccount;
struct FAccountMeta;
struct FInstruction;

class SOLANA_API FTransaction
{
public:
	FTransaction(const FString& CurrentBlockHash);
	
	void AddInstruction(const FInstruction& Instruction);
	void AddInstructions(const TArray<FInstruction>& InInstructions);

	TArray<uint8> Build(const FAccount& Signer);
	TArray<uint8> Build(const TArray<FAccount>& Signers);

	static TArray<uint8> Sign(const TArray<uint8>& Message, const TArray<FAccount>& Signers);

private:
	TArray<uint8> BuildMessage();
	TArray<uint8> CompileInstructions();

	void UpdateAccountList(const TArray<FAccount>& Signers);
	void UpdateHeaderInfo(const FAccountMeta& AccountMeta);

	uint8 GetAccountIndex(const FPublicKey& Key) const;

	TArray<FInstruction> Instructions;
	TArray<FAccountMeta> AccountList;

	FString BlockHash;

	uint8 RequiredSignatures;
	uint8 ReadOnlySignedAccounts;
	uint8 ReadOnlyUnsignedAccounts;
};
