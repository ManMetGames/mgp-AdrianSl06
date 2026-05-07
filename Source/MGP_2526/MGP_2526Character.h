// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MGP_2526Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AMGP_2526Character : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

public:

	/** Constructor */
	AMGP_2526Character();	

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	// ------------------------------------------

	// Blink Ability

	// Distance of blink
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink")
	float BlinkDistance = 600.0f;

	// Current blink number
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink")
	int32 BlinkCharge = 2;

	// Max blink number 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink")
	int32 MaxBlinkCharges = 2;
	
	// Distance from wall to player, if player blinks into wall or another solid object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink")
	float BlinkWallOffset = 50.0f;

	UFUNCTION(BlueprintCallable, Category = "Blink")
	void TryBlink();

	// used for the feedback (animations, etc)
	UFUNCTION(BlueprintImplementableEvent, Category = "Blink")
	void OnBlinkSuccessful();

	// Time between each recharge
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink")
	float BlinkRechargeTime = 2.0f;

	// Stops multiple chargers to be running at the same time
	UPROPERTY(BlueprintReadOnly, Category = "Blink")
	bool IsRechargingBlink = false;

	// Stores timer for recharging blinks
	FTimerHandle BlinkRechargeTimerHandle;

	// If the blink fails this is ran
	UFUNCTION(BlueprintImplementableEvent, Category = "Blink")
	void OnBlinkFailed();

	// +1 blink charge
	void RechargeBlink();

	// Starts blink recharge
	void StartBlinkRecharge();

	// Used when blink charge changes (FOR UI)
	UFUNCTION(BlueprintImplementableEvent, Category = "Blink")
	void OnBlinkChargesChanged();

	// Double Jump (FlyBoost)

	// Checks if player can boost
	UPROPERTY(BlueprintReadWrite, Category = "Fly Boost")
	bool CanFlyBoost = true;

	// Strength of the boost
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fly Boost")
	float FlyBoostStrength = 900.0f;

	// Used for FlyBoost feedback
	UFUNCTION(BlueprintImplementableEvent, Category = "Fly Boost")
	void OnFlyBoostSuccessful();

	// 
	UFUNCTION(BlueprintCallable, Category = "Fly Boost")
	void TryJumpOrFlyBoost();

	// NEWWW: Resets flyboost when the player lands.
	virtual void Landed(const FHitResult& Hit) override;

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

