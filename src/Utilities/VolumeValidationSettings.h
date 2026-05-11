#pragma once

/**
 * @brief Geometry validation tolerances for DICOM volume construction.
 *
 * These internal tolerances control geometry quality checks during MPR and 3D
 * derived-volume workflows. Geometry findings are reported to the user as
 * warnings; only physically unbuildable input, such as missing pixels or
 * mismatched image dimensions, blocks volume construction.
 */
struct VolumeValidationSettings
{
    double orientationAlignmentTolerance{1.0e-3};
    double spacingTolerance{1.0e-3};
};
