// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartPawn.h"

#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

// Sets default values
AGoKartPawn::AGoKartPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("MovementComp"));
	MovementReplicationComp = CreateDefaultSubobject<UGoKartMovementReplicationComp>(TEXT("MovementReplicationComp"));
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


