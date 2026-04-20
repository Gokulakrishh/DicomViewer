#include "Services/VolumeRenderPreprocessingService.h"

#include "Model/IVolumeData.h"
#include "Model/VolumeData.h"

#include <cstdint>
#include <memory>
#include <vector>

VolumeRenderPreprocessingService::VolumeRenderPreprocessingService()
    : VolumeRenderPreprocessingService(Settings{})
{
}

VolumeRenderPreprocessingService::VolumeRenderPreprocessingService(Settings settings)
    : m_settings(std::move(settings))
{
}

PreparedVolumeRenderInputs VolumeRenderPreprocessingService::prepare(
    const std::shared_ptr<IVolumeData>& diagnosticVolume) const
{
    if (!diagnosticVolume)
    {
        return {};
    }

    PreparedVolumeRenderInputs inputs;
    inputs.baseVolume = diagnosticVolume;
    inputs.boneFocusedVolume = createLowerThresholdMaskedVolume(*diagnosticVolume);
    return inputs;
}

std::shared_ptr<IVolumeData> VolumeRenderPreprocessingService::createLowerThresholdMaskedVolume(
    const IVolumeData& diagnosticVolume) const
{
    const VolumeGeometry& geometry = diagnosticVolume.geometry();
    std::vector<std::int16_t> voxels;
    voxels.reserve(static_cast<std::size_t>(geometry.voxelCount()));

    for (int z = 0; z < geometry.dimensions.z; ++z)
    {
        for (int y = 0; y < geometry.dimensions.y; ++y)
        {
            for (int x = 0; x < geometry.dimensions.x; ++x)
            {
                const std::int16_t voxelValue = static_cast<std::int16_t>(diagnosticVolume.scalarAt(x, y, z));
                voxels.push_back(voxelValue >= m_settings.boneLowerThresholdHu ? voxelValue : m_settings.outsideValueHu);
            }
        }
    }

    return std::make_shared<VolumeData<std::int16_t>>(geometry, std::move(voxels));
}
