// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GoKartMovementComponent.h"

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKartPawn.generated.h"

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
	UPROPERTY(EditAnywhere) // EditAnywhere so that we can select the movement component on the Editor and see+edit its properties.
	UGoKartMovementComponent* MovementComponent;
	
	UPROPERTY(ReplicatedUsing=OnRep_ServerState)
	FGoKartState ServerState;


	FString GetEnumText(ENetRole InRole);

	/** Resets the car's transform and velocity to that of the server. Then removes all the moves that have already been simulated on the server from
	 * the queue. Then reapplies all the remaining unacknowledged moves over 1 tick frame to allow the server state to catch up to the local state. */
	UFUNCTION()
	void OnRep_ServerState(); // OnRep functions must be UFUNCTION
	TArray<FGoKartMove> UnacknowledgedMoves;

	/** Sets the UnacknowledgedMoves array to a new array that only contains the moves that happened after ServerState.LastMove */
	void ClearAcknowledgedMoves(FGoKartMove LastMove);

	/** Server RPC that sends Move to the server, simulates the move on the server, and then updates the ServerState */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

	
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
};
