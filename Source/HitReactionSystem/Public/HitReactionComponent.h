#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "HitReactionComponent.generated.h"

class UAnimMontage;
class UHellRunPhysicalAnimationComponent;

/** Explicit montage slots relative to the character receiving the hit. */
USTRUCT(BlueprintType)
struct FDirectionalHitReactionSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Directional Reactions")
    TObjectPtr<UAnimMontage> Front = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Directional Reactions")
    TObjectPtr<UAnimMontage> Back = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Directional Reactions")
    TObjectPtr<UAnimMontage> Left = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Directional Reactions")
    TObjectPtr<UAnimMontage> Right = nullptr;

    UAnimMontage* GetByDirectionIndex(int32 DirectionIndex) const
    {
        switch (DirectionIndex)
        {
        case 0: return Front;
        case 1: return Back;
        case 2: return Left;
        case 3: return Right;
        default: return nullptr;
        }
    }
};

/** Defines a hit zone on the body with its own reaction settings */
USTRUCT(BlueprintType)
struct FHitZone
{
    GENERATED_BODY()

    /** Name of the bone that defines this hit zone */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zone")
    FName BoneName = NAME_None;

    /** Radius around the bone to consider hits in this zone (0 = exact bone only) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zone", meta = (ClampMin = "0.0", Units = "cm"))
    float Radius = 0.0f;

    /** Gameplay tag to identify this zone (e.g., "HitZone.Head", "HitZone.Chest") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zone")
    FGameplayTag ZoneTag;

    /** Physical impulse multiplier for this zone (1.0 = normal, 2.0 = double force) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zone", meta = (ClampMin = "0.0"))
    float ImpulseMultiplier = 1.0f;

    /** Additive animation to play when hit in this zone */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zone|Animation")
    UAnimMontage* HitReactionMontage = nullptr;

    /** Blend weight for the additive animation (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Zone|Animation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AnimationBlendWeight = 1.0f;
};

/** Info about the last hit received */
USTRUCT(BlueprintType)
struct FLastHitInfo
{
    GENERATED_BODY()

    /** Direction the hit came from */
    UPROPERTY(BlueprintReadOnly, Category = "Hit Info")
    FVector HitDirection = FVector::ZeroVector;

    /** Bone that was hit (closest to impact point) */
    UPROPERTY(BlueprintReadOnly, Category = "Hit Info")
    FName HitBone = NAME_None;

    /** Zone that was hit */
    UPROPERTY(BlueprintReadOnly, Category = "Hit Info")
    FGameplayTag HitZone;

    /** Damage amount dealt */
    UPROPERTY(BlueprintReadOnly, Category = "Hit Info")
    float Damage = 0.0f;

    /** Time the hit occurred */
    UPROPERTY(BlueprintReadOnly, Category = "Hit Info")
    float HitTime = 0.0f;

    /** Impact location in world space */
    UPROPERTY(BlueprintReadOnly, Category = "Hit Info")
    FVector ImpactLocation = FVector::ZeroVector;

    /** World location the hit came from, usually the instigator pawn location. */
    UPROPERTY(BlueprintReadOnly, Category = "Hit Info", meta = (DisplayName = "Hit From Location"))
    FVector HitFromLocation = FVector::ZeroVector;
};

/** Delegate for hit reaction events */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitReaction, const FLastHitInfo&, HitInfo, bool, bWasFatal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHitReactionWithFromLocation, const FLastHitInfo&, HitInfo, bool, bWasFatal, FVector, HitFromLocation);

