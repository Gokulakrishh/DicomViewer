#pragma once

#include "Services/ThreeDMesh/LaplacianMeshSmoothingPostProcessor.h"
#include "Services/ThreeDMesh/MarchingCubesMeshExtractionStrategy.h"
#include "Services/ThreeDProfiles/I3dPipelineProfile.h"
#include "Services/ThreeDSegmentation/ThresholdSegmentationStrategy.h"

/**
 * @brief Tunable parameters for the lung 3D reconstruction profile.
 */
struct Lung3dPipelineProfileParameters
{
    ThresholdSegmentationParameters segmentation{-1200.0, -300.0, true, true};
    MarchingCubesParameters extraction{};
    bool enableMeshSmoothing{true};
    LaplacianMeshSmoothingParameters smoothing{4, 0.12F};
};

/**
 * @brief 3D pipeline profile optimized for lung-like CT attenuation ranges.
 *
 * Responsibilities:
 * - Configure threshold segmentation and mesh post-processing for lung surfaces.
 */
class Lung3dPipelineProfile final : public I3dPipelineProfile
{
public:
    /** @brief Creates the profile with optional parameter overrides. */
    explicit Lung3dPipelineProfile(Lung3dPipelineProfileParameters parameters = {});

    /** @brief Returns the stable profile name. */
    [[nodiscard]] std::string_view name() const override;
    /** @brief Creates the lung segmentation strategy. */
    [[nodiscard]] std::shared_ptr<ISegmentationStrategy> createSegmentationStrategy() const override;
    /** @brief Creates the component selection strategy. */
    [[nodiscard]] std::shared_ptr<IConnectedComponentStrategy> createConnectedComponentStrategy() const override;
    /** @brief Creates the mesh extraction strategy. */
    [[nodiscard]] std::shared_ptr<IMeshExtractionStrategy> createMeshExtractionStrategy() const override;
    /** @brief Creates the mesh post-processing chain. */
    [[nodiscard]] std::shared_ptr<IMeshPostProcessor> createMeshPostProcessor() const override;
    /** @brief Returns profile parameters. */
    [[nodiscard]] const Lung3dPipelineProfileParameters& parameters() const;

private:
    Lung3dPipelineProfileParameters m_parameters;
};
