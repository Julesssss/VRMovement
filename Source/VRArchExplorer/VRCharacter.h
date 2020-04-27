// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "Components/PostProcessComponent.h"
#include "Components/SplineComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Curves/CurveFloat.h"
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
	// Functions

	bool FindDestinationMarker(TArray<FVector>& OutPath, FVector& OutLocation);
	void UpdateDestinationMarker();
	void UpdateBlinkers();
	void DrawTeleportPath(const TArray<FVector> &Path);
	void UpdateSpline(const TArray<FVector> &Path);
	FVector2D GetVectorCenter();

	void MoveForward(float throttle);
	void MoveRight(float throttle);

	void BeginTeleport();
	void DoTeleport();
	void EndTeleport();

	void CameraFade(float FromAlpha, float ToAlpha, bool ShouldHold);

private:
	// Globals

	APlayerController* PlayerController;

	bool IsTeleporting = false;

	bool IsFading = false;

	FVector NewTeleportLocation;

	// References

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* LeftController;

	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* RightController;

	UPROPERTY(VisibleAnywhere)
	class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
	class USceneComponent* HandsRoot;

	UPROPERTY(VisibleAnywhere)
	class USplineComponent* TeleportPath;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY(VisibleAnywhere)
	class UPostProcessComponent* PostProcessComponent;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DynamicMesh;

	UPROPERTY(VisibleAnywhere)
	UMaterialInstanceDynamic* BlinkerMaterialInstance;

	UPROPERTY(VisibleAnywhere)
	TArray<class UStaticMeshComponent*> ArcMeshObjctPool;

	// Editable

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* BlinkerMaterialBase;

	UPROPERTY(EditAnywhere)
	class UCurveFloat* RadiusVsVelocity;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileSpeed = 1000;

	UPROPERTY(EditAnywhere)
	float TeleportSimulationTime = 5;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileRadius = 10;

	UPROPERTY(EditAnywhere)
	float TeleportFadeTime = 0.6f;

	UPROPERTY(EditAnywhere)
	float TeleportPauseTime = 0.2f;

	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(100, 100, 100);

	// Editable by Blueprint

	UPROPERTY(EditDefaultsOnly)
	UStaticMesh* TeleportArcMesh;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* TeleportArcMaterial;
};
