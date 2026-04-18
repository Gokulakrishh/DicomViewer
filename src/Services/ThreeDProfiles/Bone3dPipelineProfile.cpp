#include "Services/ThreeDProfiles/Bone3dPipelineProfile.h"

#include "Services/CompositeMeshPostProcessor.h"
#include "Services/ThreeDMesh/MeshNormalGenerationPostProcessor.h"
#include "Services/ThreeDSegmentation/KeepLargestNComponentsStrategy.h"

#include <memory>
#include <vector>

Bone3dPipelineProfile::Bone3dPipelineProfile(Bone3dPipelineProfileParameters parameters)
    : m_parameters(parameters)
{
}

std::string_view Bone3dPipelineProfile::name() const
{
    return "bone";
}

std::shared_ptr<ISegmentationStrategy> Bone3dPipelineProfile::createSegmentationStrategy() const
{
    return std::make_shared<ThresholdSegmentationStrategy>(m_parameters.segmentation);
}

std::shared_ptr<IConnectedComponentStrategy> Bone3dPipelineProfile::createConnectedComponentStrategy() const
{
    return std::make_shared<KeepLargestNComponentsStrategy>(
        ConnectedComponentSelectionParameters{ConnectedComponentKeepPreset::LargestOne, 1});
}

std::shared_ptr<IMeshExtractionStrategy> Bone3dPipelineProfile::createMeshExtractionStrategy() const
{
    return std::make_shared<MarchingCubesMeshExtractionStrategy>(m_parameters.extraction);
}

std::shared_ptr<IMeshPostProcessor> Bone3dPipelineProfile::createMeshPostProcessor() const
{
    std::vector<std::shared_ptr<IMeshPostProcessor>> processors;
    processors.push_back(std::make_shared<MeshNormalGenerationPostProcessor>());

    if (m_parameters.enableMeshSmoothing)
    {
        processors.push_back(std::make_shared<LaplacianMeshSmoothingPostProcessor>(m_parameters.smoothing));
    }

    return std::make_shared<CompositeMeshPostProcessor>(std::move(processors));
}

const Bone3dPipelineProfileParameters& Bone3dPipelineProfile::parameters() const
{
    return m_parameters;
}
