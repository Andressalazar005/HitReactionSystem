#include "HitReactionComponent.h"
#include "HitReactionSettings.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/BodyInstance.h"
#include "HellRunPhysicalAnimationComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Engine/World.h"

UHitReactionComponent::UHitReactionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Load defaults from settings
    const UHitReactionSettings* Settings = GetDefault<UHitReactionSettings>();
    if (Settings)
    {
        BaseImpulseStrength = Settings->DefaultBaseImpulseStrength;
        MinImpulse = Settings->DefaultMinImpulse;
        MaxImpulse = Settings->DefaultMaxImpulse;
        DeathImpulseStrength = Settings->DefaultDeathImpulseStrength;
        PhysicsBlendWeight = Settings->DefaultPhysicsBlendWeight;
        PhysicsBlendDuration = Settings->DefaultPhysicsBlendDuration;
        bEnableLocalizedPhysicsReaction = Settings->bDefaultEnableLocalizedPhysicsReaction;
        PhysicsParentsAboveHit = Settings->DefaultPhysicsParentsAboveHit;
        MinTimeBetweenHitAnims = Settings->DefaultMinTimeBetweenHitAnims;
    }
}

void UHitReactionComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache the skeletal mesh component
    CachedMeshComponent = GetSkeletalMeshComponent();
    CachedPhysicalAnimationComponent = GetOwner()
        ? GetOwner()->FindComponentByClass<UHellRunPhysicalAnimationComponent>()
        : nullptr;
}

USkeletalMeshComponent* UHitReactionComponent::GetSkeletalMeshComponent() const
{
    if (CachedMeshComponent)
    {
        return CachedMeshComponent;
    }

    // Try to get from character
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        return Character->GetMesh();
    }

    // Try to find on owner
    return GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
}

bool UHitReactionComponent::IsLocalOwningPlayer() const
{
    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return false;
    }

    if (const APawn* PawnOwner = Cast<APawn>(OwnerActor))
    {
        return PawnOwner->IsLocallyControlled();
    }

    if (const AController* ControllerOwner = Cast<AController>(OwnerActor))
    {
        return ControllerOwner->IsLocalController();
    }

    if (const APawn* OwnerPawn = OwnerActor->GetInstigator())
    {
        return OwnerPawn->IsLocallyControlled();
    }

    return false;
}

void UHitReactionComponent::ApplyHitReaction(float Damage, const FVector& HitDirection, const FVector& ImpactLocation, FName HitBone, bool bIsFatalHit, FVector HitFromLocation, bool bBroadcastEvent)
{
    if (bDeathRagdollHandoffActive)
    {
        return;
    }

    USkeletalMeshComponent* MeshComp = GetSkeletalMeshComponent();
    if (!MeshComp)
    {
        return;
    }

    // Find which zone was hit
    FName ClosestBone = HitBone;
    FHitZone* HitZonePtr = FindHitZone(ImpactLocation, ClosestBone);

    // Update last hit info
    LastHitInfo.HitDirection = HitDirection;
    LastHitInfo.HitBone = ClosestBone;
    LastHitInfo.HitZone = HitZonePtr ? HitZonePtr->ZoneTag : FGameplayTag();
    LastHitInfo.Damage = Damage;
    LastHitInfo.HitTime = GetWorld()->GetTimeSeconds();
    LastHitInfo.ImpactLocation = ImpactLocation;
    LastHitInfo.HitFromLocation = HitFromLocation;

    if (!bIsFatalHit)
    {
        const float ImpulseMultiplier = HitZonePtr ? HitZonePtr->ImpulseMultiplier : 1.0f;
        ApplyPhysicalReaction(HitDirection, ImpactLocation, Damage, ClosestBone, ImpulseMultiplier);

        // Play hit animation
        if (bPlayHitAnimations)
        {
            PlayHitAnimation(HitZonePtr);
        }
    }

    if (bBroadcastEvent)
    {
        OnHitReaction.Broadcast(LastHitInfo, bIsFatalHit);
        OnHitReactionWithFromLocation.Broadcast(LastHitInfo, bIsFatalHit, HitFromLocation);
    }
}

