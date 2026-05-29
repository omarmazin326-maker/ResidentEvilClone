// ============================================================
// SurvivalGameMode.h — Core Game Loop + Save System
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/SaveGame.h"
#include "SurvivalGameMode.generated.h"

// ────────────────────────────────────────────────────────────
// Save Game Data
// ────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FSavedInventoryItem
{
    GENERATED_BODY()
    UPROPERTY() FName ItemID;
    UPROPERTY() int32 StackSize = 1;
};

USTRUCT(BlueprintType)
struct FSaveData
{
    GENERATED_BODY()

    UPROPERTY() FString LevelName;
    UPROPERTY() FTransform PlayerTransform;
    UPROPERTY() float PlayerHealth = 100.f;
    UPROPERTY() float PlayerStamina = 100.f;
    UPROPERTY() float InfectionLevel = 0.f;
    UPROPERTY() TArray<FSavedInventoryItem> InventoryItems;
    UPROPERTY() TArray<FName> SolvedPuzzles;
    UPROPERTY() TArray<FName> CollectedPickups;
    UPROPERTY() TArray<FName> UnlockedDoors;
    UPROPERTY() FDateTime SaveTime;
    UPROPERTY() int32 PlaytimeSeconds = 0;
};

UCLASS()
class RESIDENTEVILCLONE_API USurvivalSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY() TArray<FSaveData> SaveSlots; // Multiple save slots
    UPROPERTY() int32 ActiveSlot = 0;
};

// ────────────────────────────────────────────────────────────
// Game Mode
// ────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameSaved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameLoaded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerRespawn);

UCLASS()
class RESIDENTEVILCLONE_API ASurvivalGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASurvivalGameMode();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // ─── Save / Load ─────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Save")
    FString SaveSlotName = TEXT("SurvivalSave");

    UPROPERTY(BlueprintReadOnly, Category = "Save")
    USurvivalSaveGame* SaveGameObject;

    // ─── Typewriter Save Terminals ───────────────────────────
    UPROPERTY(EditAnywhere, Category = "Game")
    bool bRequireSaveTerminal = true; // Classic RE: only save at typewriters

    // ─── Difficulty ──────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    float EnemyDamageMultiplier = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    float ItemSpawnMultiplier = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    bool bLimitedSaves = false; // Hardcore: limited ink ribbons

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    int32 SavesRemaining = 6; // Only for hardcore

    // ─── Playtime Tracking ───────────────────────────────────
    float PlaytimeTimer = 0.f;
    int32 TotalPlaytimeSeconds = 0;

    // ─── Zombie Spawning ─────────────────────────────────────
    UPROPERTY(EditAnywhere, Category = "Spawning")
    TSubclassOf<class AZombieCharacter> ZombieClass;

    UPROPERTY(EditAnywhere, Category = "Spawning")
    TArray<AActor*> SpawnPoints;

    UPROPERTY(EditAnywhere, Category = "Spawning")
    int32 MaxZombiesAlive = 20;

    UPROPERTY(BlueprintReadOnly, Category = "Spawning")
    int32 ZombiesAlive = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Spawning")
    int32 ZombiesKilled = 0;

    // ─── Camera Transition (fixed cameras classic mode) ──────
    UPROPERTY(EditAnywhere, Category = "Cameras")
    bool bUseFixedCameras = false; // Optional: classic RE mode

    UPROPERTY(EditAnywhere, Category = "Cameras")
    TArray<ACameraActor*> RoomCameras;

    int32 ActiveCameraIndex = 0;

public:
    UPROPERTY(BlueprintAssignable) FOnGameSaved   OnGameSaved;
    UPROPERTY(BlueprintAssignable) FOnGameLoaded  OnGameLoaded;
    UPROPERTY(BlueprintAssignable) FOnPlayerRespawn OnPlayerRespawn;

    // ─── Save / Load ─────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Save")
    bool SaveGame(int32 SlotIndex = 0);

    UFUNCTION(BlueprintCallable, Category = "Save")
    bool LoadGame(int32 SlotIndex = 0);

    UFUNCTION(BlueprintCallable, Category = "Save")
    bool DeleteSave(int32 SlotIndex = 0);

    UFUNCTION(BlueprintPure, Category = "Save")
    bool HasSaveData(int32 SlotIndex = 0) const;

    UFUNCTION(BlueprintPure, Category = "Save")
    bool CanSave() const;

    // ─── Spawning ────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Spawning")
    AZombieCharacter* SpawnZombieAtPoint(AActor* SpawnPoint);

    UFUNCTION(BlueprintCallable, Category = "Spawning")
    void SpawnInitialZombies();

    UFUNCTION(BlueprintCallable, Category = "Spawning")
    void OnZombieDied(AZombieCharacter* DeadZombie);

    // ─── Game State ──────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Game")
    void RestartFromLastSave();

    UFUNCTION(BlueprintPure, Category = "Game")
    int32 GetPlaytimeSeconds() const { return TotalPlaytimeSeconds; }

    UFUNCTION(BlueprintPure, Category = "Game")
    int32 GetZombiesKilled() const { return ZombiesKilled; }

    // ─── Camera ──────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void SwitchToCamera(int32 CameraIndex);

private:
    void CollectSaveData(FSaveData& OutData) const;
    void ApplySaveData(const FSaveData& Data);
    AActor* FindBestSpawnPoint() const;
};

