#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "HitReactionSettings.generated.h"

/**
 * Global settings for the Hit Reaction System
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Hit Reaction System"))
class HITREACTIONSYSTEM_API UHitReactionSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UHitReactionSettings();

    // ===== Default Physical Reaction Settings =====

    /** Default base impulse strength applied to mesh on hit (scales with damage) */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Defaults|Physical", meta = (ClampMin = "0.0"))
    float DefaultBaseImpulseStrength = 12.0f;

    /** Default minimum impulse force */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Defaults|Physical", meta = (ClampMin = "0.0"))
    float DefaultMinImpulse = 120.0f;

    /** Default maximum impulse force */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Defaults|Physical", meta = (ClampMin = "0.0"))
    float DefaultMaxImpulse = 1200.0f;

    /** Default death impulse strength when character dies */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Defaults|Physical", meta = (ClampMin = "0.0"))
    float DefaultDeathImpulseStrength = 5000.0f;

    /** Default physics blend weight for living characters (0 = animation only, 1 = full physics) */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Defaults|Physical", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DefaultPhysicsBlendWeight = 0.12f;

    /** Default duration of physics blend on hit */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Defaults|Physical", meta = (ClampMin = "0.0", Units = "s"))
    float DefaultPhysicsBlendDuration = 0.14f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Defaults|Physical")
    bool bDefaultEnableLocalizedPhysicsReaction = true;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Defaults|Physical", meta = (ClampMin = "0", ClampMax = "3"))
    int32 DefaultPhysicsParentsAboveHit = 1;

    // ===== Default Animation Settings =====

    /** Default minimum time between hit animation triggers */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Defaults|Animation", meta = (ClampMin = "0.0", Units = "s"))
    float DefaultMinTimeBetweenHitAnims = 0.2f;

    // ===== Ragdoll Settings =====

    /** Enable detailed ragdoll logging for debugging */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ragdoll|Debug")
    bool bLogRagdollActivation = false;

    /** Time before ragdolled bodies are destroyed */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ragdoll", meta = (ClampMin = "1.0", Units = "s"))
    float RagdollLifeSpan = 10.0f;

    /** Enable physics on all bones when ragdolling (if false, uses simplified ragdoll) */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
    bool bEnableFullBodyRagdoll = true;

    /** Linear damping for ragdoll bodies (reduces sliding) */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ragdoll|Physics", meta = (ClampMin = "0.0"))
    float RagdollLinearDamping = 0.1f;

    /** Angular damping for ragdoll bodies (reduces spinning) */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ragdoll|Physics", meta = (ClampMin = "0.0"))
    float RagdollAngularDamping = 0.1f;

    // ===== UDeveloperSettings Interface =====

    virtual FName GetCategoryName() const override;

#if WITH_EDITOR
    virtual FText GetSectionText() const override;
    virtual FText GetSectionDescription() const override;
#endif
};
