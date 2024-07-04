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
struct MoveLeftAccounts
{
	FAccountMeta GameDataAccount;
};

struct MoveLeftInstructionData
{
	TStaticArray<uint8, 8> Discriminator = { 45, 212, 186, 188, 248, 238, 45, 99 };
};

inline auto serialize(MoveLeftInstructionData& Data, borsh::Serializer& Serializer) { return Serializer(Data.Discriminator); }

struct MoveLeftInstruction : FInstruction
{
	MoveLeftInstruction(FPublicKey GameDataAccount)
	{
		ProgramId = GTinyAdventureID;
		Accounts.Add(FAccountMeta(GameDataAccount, true, false));
		MoveLeftInstructionData Data;
		TArray<uint8_t>			SerializedData = BorshSerialize(Data);
	}
};