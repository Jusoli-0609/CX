// Copyright Epic Games, Inc. All Rights Reserved.

#include "CXPawn.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "Components/ArrowComponent.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
	bReplicates = true;
	SetCanBeDamaged(true);

	AttackOrigin = CreateDefaultSubobject<UArrowComponent>(TEXT("AttackOrigin"));
	AttackOrigin->SetupAttachment(GetMesh());
	AttackOrigin->SetRelativeLocation(FVector(250.0f, 0.0f, 50.0f));
	AttackOrigin->SetHiddenInGame(true);

	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ACXPawn::Fire);
		}
		else
		{
			UE_LOG(LogTemplateVehicle, Warning, TEXT("Assign FireAction in the vehicle Blueprint."));
		}

		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	LimitMaxSpeed();

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
	// get the input magnitude for steering
	float SteeringValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	// get the input magnitude for the throttle
	float ThrottleValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	// get the input magnitude for the brakes
	float BreakValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetBrakeInput(BreakValue);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(false);

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	// get the flat angle value for the input 
	float LookValue = Value.Get<float>();

	// add the input
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;
	
	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);

	UE_LOG(LogTemplateVehicle, Error, TEXT("Reset Vehicle"));
}

float ACXPawn::GetEffectiveMaxSpeedKmh() const
{
	const ACXPlayerState* State = GetPlayerState<ACXPlayerState>();
	const float Bonus = State ? State->GetBonusMaxSpeed() : 0.0f;
	return FMath::Max(1.0f, BaseMaxSpeedKmh + Bonus);
}

void ACXPawn::LimitMaxSpeed()
{
	// The authority enforces the cap; the owning client also predicts it locally.
	// Simulated proxies do not have owner-only bonus data and must not clamp it.
	if ((!HasAuthority() && !IsLocallyControlled()) || !GetMesh() || !GetMesh()->IsSimulatingPhysics())
	{
		return;
	}

	const float MaxSpeedCmPerSecond = GetEffectiveMaxSpeedKmh() / 0.036f;
	const FVector Velocity = GetMesh()->GetPhysicsLinearVelocity();
	const double HorizontalSpeedSquared = Velocity.SizeSquared2D();
	const double MaxSpeedSquared = FMath::Square(static_cast<double>(MaxSpeedCmPerSecond));
	if (HorizontalSpeedSquared <= MaxSpeedSquared)
	{
		return;
	}

	const double Scale = MaxSpeedCmPerSecond / FMath::Sqrt(HorizontalSpeedSquared);
	const FVector LimitedVelocity(Velocity.X * Scale, Velocity.Y * Scale, Velocity.Z);
	GetMesh()->SetPhysicsLinearVelocity(LimitedVelocity, false);
}

bool ACXPawn::CanFire() const
{
	if (!HasAuthority() || !GetWorld() || !GetController() ||
		!GetPlayerState<ACXPlayerState>() || !AttackOrigin)
	{
		return false;
	}

	const ACXGameState* MatchState = GetWorld()->GetGameState<ACXGameState>();
	if (!MatchState || !MatchState->IsPlaying())
	{
		return false;
	}


	return LastFireTime < 0.0 ||
		GetWorld()->GetTimeSeconds() - LastFireTime >= FMath::Max(0.05f, FireCooldown);
}

void ACXPawn::Fire(const FInputActionValue& Value)
{
	if (IsLocallyControlled())
	{
		ServerFire();
	}
}

void ACXPawn::ServerFire_Implementation()
{
	if (!CanFire())
	{
		return;
	}
	LastFireTime = GetWorld()->GetTimeSeconds();

	const FVector TraceStart = AttackOrigin->GetComponentLocation();
	const FVector TraceEnd = TraceStart +
		AttackOrigin->GetForwardVector() * FMath::Max(1.0f, AttackRange);

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VehicleAttack), false, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	bool bStoleCoins = false;
	if (ACXPawn* Victim = Cast<ACXPawn>(Hit.GetActor()))
	{
		if (Victim != this)
		{
			bStoleCoins = UGameplayStatics::ApplyDamage(
				Victim, 5.0f, GetController(), this, UDamageType::StaticClass()) > 0.0f;
		}
	}

	MulticastPlayAttackEffects(TraceStart, bHit ? Hit.ImpactPoint : TraceEnd, bStoleCoins);
}

float ACXPawn::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || !CanBeDamaged() || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const ACXGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<ACXGameState>() : nullptr;
	if (!MatchState || !MatchState->IsPlaying())
	{
		return 0.0f;
	}


	ACXPawn* Attacker = Cast<ACXPawn>(DamageCauser);
	if (!Attacker || Attacker == this || !EventInstigator ||
		EventInstigator != Attacker->GetController())
	{
		return 0.0f;
	}

	ACXPlayerState* VictimState = GetPlayerState<ACXPlayerState>();
	ACXPlayerState* AttackerState = Attacker->GetPlayerState<ACXPlayerState>();
	if (!VictimState || !AttackerState || VictimState == AttackerState ||
		VictimState->GetCoins() < 5 || AttackerState->GetCoins() > MAX_int32 - 5)
	{
		return 0.0f;
	}

	if (Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser) <= 0.0f)
	{
		return 0.0f;
	}

	if (!VictimState->TrySpendCoins(5))
	{
		return 0.0f;
	}
	if (!AttackerState->AddCoins(5))
	{
		// The existing setters are synchronous; refund if crediting was rejected.
		VictimState->AddCoins(5);
		return 0.0f;
	}

	return 5.0f;
}

void ACXPawn::MulticastPlayAttackEffects_Implementation(
	FVector TraceStart, FVector TraceEnd, bool bStoleCoins)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, TraceStart);
	}

	DrawDebugLine(GetWorld(), TraceStart, TraceEnd,
		bStoleCoins ? FColor::Green : FColor::Red, false, 1.0f, 0, 2.0f);

	if (bStoleCoins && AttackHitNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), AttackHitNiagara, TraceEnd, FRotator::ZeroRotator);
	}
}

#undef LOCTEXT_NAMESPACE