/**
 * Component that handles physical hit reactions with animation blending and zone tracking
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HITREACTIONSYSTEM_API UHitReactionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHitReactionComponent();

    // ===== Physical Reaction Settings =====

    /** Base impulse strength applied to mesh on hit (scales with damage) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical", meta = (ClampMin = "0.0"))
    float BaseImpulseStrength = 12.0f;

    /** Minimum impulse force */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical", meta = (ClampMin = "0.0"))
    float MinImpulse = 120.0f;

    /** Maximum impulse force */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical", meta = (ClampMin = "0.0"))
    float MaxImpulse = 1200.0f;

    /** Death impulse strength when character dies */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical", meta = (ClampMin = "0.0"))
    float DeathImpulseStrength = 5000.0f;

    /** If true, apply impulses at the hit bone instead of the root */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical")
    bool bApplyImpulseAtHitBone = true;

    /** Enable physics blending on hit for living characters (0-1, 0 = animation only, 1 = full physics) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PhysicsBlendWeight = 0.12f;

    /** Duration of physics blend on hit */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical", meta = (ClampMin = "0.0", Units = "s"))
    float PhysicsBlendDuration = 0.14f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical Animation")
    FName HitPhysicalAnimationProfile = FName("HellRun_HitReaction");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical Animation", meta=(ClampMin="0.0"))
    float HitOrientationStrength = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical Animation", meta=(ClampMin="0.0"))
    float HitAngularVelocityStrength = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical Animation", meta=(ClampMin="0.0"))
    float HitMaxAngularForce = 0.0f;

    /** Enables localized, temporary physics on living characters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical")
    bool bEnableLocalizedPhysicsReaction = true;

    /** Number of physics-body parents included above the struck bone. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical", meta = (EditCondition = "bEnableLocalizedPhysicsReaction", ClampMin = "0", ClampMax = "3"))
    int32 PhysicsParentsAboveHit = 1;

    /** Prevents pelvis/root hits from enabling physics on the entire living character. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical", meta = (EditCondition = "bEnableLocalizedPhysicsReaction"))
    bool bExcludePhysicsAssetRootFromLocalizedReaction = true;

    /** Axial bodies that must remain animation-driven while alive. Simulating these branches can lever the mesh away from its capsule. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Physical", meta = (EditCondition = "bEnableLocalizedPhysicsReaction"))
    TArray<FName> LocalizedPhysicsExcludedBones = {
        FName("root"), FName("pelvis"), FName("spine_01"), FName("spine_02"), FName("spine_03")
    };

    // ===== Animation Settings =====

    /** If true, play hit reaction animations */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Animation")
    bool bPlayHitAnimations = true;

    /** If true, interrupt current montage when playing hit reaction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Animation")
    bool bInterruptCurrentMontage = false;

    /** Minimum time between hit animation triggers (prevents spam) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Animation", meta = (ClampMin = "0.0", Units = "s"))
    float MinTimeBetweenHitAnims = 0.2f;

    /** Default hit reaction montage if no zone-specific one is found */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Animation")
    UAnimMontage* DefaultHitReactionMontage = nullptr;

    /** Light reactions explicitly mapped to Front, Back, Left, and Right. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|L4D Style")
    FDirectionalHitReactionSet LightReactionsByDirection;

    /** Heavy reactions explicitly mapped to Front, Back, Left, and Right. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|L4D Style")
    FDirectionalHitReactionSet HeavyReactionsByDirection;

    /** Legacy compatibility fallback, indexed as Front, Back, Left, Right. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|L4D Style")
    TArray<TObjectPtr<UAnimMontage>> DirectionalLightReactions;

    /** Legacy compatibility fallback, indexed as Front, Back, Left, Right. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|L4D Style")
    TArray<TObjectPtr<UAnimMontage>> DirectionalHeavyReactions;

    /** Full-body moving stumbles used when accumulated stagger crosses the threshold. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|L4D Style")
    TArray<TObjectPtr<UAnimMontage>> RunningStumbleReactions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|L4D Style", meta = (ClampMin = "0.0"))
    float HeavyReactionStaggerThreshold = 55.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|L4D Style", meta = (ClampMin = "0.0", Units = "s"))
    float StaggerMemoryDuration = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|L4D Style", meta = (ClampMin = "0.0", Units = "cm/s"))
    float RunningStumbleMinSpeed = 180.0f;

    // ===== Hit Zones =====

    /** Defined hit zones on this character */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Zones")
    TArray<FHitZone> HitZones;

    /** If true, automatically find closest bone even if no zone is hit */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Reaction|Zones")
    bool bAlwaysFindClosestBone = true;

    // ===== Events =====

    /** Called when a hit reaction is triggered */
    UPROPERTY(BlueprintAssignable, Category = "Hit Reaction|Events")
    FOnHitReaction OnHitReaction;

    /** Called when a hit reaction is triggered, with the shooter/source location exposed as a direct Blueprint pin. */
    UPROPERTY(BlueprintAssignable, Category = "Hit Reaction|Events")
    FOnHitReactionWithFromLocation OnHitReactionWithFromLocation;

    // ===== Public API =====

    /** Apply a hit reaction */
    UFUNCTION(BlueprintCallable, Category = "Hit Reaction")
    void ApplyHitReaction(float Damage, const FVector& HitDirection, const FVector& ImpactLocation, FName HitBone = NAME_None, bool bIsFatalHit = false, FVector HitFromLocation = FVector::ZeroVector, bool bBroadcastEvent = true);

    UFUNCTION(BlueprintPure, Category = "Hit Reaction")
    FVector GetLastHitFromLocation() const { return LastHitInfo.HitFromLocation; }

    /** True only on the machine that owns this component's pawn/controller. Use this for local HUD effects, not HasAuthority. */
    UFUNCTION(BlueprintPure, Category = "Hit Reaction|Networking", meta = (DisplayName = "Is Local Owning Player"))
    bool IsLocalOwningPlayer() const;

    /** Get info about the last hit */
    UFUNCTION(BlueprintPure, Category = "Hit Reaction")
    const FLastHitInfo& GetLastHitInfo() const { return LastHitInfo; }

    /** Get the skeletal mesh component this is attached to */
    UFUNCTION(BlueprintPure, Category = "Hit Reaction")
    USkeletalMeshComponent* GetSkeletalMeshComponent() const;

    /** Cancels living-character recovery without disabling its active bodies, allowing full death ragdoll to take ownership. */
    UFUNCTION(BlueprintCallable, Category = "Hit Reaction|Physical")
    void HandoffToDeathRagdoll();

    /** Adds or updates sane hit-reaction and death profiles on the assigned Physics Asset. */
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Hit Reaction|Physical Animation")
    void GenerateOrUpdatePhysicalAnimationProfiles();

