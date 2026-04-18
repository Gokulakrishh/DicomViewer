#include "Services/ThreeDProfiles/Lung3dPipelineProfile.h"

#include "Services/CompositeMeshPostProcessor.h"
#include "Services/ThreeDMesh/MeshNormalGenerationPostProcessor.h"
#include "Services/ThreeDSegmentation/KeepLargestNComponentsStrategy.h"

#include <memory>
#include <vector>

Lung3dPipelineProfile::Lung3dPipelineProfile(Lung3dPipelineProfileParameters parameters)
    : m_parameters(parameters)
{
}

std::string_view Lung3dPipelineProfile::name() const
{
    return "lung";
}

std::shared_ptr<ISegmentationStrategy> Lung3dPipelineProfile::createSegmentationStrategy() const
{
    return std::make_shared<ThresholdSegmentationStrategy>(m_parameters.segmentation);
}

std::shared_ptr<IConnectedComponentStrategy> Lung3dPipelineProfile::createConnectedComponentStrategy() const
{
    return std::make_shared<KeepLargestNComponentsStrategy>(
        ConnectedComponentSelectionParameters{
            ConnectedComponentKeepPreset::LargestTwo,
            2,
            true});
}

std::shared_ptr<IMeshExtractionStrategy> Lung3dPipelineProfile::createMeshExtractionStrategy() const
{
    return std::make_shared<MarchingCubesMeshExtractionStrategy>(m_parameters.extraction);
}

std::shared_ptr<IMeshPostProcessor> Lung3dPipelineProfile::createMeshPostProcessor() const
{
    std::vector<std::shared_ptr<IMeshPostProcessor>> processors;
    processors.push_back(std::make_shared<MeshNormalGenerationPostProcessor>());

    if (m_parameters.enableMeshSmoothing)
    {
        processors.push_back(std::make_shared<LaplacianMeshSmoothingPostProcessor>(m_parameters.smoothing));
    }

    return std::make_shared<CompositeMeshPostProcessor>(std::move(processors));
}

const Lung3dPipelineProfileParameters& Lung3dPipelineProfile::parameters() const
{
    return m_parameters;
}
