#pragma once

/**
 * @brief Geometry validation tolerances for DICOM volume construction.
 *
 * These tolerances control when inconsistent orientation or spacing blocks MPR
 * and 3D derived-volume workflows.
 */
struct VolumeValidationSettings
{
    double orientationAlignmentTolerance{1.0e-3};
    double spacingTolerance{1.0e-3};
    bool validateUniformSliceSpacing{true};
};
