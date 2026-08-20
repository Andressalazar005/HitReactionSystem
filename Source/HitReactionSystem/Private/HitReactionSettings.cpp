#include "HitReactionSettings.h"

UHitReactionSettings::UHitReactionSettings()
{
}

FName UHitReactionSettings::GetCategoryName() const
{
    return TEXT("Plugins");
}

#if WITH_EDITOR
FText UHitReactionSettings::GetSectionText() const
{
    return NSLOCTEXT("HitReactionSystem", "SectionName", "Hit Reaction System");
}

FText UHitReactionSettings::GetSectionDescription() const
{
    return NSLOCTEXT("HitReactionSystem", "SectionDescription", "Configure default settings for the Hit Reaction System plugin");
}
#endif
