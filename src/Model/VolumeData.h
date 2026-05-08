#pragma once

#include "IVolumeData.h"
#include "VolumeConcepts.h"

#include <stdexcept>
#include <utility>
#include <vector>

/**
 * @brief Typed contiguous scalar volume buffer.
 *
 * Responsibilities:
 * - Own voxel memory and immutable geometry.
 * - Provide typed voxel access and IVolumeData scalar access.
 *
 * Assumptions:
 * - Buffer size must match geometry.voxelCount().
 */
template<VolumeVoxel TVoxel>
class VolumeData final : public IVolumeData
{
public:
    VolumeData() = default;

    /**
     * @brief Creates a volume from geometry and voxel buffer.
     * @param geometry Volume dimensions and physical metadata.
     * @param voxels Contiguous voxel buffer in x-fastest order.
     */
    VolumeData(VolumeGeometry geometry, std::vector<TVoxel> voxels)
        : m_geometry(std::move(geometry)),
          m_voxels(std::move(voxels))
    {
        if (!m_geometry.isValid())
        {
            throw std::invalid_argument("Volume geometry is invalid");
        }

        if (static_cast<int>(m_voxels.size()) != m_geometry.voxelCount())
        {
            throw std::invalid_argument("Voxel buffer size does not match geometry");
        }
    }

    /**
     * @brief Returns volume geometry.
     * @return Geometry describing the voxel buffer.
     */
    [[nodiscard]] const VolumeGeometry& geometry() const override
    {
        return m_geometry;
    }

    /**
     * @brief Returns a scalar value at a coordinate.
     * @param x X voxel index.
     * @param y Y voxel index.
     * @param z Z voxel index.
     * @return Voxel value converted to double.
     */
    [[nodiscard]] double scalarAt(int x, int y, int z) const override
    {
        return static_cast<double>(voxelAt(x, y, z));
    }

    /**
     * @brief Checks whether a coordinate is inside the volume.
     * @param x X voxel index.
     * @param y Y voxel index.
     * @param z Z voxel index.
     * @return True when valid.
     */
    [[nodiscard]] bool isValidIndex(int x, int y, int z) const override
    {
        return x >= 0 && y >= 0 && z >= 0 &&
               x < m_geometry.dimensions.x &&
               y < m_geometry.dimensions.y &&
               z < m_geometry.dimensions.z;
    }

    /**
     * @brief Returns the typed voxel value at a coordinate.
     * @param x X voxel index.
     * @param y Y voxel index.
     * @param z Z voxel index.
     * @return Typed voxel value.
     */
    [[nodiscard]] TVoxel voxelAt(int x, int y, int z) const
    {
        if (!isValidIndex(x, y, z))
        {
            throw std::out_of_range("Volume index is out of range");
        }

        return m_voxels[flatIndex(x, y, z)];
    }

    /**
     * @brief Returns the contiguous voxel buffer.
     * @return Voxel buffer in x-fastest order.
     */
    [[nodiscard]] const std::vector<TVoxel>& voxels() const
    {
        return m_voxels;
    }

private:
    [[nodiscard]] int flatIndex(int x, int y, int z) const
    {
        return (z * m_geometry.dimensions.y * m_geometry.dimensions.x) +
               (y * m_geometry.dimensions.x) +
               x;
    }

private:
    VolumeGeometry m_geometry;
    std::vector<TVoxel> m_voxels;
};
