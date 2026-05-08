#pragma once

#include "VolumeGeometry.h"

/**
 * @brief Read-only interface for scalar volume data.
 *
 * Responsibilities:
 * - Expose volume geometry and scalar access without binding callers to a
 *   concrete voxel type.
 * - Support MPR, segmentation, and 3D reconstruction services.
 *
 * Assumptions:
 * - Implementations validate indices before accessing voxel buffers.
 */
class IVolumeData
{
public:
    virtual ~IVolumeData() = default;

    /**
     * @brief Returns volume geometry.
     * @return Geometry describing dimensions and physical spacing.
     */
    [[nodiscard]] virtual const VolumeGeometry& geometry() const = 0;

    /**
     * @brief Returns a scalar value at a voxel coordinate.
     * @param x X voxel index.
     * @param y Y voxel index.
     * @param z Z voxel index.
     * @return Scalar value converted to double.
     */
    [[nodiscard]] virtual double scalarAt(int x, int y, int z) const = 0;

    /**
     * @brief Checks whether a voxel coordinate is inside the volume.
     * @param x X voxel index.
     * @param y Y voxel index.
     * @param z Z voxel index.
     * @return True when the coordinate is valid.
     */
    [[nodiscard]] virtual bool isValidIndex(int x, int y, int z) const = 0;
};
