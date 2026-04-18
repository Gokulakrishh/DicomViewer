#pragma once

#include "Services/ThreeDMesh/LaplacianMeshSmoothingPostProcessor.h"
#include "Services/ThreeDMesh/MarchingCubesMeshExtractionStrategy.h"
#include "Services/ThreeDProfiles/I3dPipelineProfile.h"
#include "Services/ThreeDSegmentation/ThresholdSegmentationStrategy.h"

struct Lung3dPipelineProfileParameters
{
    ThresholdSegmentationParameters segmentation{-1200.0, -300.0, true, true};
    MarchingCubesParameters extraction{};
    bool enableMeshSmoothing{true};
    LaplacianMeshSmoothingParameters smoothing{4, 0.12F};
};

class Lung3dPipelineProfile final : public I3dPipelineProfile
{
public:
    explicit Lung3dPipelineProfile(Lung3dPipelineProfileParameters parameters = {});

    [[nodiscard]] std::string_view name() const override;
    [[nodiscard]] std::shared_ptr<ISegmentationStrategy> createSegmentationStrategy() const override;
    [[nodiscard]] std::shared_ptr<IConnectedComponentStrategy> createConnectedComponentStrategy() const override;
    [[nodiscard]] std::shared_ptr<IMeshExtractionStrategy> createMeshExtractionStrategy() const override;
    [[nodiscard]] std::shared_ptr<IMeshPostProcessor> createMeshPostProcessor() const override;
    [[nodiscard]] const Lung3dPipelineProfileParameters& parameters() const;

private:
    Lung3dPipelineProfileParameters m_parameters;
};
