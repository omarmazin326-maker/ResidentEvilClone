// ============================================================
// PuzzleSystem.h — Resident Evil-style Puzzle Framework
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleSystem.generated.h"

// ────────────────────────────────────────────────────────────
// Interactable Interface (used by puzzles, doors, pickups)
// ────────────────────────────────────────────────────────────
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

class IInteractableInterface
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void Interact(AActor* Interactor);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    FText GetInteractPrompt();
};

// ────────────────────────────────────────────────────────────
// Combination Lock Puzzle
// ────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleSolved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPuzzleFailed);

UCLASS()
class RESIDENTEVILCLONE_API ACombinationLockPuzzle : public AActor, public IInteractableInterface
{
    GENERATED_BODY()

public:
    ACombinationLockPuzzle();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
    TArray<int32> Solution = {3, 7, 1, 4}; // Correct digit sequence

    UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
    TArray<int32> CurrentInput;

    UPROPERTY(EditAnywhere, Category = "Puzzle")
    int32 NumDigits = 4;

    UPROPERTY(EditAnywhere, Category = "Puzzle")
    bool bIsSolved = false;

    UPROPERTY(EditAnywhere, Category = "Puzzle")
    bool bIsLocked = true;

    UPROPERTY(EditAnywhere, Category = "Puzzle")
    int32 MaxAttempts = -1; // -1 = unlimited

    UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
    int32 AttemptsRemaining;

    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    USoundBase* ClickSound;

    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    USoundBase* SolvedSound;

    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    USoundBase* FailedSound;

    // Widget for combination UI
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> PuzzleWidgetClass;

    UUserWidget* PuzzleWidget = nullptr;

public:
    UPROPERTY(BlueprintAssignable)
    FOnPuzzleSolved OnPuzzleSolved;

    UPROPERTY(BlueprintAssignable)
    FOnPuzzleFailed OnPuzzleFailed;

    // IInteractableInterface
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractPrompt_Implementation() override;

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void InputDigit(int32 Digit);

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void ClearInput();

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void SubmitCombination();

    UFUNCTION(BlueprintPure, Category = "Puzzle")
    bool IsSolved() const { return bIsSolved; }

    UFUNCTION(BlueprintPure, Category = "Puzzle")
    const TArray<int32>& GetCurrentInput() const { return CurrentInput; }

private:
    bool CheckSolution() const;
};

// ────────────────────────────────────────────────────────────
// Keypad Puzzle
// ────────────────────────────────────────────────────────────
UCLASS()
class RESIDENTEVILCLONE_API AKeypadPuzzle : public AActor, public IInteractableInterface
{
    GENERATED_BODY()

public:
    AKeypadPuzzle();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
    FString CorrectCode = TEXT("2501");

    UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
    FString EnteredCode;

    UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
    bool bIsSolved = false;

    UPROPERTY(EditAnywhere, Category = "Puzzle")
    AActor* DoorToUnlock; // Door/gate this keypad controls

public:
    UPROPERTY(BlueprintAssignable)
    FOnPuzzleSolved OnPuzzleSolved;

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractPrompt_Implementation() override;

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void EnterCode(const FString& Code);
};

// ────────────────────────────────────────────────────────────
// Item Required Puzzle (Key items gate doors)
// ────────────────────────────────────────────────────────────
UCLASS()
class RESIDENTEVILCLONE_API AItemRequiredPuzzle : public AActor, public IInteractableInterface
{
    GENERATED_BODY()

public:
    AItemRequiredPuzzle();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
    FName RequiredItemID; // Must match inventory item ID

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
    bool bConsumeItem = true; // Remove item from inventory on use

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
    FText LockedPrompt = FText::FromString(TEXT("Requires an item..."));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
    FText UnlockPrompt = FText::FromString(TEXT("Use item"));

    UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
    bool bIsSolved = false;

