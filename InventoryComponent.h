// ============================================================
// WeaponComponent.h
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    Pistol      UMETA(DisplayName = "Pistol"),
    Shotgun     UMETA(DisplayName = "Shotgun"),
    SMG         UMETA(DisplayName = "SMG"),
    Rifle       UMETA(DisplayName = "Rifle"),
    Knife       UMETA(DisplayName = "Knife"),
    Flamethrower UMETA(DisplayName = "Flamethrower"),
    Grenade     UMETA(DisplayName = "Grenade"),
    None        UMETA(DisplayName = "None")
};

USTRUCT(BlueprintType)
struct FWeaponData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName WeaponID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EWeaponType WeaponType = EWeaponType::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Damage = 30.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float HeadshotMultiplier = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MagazineSize = 12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CurrentAmmo = 12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ReserveAmmo = 60;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FireRate = 0.3f;    // Seconds between shots
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ReloadTime = 2.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxRange = 5000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float SpreadAngle = 1.5f; // Degrees
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PelletsPerShot = 1; // Shotgun > 1
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName AmmoType;           // Matches item ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite) class UAnimMontage* FireMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) class UAnimMontage* ReloadMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) class USoundBase* FireSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) class USoundBase* EmptySound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) class USoundBase* ReloadSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) class UNiagaraSystem* MuzzleFlashFX;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) class UNiagaraSystem* ImpactFX;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFired, FWeaponData, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloaded, FWeaponData, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, CurrentAmmo, int32, ReserveAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEquipped, FWeaponData, Weapon);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RESIDENTEVILCLONE_API UWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon")
    FWeaponData CurrentWeapon;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon")
    bool bHasWeapon = false;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon")
    bool bIsAiming = false;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon")
    bool bIsReloading = false;

    float FireCooldownTimer = 0.f;

    // Recoil
    UPROPERTY(EditAnywhere, Category = "Weapon|Recoil")
    float RecoilPitchMin = -1.5f;

    UPROPERTY(EditAnywhere, Category = "Weapon|Recoil")
    float RecoilPitchMax = -3.f;

    UPROPERTY(EditAnywhere, Category = "Weapon|Recoil")
    float RecoilYawRange = 0.8f;

    UPROPERTY(EditAnywhere, Category = "Weapon|Recoil")
    float RecoilRecoverySpeed = 5.f;

    FRotator AccumulatedRecoil = FRotator::ZeroRotator;
    FRotator TargetRecoil = FRotator::ZeroRotator;

public:
    UPROPERTY(BlueprintAssignable) FOnWeaponFired    OnWeaponFired;
    UPROPERTY(BlueprintAssignable) FOnWeaponReloaded OnWeaponReloaded;
    UPROPERTY(BlueprintAssignable) FOnAmmoChanged    OnAmmoChanged;
    UPROPERTY(BlueprintAssignable) FOnWeaponEquipped OnWeaponEquipped;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void EquipWeapon(const FWeaponData& WeaponData);

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void UnequipWeapon();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void Fire();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void Reload();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SetAiming(bool bNewAiming) { bIsAiming = bNewAiming; }

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void AddAmmo(FName AmmoType, int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool HasWeaponEquipped() const { return bHasWeapon; }

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool CanFire() const;

    UFUNCTION(BlueprintPure, Category = "Weapon")
    const FWeaponData& GetCurrentWeapon() const { return CurrentWeapon; }

private:
    void PerformHitscan();
    void PerformMeleeAttack();
    void ApplyHitDamage(const FHitResult& Hit, float Damage);
    void SpawnMuzzleFlash();
    void SpawnImpactEffect(const FHitResult& Hit);
    void ApplyRecoil();
    void RecoverRecoil(float DeltaTime);
    void FinishReload();
    FVector CalculateSpread() const;

    FTimerHandle ReloadTimerHandle;
};

// ============================================================
// WeaponComponent.cpp — Implementation
// ============================================================
// (Appended below as inline implementation for single-file clarity)

#include "WeaponComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "../AI/ZombieAI.h"

UWeaponComponent::UWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    FireCooldownTimer = FMath::Max(0.f, FireCooldownTimer - DeltaTime);
    RecoverRecoil(DeltaTime);
}

void UWeaponComponent::EquipWeapon(const FWeaponData& WeaponData)
{
    CurrentWeapon = WeaponData;
    bHasWeapon = true;
    bIsReloading = false;
    FireCooldownTimer = 0.f;
    OnWeaponEquipped.Broadcast(CurrentWeapon);
    OnAmmoChanged.Broadcast(CurrentWeapon.CurrentAmmo, CurrentWeapon.ReserveAmmo);
}

