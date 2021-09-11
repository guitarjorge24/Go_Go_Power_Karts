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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	FVector Velocity;
	
	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedTransform)
	FTransform ReplicatedTransform;
	UFUNCTION()
	void OnRep_ReplicatedTransform(); // OnRep functions must be UFUNCTION

	/* Mass of the car in kg */
	UPROPERTY(EditAnywhere)
	float Mass = 1000.f;
	/* The force applied to the car when the throttle is fully down (in Newtons) */
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000.f; // 10,000N is the force required to cause a 1000kg car to accelerate at 10m/s^2 
	/* Minimum radius of the car's turning circle at full lock (in meters) */
	float MinTurningRadius = 10.f;

	/* Higher number means more drag and thus more air resistance. Lower number means the car is more aerodynamic. */
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16.f; // 16 is the drag coefficient necessary to produce a drag of 10,000N when moving at 25m/s which is ~56mph
	UPROPERTY(EditAnywhere)
	/* Higher number means more rolling resistance which slows down the car to a stop more quickly. */
	float RollingResistanceCoefficient = 0.02f;

	UPROPERTY(Replicated)
	float Throttle;
	UPROPERTY(Replicated)
	float SteeringThrow;
	
	FString GetEnumText(ENetRole InRole);
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveForward(float AxisValue);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveRight(float AxisValue);

	FVector GetAirResistance();
	FVector GetRollingResistance();
	void ApplyRotation(float DeltaTime);
	/* Calculates translation based on velocity. Resets velocity to zero if we collide with something */
	void UpdateLocationFromVelocity(float DeltaTime);
};
