// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"


UCLASS()
class VRARCHEXPLORER_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	void UpdateDestinationMarker();

	void MoveForward(float throttle);
	void MoveRight(float throttle);

	void BeginTeleport();
	void EndTeleport();

private:

	APlayerController* PlayerController;

	UPROPERTY(EditAnywhere)
	float MaxTeleportDistance = 1000;

	UPROPERTY(EditAnywhere)
	float TeleportFadeTime = 0.6f;

private:

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;
};
