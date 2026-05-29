#include "SurvivorCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Perception/AISense_Hearing.h"
#include "../Inventory/InventoryComponent.h"
#include "../Combat/WeaponComponent.h"

// ──────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────
ASurvivorCharacter::ASurvivorCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

    // Movement defaults (Resident Evil-style: slower, weightier)
    GetCharacterMovement()->MaxWalkSpeed = 250.f;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 120.f;
    GetCharacterMovement()->JumpZVelocity = 0.f; // No jumping
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

    bUseControllerRotationYaw = true; // Face the direction controller faces

    // Camera Boom
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 350.f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);

    // Follow Camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // Aim Camera (over shoulder)
    AimCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("AimCamera"));
    AimCamera->SetupAttachment(GetMesh(), FName("head"));
    AimCamera->SetRelativeLocation(AimCameraOffset);
    AimCamera->bUsePawnControlRotation = true;
    AimCamera->SetAutoActivate(false);

    // Components
    InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
    WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));

    // Defaults
    CurrentStance = EPlayerStance::Standing;
    CurrentHealthState = EHealthState::Fine;
    bIsAiming = false;
    bIsSprinting = false;
    bIsDead = false;
}

// ──────────────────────────────────────────────────────────────
// BeginPlay
// ──────────────────────────────────────────────────────────────
void ASurvivorCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    Stats.CurrentHealth = Stats.MaxHealth;
    Stats.CurrentStamina = Stats.MaxStamina;

    // Show interact prompt (hidden by default)
    if (InteractPromptClass)
    {
        InteractPromptWidget = CreateWidget<UUserWidget>(GetWorld(), InteractPromptClass);
        if (InteractPromptWidget)
        {
            InteractPromptWidget->AddToViewport();
            InteractPromptWidget->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

// ──────────────────────────────────────────────────────────────
// Tick
// ──────────────────────────────────────────────────────────────
void ASurvivorCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDead) return;

    UpdateStamina(DeltaTime);
    UpdateInfection(DeltaTime);
    HandleFootsteps(DeltaTime);
    CheckInteractable();
    BlendCamera(DeltaTime);
}

// ──────────────────────────────────────────────────────────────
// Input Setup
// ──────────────────────────────────────────────────────────────
void ASurvivorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EIC->BindAction(MoveAction,      ETriggerEvent::Triggered, this, &ASurvivorCharacter::Move);
        EIC->BindAction(LookAction,      ETriggerEvent::Triggered, this, &ASurvivorCharacter::Look);
        EIC->BindAction(AimAction,       ETriggerEvent::Started,   this, &ASurvivorCharacter::StartAiming);
        EIC->BindAction(AimAction,       ETriggerEvent::Completed, this, &ASurvivorCharacter::StopAiming);
        EIC->BindAction(CrouchAction,    ETriggerEvent::Started,   this, &ASurvivorCharacter::StartCrouching);
        EIC->BindAction(CrouchAction,    ETriggerEvent::Completed, this, &ASurvivorCharacter::StopCrouching);
        EIC->BindAction(FireAction,      ETriggerEvent::Started,   this, &ASurvivorCharacter::FireWeapon);
        EIC->BindAction(ReloadAction,    ETriggerEvent::Started,   this, &ASurvivorCharacter::Reload);
        EIC->BindAction(InteractAction,  ETriggerEvent::Started,   this, &ASurvivorCharacter::Interact);
        EIC->BindAction(InventoryAction, ETriggerEvent::Started,   this, &ASurvivorCharacter::OpenInventory);
        EIC->BindAction(SprintAction,    ETriggerEvent::Triggered, this, &ASurvivorCharacter::Sprint);
    }
}

// ──────────────────────────────────────────────────────────────
// Movement
// ──────────────────────────────────────────────────────────────
void ASurvivorCharacter::Move(const FInputActionValue& Value)
{
    if (bIsDead || CurrentStance == EPlayerStance::Dead) return;

    const FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDir   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // Speed penalty when aiming
        float SpeedMult = bIsAiming ? 0.4f : 1.f;

        AddMovementInput(ForwardDir, MovementVector.Y * SpeedMult);
        AddMovementInput(RightDir,   MovementVector.X * SpeedMult);
    }
}

void ASurvivorCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    AddControllerYawInput(LookAxisVector.X);
    AddControllerPitchInput(LookAxisVector.Y);
}

void ASurvivorCharacter::Sprint(const FInputActionValue& Value)
{
    if (bIsAiming || bIsDead) return;

    const bool bWantsToSprint = Value.Get<bool>();

    if (bWantsToSprint && Stats.CurrentStamina > 5.f)
    {
        bIsSprinting = true;
        GetCharacterMovement()->MaxWalkSpeed = 550.f;
    }
    else
    {
        bIsSprinting = false;
        GetCharacterMovement()->MaxWalkSpeed = 250.f;
    }
}

