// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementReplicationComp.h"

#include "Net/UnrealNetwork.h"

UGoKartMovementReplicationComp::UGoKartMovementReplicationComp()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);
}

void UGoKartMovementReplicationComp::BeginPlay()
{
	Super::BeginPlay();
	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}

void UGoKartMovementReplicationComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ensure(MovementComponent)) return;

	FGoKartMove LastMove = MovementComponent->GetLastMove();

	// If this pawn is on a client and is locally controlled
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowledgedMoves.Add(LastMove);
		UE_LOG(LogTemp, Warning, TEXT("Num of Unacknowledged Moves in Queue: %d"), UnacknowledgedMoves.Num());

		Server_SendMove(LastMove);
	}

	// If this pawn is on a listen server (it has authority) but the pawn is also locally controlled
	if (GetOwnerRole() == ROLE_Authority && Cast<APawn>(GetOwner())->IsLocallyControlled())
	{
		// The server has no need of adding moves to the UnacknowledgedMoves queue.
		UpdateServerState(LastMove);
	}

	// If this pawn is on a client and is not locally controlled
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ClientTick(DeltaTime);
	}
}

void UGoKartMovementReplicationComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGoKartMovementReplicationComp, ServerState);
}

/** Uses argument to update the ServerState struct members (LastMove, Transform, Velocity) */
void UGoKartMovementReplicationComp::UpdateServerState(const FGoKartMove& Move)
{
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetTransform();
	ServerState.Velocity = MovementComponent->Velocity;
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
	UpdateServerState(Move);
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
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		AutonomousProxy_OnRep_ServerState();
	}
	else if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		SimulateProxy_OnRep_ServerState();
	}
}

void UGoKartMovementReplicationComp::AutonomousProxy_OnRep_ServerState()
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

void UGoKartMovementReplicationComp::SimulateProxy_OnRep_ServerState()
{
	ClientTimeBetweenLast2Updates = ClientTimeSinceLastServerUpdate;
	ClientTimeSinceLastServerUpdate = 0;

	ClientStartLocation = GetOwner()->GetActorLocation();
}

void UGoKartMovementReplicationComp::ClientTick(float DeltaTime)
{
	ClientTimeSinceLastServerUpdate += DeltaTime;

	// Lerping very small numbers 
	if (ClientTimeBetweenLast2Updates < KINDA_SMALL_NUMBER) return;

	FVector TargetLocation = ServerState.Transform.GetLocation();
	float LerpRatio = ClientTimeSinceLastServerUpdate / ClientTimeBetweenLast2Updates;
	FVector StartLocation = ClientStartLocation;

	// StartLocation is the location you start on the client. TargetLocation is the location the car has moved to on the server that we want to lerp to.
	FVector NewLocation = FMath::LerpStable(StartLocation, TargetLocation, LerpRatio);
	GetOwner()->SetActorLocation(NewLocation);
}
