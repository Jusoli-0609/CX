// CXPlayerController.cpp


#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "CXChatWidget.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "InputCoreTypes.h"

void ACXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}
	
	// spawn the UI widget and add it to the viewport
	VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass);

	check(VehicleUI);

	VehicleUI->AddToViewport();

	if (ChatWidgetClass)
	{
		ChatWidget = CreateWidget<UCXChatWidget>(this, ChatWidgetClass);
		if (ChatWidget)
		{
			ChatWidget->AddToViewport(10);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Assign ChatWidgetClass in the PlayerController Blueprint."));
	}
}

void ACXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ACXPlayerController::OpenChat);
	}
	
	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);

		// optionally add the steering wheel context
		if (bUseSteeringWheelControls && SteeringWheelInputMappingContext)
		{
			Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 1);
		}
	}
}

void ACXPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, VehiclePawn);
}

void ACXPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
	}
}

void ACXPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// get a pointer to the controlled pawn
	VehiclePawn = CastChecked<ACXPawn>(InPawn);
}

void ACXPlayerController::OpenChat()
{
	if (!IsLocalController() || !ChatWidget)
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ChatWidget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	ChatWidget->FocusChatInput();
}

void ACXPlayerController::CloseChat()
{
	if (!IsLocalController())
	{
		return;
	}

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ACXPlayerController::SubmitChatMessage(const FString& Message)
{
	const FString CleanMessage = Message.TrimStartAndEnd().Left(128);
	if (IsLocalController() && !CleanMessage.IsEmpty())
	{
		ServerSendChatMessage(CleanMessage);
	}
}

void ACXPlayerController::ServerSendChatMessage_Implementation(const FString& Message)
{
	if (!GetWorld())
	{
		return;
	}

	FString CleanMessage = Message.TrimStartAndEnd().Left(128);
	CleanMessage.ReplaceInline(TEXT("\r"), TEXT(" "));
	CleanMessage.ReplaceInline(TEXT("\n"), TEXT(" "));
	if (CleanMessage.IsEmpty())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (LastChatMessageTime >= 0.0f && Now - LastChatMessageTime < 0.25f)
	{
		return;
	}
	LastChatMessageTime = Now;

	const APlayerState* SenderState = GetPlayerState<APlayerState>();
	const FString SenderName = SenderState && !SenderState->GetPlayerName().IsEmpty()
		? SenderState->GetPlayerName()
		: GetName();
	const FString DisplayMessage = FString::Printf(TEXT("%s: %s"), *SenderName, *CleanMessage);

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ACXPlayerController* Recipient = Cast<ACXPlayerController>(It->Get()))
		{
			Recipient->ClientReceiveChatMessage(DisplayMessage);
		}
	}
}

void ACXPlayerController::ClientReceiveChatMessage_Implementation(const FString& Message)
{
	if (IsLocalController() && ChatWidget)
	{
		ChatWidget->AddChatMessage(Message);
	}
}

