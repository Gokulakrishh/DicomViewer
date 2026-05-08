#pragma once

#include <QString>
#include <array>

/**
 * @brief Built-in WL/WW preset definition.
 */
struct WindowLevelPreset
{
    const char* name;
    int level;
    int width;
};

/**
 * @brief Identifier for built-in window-level presets.
 */
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

/**
 * @brief Returns the display label for a preset.
 * @param preset Preset to label.
 * @return User-visible preset name.
 */
inline QString windowLevelPresetLabel(const WindowLevelPreset& preset)
{
    return QString::fromLatin1(preset.name);
}

/**
 * @brief Resolves a built-in preset by id.
 * @param presetId Built-in preset id.
 * @return Preset definition.
 */
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
