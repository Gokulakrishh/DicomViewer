#pragma once

#include <memory>
#include <string>

class ISegmentationMask;
class IMeshData;

struct ThreeDimensionalPipelineDiagnostics
{
    std::string profileName;
    int foregroundVoxelCount{0};
    std::size_t meshVertexCount{0};
    std::size_t meshTriangleCount{0};
    // Timing and stage durations are intentionally deferred until the full
    // pipeline surface is stable and ready for profiling-based optimization.
};

struct ThreeDimensionalPipelineResult
{
    std::shared_ptr<ISegmentationMask> segmentedMask;
    std::shared_ptr<ISegmentationMask> filteredMask;
    std::shared_ptr<IMeshData> mesh;
    ThreeDimensionalPipelineDiagnostics diagnostics;

    [[nodiscard]] bool isValid() const
    {
        return static_cast<bool>(segmentedMask) &&
               static_cast<bool>(filteredMask) &&
               static_cast<bool>(mesh);
    }
};
