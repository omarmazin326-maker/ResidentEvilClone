#include "ZombieAI.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "../Characters/SurvivorCharacter.h"
#include "PickupItem.h"

// ══════════════════════════════════════════════════════════════
//  AZombieAIController
// ══════════════════════════════════════════════════════════════

AZombieAIController::AZombieAIController()
{
    PrimaryActorTick.bCanEverTick = true;

    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));

    // Sight config
    auto SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 1200.f;
    SightConfig->LoseSightRadius = 1500.f;
    SightConfig->PeripheralVisionAngleDegrees = 60.f;
    SightConfig->SetMaxAge(5.f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    AIPerceptionComponent->ConfigureSense(*SightConfig);

    // Hearing config
    auto HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = 800.f;
    HearingConfig->SetMaxAge(10.f);
    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
    AIPerceptionComponent->ConfigureSense(*HearingConfig);

    // Damage config
    auto DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
    AIPerceptionComponent->ConfigureSense(*DamageConfig);

    AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
    AIPerceptionComponent->OnPerceptionUpdated.AddDynamic(this, &AZombieAIController::OnPerceptionUpdated);
}

void AZombieAIController::BeginPlay()
{
    Super::BeginPlay();
}

void AZombieAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (BehaviorTree)
    {
        RunBehaviorTree(BehaviorTree);
        SetAIState(EZombieAIState::Patrol);
    }
}

void AZombieAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Chase timeout
    if (CurrentState == EZombieAIState::Chase && CurrentTarget)
    {
        bool bCanSee = false;
        FAIStimulus Stimulus;
        if (!AIPerceptionComponent->HasActiveStimulus(*CurrentTarget, bCanSee, Stimulus))
        {
            TimeWithoutSight += DeltaTime;
            if (TimeWithoutSight >= MaxChaseTimeWithoutSight)
            {
                SetAIState(EZombieAIState::Investigate);
                TimeWithoutSight = 0.f;
            }
        }
        else
        {
            TimeWithoutSight = 0.f;
        }
    }

    // Alert decay
    if (bIsAlerted)
    {
        AlertDecayTimer += DeltaTime;
        if (AlertDecayTimer >= AlertDecayTime)
        {
            bIsAlerted = false;
            AlertDecayTimer = 0.f;
            if (Blackboard)
            {
                Blackboard->SetValueAsBool(ZombieBBKeys::bIsAlerted, false);
            }
        }
    }
}

void AZombieAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
    for (AActor* Actor : UpdatedActors)
    {
        FActorPerceptionBlueprintInfo PerceptionInfo;
        AIPerceptionComponent->GetActorsPerception(Actor, PerceptionInfo);

        for (const FAIStimulus& Stimulus : PerceptionInfo.LastSensedStimuli)
        {
            if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
            {
                HandleSightStimulus(Actor, Stimulus);
            }
            else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
            {
                HandleHearingStimulus(Actor, Stimulus);
            }
            else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
            {
                HandleDamageStimulus(Actor, Stimulus);
            }
        }
    }
}

void AZombieAIController::HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
    if (!Cast<ASurvivorCharacter>(Actor)) return;

    if (Stimulus.WasSuccessfullySensed())
    {
        CurrentTarget = Actor;
        LastKnownTargetLocation = Actor->GetActorLocation();
        TimeWithoutSight = 0.f;
        SetAIAlerted(true);

        if (Blackboard)
        {
            Blackboard->SetValueAsObject(ZombieBBKeys::TargetActor, Actor);
            Blackboard->SetValueAsBool(ZombieBBKeys::bCanSeeTarget, true);
            Blackboard->SetValueAsVector(ZombieBBKeys::LastKnownLocation, LastKnownTargetLocation);
        }

        SetAIState(EZombieAIState::Chase);
    }
    else // Lost sight
    {
        if (Blackboard)
        {
            Blackboard->SetValueAsBool(ZombieBBKeys::bCanSeeTarget, false);
            Blackboard->SetValueAsVector(ZombieBBKeys::LastKnownLocation, LastKnownTargetLocation);
        }
    }
}

void AZombieAIController::HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
    if (!Stimulus.WasSuccessfullySensed()) return;
    if (CurrentState == EZombieAIState::Chase) return; // Already chasing

    const FVector NoiseLocation = Stimulus.StimulusLocation;

    if (Blackboard)
    {
        Blackboard->SetValueAsBool(ZombieBBKeys::bHeardNoise, true);
        Blackboard->SetValueAsVector(ZombieBBKeys::NoiseLocation, NoiseLocation);
    }

    SetAIAlerted(true);
    SetAIState(EZombieAIState::Investigate);
}

void AZombieAIController::HandleDamageStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
    if (!Stimulus.WasSuccessfullySensed()) return;

    // If damaged, always become alerted and chase the attacker
    SetAIAlerted(true);

    if (CurrentState != EZombieAIState::Chase)
    {
        CurrentTarget = Actor;
        if (Blackboard)
        {
            Blackboard->SetValueAsObject(ZombieBBKeys::TargetActor, Actor);
        }
        SetAIState(EZombieAIState::Chase);
    }
}

void AZombieAIController::SetAIState(EZombieAIState NewState)
{
    CurrentState = NewState;
    if (Blackboard)
    {
        Blackboard->SetValueAsEnum(ZombieBBKeys::AIState,
            static_cast<uint8>(NewState));
    }

    // Adjust speed by state
    if (APawn* Pawn = GetPawn())
    {
        if (ACharacter* Char = Cast<ACharacter>(Pawn))
        {
            float Speed = 0.f;
            switch (NewState)
            {
                case EZombieAIState::Patrol:      Speed = 120.f; break;
                case EZombieAIState::Investigate: Speed = 200.f; break;
                case EZombieAIState::Chase:       Speed = 380.f; break;
                case EZombieAIState::Attack:      Speed = 0.f;   break;
                default:                          Speed = 0.f;   break;
            }
            Char->GetCharacterMovement()->MaxWalkSpeed = Speed;
        }
    }
}

