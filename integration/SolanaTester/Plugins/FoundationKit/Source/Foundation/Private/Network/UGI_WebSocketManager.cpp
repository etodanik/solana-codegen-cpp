#include "Network/UGI_WebSocketManager.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "FoundationSettings.h"
#include "Network/RequestUtils.h"
#include "TimerManager.h"

int64 GLastMessageID = 0;
FTimerHandle GHeartbeatHandler;

int64 UGI_WebSocketManager::GetNextSubID()
{
	return GLastMessageID++;
}

int64 UGI_WebSocketManager::GetLastSubID()
{
	return GLastMessageID;
}

void UGI_WebSocketManager::Init()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "Initializing WebSockets");
	Super::Init();
	FString Url = GetDefault<UFoundationSettings>()->GetNetworkURL();
	if (Url.IsEmpty())
	{
		Url = GetDefault<UFoundationSettings>()->GetNetwork() == ESolanaNetwork::DevNet
			      ? "ws://api.devnet.solana.com/"
			      : "ws://api.mainnet-beta.solana.com/";
	}

	if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
	{
		FModuleManager::Get().LoadModule("WebSockets");
	}
	WebSocket = FWebSocketsModule::Get().CreateWebSocket(Url);

	WebSocket->OnConnected().AddLambda([]
	{
		OnConnected_Helper();
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Connection succesfull");
	});

	WebSocket->OnConnectionError().AddLambda([](const FString& Error)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, Error);
	});

	WebSocket->OnClosed().AddLambda([](int32 StatusCode, const FString& Reason, bool bWasClean)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, bWasClean ? FColor::Green : FColor::Red,
		                                 "Connection closed " + Reason);
	});

	WebSocket->OnMessage().AddLambda([](const FString& Response)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, "Message received");
		OnResponse(Response);
	});

	WebSocket->Connect();
}

void UGI_WebSocketManager::Shutdown()
{
	if (WebSocket->IsConnected())
	{
		// Stop the timer
		WebSocket->Close();
	}
	Super::Shutdown();
}


void UGI_WebSocketManager::Subscribe(FSubscriptionData* SubData)
{
	if (!WebSocket->IsConnected())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Could not send message, no connection.");
		return;
	}
	WebSocket->Send(SubData->Body);
	ActiveSubscriptions.Add(SubData->Id, SubData);
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Subscription Requested");
}


void UGI_WebSocketManager::Unsubscribe(int subID)
{
	WebSocket->Send(ActiveSubscriptions[subID]->UnsubMsg);
}

FSubscriptionData* UGI_WebSocketManager::GetSubData(int subID)
{
	return ActiveSubscriptions[subID];
}


void UGI_WebSocketManager::OnConnected_Helper()
{
	OnConnected.Broadcast();
}

void UGI_WebSocketManager::OnResponse(const FString& Response)
{
	GEngine->AddOnScreenDebugMessage(-1, 17.0f, FColor::Purple, Response);

	TSharedPtr<FJsonObject> ParsedJSON;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<>::Create(Response);
	if (FJsonSerializer::Deserialize(Reader, ParsedJSON))
	{
		const TSharedPtr<FJsonObject>* Params;
		float SubId;
		if (ParsedJSON->TryGetNumberField("id", SubId))
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, "Attempting to parse confirmation");
			ParseSubConfirmation(Response);
		}
		if (ParsedJSON->TryGetObjectField("params", Params))
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, "Attempting to parse notification");
			ParseNotification(Response);
		}
	}
}

void UGI_WebSocketManager::HeartbeatHelper()
{
	// Check if wb != NULL
	if (WebSocket != nullptr)
	{
		if (WebSocket->IsConnected())
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, "Heartbeat sent!");
			WebSocket->Send("ping");
		}
	}
}

void UGI_WebSocketManager::InitializeHeartbeat()
{
	GetWorld()->GetTimerManager().SetTimer(GHeartbeatHandler, this, &UGI_WebSocketManager::HeartbeatHelper, 30.0, true,
	                                       -1);
}

void UGI_WebSocketManager::ParseSubConfirmation(const FString& Response)
{
	TSharedPtr<FJsonObject> ParsedJSON;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<>::Create(Response);

	if (FJsonSerializer::Deserialize(Reader, ParsedJSON))
	{
		const int Id = ParsedJSON->GetIntegerField("id");
		if (!ActiveSubscriptions.IsEmpty())
		{
			FSubscriptionData* Subscription = ActiveSubscriptions[Id];
			if (Subscription)
			{
				Subscription->SubscriptionNumber = ParsedJSON->GetIntegerField("result");
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Subcription Confirmed");
			}
		}
		else
		{
			if (ParsedJSON->GetBoolField("result"))
			{
				// Delete the object from hashmap
				ActiveSubscriptions.Remove(Id);
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Unsubcription Confirmed");
			}
			else
			{
				// Declare on fail
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Unsubcription failed");
			}
		}
	}
}

void UGI_WebSocketManager::ParseNotification(const FString& Response)
{
	TSharedPtr<FJsonObject> ParsedJSON;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<>::Create(Response);

	if (FJsonSerializer::Deserialize(Reader, ParsedJSON))
	{
		const TSharedPtr<FJsonObject>* outObject;

		if (!ParsedJSON->TryGetObjectField("error", outObject))
		{
			const int SubNum = ParsedJSON->GetObjectField("params")->GetIntegerField("subscription");
			if (!ActiveSubscriptions.IsEmpty())
			{
				for (const TTuple<int, FSubscriptionData*>& Elem : ActiveSubscriptions)
				{
					if (Elem.Value->SubscriptionNumber == SubNum)
					{
						FSubscriptionData* Subscription = Elem.Value;
						GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Subcription Updated");
						Subscription->Response = ParsedJSON->GetObjectField("params");
					}
				}
			}
		}
		else
		{
			const TSharedPtr<FJsonObject> error = ParsedJSON->GetObjectField("error");
			// FRequestUtils::DisplayError(error->GetStringField("message"));
		}
	}
	else
	{
		// FRequestUtils::DisplayError("Failed to parse Response from the server");
	}
}

void UGI_WebSocketManager::OnStart()
{
	Super::OnStart();
	this->InitializeHeartbeat();
}
