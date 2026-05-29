#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "../Inventory/InventoryComponent.h"
#include "../Combat/WeaponComponent.h"
#include "SurvivorCharacter.generated.h"

// ============================================================
// Enums
// ============================================================
UENUM(BlueprintType)
enum class EPlayerStance : uint8
{
    Standing    UMETA(DisplayName = "Standing"),
    Crouching   UMETA(DisplayName = "Crouching"),
    Aiming      UMETA(DisplayName = "Aiming"),
    Dead        UMETA(DisplayName = "Dead")
};

UENUM(BlueprintType)
enum class EHealthState : uint8
{
    Fine        UMETA(DisplayName = "Fine"),
    Caution     UMETA(DisplayName = "Caution"),     // < 60%
    Danger      UMETA(DisplayName = "Danger"),      // < 30%
    Critical    UMETA(DisplayName = "Critical")     // < 10%
};

// ============================================================
// Delegates
// ============================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDied, ACharacter*, DeadCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStanceChanged, EPlayerStance, NewStance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthStateChanged, EHealthState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInfectionChanged, float, NewLevel, bool, bIsCured);

// ============================================================
// Structs
// ============================================================
USTRUCT(BlueprintType)
struct FPlayerStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float MaxHealth = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float CurrentHealth = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float MaxStamina = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float CurrentStamina = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float InfectionLevel = 0.f; // 0-100, 100 = turned

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float StaminaRegenRate = 15.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float InfectionProgressRate = 0.5f; // per second
};

UCLASS(config=Game)
class RESIDENTEVILCLONE_API ASurvivorCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ASurvivorCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser) override;

    // ─── Input Actions ────────────────────────────────────────
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartAiming();
    void StopAiming();
    void StartCrouching();
    void StopCrouching();
    void FireWeapon();
    void Reload();
    void Interact();
    void OpenInventory();
    void Sprint(const FInputActionValue& Value);

    // ─── Camera ──────────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class UCameraComponent* FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class UCameraComponent* AimCamera; // Over-the-shoulder aim cam

    // ─── Components ──────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInventoryComponent* InventoryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UWeaponComponent* WeaponComponent;

    // ─── Input Mapping ───────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* AimAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* CrouchAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* FireAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* ReloadAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* InteractAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* InventoryAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* SprintAction;

    // ─── Stats ───────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    FPlayerStats Stats;

    // ─── State ───────────────────────────────────────────────
    UPROPERTY(BlueprintReadOnly, Category = "State")
    EPlayerStance CurrentStance;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    EHealthState CurrentHealthState;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsAiming;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsSprinting;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsDead;

    // ─── Footstep & Noise ────────────────────────────────────
    UPROPERTY(EditAnywhere, Category = "Stealth")
    float WalkNoiseRadius = 300.f;

    UPROPERTY(EditAnywhere, Category = "Stealth")
    float SprintNoiseRadius = 700.f;

    UPROPERTY(EditAnywhere, Category = "Stealth")
    float CrouchNoiseRadius = 100.f;

    UPROPERTY(EditDefaultsOnly, Category = "Stealth")
    class USoundBase* FootstepSound;

    float FootstepTimer = 0.f;
    float FootstepInterval = 0.4f;

    // ─── Interaction ─────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category = "Interaction")
    float InteractRange = 200.f;

    UPROPERTY(EditAnywhere, Category = "Interaction")
    TSubclassOf<class UUserWidget> InteractPromptClass;

    class UUserWidget* InteractPromptWidget;

    // ─── Camera Blend ────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category = "Camera")
    float AimCameraBlendSpeed = 5.f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    FVector AimCameraOffset = FVector(50.f, 70.f, 10.f);

    // ─── Footstep Sounds ─────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    TMap<TEnumAsByte<EPhysicalSurface>, USoundBase*> FootstepSoundMap;

public:
    // ─── Public Events ───────────────────────────────────────
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPlayerDied OnPlayerDied;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStanceChanged OnStanceChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnHealthStateChanged OnHealthStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnInfectionChanged OnInfectionChanged;

    // ─── Public API ──────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Health")
    void HealPlayer(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void CureInfection(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Health")
    bool IsAlive() const { return !bIsDead; }

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetHealthPercent() const;

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetStaminaPercent() const;

    UFUNCTION(BlueprintPure, Category = "State")
    EPlayerStance GetCurrentStance() const { return CurrentStance; }

    UFUNCTION(BlueprintPure, Category = "State")
    bool GetIsAiming() const { return bIsAiming; }

private:
    void UpdateHealthState();
    void UpdateStamina(float DeltaTime);
    void UpdateInfection(float DeltaTime);
    void HandleFootsteps(float DeltaTime);
    void EmitNoise(float Radius);
    void TryInteract();
    void CheckInteractable();
    void BlendCamera(float DeltaTime);
    void Die();
};