// ──────────────────────────────────────────────────────────────
// Aiming & Stance
// ──────────────────────────────────────────────────────────────
void ASurvivorCharacter::StartAiming()
{
    if (bIsDead || !WeaponComponent->HasWeaponEquipped()) return;

    bIsAiming = true;
    bIsSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = 150.f;

    FollowCamera->SetActive(false);
    AimCamera->SetActive(true);

    CurrentStance = EPlayerStance::Aiming;
    OnStanceChanged.Broadcast(CurrentStance);

    WeaponComponent->SetAiming(true);
}

void ASurvivorCharacter::StopAiming()
{
    bIsAiming = false;
    GetCharacterMovement()->MaxWalkSpeed = 250.f;

    AimCamera->SetActive(false);
    FollowCamera->SetActive(true);

    CurrentStance = EPlayerStance::Standing;
    OnStanceChanged.Broadcast(CurrentStance);

    WeaponComponent->SetAiming(false);
}

void ASurvivorCharacter::StartCrouching()
{
    if (bIsDead || bIsAiming) return;
    Crouch();
    CurrentStance = EPlayerStance::Crouching;
    FootstepInterval = 0.6f;
    OnStanceChanged.Broadcast(CurrentStance);
}

void ASurvivorCharacter::StopCrouching()
{
    UnCrouch();
    CurrentStance = EPlayerStance::Standing;
    FootstepInterval = 0.4f;
    OnStanceChanged.Broadcast(CurrentStance);
}

// ──────────────────────────────────────────────────────────────
// Combat
// ──────────────────────────────────────────────────────────────
void ASurvivorCharacter::FireWeapon()
{
    if (bIsDead || !bIsAiming) return;
    WeaponComponent->Fire();
}

void ASurvivorCharacter::Reload()
{
    if (bIsDead) return;
    WeaponComponent->Reload();
}

// ──────────────────────────────────────────────────────────────
// Interaction
// ──────────────────────────────────────────────────────────────
void ASurvivorCharacter::Interact()
{
    if (bIsDead) return;
    TryInteract();
}

void ASurvivorCharacter::TryInteract()
{
    FVector Start = GetActorLocation();
    FVector End   = Start + GetActorForwardVector() * InteractRange;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (AActor* HitActor = Hit.GetActor())
        {
            // Execute interface if actor implements IInteractable
            if (HitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
            {
                IInteractableInterface::Execute_Interact(HitActor, this);
            }
        }
    }
}

void ASurvivorCharacter::CheckInteractable()
{
    FVector Start = GetActorLocation();
    FVector End   = Start + GetActorForwardVector() * InteractRange;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bFound = false;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (AActor* HitActor = Hit.GetActor())
        {
            bFound = HitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass());
        }
    }

    if (InteractPromptWidget)
    {
        InteractPromptWidget->SetVisibility(bFound
            ? ESlateVisibility::Visible
            : ESlateVisibility::Hidden);
    }
}

// ──────────────────────────────────────────────────────────────
// Inventory
// ──────────────────────────────────────────────────────────────
void ASurvivorCharacter::OpenInventory()
{
    if (bIsDead) return;
    InventoryComponent->ToggleInventoryUI();
}

// ──────────────────────────────────────────────────────────────
// Damage & Death
// ──────────────────────────────────────────────────────────────
float ASurvivorCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                                      AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f;

    const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    Stats.CurrentHealth = FMath::Clamp(Stats.CurrentHealth - ActualDamage, 0.f, Stats.MaxHealth);
    OnHealthChanged.Broadcast(Stats.CurrentHealth, Stats.MaxHealth);

    // Add infection on zombie damage
    if (DamageCauser && DamageCauser->ActorHasTag(FName("Zombie")))
    {
        Stats.InfectionLevel = FMath::Clamp(Stats.InfectionLevel + 5.f, 0.f, 100.f);
        OnInfectionChanged.Broadcast(Stats.InfectionLevel, false);
    }

    UpdateHealthState();

    if (Stats.CurrentHealth <= 0.f)
    {
        Die();
    }

    return ActualDamage;
}

void ASurvivorCharacter::Die()
{
    bIsDead = true;
    CurrentStance = EPlayerStance::Dead;

    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Play death animation via montage (set in BP)
    OnPlayerDied.Broadcast(this);

    // Ragdoll
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetSimulatePhysics(true);
}

// ──────────────────────────────────────────────────────────────
// Healing & Infection
// ──────────────────────────────────────────────────────────────
void ASurvivorCharacter::HealPlayer(float Amount)
{
    if (bIsDead) return;
    Stats.CurrentHealth = FMath::Clamp(Stats.CurrentHealth + Amount, 0.f, Stats.MaxHealth);
    OnHealthChanged.Broadcast(Stats.CurrentHealth, Stats.MaxHealth);
    UpdateHealthState();
}

