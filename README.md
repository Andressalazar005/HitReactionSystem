# Hit Reaction System Plugin

## Overview
A comprehensive hit reaction system with physical impulses, animation blending, and bone-based zone tracking.

## Features
- **Physical Hit Reactions**: Configurable impulse forces applied when hit
- **Hit Zone System**: Define multiple zones (head, chest, limbs) with unique reactions
- **Animation Support**: Play additive hit reaction animations per zone
- **Bone Tracking**: Automatically finds closest bone to impact point
- **Death Impulses**: Ragdoll activation with directional force
- **Event System**: Blueprint-accessible delegates for custom logic
- **Project Settings**: Configure global defaults via Edit ? Project Settings ? Plugins ? Hit Reaction System

## Setup

### 1. Configure Global Settings (Optional)
1. Open **Edit ? Project Settings**
2. Navigate to **Plugins ? Hit Reaction System**
3. Adjust default values:
   - **Physical defaults**: Base/min/max impulse, death impulse, physics blend
   - **Animation defaults**: Min time between hit anims
   - **Ragdoll settings**: Life span, physics damping, full body enable

### 2. Enable the Plugin
The plugin is automatically enabled in `Hell_Run.uproject`.

### 2. Add Hit Zones (In Blueprint)
1. Select your character blueprint
2. Find the **Hit Reaction Component**
3. Add entries to the **Hit Zones** array
4. For each zone:
   - Set **Bone Name** (e.g., "head", "spine_03", "hand_r")
   - Set **Radius** (how far from bone to detect, 0 = exact)
   - Set **Zone Tag** (e.g., "HitZone.Head")
   - Set **Impulse Multiplier** (1.0 = normal, 2.0 = double force)
   - Assign **Hit Reaction Montage** (additive animation to play)
   - Set **Animation Blend Weight** (0-1)

### 3. Configure Physical Settings
Adjust these properties on the Hit Reaction Component:

**Physical Reaction:**
- `Base Impulse Strength`: Base force multiplier (default: 50.0)
- `Min Impulse`: Minimum force applied (default: 1000.0)
- `Max Impulse`: Maximum force applied (default: 10000.0)
- `Death Impulse Strength`: Force on ragdoll activation (default: 5000.0)
- `Apply Impulse At Hit Bone`: If true, apply force at hit location

**Animation:**
- `Play Hit Animations`: Enable/disable hit anims
- `Interrupt Current Montage`: Stop current anim when hit
- `Min Time Between Hit Anims`: Cooldown between anims (default: 0.2s)
- `Default Hit Reaction Montage`: Fallback anim if no zone match

## Example Hit Zones

```cpp
// Head zone - high impulse, unique anim
Bone Name: head
Radius: 15.0
Zone Tag: HitZone.Head
Impulse Multiplier: 1.5
Hit Reaction Montage: AM_HeadHit_Additive

// Chest zone
Bone Name: spine_03
Radius: 20.0
Zone Tag: HitZone.Chest  
Impulse Multiplier: 1.0
Hit Reaction Montage: AM_ChestHit_Additive

// Left arm
Bone Name: lowerarm_l
Radius: 10.0
Zone Tag: HitZone.ArmLeft
Impulse Multiplier: 0.8
Hit Reaction Montage: AM_ArmHit_Additive
```

## Blueprint Events

### On Hit Reaction
Fires when a hit reaction is applied:
- **Hit Info**: Contains direction, bone, zone, damage, time, impact location
- **Was Fatal**: True if this hit killed the character

## How It Works

1. **Weapon fires** ? Applies point damage with hit info
2. **BaseCharacter.TakeDamage()** ? Extracts direction, location, bone
3. **HitReactionComponent.ApplyHitReaction()** ? Finds matching zone
4. **Physical reaction** ? Applies impulse at hit bone
5. **Animation** ? Plays zone-specific additive montage
6. **Event broadcast** ? Notifies listeners

## Notes
- Hit zones are checked in array order (first match wins)
- Impulse force scales with damage amount
- Animations should be **additive** for best results
- Death reactions bypass animation and go straight to ragdoll