    UPROPERTY(EditAnywhere, Category = "Puzzle")
    TArray<AActor*> ActorsToActivate; // e.g., doors, elevators

    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    USoundBase* UnlockSound;

public:
    UPROPERTY(BlueprintAssignable)
    FOnPuzzleSolved OnPuzzleSolved;

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractPrompt_Implementation() override;
};

// ────────────────────────────────────────────────────────────
// Symbol / Tile Puzzle (e.g. RE2-style statue room)
// ────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FSymbolTile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TileIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentSymbol = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CorrectSymbol = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 NumSymbols = 4; // How many symbols this tile cycles through
};

UCLASS()
class RESIDENTEVILCLONE_API ASymbolPuzzle : public AActor, public IInteractableInterface
{
    GENERATED_BODY()

public:
    ASymbolPuzzle();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
    TArray<FSymbolTile> Tiles;

    UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
    bool bIsSolved = false;

    UPROPERTY(EditAnywhere, Category = "Puzzle")
    int32 SelectedTileIndex = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    USoundBase* TileClickSound;

    UPROPERTY(EditDefaultsOnly, Category = "Audio")
    USoundBase* SolvedSound;

public:
    UPROPERTY(BlueprintAssignable)
    FOnPuzzleSolved OnPuzzleSolved;

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractPrompt_Implementation() override;

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void CycleTile(int32 TileIndex);

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void SelectTile(int32 TileIndex);

    UFUNCTION(BlueprintPure, Category = "Puzzle")
    bool CheckAllTiles() const;

    UFUNCTION(BlueprintPure, Category = "Puzzle")
    bool IsSolved() const { return bIsSolved; }
};

// ──────────────────────────────────────────────────────────────
// Implementations
// ──────────────────────────────────────────────────────────────
#include "PuzzleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "../Inventory/InventoryComponent.h"
#include "../Characters/SurvivorCharacter.h"
#include "Blueprint/UserWidget.h"

// ─── ACombinationLockPuzzle ──────────────────────────────────
ACombinationLockPuzzle::ACombinationLockPuzzle()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ACombinationLockPuzzle::BeginPlay()
{
    Super::BeginPlay();
    CurrentInput.SetNum(NumDigits);
    for (int32& D : CurrentInput) D = 0;
    AttemptsRemaining = MaxAttempts;
}

void ACombinationLockPuzzle::Interact_Implementation(AActor* Interactor)
{
    if (bIsSolved) return;

    if (PuzzleWidgetClass && !PuzzleWidget)
    {
        PuzzleWidget = CreateWidget<UUserWidget>(GetWorld(), PuzzleWidgetClass);
        if (PuzzleWidget)
        {
            PuzzleWidget->AddToViewport();
        }
    }
}

FText ACombinationLockPuzzle::GetInteractPrompt_Implementation()
{
    return bIsSolved
        ? FText::FromString(TEXT("Already unlocked"))
        : FText::FromString(TEXT("Enter combination"));
}

void ACombinationLockPuzzle::InputDigit(int32 Digit)
{
    // Fill left to right
    for (int32& D : CurrentInput)
    {
        if (D == -1)
        {
            D = Digit;
            if (ClickSound)
                UGameplayStatics::PlaySoundAtLocation(this, ClickSound, GetActorLocation());
            return;
        }
    }
}

void ACombinationLockPuzzle::ClearInput()
{
    for (int32& D : CurrentInput) D = -1;
}

void ACombinationLockPuzzle::SubmitCombination()
{
    if (CheckSolution())
    {
        bIsSolved = true;
        bIsLocked = false;
        if (SolvedSound)
            UGameplayStatics::PlaySoundAtLocation(this, SolvedSound, GetActorLocation());
        OnPuzzleSolved.Broadcast();
    }
    else
    {
        if (MaxAttempts > 0)
        {
            AttemptsRemaining--;
        }
        if (FailedSound)
            UGameplayStatics::PlaySoundAtLocation(this, FailedSound, GetActorLocation());
        OnPuzzleFailed.Broadcast();
        ClearInput();
    }
}

