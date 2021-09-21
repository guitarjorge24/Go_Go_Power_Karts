// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicationComp.h"

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKartPawn.generated.h"

UCLASS()
class GOGOPOWERKARTS_API AGoKartPawn : public APawn
{
	GENERATED_BODY()

public:
	AGoKartPawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere) 	// VisibleAnywhere so that we can select the movement component on the Editor and see its properties
	UGoKartMovementComponent* MovementComponent;
	UPROPERTY(VisibleAnywhere) 
	UGoKartMovementReplicationComp* MovementReplicationComp;
	
	FString GetEnumText(ENetRole InRole);
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
};