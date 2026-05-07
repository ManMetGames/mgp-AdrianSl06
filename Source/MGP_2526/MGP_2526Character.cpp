// Copyright Epic Games, Inc. All Rights Reserved.

#include "MGP_2526Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "MGP_2526.h"
#include "TimerManager.h"

AMGP_2526Character::AMGP_2526Character()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.8f;
	GetCharacterMovement()->MaxWalkSpeed = 900.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AMGP_2526Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMGP_2526Character::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMGP_2526Character::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMGP_2526Character::Look);
	}
	else
	{
		UE_LOG(LogMGP_2526, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMGP_2526Character::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AMGP_2526Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AMGP_2526Character::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AMGP_2526Character::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMGP_2526Character::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AMGP_2526Character::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

// Blinking ------------------------------------------------------------------------------------------------------------

void AMGP_2526Character::TryBlink()
{
	// Stops the blink if the player has no more charges left
	if (BlinkCharge <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No blink charges"));
		OnBlinkFailed();

		return;
	}

	// start and end point of the blink
	const FVector StartLocation = GetActorLocation();
	const FRotator ControlRotation = GetControlRotation();
	const FVector BlinkDirection = ControlRotation.Vector();
	const FVector FullBlinkLocation = StartLocation + (BlinkDirection * BlinkDistance);


	// Line trace to stop player from blinking into the wall (Wall detection)
	FHitResult HitResult;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	const bool bHitWall = GetWorld()->LineTraceSingleByChannel
	(HitResult,StartLocation,FullBlinkLocation,ECC_Visibility,TraceParams);

	// Final - this is the location the player blinks to
	FVector FinalBlinkLocation = FullBlinkLocation;

	// If the line trace hits a wall, this 
	if (bHitWall)
	{
		FinalBlinkLocation = HitResult.ImpactPoint + (HitResult.ImpactNormal * BlinkWallOffset);
	}

	// Sends player to Final location
	SetActorLocation(FinalBlinkLocation,true);

	// -1 Blink Charge
	BlinkCharge--;

	OnBlinkChargesChanged();

	UE_LOG(LogTemp, Warning, TEXT("Charges left: %d"), BlinkCharge);

	// Starts the recharge system
	StartBlinkRecharge();

	// Feedback (on blueprint)
	OnBlinkSuccessful();
}

// ----------------------------------------------------

void AMGP_2526Character::StartBlinkRecharge()
{
	// Stops multiple recharge timers from running at the same time
	if (IsRechargingBlink)
	{
		return;
	}

	if (BlinkCharge >= MaxBlinkCharges)
	{
		return;
	}

	IsRechargingBlink = true;

	// Once blinkrechargetime ends, recharge one blink
	GetWorldTimerManager().SetTimer(BlinkRechargeTimerHandle,this,&AMGP_2526Character::RechargeBlink,BlinkRechargeTime,true);
}

void AMGP_2526Character::RechargeBlink()
{
	BlinkCharge++;

	OnBlinkChargesChanged();

	if (BlinkCharge > MaxBlinkCharges)
	{
		BlinkCharge = MaxBlinkCharges;
	}

	UE_LOG(LogTemp, Warning, TEXT("Charges: %d"), BlinkCharge);

	// stops recharge if full
	if (BlinkCharge >= MaxBlinkCharges)
	{
		GetWorldTimerManager().ClearTimer(BlinkRechargeTimerHandle);
		IsRechargingBlink = false;

		UE_LOG(LogTemp, Warning, TEXT("Blink is full"));
	}
}

void AMGP_2526Character::TryJumpOrFlyBoost()
{
	// on ground do normal jump
	if (!GetCharacterMovement()->IsFalling())
	{
		Jump();
		return;
	}

	// Player in air = able to boost
	if (CanFlyBoost)
	{
		const FVector BoostVelocity = FVector(0.0f, 0.0f, FlyBoostStrength);

		LaunchCharacter(BoostVelocity,false,true);
		CanFlyBoost = false;
		OnFlyBoostSuccessful();
	}
}

// Resets boost when player lands 
void AMGP_2526Character::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	CanFlyBoost = true;
}