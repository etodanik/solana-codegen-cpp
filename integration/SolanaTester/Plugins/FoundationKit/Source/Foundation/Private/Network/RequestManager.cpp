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
Contributers: Daniele Calanna, Riccardo Torrisi
*/


#include "Network/RequestManager.h"

#include "HttpModule.h"
#include "Network/RequestUtils.h"
#include "Interfaces/IHttpResponse.h"

#include "FoundationSettings.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

FString PrettifyJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FString OutputString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString, 0);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	return OutputString;
}

DECLARE_LOG_CATEGORY_CLASS(RequestManager, Log, All);

int64 LastMessageID = 0;
TArray<FRequestData*> PendingRequests;

int64 FRequestManager::GetNextMessageId()
{
	return LastMessageID++;
}

int64 FRequestManager::GetLastMessageId()
{
	return LastMessageID;
}

void FRequestManager::SendRequest(TSharedPtr<FRequestData> RequestData)
{
	const FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	FString Url = GetDefault<UFoundationSettings>()->GetNetworkURL();
	if (Url.IsEmpty())
	{
		Url = GetDefault<UFoundationSettings>()->GetNetwork() == ESolanaNetwork::DevNet
			      ? "https://suzy-imihkz-fast-devnet.helius-rpc.com/"
			      : "https://blisse-zgnb5y-fast-mainnet.helius-rpc.com/";
	}
	Request->SetURL(Url);
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(RequestData->Body);

	Request->OnProcessRequestComplete().BindLambda(
		[RequestData](FHttpRequestPtr InRequest, const FHttpResponsePtr& Response, const bool bSuccess)
		{
			if (!bSuccess)
			{
				FString RequestFailed(TEXT("Http Request Failed"));
				UE_LOG(LogTemp, Error, TEXT("Http Request Failed"));
				RequestData->ErrorCallback.ExecuteIfBound(RequestFailed);
			}

			TSharedPtr<FJsonObject> ParsedJSON;
			const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<>::Create(
				Response.Get()->GetContentAsString());

			if (FJsonSerializer::Deserialize(Reader, ParsedJSON))
			{
				const TSharedPtr<FJsonObject>* outObject;
				if (!ParsedJSON->TryGetObjectField("error", outObject))
				{
					RequestData->Callback.ExecuteIfBound(*ParsedJSON);
				}
				else
				{
					const TSharedPtr<FJsonObject> ErrorObjectField = ParsedJSON->GetObjectField("error");
					const TSharedPtr<FJsonObject> DataStringField = ErrorObjectField->GetObjectField("data");
					const FString PrettyDataField = PrettifyJson(DataStringField);

					UE_LOG(LogTemp, Error, TEXT("Request Error: %s, data: %s"),
					       *ErrorObjectField->GetStringField("message"), *PrettyDataField);
					auto MessageString = ErrorObjectField->GetStringField("message");
					RequestData->ErrorCallback.ExecuteIfBound(MessageString);
				}
			}
			else
			{
				FString ParseFailure(TEXT("Failed to parse response from the server"));
				UE_LOG(LogTemp, Error, TEXT("Failed to parse Response from the server"));
				RequestData->ErrorCallback.ExecuteIfBound(ParseFailure);
			}
		});

	Request->ProcessRequest();
}

void FRequestManager::CancelRequest(FRequestData* RequestData)
{
	if (RequestData)
	{
		RequestData->Callback.Unbind();
		RequestData->ErrorCallback.Unbind();
	}
}
