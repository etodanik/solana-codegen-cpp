#pragma once
#include "PublicKey.h"

/**
* Describes a single account read or written by a program during instruction execution.
* When constructing an Instruction, a list of all accounts that may be read or written during the execution of that instruction must
* be supplied. Any account that may be mutated by the program during execution, either its data or metadata such as held lamports,
* must be writable.
* 
* Note that because the Solana runtime schedules parallel transaction execution around which accounts are writable,
* care should be taken that only accounts which actually may be mutated are specified as writable.
 */
struct FAccountMeta
{
	// An account’s public key.
	FPublicKey Key;
	// True if an Instruction requires a Transaction signature matching pubkey.
	bool IsSigner;
	// True if the account data or metadata may be mutated during program execution.
	bool IsWritable;

	FAccountMeta(FPublicKey InKey, bool InIsSigner, bool InIsWriteable);
	FAccountMeta(TArray<uint8> InKeyData, bool InIsSigner, bool InIsWriteable);
};