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

#pragma once

#include "CoreMinimal.h"

struct FSubscriptionData;

class FOUNDATION_API FSubscriptionUtils
{
public:
	static FSubscriptionData* AccountSubscribe(const FString& pubKey);
	static void AccountUnsubscribe(FSubscriptionData* sub2remove);
	static double GetAccountSubInfo(FSubscriptionData* sub2read);

	static FSubscriptionData* LogsSubscribe();
	static void LogsUnsubscribe(FSubscriptionData* sub2remove);
	static FString GetLogsSubInfo(FSubscriptionData* sub2read);

	static FSubscriptionData* ProgramSubscribe(const FString& pubKey);
	static void ProgramUnsubscribe(FSubscriptionData* sub2remove);
	static int GetProgramSubInfo(FSubscriptionData* sub2read);

	static FSubscriptionData* SignatureSubscribe(const FString& signature);
	static void SignatureUnsubscribe(FSubscriptionData* sub2remove);
	static TSharedPtr<FJsonObject> GetSignatureSubInfo(FSubscriptionData* sub2read);

	static FSubscriptionData* SlotSubscribe();
	static void SlotUnsubscribe(FSubscriptionData* SubToRemove);
	static int GetSlotSubInfo(FSubscriptionData* sub2read);

	static FSubscriptionData* RootSubscribe();
	static void RootUnsubscribe(FSubscriptionData* sub2remove);
	static int GetRootSubInfo(FSubscriptionData* sub2read);
};
