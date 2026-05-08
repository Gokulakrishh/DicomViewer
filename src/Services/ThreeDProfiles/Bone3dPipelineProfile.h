#pragma once

#include "Services/ThreeDMesh/LaplacianMeshSmoothingPostProcessor.h"
#include "Services/ThreeDMesh/MarchingCubesMeshExtractionStrategy.h"
#include "Services/ThreeDProfiles/I3dPipelineProfile.h"
#include "Services/ThreeDSegmentation/ThresholdSegmentationStrategy.h"

/**
 * @brief Tunable parameters for the bone 3D reconstruction profile.
 */
struct Bone3dPipelineProfileParameters
{
    ThresholdSegmentationParameters segmentation{300.0, 3000.0, true, true};
    MarchingCubesParameters extraction{};
    bool enableMeshSmoothing{true};
    LaplacianMeshSmoothingParameters smoothing{24, 0.16F};
};

/**
 * @brief 3D pipeline profile optimized for bone-like CT attenuation ranges.
 *
 * Responsibilities:
 * - Configure threshold segmentation and mesh post-processing for bone surfaces.
 */
class Bone3dPipelineProfile final : public I3dPipelineProfile
{
public:
    /** @brief Creates the profile with optional parameter overrides. */
    explicit Bone3dPipelineProfile(Bone3dPipelineProfileParameters parameters = {});

    /** @brief Returns the stable profile name. */
    [[nodiscard]] std::string_view name() const override;
    /** @brief Creates the bone segmentation strategy. */
    [[nodiscard]] std::shared_ptr<ISegmentationStrategy> createSegmentationStrategy() const override;
    /** @brief Creates the component selection strategy. */
    [[nodiscard]] std::shared_ptr<IConnectedComponentStrategy> createConnectedComponentStrategy() const override;
    /** @brief Creates the mesh extraction strategy. */
    [[nodiscard]] std::shared_ptr<IMeshExtractionStrategy> createMeshExtractionStrategy() const override;
    /** @brief Creates the mesh post-processing chain. */
    [[nodiscard]] std::shared_ptr<IMeshPostProcessor> createMeshPostProcessor() const override;
    /** @brief Returns profile parameters. */
    [[nodiscard]] const Bone3dPipelineProfileParameters& parameters() const;

private:
    Bone3dPipelineProfileParameters m_parameters;
};
