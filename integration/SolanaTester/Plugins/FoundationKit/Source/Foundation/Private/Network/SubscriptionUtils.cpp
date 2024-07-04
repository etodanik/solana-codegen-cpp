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
*/

#include "Network/SubscriptionUtils.h"
#include "JsonObjectConverter.h"
#include "Network/UGI_WebSocketManager.h"
#include "Misc/MessageDialog.h"
#include "SolanaUtils/Utils/Types.h"

static FText ErrorMessage = FText::FromString("Error");
static FText InfoMessage = FText::FromString("Info");

FSubscriptionData* FSubscriptionUtils::AccountSubscribe(const FString& pubKey)
{
	FSubscriptionData* request = new FSubscriptionData(UGI_WebSocketManager::GetNextSubID());
	request->Body =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"accountSubscribe","params":["%s"]})")
		                , request->Id, *pubKey);
	return request;
}

void FSubscriptionUtils::AccountUnsubscribe(FSubscriptionData* sub2remove)
{
	sub2remove->UnsubMsg =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"accountUnsubscribe","params":[%d]})")
		                , sub2remove->Id,
		                sub2remove->SubscriptionNumber); // The parameter to unsub is subscription number NOT ID.
}

double FSubscriptionUtils::GetAccountSubInfo(FSubscriptionData* Sub2Read)
{
	if (Sub2Read->Response.IsValid())
	{
		const TSharedPtr<FJsonObject> result = Sub2Read->Response->GetObjectField("result");
		const TSharedPtr<FJsonObject> value = result->GetObjectField("value");
		return value->GetNumberField("lamports");
	}
	return -1.0;
}


FSubscriptionData* FSubscriptionUtils::LogsSubscribe()
{
	FSubscriptionData* request = new FSubscriptionData(UGI_WebSocketManager::GetNextSubID());
	request->Body =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"logsSubscribe","params":["all"]})")
		                , request->Id);
	request->UnsubMsg =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"logsUnsubscribe","params":[%d]})")
		                , UGI_WebSocketManager::GetNextSubID(), request->Id);
	return request;
}

void FSubscriptionUtils::LogsUnsubscribe(FSubscriptionData* sub2remove)
{
	sub2remove->UnsubMsg =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"logsUnsubscribe","params":[%d]})")
		                , UGI_WebSocketManager::GetNextSubID(), sub2remove->SubscriptionNumber);
}

FString FSubscriptionUtils::GetLogsSubInfo(FSubscriptionData* Sub2Read)
{
	if (Sub2Read->Response.IsValid())
	{
		const TSharedPtr<FJsonObject> result = Sub2Read->Response->GetObjectField("result");
		const TSharedPtr<FJsonObject> value = Sub2Read->Response->GetObjectField("value");
		return result->GetStringField("signature");
	}
	return "empty";
}

FSubscriptionData* FSubscriptionUtils::ProgramSubscribe(const FString& pubKey)
{
	FSubscriptionData* request = new FSubscriptionData(UGI_WebSocketManager::GetNextSubID());
	request->Body =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"programSubscribe","params":["%s"]})")
		                , request->Id, *pubKey);
	request->UnsubMsg =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"programUnsubscribe","params":[%d]})")
		                , UGI_WebSocketManager::GetNextSubID(), request->Id);
	return request;
}

void FSubscriptionUtils::ProgramUnsubscribe(FSubscriptionData* sub2remove)
{
	sub2remove->UnsubMsg =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"programUnsubscribe","params":[%d]})")
		                , UGI_WebSocketManager::GetNextSubID(), sub2remove->SubscriptionNumber);
}

int FSubscriptionUtils::GetProgramSubInfo(FSubscriptionData* Sub2Read)
{
	if (Sub2Read->Response.IsValid())
	{
		const TSharedPtr<FJsonObject> Result = Sub2Read->Response->GetObjectField("result");
		const TSharedPtr<FJsonObject> Value = Result->GetObjectField("value");
		const TSharedPtr<FJsonObject> Account = Result->GetObjectField("account");
		return Account->GetNumberField("lamports");
	}
	return -1.0;
}

FSubscriptionData* FSubscriptionUtils::SignatureSubscribe(const FString& signature)
{
	FSubscriptionData* request = new FSubscriptionData(UGI_WebSocketManager::GetNextSubID());

	request->Body =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"signatureSubscribe","params":["%s"]})")
		                , request->Id, *signature);

	request->UnsubMsg =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"signatureUnsubscribe","params":[%d]})")
		                , UGI_WebSocketManager::GetNextSubID(), request->Id);
	return request;
}

void FSubscriptionUtils::SignatureUnsubscribe(FSubscriptionData* sub2remove)
{
	sub2remove->UnsubMsg =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"signatureUnsubscribe","params":[%d]})")
		                , UGI_WebSocketManager::GetNextSubID(), sub2remove->SubscriptionNumber);
}

TSharedPtr<FJsonObject> FSubscriptionUtils::GetSignatureSubInfo(FSubscriptionData* Sub2Read)
{
	if (Sub2Read->Response.IsValid())
	{
		if (Sub2Read->Response->GetObjectField("result"))
		{
			const TSharedPtr<FJsonObject> Result = Sub2Read->Response->GetObjectField("result");
			return Result;
		}
	}
	return nullptr;
}

FSubscriptionData* FSubscriptionUtils::SlotSubscribe()
{
	FSubscriptionData* Request = new FSubscriptionData(UGI_WebSocketManager::GetNextSubID());

	Request->Body =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"slotSubscribe"})")
		                , Request->Id);
	Request->UnsubMsg =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"slotUnsubscribe","params":[%d]})")
		                , UGI_WebSocketManager::GetNextSubID(), Request->Id);
	return Request;
}

void FSubscriptionUtils::SlotUnsubscribe(FSubscriptionData* SubToRemove)
{
	SubToRemove->UnsubMsg =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"slotUnsubscribe","params":[%d]})")
		                , UGI_WebSocketManager::GetNextSubID(), SubToRemove->SubscriptionNumber);
}

int FSubscriptionUtils::GetSlotSubInfo(FSubscriptionData* Sub2Read)
{
	if (Sub2Read->Response.IsValid())
	{
		const TSharedPtr<FJsonObject> Result = Sub2Read->Response->GetObjectField("result");
		return Result->GetNumberField("parent");
	}
	return -1;
}

FSubscriptionData* FSubscriptionUtils::RootSubscribe()
{
	FSubscriptionData* request = new FSubscriptionData(UGI_WebSocketManager::GetNextSubID());
	request->Body =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"rootSubscribe"})")
		                , request->Id);
	request->UnsubMsg =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"rootUnsubscribe","params":[%d]})")
		                , UGI_WebSocketManager::GetNextSubID(), request->Id);

	return request;
}

void FSubscriptionUtils::RootUnsubscribe(FSubscriptionData* sub2remove)
{
	sub2remove->UnsubMsg =
		FString::Printf(TEXT(R"({"jsonrpc":"2.0","id":%d,"method":"rootUnsubscribe","params":[%d]})")
		                , UGI_WebSocketManager::GetNextSubID(), sub2remove->SubscriptionNumber);
}

int FSubscriptionUtils::GetRootSubInfo(FSubscriptionData* Sub2Read)
{
	if (Sub2Read->Response.IsValid())
	{
		return Sub2Read->Response->GetNumberField("result");
	}
	return -1;
}
