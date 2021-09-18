// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartPawn.h"

#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AGoKartPawn::AGoKartPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AGoKartPawn::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
}

void AGoKartPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// true for autonomous proxy, false for simulated proxy. Also true for locally controlled pawn with Authority on a listen server
	if (IsLocallyControlled()) 
	{
		FGoKartMove Move = CreateMove(DeltaTime);

		// In the case of a host listen server where the server has a locally controlled autonomous proxy.
		// The server has no need of adding things to the queue.
		if(!HasAuthority())
		{
			UnacknowledgedMoves.Add(Move);
			UE_LOG(LogTemp, Warning, TEXT("Queue Lenght: %d"), UnacknowledgedMoves.Num());
		}
		Server_SendMove(Move);
		SimulateMove(Move);
	}

	// Print the ENetRole and Speed of each car on the screen
	DrawDebugString(GetWorld(), FVector(0, 0, 150), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);
	DrawDebugString(GetWorld(), FVector(0, 0, 110), FString::Printf(TEXT("Speed: %f"), Velocity.Size()), this, FColor::Magenta, DeltaTime);
}

// Called to bind functionality to input
void AGoKartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKartPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKartPawn::MoveRight);
}

void AGoKartPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGoKartPawn, ServerState);
}

void AGoKartPawn::OnRep_ServerState()
{
	UE_LOG(LogTemp, Warning, TEXT("OnRep_ServerState called."))

	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;

	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		SimulateMove(Move);
	}
	
}

void AGoKartPawn::MoveForward(float AxisValue)
{
	Throttle = AxisValue;
}

void AGoKartPawn::MoveRight(float AxisValue)
{
	SteeringThrow = AxisValue;
}

bool AGoKartPawn::Server_SendMove_Validate(FGoKartMove Move)
{
	// return FMath::Abs(Move) <= 1.f;
	return true; // #ToDo: Improve 
}

void AGoKartPawn::Server_SendMove_Implementation(FGoKartMove Move)
{
	SimulateMove(Move);
	ServerState.LastMove = Move;
	ServerState.Transform = GetTransform();
	ServerState.Velocity = Velocity;
}

FString AGoKartPawn::GetEnumText(ENetRole InRole)
{
	switch (InRole)
	{
	case ROLE_None: return "None";
	case ROLE_SimulatedProxy: return "ROLE_SimulatedProxy";
	case ROLE_AutonomousProxy: return "ROLE_AutonomousProxy";
	case ROLE_Authority: return "ROLE_Authority";
	case ROLE_MAX: return "ROLE_MAX";
	default: return "Error";
	}
}

FGoKartMove AGoKartPawn::CreateMove(float DeltaTime)
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.Throttle = Throttle;
	Move.SteeringThrow = SteeringThrow;
	Move.Timestamp = GetWorld()->TimeSeconds;
	return Move;
}

void AGoKartPawn::ClearAcknowledgedMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if (Move.Timestamp > LastMove.Timestamp)
		{
			NewMoves.Add(Move);
		}
	}

	UnacknowledgedMoves = NewMoves;
}

void AGoKartPawn::SimulateMove(const FGoKartMove& Move)
{
	FVector ForwardForce = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

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

FVector AGoKartPawn::GetAirResistance()
{
	// Formula: Air Resistance = Speed^2 * DragCoefficient. Direction is opposite to velocity.
	// SizeSquared returns the speed of the velocity and then squares it.
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKartPawn::GetRollingResistance()
{
	float GravityAcceleration = -GetWorld()->GetGravityZ() / 100.f; // divide by 100 to convert cm/s to m/s
	float NormalForce = Mass * GravityAcceleration;
	FVector RollingResistance = -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
	return RollingResistance;
}

void AGoKartPawn::ApplyRotation(float DeltaTime, float InSteeringThrow)
{
	// The dot product cos will only ever be 1 or -1 since the direction the car is facing and the velocity can only be the same or the opposite. 
	float DistanceTraveledAlongTurningCircle = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngleRadians = (DistanceTraveledAlongTurningCircle / MinTurningRadius) * InSteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), RotationAngleRadians);

	// Rotate velocity vector by Quat's angle. Otherwise, only the mesh rotates but we keep moving in the same direction.
	Velocity = RotationDelta.RotateVector(Velocity);

	AddActorWorldRotation(RotationDelta); // Rotate the car mesh itself.
}

void AGoKartPawn::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * 100 * DeltaTime; // multiply by 100 to convert cm to meters
	FHitResult SweepHitResult;
	AddActorWorldOffset(Translation, true, &SweepHitResult);
	// Reset velocity back to 0 if we hit something.
	if (SweepHitResult.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}
