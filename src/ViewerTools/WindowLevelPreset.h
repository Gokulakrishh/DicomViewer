#pragma once

#include <QString>
#include <array>

struct WindowLevelPreset
{
    const char* name;
    int level;
    int width;
};

enum class BuiltInWindowLevelPresetId
{
    Brain,
    SoftTissue,
    Bone,
    Lung
};

inline constexpr WindowLevelPreset kBrainWindowLevelPreset{"Brain", 40, 80};
inline constexpr WindowLevelPreset kSoftTissueWindowLevelPreset{"Soft Tissue", 50, 350};
inline constexpr WindowLevelPreset kBoneWindowLevelPreset{"Bone", 400, 1800};
inline constexpr WindowLevelPreset kLungWindowLevelPreset{"Lung", -600, 1400};

inline constexpr std::array<WindowLevelPreset, 4> kBuiltInWindowLevelPresets{{
    kBrainWindowLevelPreset,
    kSoftTissueWindowLevelPreset,
    kBoneWindowLevelPreset,
    kLungWindowLevelPreset,
}};

inline constexpr std::array<BuiltInWindowLevelPresetId, 4> kBuiltInWindowLevelPresetIds{{
    BuiltInWindowLevelPresetId::Brain,
    BuiltInWindowLevelPresetId::SoftTissue,
    BuiltInWindowLevelPresetId::Bone,
    BuiltInWindowLevelPresetId::Lung,
}};

inline QString windowLevelPresetLabel(const WindowLevelPreset& preset)
{
    return QString::fromLatin1(preset.name);
}

inline constexpr WindowLevelPreset windowLevelPreset(BuiltInWindowLevelPresetId presetId)
{
    switch (presetId)
    {
    case BuiltInWindowLevelPresetId::Brain:
        return kBrainWindowLevelPreset;
    case BuiltInWindowLevelPresetId::SoftTissue:
        return kSoftTissueWindowLevelPreset;
    case BuiltInWindowLevelPresetId::Bone:
        return kBoneWindowLevelPreset;
    case BuiltInWindowLevelPresetId::Lung:
        return kLungWindowLevelPreset;
    }

    return kBrainWindowLevelPreset;
}
