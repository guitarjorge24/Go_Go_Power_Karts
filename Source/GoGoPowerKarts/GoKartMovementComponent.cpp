// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementComponent.h"

// Sets default values for this component's properties
UGoKartMovementComponent::UGoKartMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}


// Called when the game starts
void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
}


// Called every frame
void UGoKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

FGoKartMove UGoKartMovementComponent::CreateMove(float DeltaTime)
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.Throttle = Throttle;
	Move.SteeringThrow = SteeringThrow;
	Move.Timestamp = GetWorld()->TimeSeconds;
	return Move;
}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& Move)
{
	FVector ForwardForce = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

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

	Velocity += Acceleration * Move.DeltaTime; // add the effect of acceleration on this frame

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);
	UpdateLocationFromVelocity(Move.DeltaTime);
}

FVector UGoKartMovementComponent::GetAirResistance()
{
	// Formula: Air Resistance = Speed^2 * DragCoefficient. Direction is opposite to velocity.
	// SizeSquared returns the speed of the velocity and then squares it.
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector UGoKartMovementComponent::GetRollingResistance()
{
	float GravityAcceleration = -GetWorld()->GetGravityZ() / 100.f; // divide by 100 to convert cm/s to m/s
	float NormalForce = Mass * GravityAcceleration;
	FVector RollingResistance = -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
	return RollingResistance;
}

void UGoKartMovementComponent::ApplyRotation(float DeltaTime, float InSteeringThrow)
{
	// The dot product cos will only ever be 1 or -1 since the direction the car is facing and the velocity can only be the same or the opposite. 
	float DistanceTraveledAlongTurningCircle = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngleRadians = (DistanceTraveledAlongTurningCircle / MinTurningRadius) * InSteeringThrow;
	FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngleRadians);

	// Rotate velocity vector by Quat's angle. Otherwise, only the mesh rotates but we keep moving in the same direction.
	Velocity = RotationDelta.RotateVector(Velocity);

	GetOwner()->AddActorWorldRotation(RotationDelta); // Rotate the car mesh itself.
}

void UGoKartMovementComponent::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * 100 * DeltaTime; // multiply by 100 to convert cm to meters
	FHitResult SweepHitResult;
	GetOwner()->AddActorWorldOffset(Translation, true, &SweepHitResult);
	// Reset velocity back to 0 if we hit something.
	if (SweepHitResult.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

