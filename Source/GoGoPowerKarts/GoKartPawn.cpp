// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartPawn.h"
#include "Components/InputComponent.h"

// Sets default values
AGoKartPawn::AGoKartPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGoKartPawn::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AGoKartPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	FVector Acceleration = Force / Mass;

	Velocity += Acceleration * DeltaTime; // add the effect of acceleration on this frame

	ApplyRotation(DeltaTime);

	UpdateLocationFromVelocity(DeltaTime);
}

// Called to bind functionality to input
void AGoKartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKartPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKartPawn::MoveRight);
}

void AGoKartPawn::MoveForward(float AxisValue)
{
	Throttle = AxisValue;
}

void AGoKartPawn::MoveRight(float AxisValue)
{
	SteeringThrow = AxisValue;
}

void AGoKartPawn::ApplyRotation(float DeltaTime)
{
	float RotationAngleDegrees = MadDegreesPerSecond * DeltaTime * SteeringThrow;
	float RotationAngleRadians = FMath::DegreesToRadians(RotationAngleDegrees);
	FQuat RotationDelta(GetActorUpVector(), RotationAngleRadians);

	// Rotate velocity vector by Quat's angle. Otherwise, only the mesh rotates but we keep moving in the same direction.
	Velocity = RotationDelta.RotateVector(Velocity); 

	AddActorWorldRotation(RotationDelta); // Rotate the car mesh itself.
}

void AGoKartPawn::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation =  Velocity * 100 * DeltaTime; // multiply by 100 to convert cm to meters
	FHitResult SweepHitResult;
	AddActorWorldOffset(Translation, true, &SweepHitResult);
	// Reset velocity back to 0 if we hit something.
	if (SweepHitResult.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}