void ASurvivorCharacter::CureInfection(float Amount)
{
    Stats.InfectionLevel = FMath::Clamp(Stats.InfectionLevel - Amount, 0.f, 100.f);
    OnInfectionChanged.Broadcast(Stats.InfectionLevel, Stats.InfectionLevel <= 0.f);
}

float ASurvivorCharacter::GetHealthPercent() const
{
    return Stats.MaxHealth > 0.f ? Stats.CurrentHealth / Stats.MaxHealth : 0.f;
}

float ASurvivorCharacter::GetStaminaPercent() const
{
    return Stats.MaxStamina > 0.f ? Stats.CurrentStamina / Stats.MaxStamina : 0.f;
}

// ──────────────────────────────────────────────────────────────
// Private Helpers
// ──────────────────────────────────────────────────────────────
void ASurvivorCharacter::UpdateHealthState()
{
    const float Pct = GetHealthPercent();
    EHealthState NewState;

    if      (Pct >= 0.6f) NewState = EHealthState::Fine;
    else if (Pct >= 0.3f) NewState = EHealthState::Caution;
    else if (Pct >= 0.1f) NewState = EHealthState::Danger;
    else                  NewState = EHealthState::Critical;

    if (NewState != CurrentHealthState)
    {
        CurrentHealthState = NewState;
        OnHealthStateChanged.Broadcast(CurrentHealthState);
    }
}

void ASurvivorCharacter::UpdateStamina(float DeltaTime)
{
    if (bIsSprinting)
    {
        Stats.CurrentStamina = FMath::Clamp(Stats.CurrentStamina - 20.f * DeltaTime, 0.f, Stats.MaxStamina);
        if (Stats.CurrentStamina <= 0.f)
        {
            bIsSprinting = false;
            GetCharacterMovement()->MaxWalkSpeed = 250.f;
        }
    }
    else
    {
        // Regen stamina
        Stats.CurrentStamina = FMath::Clamp(Stats.CurrentStamina + Stats.StaminaRegenRate * DeltaTime,
                                             0.f, Stats.MaxStamina);
    }
}

void ASurvivorCharacter::UpdateInfection(float DeltaTime)
{
    if (Stats.InfectionLevel <= 0.f) return;

    Stats.InfectionLevel = FMath::Clamp(
        Stats.InfectionLevel + Stats.InfectionProgressRate * DeltaTime, 0.f, 100.f);

    // Infection causes periodic damage when high
    if (Stats.InfectionLevel >= 80.f)
    {
        TakeDamage(1.f * DeltaTime, FDamageEvent(), nullptr, nullptr);
    }

    if (Stats.InfectionLevel >= 100.f)
    {
        // Player turns into zombie — game over
        Die();
    }
}

void ASurvivorCharacter::HandleFootsteps(float DeltaTime)
{
    if (!GetCharacterMovement()->IsMovingOnGround()) return;

    const FVector Velocity = GetVelocity();
    if (Velocity.SizeSquared() < 100.f) return; // Standing still

    FootstepTimer += DeltaTime;
    if (FootstepTimer >= FootstepInterval)
    {
        FootstepTimer = 0.f;

        // Surface-based footstep sound
        FHitResult FloorHit;
        FVector Start = GetActorLocation();
        FVector End   = Start - FVector(0, 0, 100.f);
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);
        Params.bReturnPhysicalMaterial = true;

        if (GetWorld()->LineTraceSingleByChannel(FloorHit, Start, End, ECC_Visibility, Params))
        {
            EPhysicalSurface Surface = UGameplayStatics::GetSurfaceType(FloorHit);
            if (USoundBase** Sound = FootstepSoundMap.Find(Surface))
            {
                UGameplayStatics::PlaySoundAtLocation(this, *Sound, GetActorLocation());
            }
        }

        // Emit AI perception noise
        const float NoiseRadius = bIsSprinting  ? SprintNoiseRadius  :
                                  bIsCrouched() ? CrouchNoiseRadius  :
                                                  WalkNoiseRadius;
        EmitNoise(NoiseRadius);
    }
}

void ASurvivorCharacter::EmitNoise(float Radius)
{
    UAISense_Hearing::ReportNoiseEvent(
        GetWorld(), GetActorLocation(), 1.f, this, Radius, FName("Footstep"));
}

void ASurvivorCharacter::BlendCamera(float DeltaTime)
{
    // Smooth spring-arm length transition when aiming
    const float TargetLength = bIsAiming ? 150.f : 350.f;
    CameraBoom->TargetArmLength = FMath::FInterpTo(
        CameraBoom->TargetArmLength, TargetLength, DeltaTime, AimCameraBlendSpeed);
}
