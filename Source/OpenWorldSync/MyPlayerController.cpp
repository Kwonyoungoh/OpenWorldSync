// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"

AMyPlayerController::AMyPlayerController() {
	ListenSocket = nullptr;
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ListenData();
}

void AMyPlayerController::ListenData() {

	ISocketSubsystem* SocketSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

	ListenSocket = SocketSubSystem->CreateSocket(NAME_DGram, TEXT("ListenSocket"), false);

	if (ListenSocket != nullptr) {
		FIPv4Address Addr;
		FIPv4Address::Parse(TEXT("0.0.0.0"), Addr);
		FIPv4Endpoint Endpoint(Addr, 50000);

		if (!ListenSocket->Bind(*Endpoint.ToInternetAddr()))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to bind socket"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Success to bind socket"));

			ListenerTask = new FAsyncTask<FLinstenerTask>(ListenSocket);
			ListenerTask->StartBackgroundTask();
		}
	}
}

//void AMyPlayerController::HandleData(const TArray<uint8>& Data)
//{
//	FString JsonString = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(Data.GetData())));
//	TSharedPtr<FJsonObject> JsonObject;
//	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
//	if (FJsonSerializer::Deserialize(Reader, JsonObject))
//	{
//		UE_LOG(LogTemp, Warning, TEXT("Success to deserialize json"));
//	}
//	else
//	{
//		UE_LOG(LogTemp, Warning, TEXT("Failed to deserialize json"));
//	}
//}