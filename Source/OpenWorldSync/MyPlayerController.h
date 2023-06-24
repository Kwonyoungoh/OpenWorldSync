// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Containers/UnrealString.h"
#include "Containers/Array.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "Common/UdpSocketBuilder.h"
#include "Common/UdpSocketReceiver.h"
#include "LinstenerTask.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class OPENWORLDSYNC_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

	protected:
		virtual void BeginPlay() override;

	public:
		AMyPlayerController();
		void ListenData();
		//void HandleData(const TArray<uint8>& Data);

	private:
		FSocket* ListenSocket;
		FAsyncTask<FLinstenerTask>* ListenerTask;
};
