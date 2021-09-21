// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementReplicationComp.h"

#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UGoKartMovementReplicationComp::UGoKartMovementReplicationComp()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}

// Called when the game starts
void UGoKartMovementReplicationComp::BeginPlay()
{
	Super::BeginPlay();
	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}

// Called every frame
void UGoKartMovementReplicationComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ensure(MovementComponent)) return;

	// If this pawn is on a client and is locally controlled
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		UnacknowledgedMoves.Add(Move);
		UE_LOG(LogTemp, Warning, TEXT("Num of Unacknowledged Moves in Queue: %d"), UnacknowledgedMoves.Num());

		MovementComponent->SimulateMove(Move);
		Server_SendMove(Move);
	}

	// If this pawn is on a listen server (it has authority) but the pawn is also locally controlled
	if (GetOwnerRole() == ROLE_Authority && Cast<APawn>(GetOwner())->IsLocallyControlled())
	{
		// The server has no need of adding moves to the UnacknowledgedMoves queue.

		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	// If this pawn is on a client and is not locally controlled
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}
}

void UGoKartMovementReplicationComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGoKartMovementReplicationComp, ServerState);
}

bool UGoKartMovementReplicationComp::Server_SendMove_Validate(FGoKartMove Move)
{
	// return FMath::Abs(Move) <= 1.f;
	return true; // #ToDo: Improve 
}

void UGoKartMovementReplicationComp::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (!ensure(MovementComponent)) return;

	MovementComponent->SimulateMove(Move);
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetTransform();
	ServerState.Velocity = MovementComponent->Velocity;
}

void UGoKartMovementReplicationComp::ClearAcknowledgedMoves(FGoKartMove LastMove)
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

void UGoKartMovementReplicationComp::OnRep_ServerState()
{
	if (!ensure(MovementComponent)) return;

	UE_LOG(LogTemp, Warning, TEXT("OnRep_ServerState called."))

	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->Velocity = ServerState.Velocity;

	ClearAcknowledgedMoves(ServerState.LastMove);

	// Locally replay all the moves that have not yet been simulated on the server.
	// This is all done in 1 frame resulting in a mix of server state + local prediction  
	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}