// ──────────────────────────────────────────────────────────
// SurvivalGameMode.cpp
// ──────────────────────────────────────────────────────────
#include "SurvivalGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "../Characters/SurvivorCharacter.h"
#include "../AI/ZombieAI.h"

ASurvivalGameMode::ASurvivalGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASurvivalGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Load or create save object
    SaveGameObject = Cast<USurvivalSaveGame>(
        UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));

    if (!SaveGameObject)
    {
        SaveGameObject = Cast<USurvivalSaveGame>(
            UGameplayStatics::CreateSaveGameObject(USurvivalSaveGame::StaticClass()));
    }

    SpawnInitialZombies();
}

void ASurvivalGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    PlaytimeTimer += DeltaTime;
    if (PlaytimeTimer >= 1.f)
    {
        TotalPlaytimeSeconds++;
        PlaytimeTimer = 0.f;
    }
}

bool ASurvivalGameMode::CanSave() const
{
    if (bLimitedSaves && SavesRemaining <= 0) return false;
    return true;
}

bool ASurvivalGameMode::SaveGame(int32 SlotIndex)
{
    if (!CanSave() || !SaveGameObject) return false;

    // Ensure array is large enough
    while (SaveGameObject->SaveSlots.Num() <= SlotIndex)
    {
        SaveGameObject->SaveSlots.Add(FSaveData());
    }

    FSaveData& Data = SaveGameObject->SaveSlots[SlotIndex];
    CollectSaveData(Data);
    Data.PlaytimeSeconds = TotalPlaytimeSeconds;
    Data.SaveTime = FDateTime::Now();

    if (bLimitedSaves) SavesRemaining--;

    const bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveGameObject, SaveSlotName, 0);
    if (bSuccess) OnGameSaved.Broadcast();

    return bSuccess;
}

bool ASurvivalGameMode::LoadGame(int32 SlotIndex)
{
    if (!SaveGameObject) return false;
    if (!SaveGameObject->SaveSlots.IsValidIndex(SlotIndex)) return false;

    const FSaveData& Data = SaveGameObject->SaveSlots[SlotIndex];
    ApplySaveData(Data);
    TotalPlaytimeSeconds = Data.PlaytimeSeconds;

    OnGameLoaded.Broadcast();
    return true;
}

bool ASurvivalGameMode::DeleteSave(int32 SlotIndex)
{
    if (!SaveGameObject || !SaveGameObject->SaveSlots.IsValidIndex(SlotIndex)) return false;
    SaveGameObject->SaveSlots.RemoveAt(SlotIndex);
    return UGameplayStatics::SaveGameToSlot(SaveGameObject, SaveSlotName, 0);
}

bool ASurvivalGameMode::HasSaveData(int32 SlotIndex) const
{
    return SaveGameObject && SaveGameObject->SaveSlots.IsValidIndex(SlotIndex);
}

void ASurvivalGameMode::CollectSaveData(FSaveData& OutData) const
{
    OutData.LevelName = GetWorld()->GetMapName();

    if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
    {
        OutData.PlayerTransform = Player->GetActorTransform();

        if (ASurvivorCharacter* Survivor = Cast<ASurvivorCharacter>(Player))
        {
            OutData.PlayerHealth    = Survivor->Stats.CurrentHealth;
            OutData.PlayerStamina   = Survivor->Stats.CurrentStamina;
            OutData.InfectionLevel  = Survivor->Stats.InfectionLevel;
        }
    }
}

void ASurvivalGameMode::ApplySaveData(const FSaveData& Data)
{
    if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
    {
        Player->SetActorTransform(Data.PlayerTransform);

        if (ASurvivorCharacter* Survivor = Cast<ASurvivorCharacter>(Player))
        {
            Survivor->Stats.CurrentHealth  = Data.PlayerHealth;
            Survivor->Stats.CurrentStamina = Data.PlayerStamina;
            Survivor->Stats.InfectionLevel = Data.InfectionLevel;
        }
    }
}

void ASurvivalGameMode::SpawnInitialZombies()
{
    if (!ZombieClass) return;

    for (AActor* Point : SpawnPoints)
    {
        if (ZombiesAlive >= MaxZombiesAlive) break;
        SpawnZombieAtPoint(Point);
    }
}

AZombieCharacter* ASurvivalGameMode::SpawnZombieAtPoint(AActor* SpawnPoint)
{
    if (!SpawnPoint || !ZombieClass) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AZombieCharacter* Zombie = GetWorld()->SpawnActor<AZombieCharacter>(
        ZombieClass, SpawnPoint->GetActorTransform(), Params);

    if (Zombie)
    {
        ZombiesAlive++;
        Zombie->OnZombieDied.AddDynamic(this, &ASurvivalGameMode::OnZombieDied);
    }

    return Zombie;
}

void ASurvivalGameMode::OnZombieDied(AZombieCharacter* DeadZombie)
{
    ZombiesAlive = FMath::Max(0, ZombiesAlive - 1);
    ZombiesKilled++;
}

void ASurvivalGameMode::RestartFromLastSave()
{
    LoadGame(SaveGameObject ? SaveGameObject->ActiveSlot : 0);
    OnPlayerRespawn.Broadcast();
}

void ASurvivalGameMode::SwitchToCamera(int32 CameraIndex)
{
    if (!bUseFixedCameras || !RoomCameras.IsValidIndex(CameraIndex)) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC && RoomCameras[CameraIndex])
    {
        PC->SetViewTargetWithBlend(RoomCameras[CameraIndex], 0.5f);
        ActiveCameraIndex = CameraIndex;
    }
}