void UWeaponComponent::UnequipWeapon()
{
    bHasWeapon = false;
    bIsReloading = false;
    GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
}

bool UWeaponComponent::CanFire() const
{
    return bHasWeapon && !bIsReloading && FireCooldownTimer <= 0.f
        && CurrentWeapon.CurrentAmmo > 0;
}

void UWeaponComponent::Fire()
{
    if (!CanFire()) 
    {
        if (bHasWeapon && CurrentWeapon.CurrentAmmo <= 0 && CurrentWeapon.EmptySound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, CurrentWeapon.EmptySound,
                GetOwner()->GetActorLocation());
            // Auto-reload
            Reload();
        }
        return;
    }

    FireCooldownTimer = CurrentWeapon.FireRate;
    CurrentWeapon.CurrentAmmo--;

    if (CurrentWeapon.WeaponType == EWeaponType::Knife)
    {
        PerformMeleeAttack();
    }
    else
    {
        // Fire all pellets (shotgun support)
        for (int32 i = 0; i < CurrentWeapon.PelletsPerShot; ++i)
        {
            PerformHitscan();
        }
    }

    SpawnMuzzleFlash();
    ApplyRecoil();

    if (CurrentWeapon.FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, CurrentWeapon.FireSound,
            GetOwner()->GetActorLocation());
    }

    // Play fire animation
    if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
    {
        if (CurrentWeapon.FireMontage)
        {
            OwnerChar->PlayAnimMontage(CurrentWeapon.FireMontage);
        }
    }

    OnWeaponFired.Broadcast(CurrentWeapon);
    OnAmmoChanged.Broadcast(CurrentWeapon.CurrentAmmo, CurrentWeapon.ReserveAmmo);
}

void UWeaponComponent::PerformHitscan()
{
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar) return;

    // Find camera for accurate aim
    UCameraComponent* Cam = OwnerChar->FindComponentByClass<UCameraComponent>();
    FVector TraceStart = Cam ? Cam->GetComponentLocation() : OwnerChar->GetActorLocation();
    FVector TraceDir   = Cam ? Cam->GetForwardVector()     : OwnerChar->GetActorForwardVector();

    // Apply spread
    TraceDir = CalculateSpread();

    FVector TraceEnd = TraceStart + TraceDir * CurrentWeapon.MaxRange;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    Params.bReturnPhysicalMaterial = true;
    Params.bTraceComplex = true; // Hit individual bones

    if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
    {
        float FinalDamage = CurrentWeapon.Damage;

        // Check headshot
        if (Hit.BoneName == FName("head") || Hit.BoneName == FName("neck_01"))
        {
            FinalDamage *= CurrentWeapon.HeadshotMultiplier;
        }

        ApplyHitDamage(Hit, FinalDamage);
        SpawnImpactEffect(Hit);
    }

#if WITH_EDITOR
    DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 0.1f);
#endif
}

void UWeaponComponent::PerformMeleeAttack()
{
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar) return;

    FVector Start  = OwnerChar->GetActorLocation();
    FVector End    = Start + OwnerChar->GetActorForwardVector() * 120.f;

    TArray<FHitResult> Hits;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());

    FCollisionShape Sphere = FCollisionShape::MakeSphere(40.f);

    if (GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity,
        ECC_Pawn, Sphere, Params))
    {
        for (const FHitResult& Hit : Hits)
        {
            if (Hit.GetActor() && Hit.GetActor() != GetOwner())
            {
                ApplyHitDamage(Hit, CurrentWeapon.Damage);
            }
        }
    }
}

void UWeaponComponent::ApplyHitDamage(const FHitResult& Hit, float Damage)
{
    if (!Hit.GetActor()) return;

    FPointDamageEvent DmgEvent;
    DmgEvent.HitInfo = Hit;
    DmgEvent.ShotDirection = Hit.TraceEnd - Hit.TraceStart;
    DmgEvent.ShotDirection.Normalize();

    Hit.GetActor()->TakeDamage(Damage, DmgEvent, GetOwner()->GetInstigatorController(), GetOwner());
}

