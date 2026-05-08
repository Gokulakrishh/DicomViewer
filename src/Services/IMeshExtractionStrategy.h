#pragma once

#include <memory>

class ISegmentationMask;
class IMeshData;

/**
 * @brief Strategy interface for extracting a mesh from a segmentation mask.
 *
 * Responsibilities:
 * - Convert voxel-space foreground masks into triangle meshes.
 * - Isolate algorithms such as marching cubes from pipeline orchestration.
 */
class IMeshExtractionStrategy
{
public:
    virtual ~IMeshExtractionStrategy() = default;

    /**
     * @brief Extracts a mesh from a mask.
     * @param mask Segmentation mask.
     * @return Mesh data, or null on failure.
     */
    [[nodiscard]] virtual std::shared_ptr<IMeshData> extract(const ISegmentationMask& mask) const = 0;
};
