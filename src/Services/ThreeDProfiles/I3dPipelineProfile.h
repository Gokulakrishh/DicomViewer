#pragma once

#include <memory>
#include <string_view>

class ISegmentationStrategy;
class IConnectedComponentStrategy;
class IMeshExtractionStrategy;
class IMeshPostProcessor;

/**
 * @brief Factory interface for anatomy-specific 3D reconstruction pipelines.
 *
 * Responsibilities:
 * - Provide segmentation, component filtering, mesh extraction, and
 *   post-processing strategies.
 * - Keep profile selection separate from pipeline execution.
 */
class I3dPipelineProfile
{
public:
    virtual ~I3dPipelineProfile() = default;

    /** @brief Returns the stable profile name. */
    [[nodiscard]] virtual std::string_view name() const = 0;
    /** @brief Creates the segmentation strategy. */
    [[nodiscard]] virtual std::shared_ptr<ISegmentationStrategy> createSegmentationStrategy() const = 0;
    /** @brief Creates the connected-component filter strategy. */
    [[nodiscard]] virtual std::shared_ptr<IConnectedComponentStrategy> createConnectedComponentStrategy() const = 0;
    /** @brief Creates the mesh extraction strategy. */
    [[nodiscard]] virtual std::shared_ptr<IMeshExtractionStrategy> createMeshExtractionStrategy() const = 0;
    /** @brief Creates the mesh post-processor. */
    [[nodiscard]] virtual std::shared_ptr<IMeshPostProcessor> createMeshPostProcessor() const = 0;
};
