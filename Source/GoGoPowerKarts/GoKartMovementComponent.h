// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GoKartMovementComponent.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_BODY()

	UPROPERTY()
	float Throttle = 0.f;
	UPROPERTY()
	float SteeringThrow = 0.f;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float Timestamp;

	bool IsValid() const
	{
		return FMath::Abs(Throttle) <= 1 && FMath::Abs(SteeringThrow) <= 1;
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GOGOPOWERKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGoKartMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	float Throttle;
	float SteeringThrow;
	FVector Velocity;

	/** Getter function for LastMove */
	FGoKartMove GetLastMove() { return LastMove; };
	/** Uses the input data from a Move (steering throw & throttle) and delta time to set the car's velocity for this frame.
	*   Then rotates the velocity vector according to the steering throw and finally updates the car's location based on velocity. */
	void SimulateMove(const FGoKartMove& Move);

private:
	/** Mass of the car in kg */
	UPROPERTY(EditAnywhere)
	float Mass = 1000.f;
	/** The force applied to the car when the throttle is fully down (in Newtons) */
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000.f; // 10,000N is the force required to cause a 1000kg car to accelerate at 10m/s^2 
	/** Minimum radius of the car's turning circle at full lock (in meters) */
	float MinTurningRadius = 10.f;

	/** Higher number means more drag and thus more air resistance. Lower number means the car is more aerodynamic. */
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16.f; // 16 is the drag coefficient necessary to produce a drag of 10,000N when moving at 25m/s which is ~56mph
	UPROPERTY(EditAnywhere)
	/** Higher number means more rolling resistance which slows down the car to a stop more quickly. */
	float RollingResistanceCoefficient = 0.02f;

	FGoKartMove LastMove;

	FGoKartMove CreateMove(float DeltaTime);
	
	FVector GetAirResistance();
	FVector GetRollingResistance();

	void ApplyRotation(float DeltaTime, float InSteeringThrow);
	/** Calculates translation based on velocity. Resets velocity to zero if we collide with something */
	void UpdateLocationFromVelocity(float DeltaTime);
};
