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
	TArray<FGoKartMove> UnacknowledgedMoves;
	/** Used to store the car's transform & velocity in the server and the last move that was simulated on the server */
	UPROPERTY(ReplicatedUsing=OnRep_ServerState)
	FGoKartState ServerState;
	
	/** Server RPC that sends Move to the server, simulates the move on the server, and then updates the ServerState */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);
	/** Sets the UnacknowledgedMoves array to a new array that only contains the moves that happened after ServerState.LastMove */
	void ClearAcknowledgedMoves(FGoKartMove LastMove);
	/** Resets the car's transform and velocity to that of the server. Then removes all the moves that have already been simulated on the server from
	* the queue. Then reapplies all the remaining unacknowledged moves over 1 tick frame to allow the server state to catch up to the local state. */
	UFUNCTION()
	void OnRep_ServerState(); // OnRep functions must be UFUNCTION
};
