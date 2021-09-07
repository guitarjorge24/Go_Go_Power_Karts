// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartPawn.h"

#include "Components/InputComponent.h"
#include "Engine/World.h"

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

	FVector ForwardForce = GetActorForwardVector() * MaxDrivingForce * Throttle;
	
	FVector NetForce = ForwardForce + GetAirResistance();
	if (!(Velocity.Size() < 0.015f))
	{
		NetForce += GetRollingResistance();
	}
	else if (Throttle < 0.1f)
	{
		Velocity = FVector::ZeroVector;
	}
	
	FVector Acceleration = NetForce / Mass;
	
	Velocity += Acceleration * DeltaTime; // add the effect of acceleration on this frame
	
	ApplyRotation(DeltaTime);
	UpdateLocationFromVelocity(DeltaTime);

	UE_LOG(LogTemp, Warning, TEXT("Speed: %f"), Velocity.Size());
}

// Called to bind functionality to input
void AGoKartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKartPawn::Server_MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKartPawn::Server_MoveRight);
}

bool AGoKartPawn::Server_MoveForward_Validate(float AxisValue)
{
	return FMath::Abs(AxisValue) <= 1.f;
}

void AGoKartPawn::Server_MoveForward_Implementation(float AxisValue)
{
	Throttle = AxisValue;
}

bool AGoKartPawn::Server_MoveRight_Validate(float AxisValue)
{
	return FMath::Abs(AxisValue) <= 1.f;
}

void AGoKartPawn::Server_MoveRight_Implementation(float AxisValue)
{
	SteeringThrow = AxisValue;
}

FVector AGoKartPawn::GetAirResistance()
{
	// Formula: Air Resistance = Speed^2 * DragCoefficient. Direction is opposite to velocity.
	// SizeSquared returns the speed of the velocity and then squares it.
	return  -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient; 
}

FVector AGoKartPawn::GetRollingResistance()
{
	float GravityAcceleration = -GetWorld()->GetGravityZ() / 100.f; // divide by 100 to convert cm/s to m/s
	float NormalForce = Mass * GravityAcceleration;
	FVector RollingResistance = -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
	return RollingResistance;
}

void AGoKartPawn::ApplyRotation(float DeltaTime)
{
	// The dot product cos will only ever be 1 or -1 since the direction the car is facing and the velocity can only be the same or the opposite. 
	float DistanceTraveledAlongTurningCircle = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngleRadians = (DistanceTraveledAlongTurningCircle / MinTurningRadius) * SteeringThrow;
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


