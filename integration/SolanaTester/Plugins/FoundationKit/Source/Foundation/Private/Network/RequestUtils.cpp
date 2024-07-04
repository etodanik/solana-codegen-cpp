/*
Copyright 2022 ATMTA, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Author: Jon Sawler
*/

#include "Network/RequestUtils.h"

#include "JsonObjectConverter.h"
#include "Network/RequestManager.h"
#include "Misc/MessageDialog.h"
#include "SolanaUtils/Utils/Types.h"

static FText ErrorTitle = FText::FromString("Error");
static FText InfoTitle = FText::FromString("Info");

TSharedPtr<FRequestData> FRequestUtils::RequestAccountInfo(const FString& PubKey,
                                                           ERequestEncoding encoding = ERequestEncoding::Base58)
{
	auto Request = MakeShared<FRequestData>();

	Request->Body =
		FString::Printf(
			TEXT(R"({"jsonrpc":"2.0","id":%u,"method":"getAccountInfo","params":["%s",{"encoding": "%s"}]})")
			, Request->Id, *PubKey, encoding == ERequestEncoding::Base58 ? TEXT("base58") : TEXT("base64"));

	return Request;
}

FAccountInfoJson FRequestUtils::ParseAccountInfoResponse(const FJsonObject& data)
{
	FAccountInfoJson JsonData;
	if (const TSharedPtr<FJsonObject> Result = data.GetObjectField("result"))
	{
		const TSharedPtr<FJsonObject> ResultData = Result->GetObjectField("value");

		FString OutputString;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ResultData.ToSharedRef(), Writer);
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Blue, OutputString);

		FJsonObjectConverter::JsonObjectToUStruct(ResultData.ToSharedRef(), &JsonData);

		const FString Out = FString::Printf(
			TEXT("lamports: %f, owner: %s, rentEpoch: %i"), JsonData.lamports, *JsonData.owner, JsonData.rentEpoch);
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Blue, Out);
	}
	return JsonData;
}

TSharedPtr<FRequestData> FRequestUtils::RequestAccountBalance(const FString& PubKey)
{
	auto Request = MakeShared<FRequestData>();

	Request->Body =
		FString::Printf(
			TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"getBalance","params":["%s",{"commitment": "processed"}]})"),
			Request->Id, *PubKey);

	return Request;
}


double FRequestUtils::ParseAccountBalanceResponse(const FJsonObject& data)
{
	FBalanceResultJson JsonData;
	if (const TSharedPtr<FJsonObject> Result = data.GetObjectField("result"))
	{
		FJsonObjectConverter::JsonObjectToUStruct(Result.ToSharedRef(), &JsonData);
		return JsonData.value;
	}
	return -1;
}

TSharedPtr<FRequestData> FRequestUtils::RequestTokenAccount(const FString& PubKey, const FString& mint)
{
	auto Request = MakeShared<FRequestData>();

	Request->Body =
		FString::Printf(
			TEXT(
				R"({"jsonrpc":"2.0","id":%d,"method":"getTokenAccountsByOwner","params":["%s",{"mint": "%s"},{"encoding": "jsonParsed"}]})")
			, Request->Id, *PubKey, *mint);

	return Request;
}

FString FRequestUtils::ParseTokenAccountResponse(const FJsonObject& data)
{
	FTokenAccountArrayJson JsonData = ParseAllTokenAccountsResponse(data);

	FString Result;
	if (!JsonData.value.IsEmpty())
	{
		Result = JsonData.value[0].pubkey;
	}
	return Result;
}

TSharedPtr<FRequestData> FRequestUtils::RequestAllTokenAccounts(const FString& PubKey, const FString& ProgramId)
{
	auto Request = MakeShared<FRequestData>();

	Request->Body =
		FString::Printf(
			TEXT(
				R"({"jsonrpc":"2.0","id":%d,"method":"getTokenAccountsByOwner","params":["%s",{"programId": "%s"},{"encoding": "jsonParsed"}]})")
			, Request->Id, *PubKey, *ProgramId);

	return Request;
}

FTokenAccountArrayJson FRequestUtils::ParseAllTokenAccountsResponse(const FJsonObject& Data)
{
	FTokenAccountArrayJson JsonData;
	if (const TSharedPtr<FJsonObject> Result = Data.GetObjectField("result"))
	{
		FJsonObjectConverter::JsonObjectToUStruct(Result.ToSharedRef(), &JsonData);
	}
	return JsonData;
}

