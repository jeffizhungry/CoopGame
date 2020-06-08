// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "DrawDebugHelpers.h"

// Sets default values
ASWeapon::ASWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
}

// Called when the game starts or when spawned
void ASWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASWeapon::Fire()
{
	// Trace from pawn eyes to crosshair location
	AActor* ThisOwner = GetOwner();
	if (ThisOwner) {
		FVector EyeLocation;
		FRotator EyeRotation;
		ThisOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector TraceStart = EyeLocation;
		FVector TraceEnd = EyeLocation + (EyeRotation.Vector() * 10000);

		// Ignore this weapon and owner, and do complex trace so we know exactly where it's hit
		// Complex is a bit more expensive, but it's okay for now.
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(ThisOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;

		// If this trace hit something
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams)) {

		}
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);
	}
}

// Called every frame
void ASWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

