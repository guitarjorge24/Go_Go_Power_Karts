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

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("MovementComp"));
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

	if (!ensure(MovementComponent)) return;

	// If this pawn is on a client and is locally controlled
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		UnacknowledgedMoves.Add(Move);
		UE_LOG(LogTemp, Warning, TEXT("Num of Unacknowledged Moves in Queue: %d"), UnacknowledgedMoves.Num());

		MovementComponent->SimulateMove(Move);
		Server_SendMove(Move);
	}

	// If this pawn is on a listen server (it has authority) but the pawn is also locally controlled
	if (GetLocalRole() == ROLE_Authority && IsLocallyControlled())
	{
		// The server has no need of adding moves to the UnacknowledgedMoves queue.

		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	// If this pawn is on a client and is not locally controlled
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}

	// Print the ENetRole and Speed of each car on the screen
	DrawDebugString(GetWorld(), FVector(0, 0, 150), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);
	DrawDebugString(GetWorld(), FVector(0, 0, 110), FString::Printf(TEXT("Speed: %f"), MovementComponent->Velocity.Size()), this, FColor::Magenta, DeltaTime);
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
	if (!ensure(MovementComponent)) return;

	UE_LOG(LogTemp, Warning, TEXT("OnRep_ServerState called."))

	SetActorTransform(ServerState.Transform);
	MovementComponent->Velocity = ServerState.Velocity;

	ClearAcknowledgedMoves(ServerState.LastMove);

	// Locally replay all the moves that have not yet been simulated on the server.
	// This is all done in 1 frame resulting in a mix of server state + local prediction  
	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void AGoKartPawn::MoveForward(float AxisValue)
{
	if (!ensure(MovementComponent)) return;
	MovementComponent->Throttle = AxisValue;
}

void AGoKartPawn::MoveRight(float AxisValue)
{
	if (!ensure(MovementComponent)) return;
	MovementComponent->SteeringThrow = AxisValue;
}

bool AGoKartPawn::Server_SendMove_Validate(FGoKartMove Move)
{
	// return FMath::Abs(Move) <= 1.f;
	return true; // #ToDo: Improve 
}

void AGoKartPawn::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (!ensure(MovementComponent)) return;

	MovementComponent->SimulateMove(Move);
	ServerState.LastMove = Move;
	ServerState.Transform = GetTransform();
	ServerState.Velocity = MovementComponent->Velocity;
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
