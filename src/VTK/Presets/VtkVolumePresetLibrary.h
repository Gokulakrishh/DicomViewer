#pragma once

#include "Services/ThreeDProfiles/ThreeDProfileSelection.h"
#include "VTK/Presets/VtkVolumeRenderPreset.h"

/**
 * @brief Factory for VTK volume rendering presets.
 */
class VtkVolumePresetLibrary
{
public:
    /** @brief Preset mode selected in the volume viewer. */
    enum class Mode
    {
        Auto = 0,
        Bone = 1,
        Lung = 2
    };

    /** @brief Creates a preset for the selected mode. */
    [[nodiscard]] static VtkVolumeRenderPreset createPreset(Mode mode, const ThreeDProfileSelection& autoProfileSelection);
};
