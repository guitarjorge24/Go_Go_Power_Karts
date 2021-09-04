// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKartPawn.generated.h"

UCLASS()
class GOGOPOWERKARTS_API AGoKartPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKartPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	FVector Velocity;

	/* Mass of the car in kg */
	UPROPERTY(EditAnywhere)
	float Mass = 1000.f;
	/* The force applied to the car when the throttle is fully down (in Newtons) */
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000.f; // 10,000N is the force required to cause a 1000kg car to accelerate at 10m/s^2 
	/* How many degrees the car can turn per second at max input axis */
	float MadDegreesPerSecond = 90.f;

	/* Higher number means more drag and thus more air resistance. Lower number means the car is more aerodynamic. */
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16.f; // 16 is the drag coefficient necessary to produce a drag of 10,000N when moving at 25m/s which is ~56mph

	float Throttle;
	float SteeringThrow;

	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);

	FVector GetAirResistance();
	void ApplyRotation(float DeltaTime);
	/* Calculates translation based on velocity. Resets velocity to zero if we collide with something */
	void UpdateLocationFromVelocity(float DeltaTime);
};
