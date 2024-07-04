/**
 * This code was AUTOGENERATED using the solana-codegen-cpp library.
 * Please DO NOT EDIT THIS FILE, instead use visitors to add features,
 * then rerun solana-codegen-cpp to update it.
 *
 * @see https://github.com/etodanik/solana-codegen-cpp
 */

#pragma once

#include "Containers/StaticArray.h"
#include "Solana/AccountMeta.h"
#include "Solana/Instruction.h"
#include "Solana/PublicKey.h"
#include "SolanaProgram/Programs.h"
#include "Borsh/Borsh.h"

// Accounts.
struct InitializeAccounts
{
	FAccountMeta NewGameDataAccount;
	FAccountMeta Signer;
	FAccountMeta SystemProgram;
};

struct InitializeInstructionData
{
	TStaticArray<uint8, 8> Discriminator = { 175, 175, 109, 31, 13, 152, 155, 237 };
};

inline auto serialize(InitializeInstructionData& Data, borsh::Serializer& Serializer) { return Serializer(Data.Discriminator); }

struct InitializeInstruction : FInstruction
{
	InitializeInstruction(FPublicKey GameDataAccount)
	{
		ProgramId = GTinyAdventureID;
		Accounts.Add(FAccountMeta(GameDataAccount, true, false));
		InitializeInstructionData Data;
		TArray<uint8_t>			  SerializedData = BorshSerialize(Data);
	}
};
