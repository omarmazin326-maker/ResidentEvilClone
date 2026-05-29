#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

// ============================================================
// Item Types
// ============================================================
UENUM(BlueprintType)
enum class EItemType : uint8
{
    Weapon       UMETA(DisplayName = "Weapon"),
    Ammo         UMETA(DisplayName = "Ammo"),
    Heal         UMETA(DisplayName = "Heal"),
    KeyItem      UMETA(DisplayName = "Key Item"),
    Combinable   UMETA(DisplayName = "Combinable"),
    Document     UMETA(DisplayName = "Document"),
    Herb         UMETA(DisplayName = "Herb"),
    Misc         UMETA(DisplayName = "Misc")
};

UENUM(BlueprintType)
enum class EHerbType : uint8
{
    Green    UMETA(DisplayName = "Green Herb"),
    Red      UMETA(DisplayName = "Red Herb"),
    Blue     UMETA(DisplayName = "Blue Herb")
};

// ============================================================
// Item Data Structs
// ============================================================
USTRUCT(BlueprintType)
struct FItemData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemType ItemType = EItemType::Misc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class UTexture2D* Icon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StackSize = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxStackSize = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float UseValue = 0.f; // Heal amount, ammo count, etc.

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsKeyItem = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanCombine = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName CombinesWithID; // What this item combines with

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName CombinationResultID; // Result when combined

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EHerbType HerbType = EHerbType::Green; // Only for herbs

    bool IsValid() const { return !ItemID.IsNone(); }
};

USTRUCT(BlueprintType)
struct FInventorySlot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FItemData Item;

    UPROPERTY(BlueprintReadWrite)
    bool bIsEmpty = true;

    FInventorySlot() : bIsEmpty(true) {}
    FInventorySlot(const FItemData& InItem) : Item(InItem), bIsEmpty(false) {}
};

// ============================================================
// Delegates
// ============================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUsed, FItemData, UsedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemCombined, FItemData, ItemA, FItemData, Result);

// ============================================================
// Inventory Component
// ============================================================
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RESIDENTEVILCLONE_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
    int32 MaxSlots = 8; // Classic RE limited inventory

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    TArray<FInventorySlot> Slots;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    bool bInventoryOpen = false;

    // Item database (loaded from DataTable in BP)
    UPROPERTY(EditAnywhere, Category = "Inventory")
    class UDataTable* ItemDatabase;

    // Widget class for inventory UI
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> InventoryWidgetClass;

    class UUserWidget* InventoryWidget = nullptr;

public:
    // ─── Events ──────────────────────────────────────────────
    UPROPERTY(BlueprintAssignable)
    FOnInventoryChanged OnInventoryChanged;

    UPROPERTY(BlueprintAssignable)
    FOnItemUsed OnItemUsed;

    UPROPERTY(BlueprintAssignable)
    FOnItemCombined OnItemCombined;

    // ─── Core Operations ─────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(const FItemData& Item);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(FName ItemID, int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool UseItem(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool CombineItems(int32 SlotA, int32 SlotB);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void DropItem(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SwapSlots(int32 SlotA, int32 SlotB);

    // ─── Queries ─────────────────────────────────────────────
    UFUNCTION(BlueprintPure, Category = "Inventory")
    bool HasItem(FName ItemID, int32 Amount = 1) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemCount(FName ItemID) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    bool IsFull() const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetFreeSlots() const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    const TArray<FInventorySlot>& GetSlots() const { return Slots; }

    // ─── UI ──────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ToggleInventoryUI();

    UFUNCTION(BlueprintPure, Category = "Inventory")
    bool IsInventoryOpen() const { return bInventoryOpen; }

    // ─── Herb Combining ──────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Inventory|Herbs")
    bool CombineHerbs(EHerbType HerbA, EHerbType HerbB);

    FItemData GetItemDataByID(FName ItemID) const;

private:
    int32 FindSlotWithItem(FName ItemID) const;
    int32 FindFirstEmptySlot() const;
    void UseHealItem(const FItemData& Item);
    void UseAmmoItem(const FItemData& Item);
    FItemData CreateHerbMixture(EHerbType HerbA, EHerbType HerbB) const;
};