bool ACombinationLockPuzzle::CheckSolution() const
{
    if (CurrentInput.Num() != Solution.Num()) return false;
    for (int32 i = 0; i < Solution.Num(); ++i)
    {
        if (CurrentInput[i] != Solution[i]) return false;
    }
    return true;
}

// ─── AKeypadPuzzle ───────────────────────────────────────────
AKeypadPuzzle::AKeypadPuzzle()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AKeypadPuzzle::Interact_Implementation(AActor* Interactor) {}

FText AKeypadPuzzle::GetInteractPrompt_Implementation()
{
    return FText::FromString(bIsSolved ? TEXT("Unlocked") : TEXT("Enter code"));
}

void AKeypadPuzzle::EnterCode(const FString& Code)
{
    EnteredCode = Code;
    if (Code == CorrectCode)
    {
        bIsSolved = true;
        OnPuzzleSolved.Broadcast();
        if (DoorToUnlock)
        {
            DoorToUnlock->SetActorHiddenInGame(true); // Simple door open
        }
    }
    else
    {
        EnteredCode.Empty();
    }
}

// ─── AItemRequiredPuzzle ─────────────────────────────────────
AItemRequiredPuzzle::AItemRequiredPuzzle()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AItemRequiredPuzzle::Interact_Implementation(AActor* Interactor)
{
    if (bIsSolved) return;

    ASurvivorCharacter* Player = Cast<ASurvivorCharacter>(Interactor);
    if (!Player) return;

    UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
    if (!Inventory) return;

    if (Inventory->HasItem(RequiredItemID))
    {
        if (bConsumeItem)
        {
            Inventory->RemoveItem(RequiredItemID, 1);
        }

        bIsSolved = true;

        if (UnlockSound)
            UGameplayStatics::PlaySoundAtLocation(this, UnlockSound, GetActorLocation());

        // Activate linked actors (open doors etc)
        for (AActor* Actor : ActorsToActivate)
        {
            if (Actor)
            {
                Actor->SetActorHiddenInGame(true);
                Actor->SetActorEnableCollision(false);
            }
        }

        OnPuzzleSolved.Broadcast();
    }
}

FText AItemRequiredPuzzle::GetInteractPrompt_Implementation()
{
    return bIsSolved ? FText::FromString(TEXT("Unlocked")) : LockedPrompt;
}

// ─── ASymbolPuzzle ───────────────────────────────────────────
ASymbolPuzzle::ASymbolPuzzle()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASymbolPuzzle::BeginPlay()
{
    Super::BeginPlay();
}

void ASymbolPuzzle::Interact_Implementation(AActor* Interactor) {}

FText ASymbolPuzzle::GetInteractPrompt_Implementation()
{
    return FText::FromString(bIsSolved ? TEXT("Solved") : TEXT("Arrange the symbols"));
}

void ASymbolPuzzle::CycleTile(int32 TileIndex)
{
    if (!Tiles.IsValidIndex(TileIndex)) return;

    FSymbolTile& Tile = Tiles[TileIndex];
    Tile.CurrentSymbol = (Tile.CurrentSymbol + 1) % Tile.NumSymbols;

    if (TileClickSound)
        UGameplayStatics::PlaySoundAtLocation(this, TileClickSound, GetActorLocation());

    if (CheckAllTiles())
    {
        bIsSolved = true;
        if (SolvedSound)
            UGameplayStatics::PlaySoundAtLocation(this, SolvedSound, GetActorLocation());
        OnPuzzleSolved.Broadcast();
    }
}

void ASymbolPuzzle::SelectTile(int32 TileIndex)
{
    if (Tiles.IsValidIndex(TileIndex))
        SelectedTileIndex = TileIndex;
}

bool ASymbolPuzzle::CheckAllTiles() const
{
    for (const FSymbolTile& Tile : Tiles)
    {
        if (Tile.CurrentSymbol != Tile.CorrectSymbol) return false;
    }
    return !Tiles.IsEmpty();
}
