
// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	DestinationMarker->SetVisibility(true);

	PlayerController = Cast<APlayerController>(GetController());
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Calculate camera (player) movement in playspace
	FVector CameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	CameraOffset.Z = 0;

	// Move Actor to that position.
	AddActorWorldOffset(CameraOffset);
	
	// Move VRRoot in opposite Vector
	VRRoot->AddWorldOffset(-CameraOffset);

	UpdateDestinationMarker();
}

bool AVRCharacter::FindDestinationMarker(FVector& OutLocation) {
	FHitResult HitResult;
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * MaxTeleportDistance;
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);

	if (!bHit) return false;

	FVector Out;
	FNavLocation NavLocation;
	bool bOnNavMesh = UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(HitResult.Location, NavLocation, TeleportProjectionExtent);

	if (!bOnNavMesh) return false;

	OutLocation = NavLocation.Location;

	return bHit && bOnNavMesh;
}

void AVRCharacter::UpdateDestinationMarker() 
{
	FVector OutLocation;
	bool IsAllowedToTeleport = FindDestinationMarker(OutLocation);

	if (IsAllowedToTeleport) {
		// DrawDebugLine(GetWorld(), Start, End, FColor::Emerald, true, -1, 0, 10);
		DestinationMarker->SetWorldLocation(OutLocation);
		DestinationMarker->SetVisibility(true);
	}
	else {
		DestinationMarker->SetVisibility(false);
	}
;}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);
}

void AVRCharacter::MoveForward(float throttle)
{
	AddMovementInput(throttle * Camera->GetForwardVector());
}

void AVRCharacter::MoveRight(float throttle)
{
	AddMovementInput(throttle * Camera->GetRightVector());
}

void AVRCharacter::BeginTeleport()
{

	if (IsTeleporting == false && PlayerController != nullptr) {

		// Set Teleport status
		IsTeleporting = true;

		CameraFade(0, 1, true);

		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::DoTeleport, TeleportFadeTime, false);
	}
}

void AVRCharacter::DoTeleport()
{
	// Get Marker location
	FVector TeleportMarkerLocation = DestinationMarker->GetComponentLocation();
	
	// Set correct player height/location
	TeleportMarkerLocation += GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() * GetActorUpVector();
	SetActorLocation(TeleportMarkerLocation);

	// Hold dark screen for a moment
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::EndTeleport, TeleportPauseTime, false);
}

void AVRCharacter::EndTeleport()
{
	// Fade back in
	CameraFade(1, 0, false);

	// Allow user to teleport as fade in occurs
	IsTeleporting = false;
}

void AVRCharacter::CameraFade(float FromAlpha, float ToAlpha, bool ShouldHold)
{
	if (PlayerController != nullptr)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeTime, FLinearColor::Black, true, ShouldHold);
	}
}

