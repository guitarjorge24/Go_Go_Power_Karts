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
	float ProposedTime = ClientSimulatedTime + Move.DeltaTime;
	bool ClientNotRunningAheadOfServerTime = ProposedTime < GetWorld()->TimeSeconds;
	if (!ClientNotRunningAheadOfServerTime)
	{
		UE_LOG(LogTemp, Error, TEXT("Delta time received from client is too large"));
		return false;
	}
	if(!Move.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Move received from client is invalid"));
		return false;
	}
	return true;
}

void UGoKartMovementReplicationComp::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (!ensure(MovementComponent)) return;

	ClientSimulatedTime += Move.DeltaTime;
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
	if (!ensure(MovementComponent)) return;

	ClientTimeBetweenLast2Updates = ClientTimeSinceLastServerUpdate;
	ClientTimeSinceLastServerUpdate = 0;

	if (MeshOffsetRoot)
	{
		// Get location and rotation from server and store it in the client
		ClientStartTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());
		ClientStartTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());
	}
	ClientStartVelocity = MovementComponent->Velocity;

	// Only update actor collider on OnRep so that the collider location is "official" and not based on the simulated cubic interpolation
	GetOwner()->SetActorTransform(ServerState.Transform);
}

void UGoKartMovementReplicationComp::ClientTick(float DeltaTime)
{
	ClientTimeSinceLastServerUpdate += DeltaTime;

	// interpolating between very small numbers can result in errors
	if (ClientTimeBetweenLast2Updates < KINDA_SMALL_NUMBER) return;
	if (!ensure(MovementComponent)) return;

	float LerpRatio = ClientTimeSinceLastServerUpdate / ClientTimeBetweenLast2Updates;
	FHermiteCubicSpline Spline = CreateSpline();

	InterpolateLocation(LerpRatio, Spline);
	InterpolateVelocity(LerpRatio, Spline);
	InterpolateRotation(LerpRatio);
}

FHermiteCubicSpline UGoKartMovementReplicationComp::CreateSpline()
{
	FHermiteCubicSpline Spline;

	Spline.TargetLocation = ServerState.Transform.GetLocation();
	Spline.StartLocation = ClientStartTransform.GetLocation();

	// derivative = delta location / LerpRatio = delta location/ClientTimeSinceLastServerUpdate *  ClientTimeBetweenLast2Updates 
	Spline.StartDerivative = ClientStartVelocity * ClientTimeBetweenLast2Updates * 100; // *100 because velocity is m/s but location is cm
	Spline.TargetDerivative = ServerState.Velocity * ClientTimeBetweenLast2Updates * 100;

	// #Archived: Old method using linear interpolation
	// StartLocation is the location you start on the client. TargetLocation is the location the car has moved to on the server that we want to lerp to.
	// FVector NewLocation = FMath::LerpStable(StartLocation, TargetLocation, LerpRatio);

	return Spline;
}

void UGoKartMovementReplicationComp::InterpolateLocation(float LerpRatio, const FHermiteCubicSpline& Spline)
{
	if (!MeshOffsetRoot) return;
	FVector NewLocation = Spline.InterpolateLocation(LerpRatio);
	MeshOffsetRoot->SetWorldLocation(NewLocation);
}

void UGoKartMovementReplicationComp::InterpolateVelocity(float LerpRatio, const FHermiteCubicSpline& Spline)
{
	// This is the new velocity in between the start and target location
	FVector NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector NewVelocity = NewDerivative / (ClientTimeBetweenLast2Updates * 100);
	MovementComponent->Velocity = NewVelocity;
}

void UGoKartMovementReplicationComp::InterpolateRotation(float LerpRatio)
{
	if (!MeshOffsetRoot) return;
	FQuat NewRotation = FQuat::Slerp(ClientStartTransform.GetRotation(), ServerState.Transform.GetRotation(), LerpRatio);
	MeshOffsetRoot->SetWorldRotation(NewRotation);
}