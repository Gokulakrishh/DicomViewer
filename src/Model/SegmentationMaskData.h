#pragma once

#include "ISegmentationMask.h"
#include "VolumeConcepts.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

template<MaskVoxel TMaskVoxel = std::uint8_t>
class SegmentationMaskData final : public ISegmentationMask
{
public:
    SegmentationMaskData() = default;

    SegmentationMaskData(VolumeGeometry geometry, std::vector<TMaskVoxel> voxels);

    [[nodiscard]] const VolumeGeometry& geometry() const override;
    [[nodiscard]] bool isValidIndex(int x, int y, int z) const override;
    [[nodiscard]] bool isForeground(int x, int y, int z) const override;

    [[nodiscard]] TMaskVoxel voxelAt(int x, int y, int z) const;
    [[nodiscard]] const std::vector<TMaskVoxel>& voxels() const;

private:
    [[nodiscard]] int flatIndex(int x, int y, int z) const;

private:
    VolumeGeometry m_geometry;
    std::vector<TMaskVoxel> m_voxels;
};

extern template class SegmentationMaskData<std::uint8_t>;

#include "SegmentationMaskData.tpp"
