#pragma once

#include <memory>
#include <string>

class ISegmentationMask;
class IMeshData;

/**
 * @brief Diagnostic counters produced by the 3D reconstruction pipeline.
 *
 * These values support engineering review and future verification records; they
 * are not clinical measurements.
 */
struct ThreeDimensionalPipelineDiagnostics
{
    std::string profileName;
    int foregroundVoxelCount{0};
    std::size_t meshVertexCount{0};
    std::size_t meshTriangleCount{0};
    // Timing and stage durations are intentionally deferred until the full
    // pipeline surface is stable and ready for profiling-based optimization.
};

/**
 * @brief Outputs from a 3D segmentation and mesh extraction pipeline.
 *
 * Responsibilities:
 * - Carry intermediate masks and final mesh for rendering/inspection.
 * - Expose basic validity for pipeline orchestration.
 */
struct ThreeDimensionalPipelineResult
{
    std::shared_ptr<ISegmentationMask> segmentedMask;
    std::shared_ptr<ISegmentationMask> filteredMask;
    std::shared_ptr<IMeshData> mesh;
    ThreeDimensionalPipelineDiagnostics diagnostics;

    /**
     * @brief Checks whether all required pipeline outputs exist.
     * @return True when segmented mask, filtered mask, and mesh are non-null.
     */
    [[nodiscard]] bool isValid() const
    {
        return static_cast<bool>(segmentedMask) &&
               static_cast<bool>(filteredMask) &&
               static_cast<bool>(mesh);
    }
};
