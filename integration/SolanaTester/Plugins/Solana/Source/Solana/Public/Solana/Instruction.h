#pragma once
#include "AccountMeta.h"

struct FInstruction
{
	FPublicKey ProgramId;
	TArray<FAccountMeta> Accounts;
	TArray<uint8> Data;
};