protected:
    virtual void BeginPlay() override;

    /** Find which zone was hit based on impact location and bone */
    FHitZone* FindHitZone(const FVector& ImpactLocation, FName& OutClosestBone);

    /** Apply physical impulse to the mesh */
    void ApplyPhysicalReaction(const FVector& Direction, const FVector& ImpactLocation, float Damage, FName BoneName, float ImpulseMultiplier);

    /** Play hit reaction animation */
    void PlayHitAnimation(const FHitZone* HitZone);
    void ResetAccumulatedStagger();

    /** Blend back from physics to animation */
    void BlendBackToAnimation();

private:
    /** Info about the last hit */
    UPROPERTY()
    FLastHitInfo LastHitInfo;

    /** Time of last hit animation */
    float LastHitAnimTime = 0.0f;

    /** Cached skeletal mesh component */
    UPROPERTY()
    USkeletalMeshComponent* CachedMeshComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UHellRunPhysicalAnimationComponent> CachedPhysicalAnimationComponent;

    /** Timer handle for blending back to animation */
    FTimerHandle BlendBackTimerHandle;
    FTimerHandle StaggerResetTimerHandle;
    float AccumulatedStagger = 0.0f;
    double PhysicsBlendEndTime = -1.0;

    /** Last bone that was set to simulate physics */
    FName LastSimulatedBone = NAME_None;

    /** All overlapping branches remain active until the most recent hit's blend window expires. */
    TSet<FName> ActiveSimulatedRoots;

    /** Set once death ragdoll takes ownership so late hit-reaction RPCs/timers cannot alter corpse bodies. */
    bool bDeathRagdollHandoffActive = false;
};
