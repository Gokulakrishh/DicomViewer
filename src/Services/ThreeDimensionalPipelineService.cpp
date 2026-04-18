#include "Services/ThreeDimensionalPipelineService.h"

#include "Model/ISegmentationMask.h"
#include "Model/IMeshData.h"
#include "Model/IVolumeData.h"
#include "Services/ThreeDProfiles/I3dPipelineProfile.h"
#include "Services/IConnectedComponentStrategy.h"
#include "Services/IMeshExtractionStrategy.h"
#include "Services/IMeshPostProcessor.h"
#include "Services/ISegmentationStrategy.h"

#include <stdexcept>

ThreeDimensionalPipelineResult ThreeDimensionalPipelineService::buildMesh(
    const IVolumeData& volume,
    const I3dPipelineProfile& profile) const
{
    const std::shared_ptr<ISegmentationStrategy> segmentationStrategy = profile.createSegmentationStrategy();
    const std::shared_ptr<IConnectedComponentStrategy> componentStrategy = profile.createConnectedComponentStrategy();
    const std::shared_ptr<IMeshExtractionStrategy> extractionStrategy = profile.createMeshExtractionStrategy();
    const std::shared_ptr<IMeshPostProcessor> meshPostProcessor = profile.createMeshPostProcessor();

    if (!segmentationStrategy || !componentStrategy || !extractionStrategy || !meshPostProcessor)
    {
        throw std::runtime_error("3D pipeline profile returned an incomplete strategy set");
    }

    ThreeDimensionalPipelineResult result;
    result.segmentedMask = segmentationStrategy->segment(volume);
    if (!result.segmentedMask)
    {
        throw std::runtime_error("Segmentation stage returned a null mask");
    }

    result.filteredMask = componentStrategy->filter(*result.segmentedMask);
    if (!result.filteredMask)
    {
        throw std::runtime_error("Connected-component stage returned a null mask");
    }

    const std::shared_ptr<IMeshData> rawMesh = extractionStrategy->extract(*result.filteredMask);
    if (!rawMesh)
    {
        throw std::runtime_error("Mesh extraction stage returned a null mesh");
    }

    result.mesh = meshPostProcessor->process(*rawMesh);
    if (!result.mesh)
    {
        throw std::runtime_error("Mesh post-processing stage returned a null mesh");
    }

    result.diagnostics.profileName = std::string(profile.name());
    result.diagnostics.foregroundVoxelCount = countForegroundVoxels(*result.filteredMask);
    result.diagnostics.meshVertexCount = result.mesh->vertexCount();
    result.diagnostics.meshTriangleCount = result.mesh->triangleCount();
    return result;
}

int ThreeDimensionalPipelineService::countForegroundVoxels(const ISegmentationMask& mask)
{
    const VolumeGeometry& geometry = mask.geometry();
    int foregroundCount = 0;

    for (int z = 0; z < geometry.dimensions.z; ++z)
    {
        for (int y = 0; y < geometry.dimensions.y; ++y)
        {
            for (int x = 0; x < geometry.dimensions.x; ++x)
            {
                foregroundCount += mask.isForeground(x, y, z) ? 1 : 0;
            }
        }
    }

    return foregroundCount;
}
