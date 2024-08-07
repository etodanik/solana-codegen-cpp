/**
* Originally derived from https://github.com/staratlasmeta/FoundationKit
 * Original Author: Jon Sawler
 * License: https://www.apache.org/licenses/LICENSE-2.0
**/

#include "Solana/Transaction.h"
#include "Solana/Instruction.h"
#include "Solana/PublicKey.h"
#include "SolanaUtils/Account.h"
#include "Crypto/Base58.h"
#include "Crypto/CryptoUtils.h"

FTransaction::FTransaction(const FString& CurrentBlockHash)
{
	BlockHash = CurrentBlockHash;

	RequiredSignatures = 0;
	ReadOnlySignedAccounts = 0;
	ReadOnlyUnsignedAccounts = 0;
}

void FTransaction::AddInstruction(const FInstruction& Instruction)
{
	Instructions.Add(Instruction);
	for (const FAccountMeta& data : Instruction.Accounts)
	{
		const int index = AccountList.IndexOfByPredicate([data](const FAccountMeta& entry) {
			return data.Key.DecodeBase58() == entry.Key.DecodeBase58();
		});
		if (index == INDEX_NONE) { AccountList.Add(data); }
		else { if (data.IsWritable && !AccountList[index].IsWritable) { AccountList[index] = data; } }
	}
}

void FTransaction::AddInstructions(const TArray<FInstruction>& InInstructions)
{
	Instructions.Append(InInstructions);
	for (const FInstruction& Instruction : InInstructions) { AddInstruction(Instruction); }
}

uint8 FTransaction::GetAccountIndex(const FPublicKey& Key) const
{
	return AccountList.IndexOfByPredicate([Key](const FAccountMeta& data) {
		// TODO: Allow == on FPublicKey (resolve inheritance ambiguity) 
		return data.Key.DecodeBase58() == Key.DecodeBase58();
	});
}

TArray<uint8> FTransaction::Build(const FAccount& Signer)
{
	TArray<FAccount> Signers;
	Signers.Add(Signer);
	return Build(Signers);
}

TArray<uint8> FTransaction::Build(const TArray<FAccount>& Signers)
{
	UpdateAccountList(Signers);

	const TArray<uint8> Message = BuildMessage();

	TArray<uint8> Result = Sign(Message, Signers);
	Result.Append(Message);

	return Result;
}

void FTransaction::UpdateAccountList(const TArray<FAccount>& Signers)
{
	for (const FAccount& Account : Signers)
	{
		int Index = 0;
		while (Index != INDEX_NONE)
		{
			Index = AccountList.IndexOfByPredicate([Account](const FAccountMeta& entry) {
				return Account.PublicKeyData == entry.Key.DecodeBase58();
			});
			if (Index != INDEX_NONE) { AccountList.RemoveAt(Index); }
		}
	}

	// Sort write-ables to the top of list before reading Signers at the very top
	AccountList.Sort([](const FAccountMeta& A, const FAccountMeta& B) { return A.IsWritable && !B.IsWritable; });

	for (int i = 0; i < Signers.Num(); i++) { AccountList.Insert(FAccountMeta(Signers[i].PublicKeyData, true, true), i); }
}

TArray<uint8> FTransaction::BuildMessage()
{
	TArray<uint8> AccountKeysBuffer;
	for (FAccountMeta& AccountMeta : AccountList)
	{
		// Do we need to remove fee payer here????
		AccountKeysBuffer.Append(AccountMeta.Key.DecodeBase58());
		UpdateHeaderInfo(AccountMeta);
	}

	TArray<uint8> Buffer;
	Buffer.Add(RequiredSignatures);
	Buffer.Add(ReadOnlySignedAccounts);
	Buffer.Add(ReadOnlyUnsignedAccounts);

	Buffer.Append(FCryptoUtils::ShortVectorEncodeLength(AccountList.Num()));
	Buffer.Append(AccountKeysBuffer);

	Buffer.Append(FBase58::DecodeBase58(BlockHash));

	const TArray<uint8> CompiledInstructions = CompileInstructions();
	Buffer.Append(FCryptoUtils::ShortVectorEncodeLength(Instructions.Num()));
	Buffer.Append(CompiledInstructions);

	return Buffer;
}

TArray<uint8> FTransaction::CompileInstructions()
{
	TArray<uint8> Result;

	for (FInstruction& Instruction : Instructions)
	{
		const int     KeyCount = Instruction.Accounts.Num() - 1;
		TArray<uint8> KeyIndicies;
		KeyIndicies.SetNum(KeyCount);

		for (int i = 0; i < KeyCount; i++) { KeyIndicies[i] = GetAccountIndex(Instruction.Accounts[i].Key); }

		Result.Add(GetAccountIndex(
			FBase58::EncodeBase58(Instruction.ProgramId.DecodeBase58().GetData(), Instruction.ProgramId.DecodeBase58().Num())));
		Result.Append(FCryptoUtils::ShortVectorEncodeLength(KeyCount));
		Result.Append(KeyIndicies);
		Result.Append(FCryptoUtils::ShortVectorEncodeLength(Instruction.Data.Num()));
		Result.Append(Instruction.Data);
	}

	return Result;
}

void FTransaction::UpdateHeaderInfo(const FAccountMeta& AccountMeta)
{
	if (AccountMeta.IsSigner)
	{
		RequiredSignatures += 1;
		if (!AccountMeta.IsWritable) { ReadOnlySignedAccounts += 1; }
	}
	else { if (!AccountMeta.IsWritable) { ReadOnlyUnsignedAccounts += 1; } }
}

TArray<uint8> FTransaction::Sign(const TArray<uint8>& Message, const TArray<FAccount>& Signers)
{
	TArray<uint8> Signatures;

	Signatures.Append(FCryptoUtils::ShortVectorEncodeLength(Signers.Num()));

	for (FAccount Signer : Signers) { Signatures.Append(Signer.Sign(Message)); }

	return Signatures;
}