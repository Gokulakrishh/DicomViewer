#pragma once

#include "Services/ThreeDMesh/LaplacianMeshSmoothingPostProcessor.h"
#include "Services/ThreeDMesh/MarchingCubesMeshExtractionStrategy.h"
#include "Services/ThreeDProfiles/I3dPipelineProfile.h"
#include "Services/ThreeDSegmentation/ThresholdSegmentationStrategy.h"

struct Bone3dPipelineProfileParameters
{
    ThresholdSegmentationParameters segmentation{300.0, 3000.0, true, true};
    MarchingCubesParameters extraction{};
    bool enableMeshSmoothing{true};
    LaplacianMeshSmoothingParameters smoothing{24, 0.16F};
};

class Bone3dPipelineProfile final : public I3dPipelineProfile
{
public:
    explicit Bone3dPipelineProfile(Bone3dPipelineProfileParameters parameters = {});

    [[nodiscard]] std::string_view name() const override;
    [[nodiscard]] std::shared_ptr<ISegmentationStrategy> createSegmentationStrategy() const override;
    [[nodiscard]] std::shared_ptr<IConnectedComponentStrategy> createConnectedComponentStrategy() const override;
    [[nodiscard]] std::shared_ptr<IMeshExtractionStrategy> createMeshExtractionStrategy() const override;
    [[nodiscard]] std::shared_ptr<IMeshPostProcessor> createMeshPostProcessor() const override;
    [[nodiscard]] const Bone3dPipelineProfileParameters& parameters() const;

private:
    Bone3dPipelineProfileParameters m_parameters;
};