FHitZone* UHitReactionComponent::FindHitZone(const FVector& ImpactLocation, FName& OutClosestBone)
{
    USkeletalMeshComponent* MeshComp = GetSkeletalMeshComponent();
    if (!MeshComp)
    {
        return nullptr;
    }

    FHitZone* ClosestZone = nullptr;
    float ClosestDistance = FLT_MAX;

    // Check all defined zones
    for (FHitZone& Zone : HitZones)
    {
        if (Zone.BoneName == NAME_None)
        {
            continue;
        }

        // Get bone location
        FVector BoneLocation = MeshComp->GetBoneLocation(Zone.BoneName);
        float Distance = FVector::Dist(BoneLocation, ImpactLocation);

        // Check if within zone radius (or exact match if radius is 0)
        if (Zone.Radius > 0.0f)
        {
            if (Distance <= Zone.Radius && Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                ClosestZone = &Zone;
                OutClosestBone = Zone.BoneName;
            }
        }
        else if (Distance < ClosestDistance)
        {
            // No radius, just find closest
            ClosestDistance = Distance;
            ClosestZone = &Zone;
            OutClosestBone = Zone.BoneName;
        }
    }

    // If no zone found but we should find closest bone
    if (!ClosestZone && bAlwaysFindClosestBone)
    {
        // Find closest bone to impact
        OutClosestBone = MeshComp->FindClosestBone(ImpactLocation);
    }

    return ClosestZone;
}

void UHitReactionComponent::ApplyPhysicalReaction(const FVector& Direction, const FVector& ImpactLocation,
    float Damage, FName BoneName, float ImpulseMultiplier)
{
    if (bDeathRagdollHandoffActive)
    {
        return;
    }

    USkeletalMeshComponent* MeshComp = GetSkeletalMeshComponent();
    UWorld* World = GetWorld();
    UPhysicsAsset* PhysicsAsset = MeshComp ? MeshComp->GetPhysicsAsset() : nullptr;
    const USkeletalMesh* SkeletalMesh = MeshComp ? MeshComp->GetSkeletalMeshAsset() : nullptr;
    if (!bEnableLocalizedPhysicsReaction || !MeshComp || !World || !PhysicsAsset || !SkeletalMesh ||
        BoneName == NAME_None || PhysicsAsset->FindBodyIndex(BoneName) == INDEX_NONE)
    {
        return;
    }

    const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
    FName SimulationRoot = BoneName;
    int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);

    if (LocalizedPhysicsExcludedBones.Contains(BoneName))
    {
        return;
    }

    const int32 HitBodyIndex = PhysicsAsset->FindBodyIndex(BoneName);
    if (bExcludePhysicsAssetRootFromLocalizedReaction && HitBodyIndex == 0)
    {
        // The first Physics Asset body is normally pelvis. Simulating everything
        // below it would detach the rendered mesh orientation from the live capsule.
        return;
    }

    for (int32 ParentStep = 0; ParentStep < PhysicsParentsAboveHit && BoneIndex != INDEX_NONE; ++ParentStep)
    {
        int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
        while (ParentIndex != INDEX_NONE && PhysicsAsset->FindBodyIndex(RefSkeleton.GetBoneName(ParentIndex)) == INDEX_NONE)
        {
            ParentIndex = RefSkeleton.GetParentIndex(ParentIndex);
        }
        if (ParentIndex == INDEX_NONE)
        {
            break;
        }
        BoneIndex = ParentIndex;
        const FName CandidateRoot = RefSkeleton.GetBoneName(BoneIndex);
        if (LocalizedPhysicsExcludedBones.Contains(CandidateRoot))
        {
            break;
        }
        SimulationRoot = CandidateRoot;
    }

    if (bExcludePhysicsAssetRootFromLocalizedReaction && PhysicsAsset->FindBodyIndex(SimulationRoot) == 0)
    {
        // A child hit may climb to pelvis when including its parent. Keep the child
        // branch localized instead of activating full-body physics.
        SimulationRoot = BoneName;
    }

    if (LocalizedPhysicsExcludedBones.Contains(SimulationRoot))
    {
        return;
    }

    // Blueprint mesh defaults can restore QueryOnly/NoCollision after the native
    // character setup. Chaos rejects a bone impulse unless component collision is
    // physics-enabled, even when the selected body has just begun simulating.
    if (MeshComp->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics &&
        MeshComp->GetCollisionEnabled() != ECollisionEnabled::PhysicsOnly)
    {
        MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    MeshComp->SetAllBodiesBelowSimulatePhysics(SimulationRoot, true, true);
    MeshComp->SetAllBodiesBelowPhysicsBlendWeight(SimulationRoot, PhysicsBlendWeight, false, true);
    MeshComp->WakeAllRigidBodies();
    ActiveSimulatedRoots.Add(SimulationRoot);

    // A blend weight alone leaves the branch as a passive partial ragdoll. Drive
    // it toward the animation pose during the reaction window so it reacts with
    // weight and then recovers instead of flopping until the timer expires.
    if (CachedPhysicalAnimationComponent)
    {
        CachedPhysicalAnimationComponent->HitReactionProfile = HitPhysicalAnimationProfile;
        CachedPhysicalAnimationComponent->HitOrientationStrength = HitOrientationStrength;
        CachedPhysicalAnimationComponent->HitAngularVelocityStrength = HitAngularVelocityStrength;
        CachedPhysicalAnimationComponent->HitMaxAngularForce = HitMaxAngularForce;
        for (const FName ActiveRoot : ActiveSimulatedRoots)
        {
            CachedPhysicalAnimationComponent->DriveHitReaction(MeshComp, ActiveRoot);
        }
        CachedPhysicalAnimationComponent->SetDriveStrength(1.0f);
    }

    const float ImpulseStrength = FMath::Clamp(
        Damage * BaseImpulseStrength * FMath::Max(0.0f, ImpulseMultiplier), MinImpulse, MaxImpulse);
    // A hit bone can exist in the skeleton without owning a simulated body. Apply
    // to the localized simulation root in that case instead of asking Chaos to
    // impulse a kinematic/nonexistent body.
    const FBodyInstance* HitBody = MeshComp->GetBodyInstance(BoneName);
    const FName ImpulseBone = HitBody && HitBody->IsInstanceSimulatingPhysics() ? BoneName : SimulationRoot;
    const FBodyInstance* ImpulseBody = MeshComp->GetBodyInstance(ImpulseBone);
    if (ImpulseBody && ImpulseBody->IsInstanceSimulatingPhysics())
    {
        MeshComp->AddImpulseAtLocation(Direction.GetSafeNormal() * ImpulseStrength, ImpactLocation, ImpulseBone);
    }

    LastSimulatedBone = SimulationRoot;
    // A single retriggerable absolute window. Rapid hits replace this deadline;
    // they never add another duration to an accumulated timer.
    PhysicsBlendEndTime = World->GetTimeSeconds() + FMath::Max(0.01f, PhysicsBlendDuration);
    World->GetTimerManager().ClearTimer(BlendBackTimerHandle);
    World->GetTimerManager().SetTimer(
        BlendBackTimerHandle, this, &UHitReactionComponent::BlendBackToAnimation,
        FMath::Max(0.01f, PhysicsBlendDuration), false);
}

