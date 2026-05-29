#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ZombieAIController.generated.h"

// ============================================================
// Blackboard Key Names
// ============================================================
namespace ZombieBBKeys
{
    const FName TargetActor     = TEXT("TargetActor");
    const FName LastKnownLocation = TEXT("LastKnownLocation");
    const FName bCanSeeTarget   = TEXT("bCanSeeTarget");
    const FName bHeardNoise     = TEXT("bHeardNoise");
    const FName NoiseLocation   = TEXT("NoiseLocation");
    const FName AIState         = TEXT("AIState");
    const FName PatrolIndex     = TEXT("PatrolIndex");
    const FName bIsAlerted      = TEXT("bIsAlerted");
}

UENUM(BlueprintType)
enum class EZombieAIState : uint8
{
    Idle        UMETA(DisplayName = "Idle"),
    Patrol      UMETA(DisplayName = "Patrol"),
    Investigate UMETA(DisplayName = "Investigate"),
    Chase       UMETA(DisplayName = "Chase"),
    Attack      UMETA(DisplayName = "Attack"),
    Stunned     UMETA(DisplayName = "Stunned"),
    Dead        UMETA(DisplayName = "Dead")
};

UCLASS()
class RESIDENTEVILCLONE_API AZombieAIController : public AAIController
{
    GENERATED_BODY()

public:
    AZombieAIController();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void Tick(float DeltaTime) override;

    // ─── Perception ──────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
    UAIPerceptionComponent* AIPerceptionComponent;

    UFUNCTION()
    void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

    void HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus);
    void HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus);
    void HandleDamageStimulus(AActor* Actor, const FAIStimulus& Stimulus);

    // ─── Behavior Tree ───────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UBehaviorTree* BehaviorTree;

    // ─── State Management ────────────────────────────────────
    void SetAIState(EZombieAIState NewState);
    EZombieAIState CurrentState = EZombieAIState::Idle;

    // ─── Target Tracking ─────────────────────────────────────
    AActor* CurrentTarget = nullptr;
    FVector LastKnownTargetLocation = FVector::ZeroVector;
    float TimeWithoutSight = 0.f;
    float MaxChaseTimeWithoutSight = 8.f;

    // ─── Alert System ────────────────────────────────────────
    bool bIsAlerted = false;
    float AlertDecayTimer = 0.f;
    float AlertDecayTime = 15.f;

public:
    UFUNCTION(BlueprintCallable, Category = "AI")
    void SetAIAlerted(bool bAlert);

    UFUNCTION(BlueprintPure, Category = "AI")
    EZombieAIState GetCurrentAIState() const { return CurrentState; }
};

// ============================================================
// Zombie Character
// ============================================================
UCLASS()
class RESIDENTEVILCLONE_API AZombieCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AZombieCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser) override;

    // ─── Stats ───────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float MaxHealth = 100.f;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackDamage = 15.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackRange = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackCooldown = 1.5f;

    float AttackTimer = 0.f;

    // ─── Zombie Type ─────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie")
    bool bIsRunner = false; // Faster, more aggressive variant

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie")
    bool bIsBloater = false; // Explodes on death

    // ─── Hit Zones / Limb Damage ─────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limbs")
    float LimbHealthMultiplier = 0.5f; // Head: instant kill, limbs: cripple

    UPROPERTY(BlueprintReadOnly, Category = "Limbs")
    bool bLeftLegCrippled = false;

    UPROPERTY(BlueprintReadOnly, Category = "Limbs")
    bool bRightLegCrippled = false;

    UPROPERTY(BlueprintReadOnly, Category = "Limbs")
    bool bHeadless = false;

    // ─── Death & Loot ─────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Loot")
    TArray<TSubclassOf<class APickupItem>> LootTable;

    UPROPERTY(EditDefaultsOnly, Category = "Loot")
    float LootDropChance = 0.35f;

    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    class USoundBase* GrowlSound;

    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    class USoundBase* AttackSound;

    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    class USoundBase* DeathSound;

    // ─── VFX ────────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "VFX")
    class UNiagaraSystem* BloodSprayFX;

    UPROPERTY(EditDefaultsOnly, Category = "VFX")
    class UNiagaraSystem* ExplosionFX; // Bloater only

    bool bIsDead = false;

public:
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void PerformAttack(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool CanAttack() const;

    UFUNCTION(BlueprintPure, Category = "Stats")
    float GetHealthPercent() const;

    UFUNCTION(BlueprintCallable, Category = "AI")
    void ApplyLimbDamage(FName BoneName, float Damage);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZombieDied, AZombieCharacter*, Zombie);
    UPROPERTY(BlueprintAssignable)
    FOnZombieDied OnZombieDied;

private:
    void Die(AActor* Killer);
    void DropLoot();
    void BloaterExplosion();
};
