#pragma once

#include "Services/ThreeDProfiles/ThreeDProfileSelection.h"
#include "VTK/Presets/VtkVolumeRenderPreset.h"

class VtkVolumePresetLibrary
{
public:
    enum class Mode
    {
        Auto = 0,
        Bone = 1,
        Lung = 2
    };

    [[nodiscard]] static VtkVolumeRenderPreset createPreset(Mode mode, const ThreeDProfileSelection& autoProfileSelection);
};