void UHitReactionComponent::HandoffToDeathRagdoll()
{
    bDeathRagdollHandoffActive = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BlendBackTimerHandle);
        World->GetTimerManager().ClearTimer(StaggerResetTimerHandle);
    }

    // Do not blend these bodies back or disable their simulation. StartDeathRagdoll
    // immediately promotes the mesh to full simulation and retains their velocities.
    ActiveSimulatedRoots.Reset();
    LastSimulatedBone = NAME_None;
    AccumulatedStagger = 0.0f;
    PhysicsBlendEndTime = -1.0;
}

void UHitReactionComponent::GenerateOrUpdatePhysicalAnimationProfiles()
{
    if (!CachedPhysicalAnimationComponent && GetOwner())
    {
        CachedPhysicalAnimationComponent =
            GetOwner()->FindComponentByClass<UHellRunPhysicalAnimationComponent>();
    }
    if (CachedPhysicalAnimationComponent)
    {
        CachedPhysicalAnimationComponent->HitReactionProfile = HitPhysicalAnimationProfile;
        CachedPhysicalAnimationComponent->HitOrientationStrength = HitOrientationStrength;
        CachedPhysicalAnimationComponent->HitAngularVelocityStrength = HitAngularVelocityStrength;
        CachedPhysicalAnimationComponent->HitMaxAngularForce = HitMaxAngularForce;
        CachedPhysicalAnimationComponent->GenerateOrUpdateProfiles();
    }
}

void UHitReactionComponent::BlendBackToAnimation()
{
    if (bDeathRagdollHandoffActive)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(BlendBackTimerHandle);
        const double Remaining = PhysicsBlendEndTime - World->GetTimeSeconds();
        if (PhysicsBlendEndTime >= 0.0 && Remaining > KINDA_SMALL_NUMBER)
        {
            World->GetTimerManager().SetTimer(BlendBackTimerHandle, this,
                &UHitReactionComponent::BlendBackToAnimation,
                FMath::Max(0.001f, static_cast<float>(Remaining)), false);
            return;
        }
    }
    PhysicsBlendEndTime = -1.0;

    USkeletalMeshComponent* MeshComp = GetSkeletalMeshComponent();
    if (!MeshComp || !IsValid(MeshComp))
    {
        LastSimulatedBone = NAME_None;
        ActiveSimulatedRoots.Reset();
        return;
    }

    if (CachedPhysicalAnimationComponent)
    {
        CachedPhysicalAnimationComponent->ReleaseDrive();
    }

    // This is the authoritative end of a temporary living-character reaction.
    // Nested/overlapping roots can otherwise leave a parent body simulated after
    // the branch timers collapse into one timer. Death cancels this callback in
    // HandoffToDeathRagdoll, so a full reset here cannot disable a death ragdoll.
    MeshComp->SetAllBodiesPhysicsBlendWeight(0.0f, false);
    MeshComp->SetAllBodiesSimulatePhysics(false);
    MeshComp->bBlendPhysics = false;
    MeshComp->RefreshBoneTransforms();

    // Clear the stored bone
    LastSimulatedBone = NAME_None;
    ActiveSimulatedRoots.Reset();
}

