#pragma once

#include <utility>

template<MaskVoxel TMaskVoxel>
SegmentationMaskData<TMaskVoxel>::SegmentationMaskData(VolumeGeometry geometry, std::vector<TMaskVoxel> voxels)
    : m_geometry(std::move(geometry)),
      m_voxels(std::move(voxels))
{
    if (!m_geometry.isValid())
    {
        throw std::invalid_argument("Segmentation mask geometry is invalid");
    }

    if (static_cast<int>(m_voxels.size()) != m_geometry.voxelCount())
    {
        throw std::invalid_argument("Segmentation mask voxel buffer size does not match geometry");
    }
}

template<MaskVoxel TMaskVoxel>
const VolumeGeometry& SegmentationMaskData<TMaskVoxel>::geometry() const
{
    return m_geometry;
}

template<MaskVoxel TMaskVoxel>
bool SegmentationMaskData<TMaskVoxel>::isValidIndex(int x, int y, int z) const
{
    return x >= 0 && y >= 0 && z >= 0 &&
           x < m_geometry.dimensions.x &&
           y < m_geometry.dimensions.y &&
           z < m_geometry.dimensions.z;
}

template<MaskVoxel TMaskVoxel>
bool SegmentationMaskData<TMaskVoxel>::isForeground(int x, int y, int z) const
{
    return static_cast<bool>(voxelAt(x, y, z));
}

template<MaskVoxel TMaskVoxel>
TMaskVoxel SegmentationMaskData<TMaskVoxel>::voxelAt(int x, int y, int z) const
{
    if (!isValidIndex(x, y, z))
    {
        throw std::out_of_range("Segmentation mask index is out of range");
    }

    return m_voxels[flatIndex(x, y, z)];
}

template<MaskVoxel TMaskVoxel>
const std::vector<TMaskVoxel>& SegmentationMaskData<TMaskVoxel>::voxels() const
{
    return m_voxels;
}

template<MaskVoxel TMaskVoxel>
int SegmentationMaskData<TMaskVoxel>::flatIndex(int x, int y, int z) const
{
    return (z * m_geometry.dimensions.y * m_geometry.dimensions.x) +
           (y * m_geometry.dimensions.x) +
           x;
}
