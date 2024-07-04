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
#pragma once

#include "CoreMinimal.h"

#include "Interfaces/IHttpRequest.h"

DECLARE_DELEGATE_OneParam(RequestCallback, FJsonObject&);
DECLARE_DELEGATE_OneParam(RequestErrorCallback, FString& Error);

using RequestCB = TFunctionRef<void(FJsonObject&)>;

struct FOUNDATION_API FRequestData;

class FOUNDATION_API FRequestManager
{
public:
	static int64 GetNextMessageId();
	static int64 GetLastMessageId();

	static void SendRequest(TSharedPtr<FRequestData> RequestData);
	static void CancelRequest(FRequestData* RequestData);
};

struct FOUNDATION_API FRequestData
{
	FRequestData()
	{
		Id = FRequestManager::GetNextMessageId();
	}

	uint32 Id;
	FString Body;
	TSharedPtr<FJsonObject> Response;
	RequestCallback Callback;
	RequestErrorCallback ErrorCallback;
};