void UHitReactionComponent::PlayHitAnimation(const FHitZone* HitZone)
{
    USkeletalMeshComponent* MeshComp = GetSkeletalMeshComponent();
    if (!MeshComp)
    {
        return;
    }

    UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    // Check cooldown
    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastHitAnimTime < MinTimeBetweenHitAnims)
    {
        return;
    }

    AccumulatedStagger += FMath::Max(0.0f, LastHitInfo.Damage);
    GetWorld()->GetTimerManager().ClearTimer(StaggerResetTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(StaggerResetTimerHandle, this,
        &UHitReactionComponent::ResetAccumulatedStagger, FMath::Max(0.01f, StaggerMemoryDuration), false);

    const bool bHeavyReaction = HeavyReactionStaggerThreshold > 0.0f &&
        AccumulatedStagger >= HeavyReactionStaggerThreshold;
    if (bHeavyReaction)
    {
        AccumulatedStagger = FMath::Max(0.0f, AccumulatedStagger - HeavyReactionStaggerThreshold);
    }

    // HitDirection is force/bullet travel. Negate it when selecting an authored
    // reaction because montage direction describes the side the attack came from.
    const FVector LocalHitDirection = GetOwner()->GetActorTransform().InverseTransformVectorNoScale(
        -LastHitInfo.HitDirection.GetSafeNormal());
    const int32 DirectionIndex = FMath::Abs(LocalHitDirection.X) >= FMath::Abs(LocalHitDirection.Y)
        ? (LocalHitDirection.X >= 0.0f ? 0 : 1)
        : (LocalHitDirection.Y >= 0.0f ? 3 : 2);

    // Get montage to play
    UAnimMontage* MontageToPlay = nullptr;
    float BlendWeight = 1.0f;

    const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
    const bool bRunning = CharacterOwner && CharacterOwner->GetVelocity().Size2D() >= RunningStumbleMinSpeed;
    if (bHeavyReaction && bRunning && !RunningStumbleReactions.IsEmpty())
    {
        MontageToPlay = RunningStumbleReactions[FMath::RandHelper(RunningStumbleReactions.Num())];
    }
    else
    {
        const FDirectionalHitReactionSet& DirectionalSet = bHeavyReaction
            ? HeavyReactionsByDirection
            : LightReactionsByDirection;
        MontageToPlay = DirectionalSet.GetByDirectionIndex(DirectionIndex);

        // Preserve authored array data as a fallback while Blueprints migrate to
        // the named directional slots.
        const TArray<TObjectPtr<UAnimMontage>>& DirectionalPool = bHeavyReaction
            ? DirectionalHeavyReactions
            : DirectionalLightReactions;
        if (!MontageToPlay && DirectionalPool.IsValidIndex(DirectionIndex))
        {
            MontageToPlay = DirectionalPool[DirectionIndex];
        }
    }

    if (!MontageToPlay && HitZone && HitZone->HitReactionMontage)
    {
        MontageToPlay = HitZone->HitReactionMontage;
        BlendWeight = HitZone->AnimationBlendWeight;
    }
    else if (DefaultHitReactionMontage)
    {
        MontageToPlay = DefaultHitReactionMontage;
    }

    if (!MontageToPlay)
    {
        return;
    }

    // Stop current montage if needed
    if (bInterruptCurrentMontage && AnimInstance->IsAnyMontagePlaying())
    {
        AnimInstance->Montage_Stop(0.2f);
    }

    // Play the montage
    AnimInstance->Montage_Play(MontageToPlay, 1.0f);

    // If the montage is additive, set the blend weight
    // Note: This is a simplified approach - you might want to use blend per bone or layered animations
    if (MontageToPlay->IsValidAdditive())
    {
        AnimInstance->Montage_SetPlayRate(MontageToPlay, BlendWeight);
    }

    LastHitAnimTime = CurrentTime;
}

void UHitReactionComponent::ResetAccumulatedStagger()
{
    AccumulatedStagger = 0.0f;
}
