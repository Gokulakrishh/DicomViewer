#pragma once

struct VolumeValidationSettings
{
    double orientationAlignmentTolerance{1.0e-3};
    double spacingTolerance{1.0e-3};
    bool validateUniformSliceSpacing{true};
};
