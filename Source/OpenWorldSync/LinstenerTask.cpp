#include "LinstenerTask.h"

void FLinstenerTask::DoWork()
{
	UE_LOG(LogTemp, Warning, TEXT("Listen Start"));
	while (ListenSocket != nullptr)
	{
		TArray<uint8> RecvData;
		uint32 Size;

		while (ListenSocket->HasPendingData(Size))
		{
			RecvData.SetNumUninitialized(FMath::Min(Size, 65507u));
			int32 Read = 0;
			ListenSocket->Recv(RecvData.GetData(), RecvData.Num(), Read);
		}

		if (RecvData.Num() <= 0)
		{
			continue;
		}

		// log RecvData
		HandleData(RecvData);

		FPlatformProcess::Sleep(0.01f);
	}
}

void FLinstenerTask::HandleData(const TArray<uint8>& Data) 
{
	FString JsonString = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(Data.GetData())));
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("RecvData : %s"), *JsonString);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to deserialize json"));
	}
}