void AZombieAIController::SetAIAlerted(bool bAlert)
{
    bIsAlerted = bAlert;
    AlertDecayTimer = 0.f;
    if (Blackboard)
    {
        Blackboard->SetValueAsBool(ZombieBBKeys::bIsAlerted, bAlert);
    }
}

// ══════════════════════════════════════════════════════════════
//  AZombieCharacter
// ══════════════════════════════════════════════════════════════

AZombieCharacter::AZombieCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    Tags.Add(FName("Zombie")); // Used by player damage detection

    GetCharacterMovement()->MaxWalkSpeed = 120.f;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 200.f, 0.f);

    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AZombieCharacter::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;

    if (bIsRunner)
    {
        GetCharacterMovement()->MaxWalkSpeed = 550.f;
        AttackDamage *= 1.2f;
    }

    if (bIsBloater)
    {
        GetCapsuleComponent()->SetCapsuleHalfHeight(130.f);
        MaxHealth *= 2.f;
        CurrentHealth = MaxHealth;
        AttackDamage *= 1.5f;
    }

    // Periodic growl to keep player on edge
    FTimerHandle GrowlTimer;
    GetWorldTimerManager().SetTimer(GrowlTimer, [this]()
    {
        if (!bIsDead && GrowlSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, GrowlSound, GetActorLocation(),
                FMath::RandRange(0.8f, 1.2f));
        }
    }, FMath::RandRange(5.f, 15.f), true);
}

void AZombieCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDead) return;

    AttackTimer = FMath::Max(0.f, AttackTimer - DeltaTime);
}

float AZombieCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                                    AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f;

    // Point damage — check bone hit for limb system
    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent& PointDmgEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
        ApplyLimbDamage(PointDmgEvent.HitInfo.BoneName, DamageAmount);
    }

    const float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    CurrentHealth = FMath::Clamp(CurrentHealth - Actual, 0.f, MaxHealth);

    // Blood VFX
    if (BloodSprayFX && DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent& PDmg = static_cast<const FPointDamageEvent&>(DamageEvent);
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BloodSprayFX,
            PDmg.HitInfo.ImpactPoint, PDmg.HitInfo.ImpactNormal.Rotation());
    }

    if (CurrentHealth <= 0.f)
    {
        Die(DamageCauser);
    }

    return Actual;
}

void AZombieCharacter::ApplyLimbDamage(FName BoneName, float Damage)
{
    // Head: instant kill threshold
    if (BoneName == FName("head") || BoneName == FName("neck_01"))
    {
        if (Damage >= MaxHealth * 0.25f) // Headshot threshold
        {
            CurrentHealth = 0.f; // Will trigger die on next check
        }
        return;
    }

    // Legs: cripple on enough damage
    const float LimbThreshold = MaxHealth * LimbHealthMultiplier;
    if (Damage >= LimbThreshold)
    {
        if (BoneName == FName("thigh_l") || BoneName == FName("calf_l"))
        {
            if (!bLeftLegCrippled)
            {
                bLeftLegCrippled = true;
                GetCharacterMovement()->MaxWalkSpeed *= 0.4f;
            }
        }
        else if (BoneName == FName("thigh_r") || BoneName == FName("calf_r"))
        {
            if (!bRightLegCrippled)
            {
                bRightLegCrippled = true;
                GetCharacterMovement()->MaxWalkSpeed *= 0.4f;
            }
        }
    }
}

void AZombieCharacter::PerformAttack(AActor* Target)
{
    if (!CanAttack() || !Target) return;

    AttackTimer = AttackCooldown;

    if (AttackSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation());
    }

    FDamageEvent DmgEvent;
    Target->TakeDamage(AttackDamage, DmgEvent, GetController(), this);
}

bool AZombieCharacter::CanAttack() const
{
    return !bIsDead && AttackTimer <= 0.f;
}

float AZombieCharacter::GetHealthPercent() const
{
    return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f;
}

void AZombieCharacter::Die(AActor* Killer)
{
    if (bIsDead) return;
    bIsDead = true;

    if (DeathSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
    }

    if (bIsBloater)
    {
        BloaterExplosion();
        return;
    }

    // Ragdoll
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetSimulatePhysics(true);
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    DropLoot();

    OnZombieDied.Broadcast(this);

    // Destroy after 15 seconds to free memory
    SetLifeSpan(15.f);
}

void AZombieCharacter::DropLoot()
{
    if (LootTable.IsEmpty()) return;
    if (FMath::FRand() > LootDropChance) return;

    const int32 Index = FMath::RandRange(0, LootTable.Num() - 1);
    if (LootTable[Index])
    {
        FVector SpawnLoc = GetActorLocation() + FVector(0, 0, 50.f);
        GetWorld()->SpawnActor<APickupItem>(LootTable[Index], SpawnLoc, FRotator::ZeroRotator);
    }
}

void AZombieCharacter::BloaterExplosion()
{
    const float ExplosionRadius = 350.f;
    const float ExplosionDamage = 60.f;

    if (ExplosionFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionFX, GetActorLocation());
    }

    // Apply radial damage
    UGameplayStatics::ApplyRadialDamage(GetWorld(), ExplosionDamage, GetActorLocation(),
        ExplosionRadius, UDamageType::StaticClass(), {this}, this, GetController(), true);

    OnZombieDied.Broadcast(this);
    Destroy();
}
