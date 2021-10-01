// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GoKartMovementComponent.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementReplicationComp.generated.h"

USTRUCT()
struct FGoKartState
{
	GENERATED_BODY()

	UPROPERTY()
	FTransform Transform;
	UPROPERTY()
	FVector Velocity;
	UPROPERTY()
	FGoKartMove LastMove;
};

struct FHermiteCubicSpline
{
	FVector StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio;

	FVector InterpolateLocation(float InLerpRatio) const
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, InLerpRatio);
	}
	
	FVector InterpolateDerivative(float InLerpRatio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, InLerpRatio);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GOGOPOWERKARTS_API UGoKartMovementReplicationComp : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGoKartMovementReplicationComp();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;
	UPROPERTY()
	USceneComponent* MeshOffsetRoot;
	TArray<FGoKartMove> UnacknowledgedMoves;
	
	float ClientTimeSinceLastServerUpdate = 0.f;
	float ClientTimeBetweenLast2Updates = 0.f;
	FTransform ClientStartTransform;
	FVector ClientStartVelocity;
	
	/** Used to store the car's transform & velocity in the server and the last move that was simulated on the server */
	UPROPERTY(ReplicatedUsing=OnRep_ServerState)
	FGoKartState ServerState;

	UFUNCTION(BlueprintCallable)
	void SetMeshOffsetRoot(USceneComponent* Root) {MeshOffsetRoot = Root;}
	
	void UpdateServerState(const FGoKartMove& Move);
	/** Server RPC that sends Move to the server, simulates the move on the server, and then calls UpdateServerState() */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);
	void AutonomousProxy_OnRep_ServerState();
	void SimulateProxy_OnRep_ServerState();
	/** Sets the UnacknowledgedMoves array to a new array that only contains the moves that happened after ServerState.LastMove */
	void ClearAcknowledgedMoves(FGoKartMove LastMove);
	/** Resets the car's transform and velocity to that of the server. Then removes all the moves that have already been simulated on the server from
	* the queue. Then reapplies all the remaining unacknowledged moves over 1 tick frame to allow the server state to catch up to the local state. */
	UFUNCTION()
	void OnRep_ServerState(); // OnRep functions must be UFUNCTION

	void ClientTick(float DeltaTime);
	FHermiteCubicSpline CreateSpline();
	void InterpolateLocation(float LerpRatio, const FHermiteCubicSpline& Spline);
	void InterpolateVelocity(float LerpRatio, const FHermiteCubicSpline& Spline);
	void InterpolateRotation(float LerpRatio);
};
