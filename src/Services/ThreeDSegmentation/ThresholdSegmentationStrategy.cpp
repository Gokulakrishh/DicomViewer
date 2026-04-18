#include "Services/ThreeDSegmentation/ThresholdSegmentationStrategy.h"

#include "Model/ISegmentationMask.h"
#include "Model/IVolumeData.h"
#include "Model/SegmentationMaskData.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

ThresholdSegmentationStrategy::ThresholdSegmentationStrategy(ThresholdSegmentationParameters parameters)
    : m_parameters(parameters)
{
    if (m_parameters.low > m_parameters.high)
    {
        std::swap(m_parameters.low, m_parameters.high);
    }
}

std::shared_ptr<ISegmentationMask> ThresholdSegmentationStrategy::segment(const IVolumeData& volume) const
{
    const VolumeGeometry& geometry = volume.geometry();
    std::vector<std::uint8_t> maskVoxels(static_cast<std::size_t>(geometry.voxelCount()), std::uint8_t{0});

    int flatIndex = 0;
    for (int z = 0; z < geometry.dimensions.z; ++z)
    {
        for (int y = 0; y < geometry.dimensions.y; ++y)
        {
            for (int x = 0; x < geometry.dimensions.x; ++x)
            {
                const double value = volume.scalarAt(x, y, z);
                maskVoxels[static_cast<std::size_t>(flatIndex++)] =
                    isInsideThreshold(value) ? std::uint8_t{1} : std::uint8_t{0};
            }
        }
    }

    return std::make_shared<SegmentationMaskData<std::uint8_t>>(geometry, std::move(maskVoxels));
}

const ThresholdSegmentationParameters& ThresholdSegmentationStrategy::parameters() const
{
    return m_parameters;
}

bool ThresholdSegmentationStrategy::isInsideThreshold(double value) const
{
    const bool lowerOk = m_parameters.inclusiveLower ? value >= m_parameters.low : value > m_parameters.low;
    const bool upperOk = m_parameters.inclusiveUpper ? value <= m_parameters.high : value < m_parameters.high;
    return lowerOk && upperOk;
}