TSharedPtr<FRequestData> FRequestUtils::RequestProgramAccounts(const FString& ProgramId, const uint32& Size,
                                                               const FString& PubKey)
{
	auto Request = MakeShared<FRequestData>();

	Request->Body =
		FString::Printf(
			TEXT(
				R"({"jsonrpc":"2.0","id":%d,"method":"getProgramAccounts","params":["%s",{"encoding":"base64","filters":[{"dataSize":%d},{"memcmp":{"offset":8,"bytes":"%s"}}]}]})")
			, Request->Id, *ProgramId, Size, *PubKey);

	return Request;
}

TArray<FProgramAccountJson> FRequestUtils::ParseProgramAccountsResponse(const FJsonObject& Data)
{
	TArray<FProgramAccountJson> List;
	TArray<TSharedPtr<FJsonValue>> DataArray = Data.GetArrayField("result");
	for (const TSharedPtr<FJsonValue> Entry : DataArray)
	{
		const TSharedPtr<FJsonObject> EntryObject = Entry->AsObject();
		if (TSharedPtr<FJsonObject> Account = EntryObject->GetObjectField("account"))
		{
			FProgramAccountJson AccountData;
			FJsonObjectConverter::JsonObjectToUStruct(Account.ToSharedRef(), &AccountData);
			List.Add(AccountData);
		}
		FString PubKey = EntryObject->GetStringField("pubkey");
	}

	return List;
}

TSharedPtr<FRequestData> FRequestUtils::RequestMultipleAccounts(const TArray<FString>& PubKey)
{
	auto Request = MakeShared<FRequestData>();

	FString List;
	for (FString Key : PubKey)
	{
		List.Append(FString::Printf(TEXT(R"("%s")"), *Key));

		if (Key != PubKey.Last())
		{
			List.Append(",");
		}
	}

	Request->Body =
		FString::Printf(
			TEXT(
				R"({"jsonrpc":"2.0","id":%d,"method": "getMultipleAccounts","params":[[%s],{"dataSlice":{"offset":0,"length":0}}]})")
			, Request->Id, *List);

	return Request;
}

TArray<FAccountInfoJson> FRequestUtils::ParseMultipleAccountsResponse(const FJsonObject& data)
{
	TArray<FAccountInfoJson> JsonData;
	if (const TSharedPtr<FJsonObject> Result = data.GetObjectField("result"))
	{
		FJsonObjectConverter::JsonArrayToUStruct(Result->GetArrayField("value"), &JsonData);
	}
	return JsonData;
}

TSharedPtr<FRequestData> FRequestUtils::SendTransaction(const FString& Transaction)
{
	auto Request = MakeShared<FRequestData>();

	Request->Body =
		FString::Printf(
			TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"sendTransaction","params":["%s",{"encoding": "base64"}]})")
			, Request->Id, *Transaction);

	return Request;
}

FString FRequestUtils::ParseTransactionResponse(const FJsonObject& Data)
{
	return Data.GetStringField("result");
}

TSharedPtr<FRequestData> FRequestUtils::RequestBlockHash()
{
	auto Request = MakeShared<FRequestData>();

	Request->Body =
		FString::Printf(
			TEXT(R"({"id":%d,"jsonrpc":"2.0","method":"getRecentBlockhash","params":[{"commitment":"finalized"}]})")
			, Request->Id);

	return Request;
}

FString FRequestUtils::ParseBlockHashResponse(const FJsonObject& Data)
{
	FString Hash;
	if (const TSharedPtr<FJsonObject> Result = Data.GetObjectField("result"))
	{
		const TSharedPtr<FJsonObject> Value = Result->GetObjectField("value");
		Hash = Value->GetStringField("blockhash");
	}
	return Hash;
}

TSharedPtr<FRequestData> FRequestUtils::GetTransactionFeeAmount(const FString& transaction)
{
	auto Request = MakeShared<FRequestData>();

	Request->Body =
		FString::Printf(
			TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"getFeeForMessage", "params":[%s,{"commitment":"processed"}]})")
			, Request->Id, *transaction);

	return Request;
}

int FRequestUtils::ParseTransactionFeeAmountResponse(const FJsonObject& Data)
{
	int Fee = 0;
	if (const TSharedPtr<FJsonObject> Result = Data.GetObjectField("result"))
	{
		const TSharedPtr<FJsonObject> Value = Result->GetObjectField("value");
		Fee = Value->GetNumberField("value");
	}
	return Fee;
}

TSharedPtr<FRequestData> FRequestUtils::RequestAirDrop(const FString& PubKey)
{
	auto Request = MakeShared<FRequestData>();

	Request->Body =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d, "method":"requestAirdrop", "params":["%s", 1000000000]})")
		                , Request->Id, *PubKey);

	return Request;
}

void FRequestUtils::DisplayError(const FString& error)
{
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(error), &ErrorTitle);
}

void FRequestUtils::DisplayInfo(const FString& info)
{
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(info), &InfoTitle);
}