void UWeaponComponent::SpawnMuzzleFlash()
{
    if (!CurrentWeapon.MuzzleFlashFX) return;

    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar) return;

    FVector MuzzleLocation = OwnerChar->GetMesh()->GetSocketLocation(FName("Muzzle"));
    FRotator MuzzleRotation = OwnerChar->GetMesh()->GetSocketRotation(FName("Muzzle"));

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),
        CurrentWeapon.MuzzleFlashFX, MuzzleLocation, MuzzleRotation);
}

void UWeaponComponent::SpawnImpactEffect(const FHitResult& Hit)
{
    if (!CurrentWeapon.ImpactFX) return;
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),
        CurrentWeapon.ImpactFX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
}

void UWeaponComponent::ApplyRecoil()
{
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar) return;

    APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController());
    if (!PC) return;

    const float Pitch = FMath::RandRange(RecoilPitchMin, RecoilPitchMax);
    const float Yaw   = FMath::RandRange(-RecoilYawRange, RecoilYawRange);

    PC->AddPitchInput(Pitch);
    PC->AddYawInput(Yaw);

    AccumulatedRecoil += FRotator(Pitch, Yaw, 0.f);
}

void UWeaponComponent::RecoverRecoil(float DeltaTime)
{
    if (AccumulatedRecoil.IsNearlyZero(0.01f)) return;

    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar) return;

    APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController());
    if (!PC) return;

    FRotator Recovery = FMath::RInterpTo(AccumulatedRecoil, FRotator::ZeroRotator,
                                          DeltaTime, RecoilRecoverySpeed);
    FRotator Delta = Recovery - AccumulatedRecoil;

    PC->AddPitchInput(-Delta.Pitch * 0.5f);
    AccumulatedRecoil = Recovery;
}

void UWeaponComponent::Reload()
{
    if (!bHasWeapon || bIsReloading) return;
    if (CurrentWeapon.CurrentAmmo == CurrentWeapon.MagazineSize) return;
    if (CurrentWeapon.ReserveAmmo <= 0) return;

    bIsReloading = true;

    if (CurrentWeapon.ReloadSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, CurrentWeapon.ReloadSound,
            GetOwner()->GetActorLocation());
    }

    if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
    {
        if (CurrentWeapon.ReloadMontage)
        {
            OwnerChar->PlayAnimMontage(CurrentWeapon.ReloadMontage);
        }
    }

    GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, this,
        &UWeaponComponent::FinishReload, CurrentWeapon.ReloadTime, false);
}

void UWeaponComponent::FinishReload()
{
    const int32 AmmoNeeded = CurrentWeapon.MagazineSize - CurrentWeapon.CurrentAmmo;
    const int32 AmmoToLoad = FMath::Min(AmmoNeeded, CurrentWeapon.ReserveAmmo);

    CurrentWeapon.CurrentAmmo  += AmmoToLoad;
    CurrentWeapon.ReserveAmmo  -= AmmoToLoad;
    bIsReloading = false;

    OnWeaponReloaded.Broadcast(CurrentWeapon);
    OnAmmoChanged.Broadcast(CurrentWeapon.CurrentAmmo, CurrentWeapon.ReserveAmmo);
}

void UWeaponComponent::AddAmmo(FName AmmoType, int32 Amount)
{
    if (CurrentWeapon.AmmoType == AmmoType)
    {
        CurrentWeapon.ReserveAmmo += Amount;
        OnAmmoChanged.Broadcast(CurrentWeapon.CurrentAmmo, CurrentWeapon.ReserveAmmo);
    }
}

FVector UWeaponComponent::CalculateSpread() const
{
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar) return FVector::ForwardVector;

    UCameraComponent* Cam = OwnerChar->FindComponentByClass<UCameraComponent>();
    FVector BaseDir = Cam ? Cam->GetForwardVector() : OwnerChar->GetActorForwardVector();

    // Reduce spread when aiming
    float EffectiveSpread = bIsAiming
        ? CurrentWeapon.SpreadAngle * 0.3f
        : CurrentWeapon.SpreadAngle;

    const float AngleRad = FMath::DegreesToRadians(EffectiveSpread);
    const float X = FMath::RandRange(-AngleRad, AngleRad);
    const float Y = FMath::RandRange(-AngleRad, AngleRad);

    FVector SpreadDir = BaseDir.RotateAngleAxis(FMath::RadiansToDegrees(X), FVector::UpVector);
    SpreadDir = SpreadDir.RotateAngleAxis(FMath::RadiansToDegrees(Y), FVector::RightVector);

    return SpreadDir.GetSafeNormal();
}
