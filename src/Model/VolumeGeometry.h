#pragma once

#include <array>

/**
 * @brief Integer voxel dimensions or indices in volume space.
 */
struct VolumeIndex3D
{
    int x{0};
    int y{0};
    int z{0};
};

/**
 * @brief Physical 3D vector used for spacing and origin.
 */
struct VolumeVector3D
{
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

/**
 * @brief Geometry metadata for a reconstructed DICOM volume.
 *
 * Responsibilities:
 * - Store dimensions, voxel spacing, origin, and direction cosines.
 * - Provide basic validity and buffer-size calculations for volume services.
 */
struct VolumeGeometry
{
    VolumeIndex3D dimensions;
    VolumeVector3D spacing{1.0, 1.0, 1.0};
    VolumeVector3D origin{0.0, 0.0, 0.0};
    std::array<double, 9> direction{
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0};

    /**
     * @brief Checks whether dimensions and spacing are usable.
     * @return True when all dimensions and spacing values are positive.
     */
    [[nodiscard]] bool isValid() const
    {
        return dimensions.x > 0 && dimensions.y > 0 && dimensions.z > 0 &&
               spacing.x > 0.0 && spacing.y > 0.0 && spacing.z > 0.0;
    }

    /**
     * @brief Returns total voxel count.
     * @return dimensions.x * dimensions.y * dimensions.z.
     */
    [[nodiscard]] int voxelCount() const
    {
        return dimensions.x * dimensions.y * dimensions.z;
    }
};
