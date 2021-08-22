// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartPawn.h"
#include "Components/InputComponent.h"

// Sets default values
AGoKartPawn::AGoKartPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGoKartPawn::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AGoKartPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Translation =  Velocity * 100 * DeltaTime; // convert cm to meters
	AddActorWorldOffset(Translation);
}

// Called to bind functionality to input
void AGoKartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKartPawn::MoveForward);
}

void AGoKartPawn::MoveForward(float AxisValue)
{
	Velocity = GetActorForwardVector() * (20.f * AxisValue);
}