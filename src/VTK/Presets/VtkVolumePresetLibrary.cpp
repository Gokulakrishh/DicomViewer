#include "VTK/Presets/VtkVolumePresetLibrary.h"

namespace
{
VtkVolumeRenderPreset createBonePreset()
{
    VtkVolumeRenderPreset preset;
    preset.inputKind = VtkVolumeInputKind::BoneFocusedVolume;
    preset.colorPoints = {
        {-1000.0, 0.0, 0.0, 0.0},
        {220.0, 0.0, 0.0, 0.0},
        {320.0, 0.66, 0.58, 0.40},
        {650.0, 0.90, 0.80, 0.56},
        {1400.0, 0.95, 0.88, 0.67},
        {2600.0, 0.98, 0.94, 0.82},
    };
    preset.scalarOpacityPoints = {
        {-1000.0, 0.0, 0.0, 0.0},
        {220.0, 0.0, 0.0, 0.0},
        {280.0, 0.0, 0.0, 0.0},
        {360.0, 0.06, 0.0, 0.0},
        {550.0, 0.22, 0.0, 0.0},
        {900.0, 0.48, 0.0, 0.0},
        {1600.0, 0.78, 0.0, 0.0},
        {3000.0, 0.96, 0.0, 0.0},
    };
    preset.gradientOpacityPoints = {
        {0.0, 0.0, 0.0, 0.0},
        {20.0, 0.0, 0.0, 0.0},
        {60.0, 0.12, 0.0, 0.0},
        {120.0, 0.42, 0.0, 0.0},
        {240.0, 0.82, 0.0, 0.0},
    };
    return preset;
}

VtkVolumeRenderPreset createLungPreset()
{
    VtkVolumeRenderPreset preset;
    preset.inputKind = VtkVolumeInputKind::BaseVolume;
    preset.colorPoints = {
        {-1000.0, 0.0, 0.0, 0.0},
        {-900.0, 0.35, 0.72, 0.84},
        {-700.0, 0.49, 0.82, 0.89},
        {-450.0, 0.62, 0.86, 0.92},
        {-300.0, 0.0, 0.0, 0.0},
        {200.0, 0.0, 0.0, 0.0},
    };
    preset.scalarOpacityPoints = {
        {-1000.0, 0.0, 0.0, 0.0},
        {-900.0, 0.01, 0.0, 0.0},
        {-800.0, 0.06, 0.0, 0.0},
        {-650.0, 0.14, 0.0, 0.0},
        {-500.0, 0.08, 0.0, 0.0},
        {-350.0, 0.01, 0.0, 0.0},
        {-250.0, 0.0, 0.0, 0.0},
        {3000.0, 0.0, 0.0, 0.0},
    };
    preset.gradientOpacityPoints = {
        {0.0, 0.0, 0.0, 0.0},
        {10.0, 0.0, 0.0, 0.0},
        {40.0, 0.08, 0.0, 0.0},
        {90.0, 0.30, 0.0, 0.0},
        {180.0, 0.70, 0.0, 0.0},
    };
    return preset;
}
}

VtkVolumeRenderPreset VtkVolumePresetLibrary::createPreset(Mode mode, const ThreeDProfileSelection& autoProfileSelection)
{
    switch (mode)
    {
    case Mode::Bone:
        return createBonePreset();
    case Mode::Lung:
        return createLungPreset();
    case Mode::Auto:
        return autoProfileSelection.visualStyle.anatomyKind == ThreeDAnatomyKind::Lung
            ? createLungPreset()
            : createBonePreset();
    }

    return createBonePreset();
}
