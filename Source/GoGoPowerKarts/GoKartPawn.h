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
	float Throttle;
	
	void MoveForward(float AxisValue);
	/* Calculates translation based on velocity. Resets velocity to zero if we collide with something */
	void UpdateLocationFromVelocity(float DeltaTime);
};
