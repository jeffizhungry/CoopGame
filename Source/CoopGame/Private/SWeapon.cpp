// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame/CoopGame.h"
#include "TimerManager.h"

// Sets default values
ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "BeamEnd";

	BaseDamage = 20.0f;
	RateOfFire = 9.0f;
}

// Called when the game starts or when spawned
void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 1 / RateOfFire;
}

void ASWeapon::StartFire()
{
	float FirstDelay = LastFiredTime + TimeBetweenShots - GetWorld()->TimeSeconds;
	if (FirstDelay < 0) {
		FirstDelay = 0.0f;
	}
	GetWorldTimerManager().SetTimer(TimerHandleTimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandleTimeBetweenShots);
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
		FVector ShotDirection = EyeRotation.Vector();

		// Tracer particle end parameter
		FVector TraceEndPoint = TraceEnd;

		// Ignore this weapon and owner, and do complex trace so we know exactly where it's hit
		// Complex is a bit more expensive, but it's okay for now.
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(ThisOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		// If this trace hit something
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, COLLISION_WEAPON, QueryParams)) {
			AActor* HitActor = Hit.GetActor();

			EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			// Apply damage (bonus for headshots)
			float ApplyDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESH_VULNERABLE) {
				ApplyDamage *= 4;
			}
			UGameplayStatics::ApplyPointDamage(HitActor, ApplyDamage, ShotDirection, Hit, ThisOwner->GetInstigatorController(), this, DamageType);

			// Show impact effect
			UParticleSystem* ImpactEffect = nullptr;
			switch (SurfaceType) {
			case SURFACE_FLESH_DEFAULT:
				UE_LOG(LogTemp, Warning, TEXT("Hit flesh default surface! %d"), SurfaceType);
				ImpactEffect = FleshImpactEffect;
				break;
			case SURFACE_FLESH_VULNERABLE:
				UE_LOG(LogTemp, Warning, TEXT("Hit flesh vulnerable surface! %d"), SurfaceType);
				ImpactEffect = FleshImpactEffect;
				break;
			default:
				UE_LOG(LogTemp, Warning, TEXT("Hit default surface! %d"), SurfaceType);
				ImpactEffect = DefaultImpactEffect;
				break;
			}
			if (ImpactEffect) {
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
			}

			TraceEndPoint = Hit.ImpactPoint;
		}

		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);

		PlayFireEffects(TraceEndPoint);

		LastFiredTime = GetWorld()->TimeSeconds;
	}
}

void ASWeapon::PlayFireEffects(FVector TraceEndPoint)
{
	// Show muzzle flash regardless of hit
	if (MuzzleEffect) {
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	// Show effect regardless of hit
	if (TracerEffect) {
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp) {
			TracerComp->SetVectorParameter(TracerTargetName, TraceEndPoint);
		}
	}

	APawn* ThisOwner = Cast<APawn>(GetOwner());
	if (ThisOwner) {
		APlayerController* PlayerController = Cast<APlayerController>(ThisOwner->GetController());
		if (PlayerController) {
			PlayerController->ClientPlayCameraShake(FireCameraShake);
		}
	}
}
