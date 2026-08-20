# Hit Reaction System

## Overview
Hit Reaction System is an Unreal Engine 5 runtime plugin for combining directional physical impulses with bone/zone-aware animation reactions. A reusable hit-reaction component resolves the impacted bone against authored hit zones, scales physical response from damage/hit information, optionally plays a zone-specific additive montage, broadcasts reaction events, and can hand fatal reactions into the shared Hell Run Physical Animation drive layer.

## Features
- Configurable physical impulse response for non-fatal hits.
- Bone-based hit-zone matching with per-zone radius and impulse multiplier.
- Gameplay-tag style zone identifiers for downstream reaction logic.
- Optional additive reaction montage per hit zone plus a default fallback montage.
- Closest-bone / impact-bone handling for localized reactions.
- Cooldown and montage-interruption controls for repeated hits.
- Blueprint-accessible reaction event carrying hit information and fatal state.
- Death/ragdoll response integrated with `HellRunPhysicalAnimation`.
- Project settings for shared physical, animation, and ragdoll defaults.

## Architecture
The host character forwards its damage/hit result to the Hit Reaction Component. The component resolves a configured zone, applies the corresponding physical response, selects a zone/default animation when appropriate, and broadcasts the resulting hit-reaction event. Fatal reactions bypass normal additive reaction flow and hand the character into the death physical-animation/ragdoll path.

## Installation
1. Install the required **HellRunPhysicalAnimation** plugin at `<Project>/Plugins/HellRunPhysicalAnimation`.
2. Clone or copy this repository to `<Project>/Plugins/HitReactionSystem`.
3. Delete stale `Binaries` and `Intermediate` folders from both plugins if they were compiled with another Unreal version.
4. Regenerate project files and compile your Editor target.
5. Launch Unreal Editor and verify **Hell Run Physical Animation** and **Hit Reaction System** are enabled under **Edit > Plugins**.
6. Review **Project Settings > Plugins > Hit Reaction System** for global defaults.

```bash
git clone https://github.com/Andressalazar005/HellRunPhysicalAnimation.git <Project>/Plugins/HellRunPhysicalAnimation
git clone https://github.com/Andressalazar005/HitReactionSystem.git <Project>/Plugins/HitReactionSystem
```

## Basic setup
1. Add the Hit Reaction Component to the character/actor that should react to damage.
2. Add entries to its **Hit Zones** array.
3. For each zone, configure the target bone, optional detection radius, zone tag, impulse multiplier, and optional additive montage.
4. Configure default physical strengths, animation behavior, and fatal/ragdoll settings on the component or in Project Settings.
5. From the character's damage path, pass the impact location/direction/bone and fatal state into the reaction system.
6. Bind to the reaction event if UI, audio, VFX, AI, or other gameplay systems need to react to the result.

## Example zone configuration
```text
Zone: Head
Bone Name: head
Radius: 15
Zone Tag: HitZone.Head
Impulse Multiplier: 1.5
Hit Reaction Montage: AM_HeadHit_Additive
```

## Key behavior
- Hit zones are evaluated in configured order; the first valid match wins.
- Impulse strength can scale with incoming damage.
- Additive montages work best when they are authored to layer over the character's active locomotion/action pose.
- Fatal reactions use the death physical-animation/ragdoll path instead of playing the normal hit montage.

## Dependency
- `HellRunPhysicalAnimation`

## Support
Use GitHub Issues for reproducible integration problems. Include your Unreal Engine version, skeleton/physics asset, hit bone, configured zone, damage/impulse values, whether the hit was fatal, and relevant logs.