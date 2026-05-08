#pragma once

#include "VolumeGeometry.h"

/**
 * @brief Read-only binary segmentation mask interface.
 *
 * Responsibilities:
 * - Expose mask geometry and foreground queries.
 * - Decouple segmentation algorithms from concrete voxel storage.
 */
class ISegmentationMask
{
public:
    virtual ~ISegmentationMask() = default;

    /** @brief Returns mask geometry. */
    [[nodiscard]] virtual const VolumeGeometry& geometry() const = 0;
    /** @brief Checks whether a voxel index is valid. */
    [[nodiscard]] virtual bool isValidIndex(int x, int y, int z) const = 0;
    /** @brief Reports whether a voxel belongs to the foreground mask. */
    [[nodiscard]] virtual bool isForeground(int x, int y, int z) const = 0;
};
