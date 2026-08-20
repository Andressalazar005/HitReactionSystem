using UnrealBuildTool;

public class HitReactionSystem : ModuleRules
{
    public HitReactionSystem(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "PhysicsCore",
                "GameplayTags",
                "DeveloperSettings"
                ,"HellRunPhysicalAnimation"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "AnimGraphRuntime"
            }
        );
    }
}
