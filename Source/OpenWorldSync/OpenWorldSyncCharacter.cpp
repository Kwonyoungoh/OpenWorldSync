// Copyright Epic Games, Inc. All Rights Reserved.

#include "OpenWorldSyncCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"


//////////////////////////////////////////////////////////////////////////
// AOpenWorldSyncCharacter

// 플래그
enum class conn_flags : uint8_t {
	CONNECT_FLAG = 0X01,
	DISCONNECT_FLAG = 0X02,
	DATA_FLAG = 0X03,
	CHANGE_CHUNK_FLAG = 0X04
};

AOpenWorldSyncCharacter::AOpenWorldSyncCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// Create SendSocket
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	SendSocket = FUdpSocketBuilder(TEXT("SendSocket"))
		.AsReusable()
		.WithBroadcast()
		.BoundToPort(3000)
		.Build();

	// Set RemoteEndpoint
	FIPv4Address RemoteAddress;
	FIPv4Address::Parse(TEXT("172.28.13.237"), RemoteAddress);
	int32 RemotePort = 12345;
	RemoteEndpoint = FIPv4Endpoint(RemoteAddress, RemotePort);

	// Set SendSocket to non-blocking mode
	bool bDidSetNonBlocking = SendSocket->SetNonBlocking(true);
	check(bDidSetNonBlocking == true);

}

void AOpenWorldSyncCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// 타이머 시간조정
	GetWorldTimerManager().SetTimer(SendDataTimerHandle, this, &AOpenWorldSyncCharacter::SendDataToServer, 0.1f, true, 0.1f);
}

void AOpenWorldSyncCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	  
	Super::EndPlay(EndPlayReason);

	GetWorldTimerManager().ClearTimer(SendDataTimerHandle);
	
	if (SendSocket != nullptr)
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		// 청크데이터 포함
		JsonObject->SetStringField(TEXT("Chunk"), PrevChunkInfo);
		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

		FTCHARToUTF8 Converter(*OutputString);
		int32 BytesSent = 0;

		// 종료 패킷 전송
		uint8 header_byte = static_cast<uint8>(conn_flags::DISCONNECT_FLAG);
		TArray<uint8> DisconnectPacket;
		DisconnectPacket.Add(header_byte);
		DisconnectPacket.Append((uint8*)Converter.Get(), Converter.Length());

		if (!SendSocket->SendTo(DisconnectPacket.GetData(), DisconnectPacket.Num(), BytesSent, *RemoteEndpoint.ToInternetAddr()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to send Disconnect data!"));
		}

		SendSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(SendSocket);
		SendSocket = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AOpenWorldSyncCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AOpenWorldSyncCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AOpenWorldSyncCharacter::Look);

	}

}

void AOpenWorldSyncCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AOpenWorldSyncCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

FString AOpenWorldSyncCharacter::CreateSendData() 
{

	FVector Location = GetActorLocation();

	// 같은 위치면 전송X
	if (Location == PrevLocation)
	{
		return "";
	}

	PrevLocation = Location;

	FRotator Rotation = GetActorRotation();
	FVector Velocity = GetCharacterMovement()->Velocity;

	// 청크 계산
	int32 ChunkX = FMath::FloorToInt(Location.X / ChunkUnit);
	int32 ChunkY = FMath::FloorToInt(Location.Y / ChunkUnit);
	FString ChunkInfo = FString::Printf(TEXT("%d:%d"), ChunkX, ChunkY);

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	if (ChunkInfo != PrevChunkInfo && !isFirst)
	{
		isChunkChange = true;
		JsonObject->SetStringField(TEXT("PrevChunk"), PrevChunkInfo);
	}

	PrevChunkInfo = ChunkInfo;

	JsonObject->SetStringField(TEXT("Chunk"), ChunkInfo);
	JsonObject->SetStringField(TEXT("Location"), Location.ToString());
	JsonObject->SetStringField(TEXT("Rotation"), Rotation.ToString());
	JsonObject->SetStringField(TEXT("Velocity"), Velocity.ToString());

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	return OutputString;
}

void AOpenWorldSyncCharacter::SendDataToServer()
{
	FString DataToSend = CreateSendData();

	if (DataToSend.IsEmpty())
	{
		return;
	}

	FTCHARToUTF8 Converter(*DataToSend);
	int32 BytesSent = 0;

	UE_LOG(LogTemp, Warning, TEXT("SendData : %s"), *DataToSend);

	if (isFirst)
	{
		uint8 header_byte = static_cast<uint8>(conn_flags::CONNECT_FLAG);
		TArray<uint8> InitialPacket;
		InitialPacket.Add(header_byte);
		InitialPacket.Append((uint8*)Converter.Get(), Converter.Length());

		if (!SendSocket->SendTo(InitialPacket.GetData(), InitialPacket.Num(), BytesSent, *RemoteEndpoint.ToInternetAddr()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to send initial data!"));
		}

		isFirst = false;
	}else if(isChunkChange)
	{
		uint8 header_byte = static_cast<uint8>(conn_flags::CHANGE_CHUNK_FLAG);
		TArray<uint8> ChunkPacket;
		ChunkPacket.Add(header_byte);
		ChunkPacket.Append((uint8*)Converter.Get(), Converter.Length());

		if (!SendSocket->SendTo(ChunkPacket.GetData(), ChunkPacket.Num(), BytesSent, *RemoteEndpoint.ToInternetAddr()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to send chunk data!"));
		}
		isChunkChange = false;
	}
	else
	{
		uint8 header_byte = static_cast<uint8>(conn_flags::DATA_FLAG);
		TArray<uint8> DataPacket;
		DataPacket.Add(header_byte);
		DataPacket.Append((uint8*)Converter.Get(), Converter.Length());

		if (!SendSocket->SendTo(DataPacket.GetData(), DataPacket.Num(), BytesSent, *RemoteEndpoint.ToInternetAddr()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to send data!"));
		}
	}
}
