
// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "DrawDebugHelpers.h"
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

	HandsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HandsRoot"));
	HandsRoot->SetupAttachment(VRRoot);

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(HandsRoot);
	LeftController->SetTrackingMotionSource(FXRMotionControllerBase::LeftHandSourceId);

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(HandsRoot);
	RightController->SetTrackingMotionSource(FXRMotionControllerBase::RightHandSourceId);

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(RightController);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
	
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	DestinationMarker->SetVisibility(false);

	// Setup global references
	PlayerController = Cast<APlayerController>(GetController());

	if (BlinkerMaterialBase != nullptr) {
		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);
	}
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
	
	// Move Controllers in opposite Vector // THIS DOES NOTHING?!?!?!
	RightController->AddWorldOffset(CameraOffset);

	UpdateDestinationMarker();
	UpdateBlinkers();
}

bool AVRCharacter::FindDestinationMarker(TArray<FVector> &OutPath, FVector& OutLocation) {

	FVector Start = RightController->GetComponentLocation();
	FVector LookVector = RightController->GetForwardVector();
	// LookVector = LookVector.RotateAngleAxis(30, RightController->GetRightVector());
	
	/*
		// Project in a straight line to find location

		FVector End = Start + LookVector * MaxTeleportDistance;
		FHitResult HitResult;
		bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
		//DrawDebugLine(GetWorld(), Start, End, FColor::Emerald, false, -1, 0, 10);
	*/

	// Draw parabolic arc to teleport destination
	FPredictProjectilePathParams Params(TeleportProjectileRadius, Start, LookVector * TeleportProjectileSpeed, TeleportSimulationTime, ECollisionChannel::ECC_Visibility, this);
	Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	Params.bTraceComplex = true; // to fix buggy placement

	FPredictProjectilePathResult PredictResult;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, PredictResult);

	for (FPredictProjectilePathPointData Data : PredictResult.PathData) {
		OutPath.Add(Data.Location);
	}

	if (!bHit) return false;

	FVector Out;
	FNavLocation NavLocation;
	bool bOnNavMesh = UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(PredictResult.HitResult.Location, NavLocation, TeleportProjectionExtent);

	if (!bOnNavMesh) return false;

	OutLocation = NavLocation.Location;

	return true;
}

void AVRCharacter::UpdateDestinationMarker() 
{
	FVector OutLocation;
	TArray<FVector> Path;
	bool IsAllowedToTeleport = FindDestinationMarker(Path, OutLocation);

	if (IsAllowedToTeleport) {
		DestinationMarker->SetVisibility(true);

		if (IsFading) {
			DestinationMarker->SetWorldLocation(NewTeleportLocation);
		}
		else {
			DestinationMarker->SetWorldLocation(OutLocation);
		}
		DrawTeleportPath(Path);
	}
	else {
		DestinationMarker->SetVisibility(false);
	}
;}

void AVRCharacter::UpdateBlinkers()
{
	if (RadiusVsVelocity == nullptr) return;

	float Speed = GetVelocity().Size();
	float Radius = RadiusVsVelocity->GetFloatValue(Speed);

	// UE_LOG(LogTemp, Warning, TEXT("Speed: %f"), Speed);

	BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"), Radius);

	FVector2D Center = GetVectorCenter();
	BlinkerMaterialInstance->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center.X, Center.Y, 0));
}

void AVRCharacter::DrawTeleportPath(const TArray<FVector>& Path) 
{
	UpdateSpline(Path);

	// Draw path
	for (int32 i = 0; i < Path.Num(); i++)
	{
		UStaticMeshComponent* DynamicMesh;
		// Allocate and draw meshes
		if (i >= ArcMeshObjctPool.Num()) {
			// Create new mesh for pool
			DynamicMesh = NewObject<UStaticMeshComponent>(this);
			DynamicMesh->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
			DynamicMesh->SetStaticMesh(TeleportArcMesh);
			DynamicMesh->SetMaterial(0, TeleportArcMaterial);
			DynamicMesh->RegisterComponent();
			ArcMeshObjctPool.Add(DynamicMesh);
		}
		else {
			DynamicMesh = ArcMeshObjctPool[i];
		}

		DynamicMesh->SetWorldLocation(Path[i]);
	}
}

void AVRCharacter::UpdateSpline(const TArray<FVector>& Path) {
	TeleportPath->ClearSplinePoints(false);

	for (int32 i = 0; i < Path.Num(); i++) 
	{
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);
		TeleportPath->AddPoint(FSplinePoint(i, LocalPosition, ESplinePointType::Curve));
	}
	TeleportPath->UpdateSpline();
}

FVector2D AVRCharacter::GetVectorCenter()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	if (MovementDirection.IsNearlyZero()) return FVector2D(.5, .5);

	FVector WorldStationaryLocation;
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0)
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + (MovementDirection * 1000);
	}
	else {
		WorldStationaryLocation = Camera->GetComponentLocation() - (MovementDirection * 1000);
	}
	
	if (PlayerController == nullptr) return FVector2D(.5, .5);

	FVector2D ScreenLocation;
	PlayerController->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenLocation);

	int32 ViewportX, ViewportY;
	PlayerController->GetViewportSize(ViewportX, ViewportY);

	UE_LOG(LogTemp, Warning, TEXT("ScreenLocation: %f, %f"), ScreenLocation.X, ScreenLocation.Y);
	UE_LOG(LogTemp, Warning, TEXT("ViewportSize: %i, %i"), ViewportX, ViewportY);

	ScreenLocation.X /= ViewportX;
	ScreenLocation.Y /= ViewportY;

	return ScreenLocation;
}

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
	if (IsTeleporting == false && PlayerController != nullptr && DestinationMarker->IsVisible()) {

		// Set Teleport status
		IsTeleporting = true;
		IsFading = true;
		NewTeleportLocation = DestinationMarker->GetComponentLocation();

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

	// Re-show destination marker
	IsFading = false;

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