#pragma once

#include "ISegmentationMask.h"
#include "VolumeConcepts.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

/**
 * @brief Typed binary/label segmentation mask buffer.
 *
 * Responsibilities:
 * - Own foreground mask voxels and volume geometry.
 * - Provide ISegmentationMask access for connected components and mesh
 *   extraction.
 *
 * Assumptions:
 * - Non-zero mask values are foreground.
 */
template<MaskVoxel TMaskVoxel = std::uint8_t>
class SegmentationMaskData final : public ISegmentationMask
{
public:
    SegmentationMaskData() = default;

    /**
     * @brief Creates a segmentation mask from geometry and voxel buffer.
     * @param geometry Mask geometry.
     * @param voxels Contiguous mask buffer in x-fastest order.
     */
    SegmentationMaskData(VolumeGeometry geometry, std::vector<TMaskVoxel> voxels);

    /** @brief Returns mask geometry. */
    [[nodiscard]] const VolumeGeometry& geometry() const override;
    /** @brief Checks whether a voxel coordinate is valid. */
    [[nodiscard]] bool isValidIndex(int x, int y, int z) const override;
    /** @brief Reports whether a voxel is foreground. */
    [[nodiscard]] bool isForeground(int x, int y, int z) const override;

    /** @brief Returns the typed mask voxel at a coordinate. */
    [[nodiscard]] TMaskVoxel voxelAt(int x, int y, int z) const;
    /** @brief Returns the contiguous mask buffer. */
    [[nodiscard]] const std::vector<TMaskVoxel>& voxels() const;

private:
    [[nodiscard]] int flatIndex(int x, int y, int z) const;

private:
    VolumeGeometry m_geometry;
    std::vector<TMaskVoxel> m_voxels;
};

extern template class SegmentationMaskData<std::uint8_t>;

#include "SegmentationMaskData.tpp"
